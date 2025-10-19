// =========================================================
//  RowBoy Firmware Prototype v1.0 (ESP32-S3)
//  ---------------------------------------------------------
//  controls.h — Unified Input Abstraction Layer (Header)
//
//  Defines:
//   • InputMapper — handles all input sources (gamepad, mech, touch)
//   • ControlState — shared button/axis state struct
//   • ButtonID — logical mapping for rebinding support
//
//  Notes:
//   - All reads go through `InputMapper::update()`
//   - Pins for mechanical buttons are defined in config.h.
// =========================================================

#pragma once
#include <Arduino.h>
#include "MenuUI.h"

// =========================================================
//  INPUT MAPPER CLASS
// =========================================================
class InputMapper {
public:
  // ---------------------------------------------------------
  // Logical identifiers for rebinding
  // ---------------------------------------------------------
  enum class ButtonID { A, B, X, Y, START, SELECT, NONE };

  // ---------------------------------------------------------
  // Unified control snapshot (shared across all modes)
  // ---------------------------------------------------------
  struct ControlState {
    bool up = false, down = false, left = false, right = false;
    bool confirm = false, back = false, menu = false, alt = false;
    bool start = false, select = false;

    // Edge tracking & consumption flags
    bool confirmLast = false, backLast = false;
    bool confirmConsumed = false, backConsumed = false;
  };

  // ---------------------------------------------------------
  // Constructor — sets default button bindings
  // ---------------------------------------------------------
  InputMapper() {
    _map.confirm = ButtonID::A;
    _map.back    = ButtonID::B;
    _map.menu    = ButtonID::START;
    _map.alt     = ButtonID::SELECT;
  }

  // ---------------------------------------------------------
  // Update all inputs for the given mode (call once per frame)
  // ---------------------------------------------------------
  void update(InputMode mode);

  // ---------------------------------------------------------
  // State Accessors
  // ---------------------------------------------------------
  bool up() const     { return _s.up; }
  bool down() const   { return _s.down; }
  bool left() const   { return _s.left; }
  bool right() const  { return _s.right; }
  bool start() const  { return _s.start; }
  bool select() const { return _s.select; }

  // ---------------------------------------------------------
  // Edge-detect helpers (trigger once on press)
  // ---------------------------------------------------------
  bool confirmPressed() const { return _s.confirm && !_s.confirmLast && !_s.confirmConsumed; }
  bool backPressed() const    { return _s.back    && !_s.backLast    && !_s.backConsumed; }

  void consumeBack() const    { _s.backConsumed = true; }
  void consumeConfirm() const { _s.confirmConsumed = true; }

  // ---------------------------------------------------------
  // Rebinding (useful for custom layouts)
  // ---------------------------------------------------------
  void rebindConfirm(ButtonID id) { _map.confirm = id; }
  void rebindBack(ButtonID id)    { _map.back = id; }

  // ---------------------------------------------------------
  // Mode-specific readers (internal)
  // ---------------------------------------------------------
  void _readGamepad();
  void _readMechanical();
  void _readTouch();

private:
  mutable ControlState _s;
  struct Mapping {
    ButtonID confirm;
    ButtonID back;
    ButtonID menu;
    ButtonID alt;
  } _map;
};

// =========================================================
//  GLOBAL INSTANCE
// =========================================================
extern InputMapper controls;

// ======================= End of File =======================