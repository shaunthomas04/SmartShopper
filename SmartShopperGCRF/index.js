const functions = require('@google-cloud/functions-framework');
const OpenAI = require('openai');
const fs = require('fs');
const { zodTextFormat } = require('openai/helpers/zod');
const { z } = require('zod');
const { getJson } = require('serpapi');
const getRawBody = require('raw-body');
const path = require('path');
const { tmpdir } = require('os');

const openai = new OpenAI({
  apiKey: process.env.OPENAI_API_KEY,
});

const ShoppingSuggestions = z.object({
  suggestions: z.array(z.string()),
});


exports.smartShopper = async (req, res) => {
  try {
    if (req.method !== "POST") {
      return res.status(405).send("Only POST requests are allowed.");
    }

    const zipCode = req.query.zipCode;
    if (!zipCode) {
      return res.status(400).send("Missing zipCode query parameter");
    }

    const fileBuffer = req.rawBody ?? req.body;

    if (!fileBuffer || fileBuffer.length === 0) {
      return res.status(400).send("No WAV file received in request body");
    }

    // Save WAV to temp file
    const tempFilePath = path.join(tmpdir(), `upload_${Date.now()}.wav`);
    await fs.promises.writeFile(tempFilePath, fileBuffer);
    console.log(`Received WAV file for ZIP ${zipCode}: ${fileBuffer.length} bytes`);

    // Transcribe the saved WAV file
    const transcription = await transcribeAudio(tempFilePath);
    console.log(`Transcription: ${transcription}`);

    if (!transcription) {
      return res.status(422).send("Could not transcribe audio");
    }

    // Get shopping suggestions from transcription
    const suggestions = await getSuggestions(transcription);
    console.log(`Suggestions: ${suggestions}`);

    if (!suggestions || suggestions.length === 0) {
      return res.status(422).send("Could not generate shopping suggestions");
    }

    // Get shopping results for all suggestions using zip code
    const shoppingResults = await getShoppingResultsForList(suggestions, zipCode);
    console.log(`Shopping results fetched for ${Object.keys(shoppingResults).length} suggestions`);

    return res.status(200).json({
      zipCode,
      transcription,
      suggestions,
      shoppingResults,
    });
  } catch (error) {
    console.error("Error processing request:", error);
    return res.status(500).send("Internal Server Error");
  }
};

// Transcribe audio from a WAV file path
async function transcribeAudio(filePath) {
  try {
    const transcription = await openai.audio.transcriptions.create({
      file: fs.createReadStream(filePath),
      model: "gpt-4o-transcribe",
    });

    return transcription.text;
  } catch (err) {
    console.error("Error transcribing audio:", err);
    return "";
  }
}

//get the suggestions from openAI structured outputs
async function getSuggestions(userInput) {
  try {
    const response = await openai.responses.parse({
      model: "gpt-4o-2024-08-06",
      input: [
        {
          role: "system",
          content:
            "You are a helpful shopping assistant, return a list of 3 to 5 items the user can buy to assist them.",
        },
        {
          role: "user",
          content: userInput,
        },
      ],
      text: {
        format: zodTextFormat(ShoppingSuggestions, "event"),
      },
    });

    const event = response.output_parsed;
    return event.suggestions;
  } catch (err) {
    console.error("Error fetching suggestions:", err);
    return []; 
  }
}

//function to take in user's zip code and get their location for serp requests
async function getLocationFromZip(zip, countryCode = "us") {
  try {
    const res = await fetch(`https://api.zippopotam.us/${countryCode}/${zip}`);
    if (!res.ok) throw new Error(`ZIP code not found: ${zip}`);

    const data = await res.json();

    // Grab first place (usually there's only one for US ZIPs)
    const place = data.places[0];
    const state = place["state"];
    const country = data["country"];

    return `${zip}, ${state}, ${country}`;
  } catch (err) {
    console.error(err);
    return null;
  }
}

//get the serp shopping items for one given query and its location
async function getSerpShoppingItems(query, location) {
  const apiKey = process.env.SERPAPI_KEY;
  if (!apiKey) throw new Error("Please set SERPAPI_KEY in your environment variables");

  return new Promise((resolve, reject) => {
    getJson(
      {
        api_key: apiKey,
        engine: "google_shopping",
        google_domain: "google.com",
        q: query,
        hl: "en",
        gl: "us",
        location: location,
      },
      (json) => {
        try {
          if (!json.shopping_results || json.shopping_results.length === 0) {
            return resolve([]);
          }

          // Return top 5 formatted items
          const topItems = json.shopping_results.slice(0, 5).map((item) => ({
            name: item.title,
            link: item.product_link,
            image: item.thumbnail || item.serpapi_thumbnail || null,
            price: item.price || null,
            cost: item.extracted_price || null,
            rating: item.rating || null,
            reviews: item.reviews || null,
            store: item.source || null,
            distance: item.extensions?.[0] || null,
          }));

          resolve(topItems);
        } catch (err) {
          reject(err);
        }
      }
    );
  });
}

//get all shopping results from list
async function getShoppingResultsForList(suggestions, zip) {
  const location = await getLocationFromZip(zip);
  if (!location) throw new Error("Failed to convert ZIP to location");

  const results = {};

  // Run all searches in parallel
  await Promise.all(
    suggestions.map(async (query) => {
      const items = await getSerpShoppingItems(query, location);
      results[query] = items;
    })
  );

  return results;
}