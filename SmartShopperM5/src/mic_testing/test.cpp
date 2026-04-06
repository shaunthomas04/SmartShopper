// #include <M5Unified.h>
// #include <SPI.h>
// #include "FS.h"
// #include "SD.h"

// #define SERIAL_RX_BUFFER_SIZE 4096

// #define SD_SPI_CS_PIN   4
// #define SD_SPI_SCK_PIN  18
// #define SD_SPI_MISO_PIN 38
// #define SD_SPI_MOSI_PIN 23

// const char* SAVE_AS = "/laptop.wav";

// void setup() {
//     Serial.begin(115200);
//     auto cfg = M5.config();
//     M5.begin(cfg);

//     M5.Display.fillScreen(BLACK);
//     M5.Display.setTextColor(WHITE);
//     M5.Display.setCursor(10, 10);
//     M5.Display.println("Waiting for file...");

//     SPI.begin(SD_SPI_SCK_PIN, SD_SPI_MISO_PIN, SD_SPI_MOSI_PIN, SD_SPI_CS_PIN);

//     if (!SD.begin(SD_SPI_CS_PIN, SPI, 25000000)) {
//         Serial.println("SD FAILED");
//         M5.Display.println("SD FAILED");
//         return;
//     }

//     Serial.println("SD OK. Waiting for transfer...");

//     while (true) {
//         if (Serial.available()) {
//             String line = Serial.readStringUntil('\n');
//             line.trim();
//             if (line.startsWith("START:")) {
//                 int filesize = line.substring(6).toInt();
//                 Serial.println("READY");

//                 M5.Display.printf("Receiving\n%d bytes\n", filesize);

//                 SD.remove(SAVE_AS);
//                 File f = SD.open(SAVE_AS, FILE_WRITE);
//                 if (!f) {
//                     Serial.println("ERROR: Could not open file");
//                     M5.Display.println("FILE OPEN FAILED");
//                     return;
//                 }

//                 int received = 0;
//                 const int CHUNK = 256;
//                 uint8_t buf[CHUNK];

//                 while (received < filesize) {
//                     // Wait for a full chunk or remaining bytes
//                     int toRead = min(CHUNK, filesize - received);
//                     int bytesRead = Serial.readBytes(buf, toRead);

//                     if (bytesRead == 0) {
//                         Serial.println("ERROR: Timeout");
//                         M5.Display.println("TIMEOUT");
//                         f.close();
//                         return;
//                     }

//                     f.write(buf, bytesRead);
//                     received += bytesRead;

//                     // Send ACK so Python can send next chunk
//                     Serial.println("ACK");

//                     // Update display every 10KB
//                     if (received % 10240 < CHUNK) {
//                         M5.Display.fillScreen(BLACK);
//                         M5.Display.setCursor(10, 10);
//                         M5.Display.printf("Receiving...\n%d / %d\n%.0f%%",
//                             received, filesize,
//                             (float)received / filesize * 100);
//                     }
//                 }

//                 f.close();
//                 Serial.println("DONE");

//                 M5.Display.fillScreen(BLACK);
//                 M5.Display.setCursor(10, 10);
//                 M5.Display.printf("Saved!\n%s\n%d bytes", SAVE_AS, received);
//                 return;
//             }
//         }
//     }
// }

// void loop() {}