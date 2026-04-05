const functions = require('@google-cloud/functions-framework');
const OpenAI = require('openai');
const fs = require('fs');
const path = require('path');
const { tmpdir } = require('os');
const { z } = require('zod');
const { zodTextFormat } = require('openai/helpers/zod');
const { getJson } = require('serpapi');

const openai = new OpenAI({ apiKey: process.env.OPENAI_API_KEY });

// Zod schema for structured shopping suggestions
const ShoppingSuggestions = z.object({
  suggestions: z.array(z.string()),
});

exports.smartShopper = async (req, res) => {
  try {
    if (req.method !== "POST") {
      return res.status(405).send("Only POST requests are allowed.");
    }

    const zipCode = req.query.zipCode;
    if (!zipCode) return res.status(400).send("Missing zipCode query parameter");

    const fileBuffer = req.rawBody ?? req.body;
    if (!fileBuffer || fileBuffer.length === 0) {
      return res.status(400).send("No WAV file received in request body");
    }

    // Save WAV to temp file
    const tempFilePath = path.join(tmpdir(), `upload_${Date.now()}.wav`);
    await fs.promises.writeFile(tempFilePath, fileBuffer);

    // Transcribe the audio
    const transcription = await transcribeAudio(tempFilePath);
    if (!transcription) return res.status(422).send("Could not transcribe audio");

    // Get suggested items
    const suggestions = await getSuggestions(transcription);
    if (!suggestions || suggestions.length === 0) return res.status(422).send("No suggestions generated");

    // Fetch shopping items for suggestions
    const results = await getShoppingResultsForList(suggestions, zipCode);

    // Flatten results into a single array of shopping items
    const allItems = Object.values(results).flat();

    return res.status(200).json({ shoppingResults: allItems });

  } catch (err) {
    console.error("Error:", err);
    return res.status(500).send("Internal Server Error");
  }
};

// Transcribe WAV file using OpenAI
async function transcribeAudio(filePath) {
  try {
    const transcription = await openai.audio.transcriptions.create({
      file: fs.createReadStream(filePath),
      model: "gpt-4o-transcribe",
    });
    return transcription.text;
  } catch (err) {
    console.error("Transcription error:", err);
    return "";
  }
}

// Get structured shopping suggestions from OpenAI
async function getSuggestions(userInput) {
  try {
    const response = await openai.responses.parse({
      model: "gpt-4o-2024-08-06",
      input: [
        { role: "system", content: "You are a helpful shopping assistant. Return a list of 3-5 items the user can buy." },
        { role: "user", content: userInput },
      ],
      text: { format: zodTextFormat(ShoppingSuggestions, "event") },
    });

    return response.output_parsed?.suggestions ?? [];
  } catch (err) {
    console.error("Error fetching suggestions:", err);
    return [];
  }
}

// Convert ZIP code to location string
async function getLocationFromZip(zip, countryCode = "us") {
  try {
    const res = await fetch(`https://api.zippopotam.us/${countryCode}/${zip}`);
    if (!res.ok) throw new Error(`ZIP code not found: ${zip}`);
    const data = await res.json();
    const place = data.places[0];
    return `${zip}, ${place.state}, ${data.country}`;
  } catch (err) {
    console.error(err);
    return null;
  }
}

// Get SERP shopping items for one query + location
async function getSerpShoppingItems(query, location) {
  const apiKey = process.env.SERPAPI_KEY;
  if (!apiKey) throw new Error("Please set SERPAPI_KEY in environment variables");

  return new Promise((resolve, reject) => {
    getJson(
      { api_key: apiKey, engine: "google_shopping", q: query, location, hl: "en", gl: "us", google_domain: "google.com" },
      (json) => {
        try {
          const topItems = (json.shopping_results ?? []).slice(0, 5).map((item) => ({
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

// Fetch shopping results for all suggestions
async function getShoppingResultsForList(suggestions, zip) {
  const location = await getLocationFromZip(zip);
  if (!location) throw new Error("Invalid ZIP code");

  const results = {};
  await Promise.all(
    suggestions.map(async (query) => {
      results[query] = await getSerpShoppingItems(query, location);
    })
  );

  return results;
}