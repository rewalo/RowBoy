#pragma once
#include <Arduino.h>
#include <TFT_eSPI.h>
#include <FS.h>
#include <SD.h>
#include <functional>

#include "config.h"  // Central config (orientation, colors, fonts, animations)

// ============================================================
//  RowBoy Firmware Prototype v1.0 (ESP32-S3)
//  ------------------------------------------------------------
//  MenuUI — Lightweight Menu Framework
//
//  Provides the shared base for RowBoy’s in-device UI system.
//  - Unified handling of gamepad, touch, and mechanical inputs
//  - Menu stack management (push/pop)
//  - Animation, auto-save, and layout theming
//
//  Notes:
//  - Everything here is designed to work on bare ESP32-S3 with TFT_eSPI.
//  - Touch input, SD, and gamepad hooks can be overridden externally.
// ============================================================

// -----------------------------------------------------------------------------
// Forward declarations (must come before any struct using them)
// -----------------------------------------------------------------------------
class EditMenu;
class MenuBase;

// ============================================================
//  GLOBAL LIMITS
// ============================================================
#ifndef MAX_OPT
#define MAX_OPT 15  // Maximum number of items per menu
#endif


// ============================================================
//  INPUT MODES
// ============================================================
// MenuUI supports three sources of input — all normalized to the same API.
enum class InputMode : uint8_t { TOUCH, MECH, GAMEPAD };


// ============================================================
//  MENU SETTINGS (per-menu instance)
// ============================================================
// These define response behavior for buttons, joystick, or touch repeat timing.
struct MenuSettings {
  int       deadzone           = DEADZONE;           // Analog stick deadzone
  uint16_t  initialRepeatDelay = REPEAT_INITIAL_MS;  // First repeat wait
  uint16_t  holdRepeatDelay    = REPEAT_HOLD_MS;     // Slow repeat rate
  uint16_t  fastRepeatDelay    = REPEAT_FAST_MS;     // Fast repeat rate
  uint16_t  fastRepeatAfter    = REPEAT_AFTER_MS;    // Time until fast mode
};


// ============================================================
//  WEAK EXTERNAL HOOKS
// ============================================================
// These can be overridden by the user project (e.g., gamepad.cpp) to provide
// actual hardware input. Default stubs are provided elsewhere.
bool     __attribute__((weak)) gamepadConnected();
int16_t  __attribute__((weak)) gpLX();
int16_t  __attribute__((weak)) gpLY();
bool     __attribute__((weak)) gpA();
bool     __attribute__((weak)) gpB();
bool     __attribute__((weak)) gpUp();
bool     __attribute__((weak)) gpDown();
bool     __attribute__((weak)) gpLeft();
bool     __attribute__((weak)) gpRight();
bool     __attribute__((weak)) gpX();
bool     __attribute__((weak)) gpY();
bool     __attribute__((weak)) gpStart();
bool     __attribute__((weak)) gpSelect();

// Optional touch hook — override for your own touch controller
bool __attribute__((weak)) menuGetTouch(int& x, int& y, bool& tap);


// ============================================================
//  MENU ITEM MODEL
// ============================================================
// Menu items can display static labels or editable values.
enum class IconType : uint8_t { NONE, MONO, COLOR };
enum class EditKind : uint8_t { NONE, RANGE, ARRAY };

struct EditRange {
  long minV  = 0;
  long maxV  = 0;
  long step  = 1;
  long value = 0;
};

struct EditArray {
  const char** choices = nullptr;
  uint16_t count = 0;
  uint16_t index = 0;
};

// Represents a single visible item in a menu.
struct MenuItem {
  String   text;
  IconType iconType = IconType::NONE;
  String   iconPath;
  int16_t  iconW = 0, iconH = 0;
  bool     enabled = true;

  EditKind edit = EditKind::NONE;
  EditRange r;
  EditArray a;

  EditMenu* submenu = nullptr;  // Linked submenu (optional)

  // Callback for live updates (triggered when value changes)
  std::function<void(long)> onChange = nullptr;

  // --- Helpers ---
  void setText(const String& t) { text = t; }
  void setEnabled(bool en) { enabled = en; }

  long value() const {
    if (edit == EditKind::RANGE) return r.value;
    if (edit == EditKind::ARRAY) return a.index;
    return 0;
  }

  void setValue(long v) {
    if (edit == EditKind::RANGE) r.value = v;
    else if (edit == EditKind::ARRAY && a.count)
      a.index = constrain((int)v, 0, (int)a.count - 1);
  }
};


// ============================================================
//  QUICK ITEM BUILDERS
// ============================================================
// Simple helpers to construct menu items inline.
MenuItem makeLabel(const String& text,
                   IconType it = IconType::NONE,
                   const String& iconPath = "", int iw = 0, int ih = 0);

MenuItem makeRange(const String& text, long v, long minV, long maxV, long step,
                   IconType it = IconType::NONE,
                   const String& iconPath = "", int iw = 0, int ih = 0);

MenuItem makeArray(const String& text, const char** choices, uint16_t n, uint16_t idx,
                   IconType it = IconType::NONE,
                   const String& iconPath = "", int iw = 0, int ih = 0);


// ============================================================
//  THEME STRUCTURE
// ============================================================
// Controls the look and feel of every menu instance.
struct MenuTheme {
  // --- Layout ---
  int16_t marginL = MENU_MARGIN_L;
  int16_t marginR = MENU_MARGIN_R;
  int16_t marginT = MENU_MARGIN_T;
  int16_t marginB = MENU_MARGIN_B;
  int16_t rowH    = MENU_ROW_H;
  int16_t iconPad = MENU_ICON_PAD;
  int16_t textPad = MENU_TEXT_PAD;
  int16_t selectorRadius = MENU_SELECTOR_RADIUS;
  int16_t selectorBorder = MENU_SELECTOR_BORDER;

  // --- Colors ---
  uint16_t bg        = COL_BG;
  uint16_t fg        = COL_FG;
  uint16_t muted     = COL_MUTED;
  uint16_t selFill   = COL_SEL_FILL;
  uint16_t selBorder = COL_SEL_BORD;
  uint16_t disabled  = COL_DISABLED;
  uint16_t arrow     = COL_ARROW;
  uint16_t monoTint  = COL_MONO_TINT;

  // --- Fonts ---
  uint8_t textFont  = MENU_TEXT_FONT_ID;
  uint8_t valueFont = MENU_VALUE_FONT_ID;

  // --- Orientation / Animation ---
  MenuOrientation orientation     = MENU_ORIENTATION_DEFAULT;
  TransitionStyle pageTransition  = PAGE_TRANSITION;
  bool            animations      = ANIM_ENABLE;
  uint16_t        animPageMs      = ANIM_PAGE_MS;
  uint8_t         animEase        = ANIM_EASE_STRENGTH;
};


// ============================================================
//  INPUT MAPPER (forward decl)
// ============================================================
// Declared in controls.h, used to normalize button/axis events.
class InputMapper;
extern InputMapper controls;


// ============================================================
//  MENUBASE CLASS
// ============================================================
// Abstract base class for menu rendering & input logic.
class MenuBase {
public:
  MenuSettings settings;

  MenuBase(TFT_eSPI& tft, int16_t w, int16_t h);

  // --- Dirty flag control ---
  void markDirty()  { _dirty = true; }
  void markClean()  { _dirty = false; }
  void forceRedraw(){ _dirty = true; }

  // --- Theme & Mode ---
  void setTheme(const MenuTheme& th);
  void setInputMode(InputMode m);
  InputMode inputMode() const;

  void setOrientation(MenuOrientation o) { _th.orientation = o; _dirty = true; }
  MenuOrientation orientation() const    { return _th.orientation; }

  void setPageTransition(TransitionStyle s) { _th.pageTransition = s; _dirty = true; }
  void enableAnimations(bool on)            { _th.animations = on; _dirty = true; }

  // --- Item management ---
  bool addItem(const MenuItem& it);
  void setItemEnabled(uint16_t idx, bool en);
  void setItemText(uint16_t idx, const String& s);
  long getItemValue(uint16_t idx) const;
  void setItemValue(uint16_t idx, long v);
  void linkSubmenu(uint16_t idx, EditMenu* sub) {
    if (idx < _count) _items[idx].submenu = sub;
  }

  // --- Drawing / Update ---
  void draw();
  int  update();
  void focus(uint16_t idx);
  uint16_t size() const;
  uint16_t selected() const;

  MenuItem& getItemRef(uint16_t idx) {
    static MenuItem dummy; // fallback to prevent null access
    return (idx < _count) ? _items[idx] : dummy;
  }

  // --- Accessors ---
  TFT_eSPI&        tft()        { return _tft; }
  const MenuTheme& theme() const{ return _th; }
  int16_t          width() const{ return _W; }
  int16_t          height() const{ return _H; }

  inline void publicDraw() { draw(); }

protected:
  // --- Core state ---
  TFT_eSPI& _tft;
  MenuTheme _th;
  InputMode _mode = InputMode::GAMEPAD;
  MenuItem  _items[MAX_OPT];
  uint16_t  _count = 0;
  uint16_t  _sel = 0;
  uint16_t  _firstVisible = 0;
  bool      _dirty = true;
  int       _activatedIndex = -1;
  int16_t   _W, _H;

  // --- Navigation helpers ---
  void _ensureVisible();
  void _moveSel(int delta);

  // --- Input handlers ---
  void _handleInput();
  void _handleGamepad();
  void _handleMechanical();
  void _handleTouch();

  // --- Drawing helpers ---
  void drawListToBuffer(TFT_eSprite& tft);
  void drawCarouselToBuffer(TFT_eSprite& tft);
  void drawArrowsIfNeededToBuffer(TFT_eSprite& tft);
  static String wrapTextByWidth(TFT_eSPI& tft, const String& s, int maxW, int font);

  // --- Navigation timing ---
  int8_t        _navDir = 0;
  bool          _navActive = false;
  unsigned long _navNext = 0;
  unsigned long _navStart = 0;
};


// ============================================================
//  EDITMENU CLASS
// ============================================================
// A MenuBase subclass that supports editable values + autosave.
class EditMenu : public MenuBase {
public:
  using MenuBase::MenuBase;

  int  update();
  bool inEditing() const { return _editing; }

  // --- Auto-Save ---
  void enableAutoSave(const char* path = "/settings.json");
  void disableAutoSave() { _autosave = false; }
  bool autosaveEnabled() const { return _autosave; }
  const char* autosavePath() const { return _savePath; }

  void setEditing(bool e) { _editing = e; }
  void markDirtyPublic()  { _dirty = true; }

protected:
  bool _editing = false;
  bool _autosave = false;
  const char* _savePath = "/settings.json";

  // --- Drawing ---
  void drawListWithValues();
  void drawCarouselWithValues();
  void draw();

  // --- Edit control ---
  void _handleInputEdit();
  void _editAdjust(int dir);
  void _editGamepad();
  void _editMechanical();
  void _editTouch();

  // --- Edit timing ---
  int8_t        _editDir = 0;
  bool          _editActive = false;
  unsigned long _editNext = 0;
  unsigned long _editStart = 0;
};


// ============================================================
//  MENU STACK MANAGEMENT
// ============================================================
// Allows nested menus: push() new ones, pop() to go back.
void      pushMenu(EditMenu* m);
EditMenu* popMenu();
EditMenu* currentMenu();
void      setRootMenu(EditMenu* m);


// ============================================================
//  INPUT LOCK (prevent early repeat between menus)
// ============================================================
unsigned long getMenuInputLockUntil();
void setMenuInputLockUntil(unsigned long val);


// ============================================================
//  SETTINGS SAVE / LOAD HELPERS
// ============================================================
// Convenience wrappers for storing menu state to SD/FS.
bool saveMenuSettings(MenuBase& menu, const char* path = "/settings.json");
bool loadMenuSettings(MenuBase& menu, const char* path = "/settings.json");

// ======================= End of File =======================