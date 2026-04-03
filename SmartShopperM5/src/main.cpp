#include <M5Unified.h>
#include <SPI.h>
#include "FS.h"
#include "SD.h"
#include "driver/i2s.h"

#define SAMPLE_RATE 16000
#define BUFFER_SIZE 1024

#define SD_SPI_CS_PIN   4
#define SD_SPI_SCK_PIN  18
#define SD_SPI_MISO_PIN 38
#define SD_SPI_MOSI_PIN 23

#define MIC_CLK_PIN  33
#define MIC_DATA_PIN 32

bool isRecording = false;
File file;
int totalBytes = 0;

#define REC_X 40
#define REC_Y 180
#define REC_W 100
#define REC_H 50

#define PLAY_X 180
#define PLAY_Y 180
#define PLAY_W 100
#define PLAY_H 50

void drawUI() {
    M5.Display.fillScreen(BLACK);

    M5.Display.fillRect(REC_X, REC_Y, REC_W, REC_H, isRecording ? DARKGREEN : RED);
    M5.Display.setCursor(REC_X + 20, REC_Y + 15);
    M5.Display.setTextColor(WHITE);
    M5.Display.print(isRecording ? "STOP" : "REC");

    M5.Display.fillRect(PLAY_X, PLAY_Y, PLAY_W, PLAY_H, BLUE);
    M5.Display.setCursor(PLAY_X + 20, PLAY_Y + 15);
    M5.Display.print("PLAY");
}

void setupI2S_Mic() {
    // CRITICAL: tell M5Unified to release I2S before we use it
    M5.Speaker.end();
    delay(100);
    i2s_driver_uninstall(I2S_NUM_0);
    delay(100);

    i2s_config_t config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_PDM),
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 8,
        .dma_buf_len = BUFFER_SIZE,
        .use_apll = false,
        .tx_desc_auto_clear = false,
        .fixed_mclk = 0
    };

    i2s_pin_config_t pin_config = {
        .bck_io_num   = I2S_PIN_NO_CHANGE,
        .ws_io_num    = MIC_CLK_PIN,
        .data_out_num = I2S_PIN_NO_CHANGE,
        .data_in_num  = MIC_DATA_PIN
    };

    i2s_driver_install(I2S_NUM_0, &config, 0, NULL);
    i2s_set_pin(I2S_NUM_0, &pin_config);
    i2s_set_clk(I2S_NUM_0, SAMPLE_RATE, I2S_BITS_PER_SAMPLE_16BIT, I2S_CHANNEL_MONO);

    // Discard first 200ms of garbage
    int16_t throwaway[BUFFER_SIZE];
    size_t bytesRead = 0;
    int discardBytes = (SAMPLE_RATE * 2 * 200) / 1000;
    int discarded = 0;
    while (discarded < discardBytes) {
        i2s_read(I2S_NUM_0, throwaway, sizeof(throwaway), &bytesRead, portMAX_DELAY);
        discarded += bytesRead;
    }

    Serial.println("I2S PDM mic initialized.");
}

void writeWavHeader(File &f, int sampleRate, int dataSize) {
    byte header[44] = {0};
    int fileSize = dataSize + 36;

    memcpy(header, "RIFF", 4);
    header[4] = fileSize & 0xff;
    header[5] = (fileSize >> 8) & 0xff;
    header[6] = (fileSize >> 16) & 0xff;
    header[7] = (fileSize >> 24) & 0xff;

    memcpy(header + 8, "WAVEfmt ", 8);
    header[16] = 16;
    header[20] = 1;
    header[22] = 1;

    header[24] = sampleRate & 0xff;
    header[25] = (sampleRate >> 8) & 0xff;
    header[26] = (sampleRate >> 16) & 0xff;
    header[27] = (sampleRate >> 24) & 0xff;

    int byteRate = sampleRate * 2;
    header[28] = byteRate & 0xff;
    header[29] = (byteRate >> 8) & 0xff;
    header[30] = (byteRate >> 16) & 0xff;
    header[31] = (byteRate >> 24) & 0xff;

    header[32] = 2;
    header[34] = 16;

    memcpy(header + 36, "data", 4);
    header[40] = dataSize & 0xff;
    header[41] = (dataSize >> 8) & 0xff;
    header[42] = (dataSize >> 16) & 0xff;
    header[43] = (dataSize >> 24) & 0xff;

    f.write(header, 44);
}

void startRecording() {
    Serial.println("Start recording...");
    setupI2S_Mic();

    SD.remove("/record.wav");
    file = SD.open("/record.wav", FILE_WRITE);

    if (!file) {
        Serial.println("Failed to open file for recording!");
        return;
    }

    for (int i = 0; i < 44; i++) file.write((byte)0);
    totalBytes = 0;
    isRecording = true;
    drawUI();
}

void stopRecording() {
    Serial.println("Stop recording...");
    if (!file) return;

    file.seek(0);
    writeWavHeader(file, SAMPLE_RATE, totalBytes);
    file.close();

    i2s_driver_uninstall(I2S_NUM_0);
    delay(100);
    M5.Speaker.begin();
    delay(100);

    isRecording = false;
    drawUI();

    Serial.print("Recording saved. Total bytes: ");
    Serial.println(totalBytes);
}

void playAudio() {
    Serial.println("Play pressed.");
    if (!SD.exists("/record.wav")) {
        Serial.println("File does not exist!");
        return;
    }

    File f = SD.open("/record.wav", FILE_READ);
    if (!f) {
        Serial.println("Failed to open file for playback!");
        return;
    }

    size_t fileSize = f.size();
    Serial.print("File size: ");
    Serial.println(fileSize);

    uint8_t* buffer = new uint8_t[fileSize];
    f.read(buffer, fileSize);
    f.close();

    Serial.println("Starting playback...");
    M5.Speaker.setVolume(255);
    M5.Speaker.playWav(buffer, fileSize);

    while (M5.Speaker.isPlaying()) {
        M5.update();
        delay(10);
    }

    Serial.println("Playback finished.");
    delete[] buffer;
    Serial.println("Buffer freed.");
}

void setup() {
    Serial.begin(115200);
    auto cfg = M5.config();
    M5.begin(cfg);

    SPI.begin(SD_SPI_SCK_PIN, SD_SPI_MISO_PIN, SD_SPI_MOSI_PIN, SD_SPI_CS_PIN);

    if (!SD.begin(SD_SPI_CS_PIN, SPI, 25000000)) {
        Serial.println("SD card failed!");
        M5.Display.println("SD FAILED");
    } else {
        Serial.println("SD card OK.");
    }

    drawUI();
}

void loop() {
    M5.update();

    auto t = M5.Touch.getDetail();

    if (t.wasPressed()) {
        int x = t.x;
        int y = t.y;

        if (x > REC_X && x < REC_X + REC_W &&
            y > REC_Y && y < REC_Y + REC_H) {
            if (!isRecording) startRecording();
            else stopRecording();
        }

        if (x > PLAY_X && x < PLAY_X + PLAY_W &&
            y > PLAY_Y && y < PLAY_Y + PLAY_H) {
            if (!isRecording) playAudio();
        }
    }

    if (isRecording) {
        int16_t buf[BUFFER_SIZE];
        size_t bytesRead = 0;
        i2s_read(I2S_NUM_0, buf, sizeof(buf), &bytesRead, portMAX_DELAY);
        if (bytesRead > 0 && file) {
            file.write((uint8_t*)buf, bytesRead);
            totalBytes += bytesRead;

            // Debug: print min/max every ~1 second
            static int debugCount = 0;
            if (++debugCount % 16 == 0) {
                int16_t minVal = 32767, maxVal = -32768;
                for (size_t i = 0; i < bytesRead / 2; i++) {
                    if (buf[i] < minVal) minVal = buf[i];
                    if (buf[i] > maxVal) maxVal = buf[i];
                }
                Serial.printf("Mic - min: %d, max: %d, range: %d\n",
                              minVal, maxVal, maxVal - minVal);
            }
        }
    }
}