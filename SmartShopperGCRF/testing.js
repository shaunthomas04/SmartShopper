import "dotenv/config";
import OpenAI from "openai";
import fs from "fs";
import { zodTextFormat } from "openai/helpers/zod";
import { z } from "zod";

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

(async () => {
  try {
    const transcribedText = await transcribeAudio();
    console.log("Transcribed text:", transcribedText);

    const suggestions = await getSuggestions(transcribedText);
    console.log("Shopping suggestions:", suggestions);
  } catch (err) {
    console.error("Error:", err);
  }
})();