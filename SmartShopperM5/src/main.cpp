// #include <M5Unified.h>
// #include <SPI.h>
// #include "FS.h"
// #include "SD.h"

// #define SAMPLE_RATE  16000
// #define RECORD_LENGTH 256  // samples per record call

// #define SD_SPI_CS_PIN   4
// #define SD_SPI_SCK_PIN  18
// #define SD_SPI_MISO_PIN 38
// #define SD_SPI_MOSI_PIN 23

// bool isRecording = false;
// File file;
// int totalBytes = 0;

// #define REC_X 40
// #define REC_Y 180
// #define REC_W 100
// #define REC_H 50

// #define PLAY_X 180
// #define PLAY_Y 180
// #define PLAY_W 100
// #define PLAY_H 50

// void drawUI() {
//     M5.Display.fillScreen(BLACK);
//     M5.Display.fillRect(REC_X, REC_Y, REC_W, REC_H, isRecording ? DARKGREEN : RED);
//     M5.Display.setCursor(REC_X + 20, REC_Y + 15);
//     M5.Display.setTextColor(WHITE);
//     M5.Display.print(isRecording ? "STOP" : "REC");
//     M5.Display.fillRect(PLAY_X, PLAY_Y, PLAY_W, PLAY_H, BLUE);
//     M5.Display.setCursor(PLAY_X + 20, PLAY_Y + 15);
//     M5.Display.print("PLAY");
// }

// void writeWavHeader(File &f, int sampleRate, int dataSize) {
//     byte header[44] = {0};
//     int fileSize = dataSize + 36;
//     memcpy(header, "RIFF", 4);
//     header[4] = fileSize & 0xff;
//     header[5] = (fileSize >> 8) & 0xff;
//     header[6] = (fileSize >> 16) & 0xff;
//     header[7] = (fileSize >> 24) & 0xff;
//     memcpy(header + 8, "WAVEfmt ", 8);
//     header[16] = 16;
//     header[20] = 1;
//     header[22] = 1;
//     header[24] = sampleRate & 0xff;
//     header[25] = (sampleRate >> 8) & 0xff;
//     header[26] = (sampleRate >> 16) & 0xff;
//     header[27] = (sampleRate >> 24) & 0xff;
//     int byteRate = sampleRate * 2;
//     header[28] = byteRate & 0xff;
//     header[29] = (byteRate >> 8) & 0xff;
//     header[30] = (byteRate >> 16) & 0xff;
//     header[31] = (byteRate >> 24) & 0xff;
//     header[32] = 2;
//     header[34] = 16;
//     memcpy(header + 36, "data", 4);
//     header[40] = dataSize & 0xff;
//     header[41] = (dataSize >> 8) & 0xff;
//     header[42] = (dataSize >> 16) & 0xff;
//     header[43] = (dataSize >> 24) & 0xff;
//     f.write(header, 44);
// }

// void startRecording() {
//     Serial.println("Start recording...");

//     /// Since mic and speaker share I2S, turn off speaker first
//     M5.Speaker.end();
//     delay(100);
//     M5.Mic.begin();

//     SD.remove("/record.wav");
//     file = SD.open("/record.wav", FILE_WRITE);
//     if (!file) {
//         Serial.println("Failed to open file!");
//         M5.Mic.end();
//         M5.Speaker.begin();
//         return;
//     }

//     for (int i = 0; i < 44; i++) file.write((byte)0);
//     totalBytes = 0;
//     isRecording = true;
//     drawUI();
//     Serial.println("Recording started.");
// }

// void stopRecording() {
//     Serial.println("Stop recording...");
//     if (!file) return;

//     isRecording = false;

//     file.seek(0);
//     writeWavHeader(file, SAMPLE_RATE, totalBytes);
//     file.close();

//     /// Turn off mic and restore speaker
//     M5.Mic.end();
//     delay(100);
//     M5.Speaker.begin();

//     drawUI();
//     Serial.printf("Recording saved. Total bytes: %d\n", totalBytes);
// }

// void playAudio() {
//     Serial.println("Play pressed.");
//     if (!SD.exists("/record.wav")) {
//         Serial.println("File does not exist!");
//         return;
//     }

//     File f = SD.open("/record.wav", FILE_READ);
//     if (!f) {
//         Serial.println("Failed to open file!");
//         return;
//     }

//     // Skip 44-byte WAV header
//     f.seek(44);
//     size_t dataSize = f.size() - 44;
//     Serial.printf("Audio data size: %d bytes\n", dataSize);

//     uint8_t* buffer = new uint8_t[dataSize];
//     f.read(buffer, dataSize);
//     f.close();

//     M5.Speaker.setVolume(255);
//     // Use playRaw for raw PCM data (skip WAV header manually above)
//     M5.Speaker.playRaw((int16_t*)buffer, dataSize / 2, SAMPLE_RATE, false, 1, 0);

//     while (M5.Speaker.isPlaying()) {
//         M5.update();
//         delay(10);
//     }

//     delete[] buffer;
//     Serial.println("Playback finished.");
// }

// void setup() {
//     Serial.begin(115200);
//     auto cfg = M5.config();
//     M5.begin(cfg);

//     M5.Speaker.setVolume(255);

//     SPI.begin(SD_SPI_SCK_PIN, SD_SPI_MISO_PIN, SD_SPI_MOSI_PIN, SD_SPI_CS_PIN);
//     if (!SD.begin(SD_SPI_CS_PIN, SPI, 25000000)) {
//         Serial.println("SD card failed!");
//         M5.Display.println("SD FAILED");
//     } else {
//         Serial.println("SD card OK.");
//     }

//     drawUI();
// }

// void loop() {
//     M5.update();

//     if (isRecording) {
//         int16_t buf[RECORD_LENGTH];
//         // record() blocks until the buffer is full
//         if (M5.Mic.record(buf, RECORD_LENGTH, SAMPLE_RATE, true)) {
//             // Amplify signal x8 - Core2 mic is very quiet
//             for (int i = 0; i < RECORD_LENGTH; i++) {
//                 int32_t amplified = (int32_t)buf[i] * 8;
//                 if (amplified > 32767)  amplified = 32767;
//                 if (amplified < -32768) amplified = -32768;
//                 buf[i] = (int16_t)amplified;
//             }

//             file.write((uint8_t*)buf, RECORD_LENGTH * 2);
//             totalBytes += RECORD_LENGTH * 2;

//             // Debug every ~1 second
//             static int debugCount = 0;
//             if (++debugCount % 64 == 0) {
//                 int16_t minVal = 32767, maxVal = -32768;
//                 for (int i = 0; i < RECORD_LENGTH; i++) {
//                     if (buf[i] < minVal) minVal = buf[i];
//                     if (buf[i] > maxVal) maxVal = buf[i];
//                 }
//                 Serial.printf("Mic - min: %d, max: %d, range: %d\n",
//                               minVal, maxVal, maxVal - minVal);
//             }
//         }
//     }

//     auto t = M5.Touch.getDetail();
//     if (t.wasPressed()) {
//         int x = t.x;
//         int y = t.y;

//         if (x > REC_X && x < REC_X + REC_W &&
//             y > REC_Y && y < REC_Y + REC_H) {
//             if (!isRecording) startRecording();
//             else stopRecording();
//         }

//         if (x > PLAY_X && x < PLAY_X + PLAY_W &&
//             y > PLAY_Y && y < PLAY_Y + PLAY_H) {
//             if (!isRecording) playAudio();
//         }
//     }
// }