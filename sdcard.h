// =========================================================
//  RowBoy Firmware Prototype v1.0 (ESP32-S3)
//  ---------------------------------------------------------
//  sdcard.h — SPI SD Card Interface (Header)
//
//  Provides:
//   • setupSD()  — Mounts SD card over shared SPI bus
//   • listDir()  — Recursive directory listing utility
// =========================================================

#pragma once
#include <Arduino.h>
#include <SPI.h>
#include <SD.h>

// =========================================================
//  PUBLIC API
// =========================================================

// Initializes SD card on HSPI and logs stats if enabled.
void setupSD();

// Recursively lists directory contents up to `levels` deep.
// Primarily used for debugging SD mounts or verifying files.
void listDir(fs::FS& fs, const char* dirname, uint8_t levels);

// ======================= End of File =======================