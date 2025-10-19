// =========================================================
//  RowBoy Firmware Prototype v1.0 (ESP32-S3)
//  ---------------------------------------------------------
//  A tiny, hackable handheld powered by the ESP32-S3 WROOM-1.
//  Designed for gaming, emulation, and embedded UI experiments.
//
//  - Horizontal DS-style carousel by default (switchable to vertical list)
//  - Unified input system: Gamepad + Mechanical + Touch
//  - Optional icons, smooth(ish) transitions, configurable fonts/colors
//  - Drop-in Settings menu with autosave over SD
//
//  ---------------------------------------------------------
//  HOW TO USE
//  1) Configure options in config.h (orientation, fonts, colors, debug).
//  2) Build your menu:
//       - Create an EditMenu or MenuBase.
//       - Add items via makeLabel / makeRange / makeArray.
//       - Link submenus with menu.linkSubmenu(index, &submenu).
//  3) Push as root using setRootMenu(&rootMenu).
//  4) In loop(), call update() on the *current* menu.
//  5) Handle “activation” when user confirms a non-editable item.
//
//  ORIENTATION
//  -----------
//  - Horizontal (default): DS/3DS-style side-scroll menu.
//  - Vertical: Classic list layout.
//
//  ICONS (TODO)
//  -------------
//  - Disabled by default for a clean look (enable per-item or globally).
//  - Supports raw 16-bit 565 or packed 1-bit mono icons (with tint).
//
//  ANIMATIONS (TODO)
//  -----------------
//  - SLIDE / FADE / SLIDE_FADE / NONE
//  - Fast and lightweight; DMA-aware to avoid white artifacts.
//
//  SAVING SETTINGS
//  ----------------
//  - Call menu.enableAutoSave("/path.json") on a settings menu.
//  - TFT_CS is safely toggled around SD access to avoid screen artifacts.
//
//  DEBUGGING
//  ----------
//  - Toggle Serial/on-screen debugging in config.h.
//  - On-screen overlay is subtle and off by default.
// =========================================================

#include <TFT_eSPI.h>
#include <FS.h>
#include <SD.h>
#include <WiFi.h>

#include "config.h"
#include "MenuUI.h"
#include "controls.h"
#include "gamepad.h"
#include "sdcard.h"
#include "esp_wifi.h"

// =========================================================
//  GLOBALS
// =========================================================

// --- Display ---
TFT_eSPI tft;

// --- Menus ---
static EditMenu rootMenu(tft, 480, 320);     // Root “Home” menu
static EditMenu settingsMenu(tft, 480, 320); // Settings submenu
static EditMenu powerMenu(tft, 480, 320);    // Power submenu

// --- Forward declarations ---
static void buildThemes();
static void buildRootHorizontal();
static void buildSettingsMenu();
static void buildPowerMenu();

int brightnessValue = 200;  // Default brightness (0–255)

// =========================================================
//  BACKLIGHT CONTROL
// =========================================================

void setBrightness(int val) {
  brightnessValue = constrain(val, 5, 255);
  ledcWrite(BL_CHANNEL, brightnessValue);
}

// =========================================================
//  DEBUG OVERLAY
// =========================================================
//  Tiny corner overlay for live debug messages or FPS counters.
//  Enabled via Debug::ONSCREEN.

static void drawOverlay(const char* msg) {
  if (!Debug::ONSCREEN) return;

  tft.startWrite();
  tft.fillRect(0, 0, 200, 18, rgb(0, 0, 0));
  tft.setTextFont(1);
  tft.setTextColor(rgb(255, 255, 255), rgb(0, 0, 0));
  tft.setTextDatum(TL_DATUM);
  tft.drawString(msg, 2, 4);
  tft.endWrite();
}

// =========================================================
//  SETUP
// =========================================================

void setup() {
  if (Debug::SERIAL_EN) {
    Serial.begin(115200);
    delay(50);
  }

  // Disable wireless subsystems to save RAM + avoid interference
  WiFi.mode(WIFI_OFF);
  esp_wifi_stop();
  esp_wifi_deinit();

  // --- Display Init ---
  tft.init();
  tft.setRotation(SCREEN_ROTATION);
  tft.fillScreen(COL_BG);

  // --- Backlight PWM ---
  ledcSetup(BL_CHANNEL, 5000, 8);
  ledcAttachPin(TFT_BL, BL_CHANNEL);
  setBrightness(brightnessValue);

  // --- Storage & Peripherals ---
  setupSD();        // Mount SD card
  setupGamepad();   // Init Bluepad32 or local controls

  // --- Menu System ---
  buildThemes();
  buildRootHorizontal();
  buildSettingsMenu();
  buildPowerMenu();

  // Link submenus
  // Root order: Game Library, Gallery, Music Player,
  // Settings, File Manager, Homebrew, Power
  rootMenu.linkSubmenu(3, &settingsMenu);
  rootMenu.linkSubmenu(6, &powerMenu);

  // Register root menu
  setRootMenu(&rootMenu);

  // --- Load persisted settings (if any) ---
  if (loadMenuSettings(settingsMenu, "/settings.json")) {
    int bright = settingsMenu.getItemValue(0);
    setBrightness(map(bright, 0, 100, 5, 255));

    int volume = settingsMenu.getItemValue(1);
    (void)volume; // Placeholder – TODO: route to audio system

    int ori = settingsMenu.getItemValue(2);
    rootMenu.setOrientation(ori == 0
      ? MenuOrientation::HORIZONTAL
      : MenuOrientation::VERTICAL);
    settingsMenu.setOrientation(rootMenu.orientation());
    powerMenu.setOrientation(rootMenu.orientation());

    int tr = settingsMenu.getItemValue(3);
    rootMenu.setPageTransition((TransitionStyle)tr);
    settingsMenu.setPageTransition((TransitionStyle)tr);

    bool icons = (settingsMenu.getItemValue(4) == 1);
    for (int i = 0; i < rootMenu.size(); i++)
      rootMenu.getItemRef(i).iconType = icons
        ? IconType::COLOR
        : IconType::NONE;

    DBG_IF(MENU, "[Menu] Settings applied at boot.\n");
  } else {
    DBG_IF(MENU, "[Menu] No settings file found; using defaults.\n");
  }

  // Overlay “Ready” indicator
  drawOverlay("Ready");

  DBG_IF(
    MENU,
    "[Menu] UI ready (orientation=%s)\n",
    (int)MENU_ORIENTATION_DEFAULT == (int)MenuOrientation::HORIZONTAL ? "H" : "V"
  );
}

// =========================================================
//  MENU ACTIVATION HANDLERS
// =========================================================

static void handleRootActivation(EditMenu& menu, int idx) {
  switch (idx) {
    case 0: DBG_IF(MENU, "[Action] Game Library\n"); break;
    case 1: DBG_IF(MENU, "[Action] Gallery\n"); break;
    case 2: DBG_IF(MENU, "[Action] Music Player\n"); break;
    case 3: /* Settings submenu */ break;
    case 4: DBG_IF(MENU, "[Action] File Manager\n"); break;
    case 5: DBG_IF(MENU, "[Action] Homebrew\n"); break;
    case 6: /* Power submenu */ break;
  }
}

static void handleSettingsActivation(EditMenu& menu, int idx) {
  // Reserved for non-edit labels like “Reset Defaults”
  DBG_IF(MENU, "[Settings] Activated index=%d\n", idx);
}

static bool sleeping = false;

static void handlePowerActivation(EditMenu& menu, int idx) {
  if (idx == 0) {
    DBG_IF(MENU, "[Power] Sleep\n");
    sleeping = true;
    enterLightSleep();
  } else if (idx == 1) {
    DBG_IF(MENU, "[Power] Reboot\n");
    ESP.restart();
  } else if (idx == 2) {
    DBG_IF(MENU, "[Power] Shutdown\n");
    enterDeepSleep();
  }
}

// =========================================================
//  MAIN LOOP
// =========================================================

void loop() {
  updateGamepad();

  EditMenu* m = currentMenu();
  if (!m) return;

  // Handle wake from light sleep
  if (sleeping) {
    if (gpA() || gpStart()) {
      sleeping = false;
      exitLightSleep();
      m->update();
    }
    delay(100);
    return;
  }

  // Drive the active menu
  int activated = m->update();
  if (activated >= 0) {
    if      (m == &rootMenu)      handleRootActivation(*m, activated);
    else if (m == &settingsMenu)  handleSettingsActivation(*m, activated);
    else if (m == &powerMenu)     handlePowerActivation(*m, activated);
  }
}

// =========================================================
//  THEME + MENU BUILDERS
// =========================================================

static void buildThemes() {
  MenuTheme th;

  th.marginL        = MENU_MARGIN_L;
  th.marginR        = MENU_MARGIN_R;
  th.marginT        = MENU_MARGIN_T;
  th.marginB        = MENU_MARGIN_B;
  th.rowH           = MENU_ROW_H;
  th.iconPad        = MENU_ICON_PAD;
  th.textPad        = MENU_TEXT_PAD;
  th.selectorRadius = MENU_SELECTOR_RADIUS;
  th.selectorBorder = MENU_SELECTOR_BORDER;

  th.bg        = COL_BG;
  th.fg        = COL_FG;
  th.muted     = COL_MUTED;
  th.selFill   = COL_SEL_FILL;
  th.selBorder = COL_SEL_BORD;
  th.disabled  = COL_DISABLED;
  th.arrow     = COL_ARROW;
  th.monoTint  = COL_MONO_TINT;

  th.textFont  = MENU_TEXT_FONT_ID;
  th.valueFont = MENU_VALUE_FONT_ID;

  rootMenu.setTheme(th);
  settingsMenu.setTheme(th);
  powerMenu.setTheme(th);

  // Default input: gamepad
  rootMenu.setInputMode(InputMode::GAMEPAD);
  settingsMenu.setInputMode(InputMode::GAMEPAD);
  powerMenu.setInputMode(InputMode::GAMEPAD);

  // Input cadence (anti-spam + hold repeat)
  rootMenu.settings.deadzone           = DEADZONE;
  rootMenu.settings.initialRepeatDelay = REPEAT_INITIAL_MS;
  rootMenu.settings.holdRepeatDelay    = REPEAT_HOLD_MS;
  rootMenu.settings.fastRepeatDelay    = REPEAT_FAST_MS;
  rootMenu.settings.fastRepeatAfter    = REPEAT_AFTER_MS;

  settingsMenu.settings = rootMenu.settings;
  powerMenu.settings    = rootMenu.settings;
}

// ---------------------------------------------------------
//  Root Menu (Horizontal default)
// ---------------------------------------------------------
static void buildRootHorizontal() {
  /*
    Root: horizontal default with optional icons.
    Order:
      0 Game Library
      1 Gallery
      2 Music Player
      3 Settings
      4 File Manager
      5 Homebrew
      6 Power
  */
  rootMenu.addItem(makeLabel("Game Library", MENU_SHOW_ICONS_DEFAULT ? IconType::COLOR : IconType::NONE));
  rootMenu.addItem(makeLabel("Gallery",      MENU_SHOW_ICONS_DEFAULT ? IconType::COLOR : IconType::NONE));
  rootMenu.addItem(makeLabel("Music Player", MENU_SHOW_ICONS_DEFAULT ? IconType::COLOR : IconType::NONE));
  rootMenu.addItem(makeLabel("Settings",     MENU_SHOW_ICONS_DEFAULT ? IconType::COLOR : IconType::NONE));
  rootMenu.addItem(makeLabel("File Manager", MENU_SHOW_ICONS_DEFAULT ? IconType::COLOR : IconType::NONE));
  rootMenu.addItem(makeLabel("Homebrew",     MENU_SHOW_ICONS_DEFAULT ? IconType::COLOR : IconType::NONE));
  rootMenu.addItem(makeLabel("Power",        MENU_SHOW_ICONS_DEFAULT ? IconType::COLOR : IconType::NONE));
}

// ---------------------------------------------------------
//  Settings Menu
// ---------------------------------------------------------
static void buildSettingsMenu() {
  static const char* orientations[] = { "Horizontal", "Vertical" };
  static const char* anims[]        = { "None", "Slide", "Fade", "Slide+Fade" };
  static const char* iconChoices[]  = { "Off", "On" };

  auto& m = settingsMenu;

  m.addItem(makeRange("Brightness", 75, 0, 100, 5));
  m.addItem(makeRange("Volume",     60, 0, 100, 5));
  m.addItem(makeArray("Orientation", orientations, 2,
    (MENU_ORIENTATION_DEFAULT == MenuOrientation::HORIZONTAL ? 0 : 1)));
  m.addItem(makeArray("Transitions", anims, 4,
    (ANIM_ENABLE
      ? (PAGE_TRANSITION == TransitionStyle::SLIDE      ? 1 :
         PAGE_TRANSITION == TransitionStyle::FADE       ? 2 :
         PAGE_TRANSITION == TransitionStyle::SLIDE_FADE ? 3 : 0)
      : 0)));
  m.addItem(makeArray("Icons", iconChoices, 2, MENU_SHOW_ICONS_DEFAULT ? 1 : 0));

  // --- Brightness live update ---
  m.getItemRef(0).onChange = [](long v) {
    setBrightness(map(v, 0, 100, 5, 255));
  };

  // --- Orientation live update ---
  m.getItemRef(2).onChange = [](long v) {
    MenuOrientation o = (v == 0)
      ? MenuOrientation::HORIZONTAL
      : MenuOrientation::VERTICAL;
    rootMenu.setOrientation(o);
    settingsMenu.setOrientation(o);
    rootMenu.forceRedraw();
    DBG_IF(MENU, "[Settings] Orientation changed -> %s\n",
      v == 0 ? "HORIZONTAL" : "VERTICAL");
    settingsMenu.forceRedraw();
  };

  // --- Transition style live update ---
  m.getItemRef(3).onChange = [](long v) {
    TransitionStyle s = (TransitionStyle)v;
    rootMenu.setPageTransition(s);
    settingsMenu.setPageTransition(s);
    DBG_IF(MENU, "[Settings] Transition changed -> %d\n", (int)s);
    settingsMenu.forceRedraw();
  };

  // --- Icons toggle live update ---
  m.getItemRef(4).onChange = [](long v) {
    bool icons = (v == 1);
    for (int i = 0; i < rootMenu.size(); i++)
      rootMenu.getItemRef(i).iconType = icons ? IconType::COLOR : IconType::NONE;
    rootMenu.forceRedraw();
    DBG_IF(MENU, "[Settings] Icons %s\n", icons ? "ON" : "OFF");
    settingsMenu.forceRedraw();
  };

  // Auto-save to SD
  m.enableAutoSave("/settings.json");
}

// ---------------------------------------------------------
//  Power Menu
// ---------------------------------------------------------
static void buildPowerMenu() {
  powerMenu.addItem(makeLabel("Sleep"));
  powerMenu.addItem(makeLabel("Reboot"));
  powerMenu.addItem(makeLabel("Shutdown"));
}

// =========================================================
//  POWER MANAGEMENT
// =========================================================

void enterLightSleep() {
  // Smooth fade-out to avoid flicker
  for (int b = brightnessValue; b > 0; b -= 10) {
    ledcWrite(BL_CHANNEL, b);
    delay(5);
  }

  // Enter low-power display sleep
  tft.writecommand(0x10);
  ledcWrite(BL_CHANNEL, 0);
  digitalWrite(LED_PIN, LOW);

  DBG_IF(MENU, "[Power] Entering light sleep mode...\n");
}

void exitLightSleep() {
  tft.writecommand(0x11); // Wake display
  delay(150);
  setBrightness(brightnessValue);
  DBG_IF(MENU, "[Power] Woke from light sleep.\n");
}

void enterDeepSleep() {
  // I personally have my ESP-32 hooked up to a regular slide switch, so I can't programatically turn it off, but I'm sure with something like a relay you could
  DBG_IF(MENU, "[Power] Entering deep sleep...\n");

  tft.writecommand(0x10);
  ledcWrite(BL_CHANNEL, 0);
  WiFi.mode(WIFI_OFF);
  btStop();

  delay(100);

  // Disable all wake sources (manual EN reset only)
  esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);

  esp_deep_sleep_start();
}

// ======================= End of File =======================