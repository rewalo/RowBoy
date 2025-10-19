// =========================================================
//  RowBoy Firmware Prototype v1.0 (ESP32-S3)
//  ---------------------------------------------------------
//  gamepad.cpp — Bluepad32 Gamepad Integration
//
//  Provides:
//   • Controller connection / disconnection handling
//   • Pairing mode (long-press trigger with LED feedback)
//   • Real-time button & axis state mapping
//   • Debounce handling for onboard pairing button
//   • Hooks for MenuUI (gpA(), gpLX(), etc.)
// =========================================================

#include "gamepad.h"
#include "config.h"
#include "MenuUI.h"
#include "nvs_flash.h"
#include <Bluepad32.h>

// =========================================================
//  INTERNAL STATE
// =========================================================
static ControllerPtr ctl = nullptr;
static bool connected     = false;
static bool pairingMode   = false;
static unsigned long pressStart   = 0;
static unsigned long lastBlink    = 0;
static unsigned long pairingStart = 0;
static bool buttonLast   = false;
static bool ledState     = false;
static bool firstLoop    = true;

// LED driver channel settings
const int LED_CHANNEL = 0;
const int LED_FREQ    = 5000;
const int LED_RES     = 8;

// Shared gamepad state structure
static GamepadState state;
const GamepadState& getGamepadState() { return state; }


// =========================================================
//  CALLBACKS — Connection / Disconnection
// =========================================================
static void onConnectedController(ControllerPtr c) {
  ctl = c;
  connected = true;
  pairingMode = false;
  pressStart = 0;

  ledcWrite(LED_CHANNEL, 40);
  Serial.printf("[Pad] Connected: %s\n", c ? c->getModelName().c_str() : "unknown");
}

static void onDisconnectedController(ControllerPtr c) {
  if (ctl == c) ctl = nullptr;
  connected = false;
  ledcWrite(LED_CHANNEL, 0);
  Serial.println("[Pad] Disconnected");
}


// =========================================================
//  PAIRING HELPERS
// =========================================================
static void startPairing() {
  pairingMode = true;
  pairingStart = millis();
  lastBlink = pairingStart;
  ledState = false;

  BP32.enableNewBluetoothConnections(true);
  ledcWrite(LED_CHANNEL, 0);

  Serial.println("[Pad] Pairing mode...");
}

static void stopPairing() {
  pairingMode = false;
  BP32.enableNewBluetoothConnections(false);
  ledcWrite(LED_CHANNEL, 0);
  Serial.println("[Pad] Pairing stopped");
}


// =========================================================
//  SETUP
// =========================================================
// Initializes NVS, LED channel, button, and Bluepad32
void setupGamepad() {
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    nvs_flash_erase();
    nvs_flash_init();
  }

  ledcSetup(LED_CHANNEL, LED_FREQ, LED_RES);
  ledcAttachPin(LED_PIN, LED_CHANNEL);
  pinMode(BTN_PIN, INPUT_PULLUP);
  ledcWrite(LED_CHANNEL, 0);

  Serial.println("[Pad] Bluepad32 setup...");
  BP32.setup(&onConnectedController, &onDisconnectedController);
  delay(300);
  BP32.enableNewBluetoothConnections(false);
}


// =========================================================
//  LOOP UPDATE
// =========================================================
// Handles input state refresh, pairing button logic,
// LED blinking, and controller connection management.
void updateGamepad() {
  BP32.update();

  // --- Update Gamepad State ---
  if (!ctl || !ctl->isConnected()) {
    connected = false;
    state = GamepadState();
  } else {
    connected = true;
    state.connected = true;
    state.lx = ctl->axisX();
    state.ly = ctl->axisY();
    state.rx = ctl->axisRX();
    state.ry = ctl->axisRY();
    state.dpad = ctl->dpad();
    state.a = ctl->a();
    state.b = ctl->b();
    state.x = ctl->x();
    state.y = ctl->y();
  }

  // --- Button Debounce ---
  unsigned long now = millis();
  bool btn = !digitalRead(BTN_PIN);
  static unsigned long lastDebounce = 0;
  static bool debounced = false;

  if (now - lastDebounce >= DEBOUNCE_MS) {
    debounced = btn;
    lastDebounce = now;
  }

  // Skip first iteration
  if (firstLoop) {
    firstLoop = false;
    buttonLast = debounced;
    return;
  }

  // --- Pairing Button Logic ---
  if (debounced && !buttonLast)
    pressStart = now;
  if (!debounced && buttonLast)
    pressStart = 0;
  buttonLast = debounced;

  // Long-hold initiates pairing mode
  if (debounced && !connected && !pairingMode && pressStart &&
      (now - pressStart >= HOLD_TIME_MS)) {
    startPairing();
  }

  // --- LED Feedback ---
  const int LED_BRIGHT = 40;
  const int LED_DIM = 0;

  if (pairingMode && !connected) {
    // Blink during pairing
    if (now - lastBlink >= BLINK_PERIOD_MS) {
      lastBlink = now;
      ledState = !ledState;
      ledcWrite(LED_CHANNEL, ledState ? LED_BRIGHT : LED_DIM);
    }
    // Timeout pairing mode
    if (now - pairingStart >= FLASH_TIME_MS)
      stopPairing();
  } 
  else if (connected) {
    // Solid LED when connected
    ledcWrite(LED_CHANNEL, LED_BRIGHT);
  } 
  else {
    // LED off otherwise
    ledcWrite(LED_CHANNEL, 0);
  }

  // Auto-stop pairing once connected
  if (connected && pairingMode)
    stopPairing();
}


// =========================================================
//  ACCESSORS — Weak Hooks for MenuUI
// =========================================================
// Expose unified state so MenuUI / InputMapper can query controls.
bool gamepadConnected() { return connected && ctl && ctl->isConnected(); }

int16_t gpLX() { return (ctl && ctl->isConnected()) ? ctl->axisX()  : 0; }
int16_t gpLY() { return (ctl && ctl->isConnected()) ? ctl->axisY()  : 0; }
int16_t gpRX() { return (ctl && ctl->isConnected()) ? ctl->axisRX() : 0; }
int16_t gpRY() { return (ctl && ctl->isConnected()) ? ctl->axisRY() : 0; }

// Not sure why they did the dPad so weird but eh | TODO: in-UI calibration
bool gpUp()    { return (ctl && ctl->isConnected()) && (ctl->dpad() & 0x01); }
bool gpDown()  { return (ctl && ctl->isConnected()) && (ctl->dpad() & 0x02); }
bool gpRight() { return (ctl && ctl->isConnected()) && (ctl->dpad() & 0x04); }
bool gpLeft()  { return (ctl && ctl->isConnected()) && (ctl->dpad() & 0x08); }

bool gpA()      { return (ctl && ctl->isConnected()) && ctl->a(); }
bool gpB()      { return (ctl && ctl->isConnected()) && ctl->b(); }
bool gpX()      { return (ctl && ctl->isConnected()) && ctl->x(); }
bool gpY()      { return (ctl && ctl->isConnected()) && ctl->y(); }
bool gpStart()  { return (ctl && ctl->isConnected()) && ctl->miscStart(); }
bool gpSelect() { return (ctl && ctl->isConnected()) && ctl->miscSelect(); }

// ======================= End of File =======================