// =========================================================
//  RowBoy Firmware Prototype v1.0 (ESP32-S3)
//  ---------------------------------------------------------
//  controls.cpp — Unified Input Abstraction Layer
//
//  Handles all control sources (Gamepad, Mechanical, Touch)
//  and normalizes them into a shared ControlState used by
//  MenuUI and other RowBoy modules.
//
//  Provides:
//   • Gamepad button / axis mapping
//   • Optional mechanical button fallback
//   • Optional touch-tap support
//
//  Notes:
//   - Gamepad functions (gpA(), gpLX(), etc.) are weakly linked
//     and can be overridden in your project.
//   - For mechanical buttons, define pins in config.h.
//   - Touch mode simply uses a tap = confirm event.
// =========================================================

#include "controls.h"

// Global instance (shared everywhere)
InputMapper controls;


// =========================================================
//  MAIN UPDATE ENTRYPOINT
// =========================================================
// Reads the appropriate input source based on the active mode
// and updates the unified ControlState.
void InputMapper::update(InputMode mode) {
  bool prevConfirm = _s.confirm;
  bool prevBack    = _s.back;

  // Reset state but preserve "last" flags
  _s = ControlState{};
  _s.confirmLast = prevConfirm;
  _s.backLast    = prevBack;

  switch (mode) {
    case InputMode::GAMEPAD: _readGamepad(); break;
    case InputMode::MECH:    _readMechanical(); break;
    case InputMode::TOUCH:   _readTouch(); break;
  }
}


// =========================================================
//  GAMEPAD INPUT
// =========================================================
// Reads directional axes + button states via weak hooks.
// The lambda `btnState()` abstracts ID mapping for easy remap.
void InputMapper::_readGamepad() {
  if (!gamepadConnected()) return;
  const int16_t dz = 200; // fixed deadzone; matches config default

  _s.up    = gpUp()    || (gpLY() < -dz);
  _s.down  = gpDown()  || (gpLY() >  dz);
  _s.left  = gpLeft()  || (gpLX() < -dz);
  _s.right = gpRight() || (gpLX() >  dz);

  auto btnState = [&](ButtonID id) {
    switch (id) {
      case ButtonID::A:      return gpA();
      case ButtonID::B:      return gpB();
      case ButtonID::X:      return gpX();
      case ButtonID::Y:      return gpY();
      case ButtonID::START:  return gpStart();
      case ButtonID::SELECT: return gpSelect();
      default:               return false;
    }
  };

  _s.confirm = btnState(_map.confirm);
  _s.back    = btnState(_map.back);
  _s.menu    = btnState(_map.menu);
  _s.alt     = btnState(_map.alt);
  _s.start   = gpStart();
  _s.select  = gpSelect();
}


// =========================================================
//  MECHANICAL INPUT
// =========================================================
// Reads discrete GPIO buttons (LOW = pressed).
// Configured via config.h; ignored if pin == -1.
void InputMapper::_readMechanical() {
  if (MENU_BTN_UP_PIN     >= 0) _s.up      = (digitalRead(MENU_BTN_UP_PIN)     == LOW);
  if (MENU_BTN_DOWN_PIN   >= 0) _s.down    = (digitalRead(MENU_BTN_DOWN_PIN)   == LOW);
  if (MENU_BTN_OK_PIN     >= 0) _s.confirm = (digitalRead(MENU_BTN_OK_PIN)     == LOW);
  if (MENU_BTN_BACK_PIN   >= 0) _s.back    = (digitalRead(MENU_BTN_BACK_PIN)   == LOW);
  if (MENU_BTN_START_PIN  >= 0) _s.start   = (digitalRead(MENU_BTN_START_PIN)  == LOW);
  if (MENU_BTN_SELECT_PIN >= 0) _s.select  = (digitalRead(MENU_BTN_SELECT_PIN) == LOW);

  // Encoder button as confirm
  if (MENU_ENC_BTN_PIN >= 0 && digitalRead(MENU_ENC_BTN_PIN) == LOW)
    _s.confirm = true;
}


// =========================================================
//  TOUCH INPUT
// =========================================================
// Minimal tap detection; relies on external menuGetTouch().
// Future: could add gesture / swipe recognition here.
void InputMapper::_readTouch() {
  int x, y; bool tap;
  if (menuGetTouch(x, y, tap)) {
    _s.confirm = tap;
  }
}

// ======================= End of File =======================