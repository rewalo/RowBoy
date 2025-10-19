/*
  ============================================================
   RowBoy Firmware Prototype v1.0 (ESP32-S3)
   ------------------------------------------------------------
   Global Configuration File

   This is the single source of truth for all build-time
   configuration: pins, layout, fonts, colors, animations,
   debug options, and input timing. Every module in the
   RowBoy firmware reads from here.

   Overview
   -----------------
   • Orientation:  HORIZONTAL (DS-style carousel) or VERTICAL (classic list)
   • Fonts:        Pick TFT_eSPI built-in font IDs, or use smooth fonts
   • Animations:   Enable, style, duration, and easing strength
   • Debug:        Toggle Serial + on-screen output per feature-group
   • IO Pins:      TFT, SD, LED, Buttons, Encoders
   • Input:        Deadzones and repeat timing live here too

   Notes:
   - If using FreeFonts / smooth fonts, load them in your sketch and
     set menu theme font IDs accordingly (e.g., “2” or your chosen ID).
   - This file only provides the constants; MenuUI applies them.
  ============================================================
*/
#pragma once
#include <Arduino.h>

// ============================================================
//  HARDWARE PINS
// ============================================================
#define SD_CS      10
#define TFT_CS     9
#define TFT_MOSI   42
#define TFT_SCLK   2
#define TFT_MISO   38
#define LED_PIN    4
#define BTN_PIN    5

// TFT rotation (0–3). HORIZONTAL layout looks best with 3 or 1.
#define SCREEN_ROTATION 3


// ============================================================
//  BACKLIGHT CONTROL
// ============================================================
#define TFT_BL      1
#define BL_CHANNEL  7

extern int brightnessValue;
void setBrightness(int val);


// ============================================================
//  DEBUG + LOGGING SETTINGS
// ============================================================
namespace Debug {

  // --- Master Switches ---
  static constexpr bool SERIAL_EN = true;   // Enable Serial output
  static constexpr bool ONSCREEN  = false;  // Tiny corner overlay (FPS/logs)

  // --- Feature Group Flags ---
  // Enable/disable verbose logs for subsystems
  static constexpr bool MENU_LOGS    = true;   // Menu lifecycle
  static constexpr bool INPUT_LOGS   = false;  // Button / axis states
  static constexpr bool GAMEPAD_LOGS = true;   // Controller connect/pair
  static constexpr bool SD_LOGS      = true;   // SD mount/listing
}

// Debug macro — clean conditional wrapper for group logs
#define DBG_IF(grp, ...) do { \
  if (Debug::SERIAL_EN && Debug::grp##_LOGS) { \
    Serial.printf(__VA_ARGS__); \
  } \
} while (0)


// ============================================================
//  MENU DEFAULTS
// ============================================================

// --- Orientation ---
enum class MenuOrientation : uint8_t { HORIZONTAL, VERTICAL };
static constexpr MenuOrientation MENU_ORIENTATION_DEFAULT = MenuOrientation::HORIZONTAL;

// --- Icons ---
static constexpr bool MENU_SHOW_ICONS_DEFAULT = false;

// --- Fonts ---
// TFT_eSPI built-in IDs; replace with custom IDs if loading FreeFonts.
static constexpr uint8_t MENU_TEXT_FONT_ID  = 2;
static constexpr uint8_t MENU_VALUE_FONT_ID = 2;

// --- Layout ---
static constexpr int16_t MENU_MARGIN_L        = 10;
static constexpr int16_t MENU_MARGIN_R        = 10;
static constexpr int16_t MENU_MARGIN_T        = 10;
static constexpr int16_t MENU_MARGIN_B        = 10;
static constexpr int16_t MENU_ROW_H           = 36;
static constexpr int16_t MENU_ICON_PAD        = 8;
static constexpr int16_t MENU_TEXT_PAD        = 10;
static constexpr int16_t MENU_SELECTOR_RADIUS = 8;
static constexpr int16_t MENU_SELECTOR_BORDER = 2;


// ============================================================
//  COLOR PALETTE (RGB565)
// ============================================================
// Helper to encode RGB888 → RGB565 inline
static constexpr uint16_t rgb(uint8_t r, uint8_t g, uint8_t b) {
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

// --- Theme Colors ---
static constexpr uint16_t COL_BG        = rgb(10, 11, 16);
static constexpr uint16_t COL_FG        = rgb(230, 230, 235);
static constexpr uint16_t COL_MUTED     = rgb(150, 150, 160);
static constexpr uint16_t COL_SEL_FILL  = rgb(30, 90, 200);
static constexpr uint16_t COL_SEL_BORD  = rgb(255, 255, 255);
static constexpr uint16_t COL_DISABLED  = rgb(100, 100, 110);
static constexpr uint16_t COL_ARROW     = rgb(180, 180, 190);
static constexpr uint16_t COL_MONO_TINT = rgb(230, 230, 235);


// ============================================================
//  ANIMATIONS
// ============================================================
// Simple enum + defaults for menu transitions
enum class TransitionStyle : uint8_t { NONE, SLIDE, FADE, SLIDE_FADE };

static constexpr bool            ANIM_ENABLE        = true;
static constexpr TransitionStyle PAGE_TRANSITION    = TransitionStyle::SLIDE;
static constexpr uint16_t        ANIM_PAGE_MS       = 180; // Transition duration (ms)
static constexpr uint8_t         ANIM_EASE_STRENGTH = 2;   // 1=linear, 2–3=eased


// ============================================================
//  INPUT TIMING / DEADBAND
// ============================================================
// Used by MenuUI to handle joystick + key repeat behavior.

static constexpr int       DEADZONE           = 200;  // Analog deadzone
static constexpr uint16_t  REPEAT_INITIAL_MS  = 400;  // First repeat delay
static constexpr uint16_t  REPEAT_HOLD_MS     = 220;  // Slow hold rate
static constexpr uint16_t  REPEAT_FAST_MS     = 120;  // Fast hold rate
static constexpr uint16_t  REPEAT_AFTER_MS    = 800;  // Threshold for fast repeat


// ============================================================
//  GAMEPAD (Bluepad32) PAIRING + LED FEEDBACK
// ============================================================
#define HOLD_TIME_MS     3000
#define FLASH_TIME_MS    30000
#define BLINK_PERIOD_MS  250
#define DEBOUNCE_MS      50


// ============================================================
//  OPTIONAL MECHANICAL INPUTS
// ============================================================
// Leave unused pins as -1; MenuUI will safely ignore them.
#define MENU_BTN_UP_PIN      -1
#define MENU_BTN_DOWN_PIN    -1
#define MENU_BTN_OK_PIN      -1
#define MENU_BTN_BACK_PIN    -1
#define MENU_BTN_START_PIN   -1
#define MENU_BTN_SELECT_PIN  -1
#define MENU_ENC_A_PIN       -1
#define MENU_ENC_B_PIN       -1
#define MENU_ENC_BTN_PIN     -1

// ======================= End of File =======================