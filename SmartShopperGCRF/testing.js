import "dotenv/config";
import OpenAI from "openai";
import fs from "fs";
import { zodTextFormat } from "openai/helpers/zod";
import { z } from "zod";
import fetch from "node-fetch";
import { getJson } from "serpapi";

const openai = new OpenAI({
  apiKey: process.env.OPENAI_API_KEY,
});

const ShoppingSuggestions = z.object({
  suggestions: z.array(z.string()),
});

//transcribe audio from wav file
async function transcribeAudio() {
  try {
    const transcription = await openai.audio.transcriptions.create({
      file: fs.createReadStream("audio.wav"),
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

// (async () => {
//   try {
//     const transcribedText = await transcribeAudio();
//     console.log("Transcribed text:", transcribedText);

//     const suggestions = await getSuggestions(transcribedText);
//     console.log("Shopping suggestions:", suggestions);

//     const zip = "90210";
//     const allResults = await getShoppingResultsForList(suggestions, zip);
//     console.log(JSON.stringify(allResults, null, 2));

//   } catch (err) {
//     console.error("Error:", err);
//   }
// })();




// import "dotenv/config";
// import OpenAI from "openai";
// import fs from "fs";
// import path from "path";
// import fetch from "node-fetch";
// import { getJson } from "serpapi";
// import { tmpdir } from "os";
// import formidable from "formidable"; // for parsing multipart/form-data
// import { functions } from '@google-cloud/functions-framework'; // if using ES modules

// const openai = new OpenAI({
//   apiKey: process.env.OPENAI_API_KEY,
// });

// const ShoppingSuggestions = z.object({
//   suggestions: z.array(z.string()),
// });

// //wav file to text
// async function transcribeAudio(filePath) {
//   try {
//     const transcription = await openai.audio.transcriptions.create({
//       file: fs.createReadStream(filePath),
//       model: "gpt-4o-transcribe",
//     });

//     return transcription.text;
//   } catch (err) {
//     console.error("Error transcribing audio:", err);
//     return "";
//   }
// }

// //get the suggestions from openAI structured outputs
// async function getSuggestions(userInput) {
//   try {
//     const response = await openai.responses.parse({
//       model: "gpt-4o-2024-08-06",
//       input: [
//         {
//           role: "system",
//           content:
//             "You are a helpful shopping assistant, return a list of 3 to 5 items the user can buy to assist them.",
//         },
//         {
//           role: "user",
//           content: userInput,
//         },
//       ],
//       text: {
//         format: zodTextFormat(ShoppingSuggestions, "event"),
//       },
//     });

//     const event = response.output_parsed;
//     return event.suggestions;
//   } catch (err) {
//     console.error("Error fetching suggestions:", err);
//     return []; 
//   }
// }

// //function to take in user's zip code and get their location for serp requests
// async function getLocationFromZip(zip, countryCode = "us") {
//   try {
//     const res = await fetch(`https://api.zippopotam.us/${countryCode}/${zip}`);
//     if (!res.ok) throw new Error(`ZIP code not found: ${zip}`);

//     const data = await res.json();

//     // Grab first place (usually there's only one for US ZIPs)
//     const place = data.places[0];
//     const state = place["state"];
//     const country = data["country"];

//     return `${zip}, ${state}, ${country}`;
//   } catch (err) {
//     console.error(err);
//     return null;
//   }
// }

// //get the serp shopping items for one given query and its location
// async function getSerpShoppingItems(query, location) {
//   const apiKey = process.env.SERPAPI_KEY;
//   if (!apiKey) throw new Error("Please set SERPAPI_KEY in your environment variables");

//   return new Promise((resolve, reject) => {
//     getJson(
//       {
//         api_key: apiKey,
//         engine: "google_shopping",
//         google_domain: "google.com",
//         q: query,
//         hl: "en",
//         gl: "us",
//         location: location,
//       },
//       (json) => {
//         try {
//           if (!json.shopping_results || json.shopping_results.length === 0) {
//             return resolve([]);
//           }

//           // Return top 5 formatted items
//           const topItems = json.shopping_results.slice(0, 5).map((item) => ({
//             name: item.title,
//             link: item.product_link,
//             image: item.thumbnail || item.serpapi_thumbnail || null,
//             price: item.price || null,
//             cost: item.extracted_price || null,
//             rating: item.rating || null,
//             reviews: item.reviews || null,
//             store: item.source || null,
//             distance: item.extensions?.[0] || null,
//           }));

//           resolve(topItems);
//         } catch (err) {
//           reject(err);
//         }
//       }
//     );
//   });
// }

// //get all shopping results from list
// async function getShoppingResultsForList(suggestions, zip) {
//   const location = await getLocationFromZip(zip);
//   if (!location) throw new Error("Failed to convert ZIP to location");

//   const results = {};

//   // Run all searches in parallel
//   await Promise.all(
//     suggestions.map(async (query) => {
//       const items = await getSerpShoppingItems(query, location);
//       results[query] = items;
//     })
//   );

//   return results;
// }

// //endpoint
// functions.http('shoppingPost', async (req, res) => {
//   if (req.method !== "POST") {
//     res.status(405).send("Method Not Allowed, use POST");
//     return;
//   }

//   try {
//     const form = new formidable.IncomingForm({ multiples: false, uploadDir: tmpdir(), keepExtensions: true });

//     form.parse(req, async (err, fields, files) => {
//       if (err) {
//         res.status(400).send("Error parsing form data: " + err.message);
//         return;
//       }

//       const zip = fields.zip;
//       if (!zip) {
//         res.status(400).send("Missing zip code parameter");
//         return;
//       }

//       const wavFile = files.audio;
//       if (!wavFile) {
//         res.status(400).send("Missing audio file parameter (field name: audio)");
//         return;
//       }

//       // Transcribe audio
//       const transcription = await transcribeAudio(wavFile.filepath);

//       // Get shopping suggestions from transcription
//       const suggestions = await getSuggestions(transcription);

//       // Get all shopping results for suggestions and zip code
//       const shoppingResults = await getShoppingResultsForList(suggestions, zip);

//       // Respond with JSON
//       res.status(200).json({
//         transcription,
//         suggestions,
//         shoppingResults,
//       });

//       // Clean up uploaded file
//       fs.unlink(wavFile.filepath, () => {});
//     });
//   } catch (error) {
//     console.error(error);
//     res.status(500).send("Internal Server Error");
//   }
// });
