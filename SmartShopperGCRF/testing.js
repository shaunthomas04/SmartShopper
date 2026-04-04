import "dotenv/config";
import fs from "fs";
import OpenAI from "openai";

const openai = new OpenAI({
  apiKey: process.env.OPENAI_API_KEY,
});

async function transcribeAudio() {
  try {
    const transcription = await openai.audio.transcriptions.create({
      file: fs.createReadStream("audio.wav"),
      model: "gpt-4o-transcribe",
    });

    console.log("Transcription result:", transcription.text);
  } catch (err) {
    console.error("Error transcribing audio:", err);
  }
}

transcribeAudio();