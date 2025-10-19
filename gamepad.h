// =========================================================
//  RowBoy Firmware Prototype v1.0 (ESP32-S3)
//  ---------------------------------------------------------
//  gamepad.h — Bluepad32 Controller Interface
//
//  Provides:
//   • Lightweight GamepadState snapshot (axes, buttons, dpad)
//   • Core API: setupGamepad(), updateGamepad()
//   • Weak hooks for MenuUI & InputMapper integration
//
//  Notes:
//   - Bluepad32 supports Xbox, PS, Switch, etc. but your mapping might be different!
//   - Long-hold BTN_PIN enters pairing mode (see gamepad.cpp)
//   - D-Pad bits: bit0=Up, bit1=Down, bit2=Right, bit3=Left
//   - TODO: Add in-UI calibration (especially for analog sticks)
// =========================================================

#pragma once
#include <Arduino.h>

// =========================================================
//  GAMEPAD STATE SNAPSHOT
// =========================================================
// A minimal runtime structure for current controller readings.
// Updated each frame by updateGamepad() and consumed by InputMapper.
struct GamepadState {
  bool connected = false;

  // --- Axes ---
  int16_t lx = 0;   // Left stick X
  int16_t ly = 0;   // Left stick Y
  int16_t rx = 0;   // Right stick X
  int16_t ry = 0;   // Right stick Y

  // --- D-Pad ---
  // Bitmask: bit0=Up, bit1=Down, bit2=Right, bit3=Left
  uint8_t dpad = 0;

  // --- Buttons ---
  bool a = false;
  bool b = false;
  bool x = false;
  bool y = false;

  bool start = false;
  bool select = false;
};


// =========================================================
//  PUBLIC API
// =========================================================
void setupGamepad();   // Initialize controller subsystem + pairing LED
void updateGamepad();  // Poll state + handle pairing logic

// Optional direct access to latest state
const GamepadState& getGamepadState();


// =========================================================
//  WEAK HOOKS (Queried by MenuUI / InputMapper)
// =========================================================
// These can be overridden by user code if needed.
// Each returns the latest sampled controller input.
// TODO: In-UI calibration for analog deadzones and D-pad mapping.
bool gamepadConnected();

int16_t gpLX();
int16_t gpLY();
int16_t gpRX();
int16_t gpRY();

bool gpUp();
bool gpDown();
bool gpLeft();
bool gpRight();

bool gpA();
bool gpB();
bool gpX();
bool gpY();

bool gpStart();
bool gpSelect();

// ======================= End of File =======================