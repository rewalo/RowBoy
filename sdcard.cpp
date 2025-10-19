// =========================================================
//  RowBoy Firmware Prototype v1.0 (ESP32-S3)
//  ---------------------------------------------------------
//  sdcard.cpp — SD Card Mount & Filesystem Utilities
//
//  Provides:
//   • SPI-based SD mount via shared TFT bus
//   • Safe CS toggling (prevents draw interference)
//   • Recursive directory listing (for debug)
//   • Optional serial logging (toggled in config.h)
//
//  Notes:
//   - TFT and SD share the same SPI bus — so TFT_CS must
//     be raised during SD access to avoid ghost drawing.
//   - Mount speed: 10 MHz; increase cautiously.
//   - Be kind to your flash: close files properly!
// =========================================================

#include "sdcard.h"
#include "config.h"

// =========================================================
//  DIRECTORY LISTING (recursive)
// =========================================================
// Recursively lists directory contents up to `levels` deep.
// Used mostly for debugging SD mounts and verifying structure.
void listDir(fs::FS &fs, const char *dirname, uint8_t levels) {
  File root = fs.open(dirname);
  if (!root || !root.isDirectory()) return;

  File file = root.openNextFile();
  while (file) {
    if (file.isDirectory()) {
      if (Debug::SERIAL_EN && Debug::SD_LOGS)
        Serial.printf("DIR : %s\n", file.name());
      if (levels) listDir(fs, file.path(), levels - 1);
    } else {
      if (Debug::SERIAL_EN && Debug::SD_LOGS)
        Serial.printf("FILE: %s  SIZE: %u\n", file.name(), file.size());
    }
    file = root.openNextFile();
  }
}


// =========================================================
//  SD SETUP
// =========================================================
// Initializes the SD interface on HSPI and safely toggles TFT_CS.
// Automatically logs card stats and contents if enabled.
void setupSD() {
  // Disable TFT during SPI mount
  pinMode(TFT_CS, OUTPUT);
  digitalWrite(TFT_CS, HIGH);

  static SPIClass hspi(HSPI);
  hspi.begin(TFT_SCLK, TFT_MISO, TFT_MOSI, SD_CS);

  if (Debug::SERIAL_EN && Debug::SD_LOGS)
    Serial.print("[SD] Mounting... ");

  if (!SD.begin(SD_CS, hspi, 10000000)) {
    if (Debug::SERIAL_EN && Debug::SD_LOGS)
      Serial.println("FAILED (check wiring or CS conflict)");
    digitalWrite(TFT_CS, LOW);
    return;
  }

  if (Debug::SERIAL_EN && Debug::SD_LOGS)
    Serial.println("OK");

  // Re-enable TFT for drawing
  digitalWrite(TFT_CS, LOW);

  // Optional card info
  uint64_t cardSize   = SD.cardSize() / (1024 * 1024);
  uint64_t totalBytes = SD.totalBytes() / (1024 * 1024);
  uint64_t usedBytes  = SD.usedBytes() / (1024 * 1024);

  if (Debug::SERIAL_EN && Debug::SD_LOGS)
    Serial.printf("[SD] Card: %lluMB  Total: %lluMB  Used: %lluMB\n",
                  cardSize, totalBytes, usedBytes);

  // Shallow file tree dump for verification
  if (Debug::SERIAL_EN && Debug::SD_LOGS) {
    Serial.println("[SD] Files @/ (depth 1):");
    listDir(SD, "/", 1);
  }
}

// ======================= End of File =======================