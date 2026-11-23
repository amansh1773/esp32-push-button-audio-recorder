#include <SPI.h>
#include <SD.h>
#include <driver/i2s.h>

// Silkscreen D-labels mapped to ESP32 GPIOs
static const int PIN_BTN_D4    = 4;   // D4 -> Button -> GND (INPUT_PULLUP)
static const int PIN_SD_CS_D5  = 5;   // D5 -> MicroSD CS

static const int PIN_I2S_WS_D25  = 25; // D25 -> INMP441 WS (LRC)
static const int PIN_I2S_SCK_D26 = 26; // D26 -> INMP441 SCK (BCLK)
static const int PIN_I2S_SD_D33  = 33; // D33 -> INMP441 SD (DOUT)

static const i2s_port_t I2S_PORT = I2S_NUM_0;
static const uint32_t SAMPLE_RATE = 16000;

File audioFile;
bool isRecording = false;
uint32_t pcmBytesWritten = 0;

// Debounce state
int lastRawBtn = HIGH;
int stableBtn = HIGH;
unsigned long lastDebounceMs = 0;
const unsigned long DEBOUNCE_MS = 40;

void writeWavHeader(File &f, uint32_t sampleRate, uint16_t bitsPerSample, uint16_t channels, uint32_t dataBytes) {
  uint32_t byteRate = sampleRate * channels * (bitsPerSample / 8);
  uint16_t blockAlign = channels * (bitsPerSample / 8);
  uint32_t chunkSize = 36 + dataBytes;

  uint8_t hdr[44] = {
    'R', 'I', 'F', 'F',
    (uint8_t)(chunkSize & 0xFF), (uint8_t)((chunkSize >> 8) & 0xFF), (uint8_t)((chunkSize >> 16) & 0xFF), (uint8_t)((chunkSize >> 24) & 0xFF),
    'W', 'A', 'V', 'E',
    'f', 'm', 't', ' ',
    16, 0, 0, 0, // PCM chunk size
    1, 0,        // Audio format = 1 (PCM)
    (uint8_t)(channels & 0xFF), (uint8_t)((channels >> 8) & 0xFF),
    (uint8_t)(sampleRate & 0xFF), (uint8_t)((sampleRate >> 8) & 0xFF), (uint8_t)((sampleRate >> 16) & 0xFF), (uint8_t)((sampleRate >> 24) & 0xFF),
    (uint8_t)(byteRate & 0xFF), (uint8_t)((byteRate >> 8) & 0xFF), (uint8_t)((byteRate >> 16) & 0xFF), (uint8_t)((byteRate >> 24) & 0xFF),
    (uint8_t)(blockAlign & 0xFF), (uint8_t)((blockAlign >> 8) & 0xFF),
    (uint8_t)(bitsPerSample & 0xFF), (uint8_t)((bitsPerSample >> 8) & 0xFF),
    'd', 'a', 't', 'a',
    (uint8_t)(dataBytes & 0xFF), (uint8_t)((dataBytes >> 8) & 0xFF), (uint8_t)((dataBytes >> 16) & 0xFF), (uint8_t)((dataBytes >> 24) & 0xFF)
  };

  f.seek(0);
  f.write(hdr, sizeof(hdr));
}

bool setupI2S() {
  i2s_config_t cfg = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 6,
    .dma_buf_len = 256,
    .use_apll = false,
    .tx_desc_auto_clear = false,
    .fixed_mclk = 0
  };

  i2s_pin_config_t pins = {
    .bck_io_num = PIN_I2S_SCK_D26,
    .ws_io_num = PIN_I2S_WS_D25,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = PIN_I2S_SD_D33
  };

  if (i2s_driver_install(I2S_PORT, &cfg, 0, nullptr) != ESP_OK) return false;
  if (i2s_set_pin(I2S_PORT, &pins) != ESP_OK) return false;
  return true;
}

String nextFilename() {
  for (int i = 1; i < 10000; i++) {
    char name[24];
    snprintf(name, sizeof(name), "/rec_%04d.wav", i);
    if (!SD.exists(name)) return String(name);
  }
  return "/rec_overflow.wav";
}

void startRecording() {
  String path = nextFilename();
  audioFile = SD.open(path.c_str(), FILE_WRITE);
  if (!audioFile) {
    Serial.println("Failed to open file on SD card.");
    return;
  }

  uint8_t blankHeader[44] = {0};
  audioFile.write(blankHeader, sizeof(blankHeader));
  pcmBytesWritten = 0;
  isRecording = true;

  Serial.print("Recording started -> ");
  Serial.println(path);
}

void stopRecording() {
  if (!isRecording) return;
  isRecording = false;

  writeWavHeader(audioFile, SAMPLE_RATE, 16, 1, pcmBytesWritten);
  audioFile.flush();
  audioFile.close();

  Serial.print("Recording stopped. Bytes written: ");
  Serial.println(pcmBytesWritten);
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_BTN_D4, INPUT_PULLUP);

  if (!SD.begin(PIN_SD_CS_D5)) {
    Serial.println("SD card init failed. Check wiring and 3V3/GND.");
  } else {
    Serial.println("SD card ready.");
  }

  if (!setupI2S()) {
    Serial.println("I2S init failed.");
  } else {
    Serial.println("I2S mic ready. Press D4 button to start/stop.");
  }
}

void loop() {
  int raw = digitalRead(PIN_BTN_D4);
  if (raw != lastRawBtn) {
    lastDebounceMs = millis();
    lastRawBtn = raw;
  }

  if ((millis() - lastDebounceMs) > DEBOUNCE_MS && raw != stableBtn) {
    stableBtn = raw;
    if (stableBtn == LOW) { // Button pressed (pulled to GND)
      if (!isRecording) {
        startRecording();
      } else {
        stopRecording();
      }
    }
  }

  if (isRecording && audioFile) {
    int32_t rawSamples[128];
    size_t bytesRead = 0;
    if (i2s_read(I2S_PORT, rawSamples, sizeof(rawSamples), &bytesRead, portMAX_DELAY) == ESP_OK) {
      int count = bytesRead / sizeof(int32_t);
      int16_t pcm16[128];
      for (int i = 0; i < count; i++) {
        pcm16[i] = (int16_t)(rawSamples[i] >> 16);
      }
      size_t outBytes = count * sizeof(int16_t);
      audioFile.write((const uint8_t *)pcm16, outBytes);
      pcmBytesWritten += outBytes;
    }
  }
}