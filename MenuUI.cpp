// =========================================================
//  RowBoy Firmware Prototype v1.0 (ESP32-S3)
//  ---------------------------------------------------------
//  MenuUI.cpp — Core Menu System Implementation
//
//  Provides:
//   • Menu stack management (push/pop/current)
//   • Rendering (list + carousel modes)
//   • Gamepad, touch, and mechanical input handling
//   • Editable values with autosave support
//   • SD JSON persistence helpers
//
//  Notes:
//   - Uses TFT_eSPI sprites for smooth redraws. 
//   - !! If your ESP32-S3 has limited PSRAM or it’s not configured properly, sprite creation may fail or cause instability. !!
//   - Input repeat timing is shared with config.h.
// =========================================================

#include "MenuUI.h"
#include "controls.h"
#include "config.h"
#include <ArduinoJson.h>
#include <vector>

// =========================================================
//  GLOBAL STATE
// =========================================================
static TFT_eSprite* spriteA = nullptr;
static TFT_eSprite* spriteB = nullptr; // reserved for future transitions
static std::vector<EditMenu*> menuStack;
static unsigned long inputLockUntil = 0;
static EditMenu* rootMenu = nullptr;


// =========================================================
//  ACCESSORS
// =========================================================
unsigned long getMenuInputLockUntil() { return inputLockUntil; }
void setMenuInputLockUntil(unsigned long val) { inputLockUntil = val; }


// =========================================================
//  STACK HELPERS (push / pop / current / root)
// =========================================================
void setRootMenu(EditMenu* m) {
  rootMenu = m;
  menuStack.clear();
  if (m) menuStack.push_back(m);
}

EditMenu* currentMenu() {
  if (menuStack.empty()) return nullptr;
  return menuStack.back();
}

void pushMenu(EditMenu* m) {
  if (!m) return;
  menuStack.push_back(m);
  m->forceRedraw();
}

EditMenu* popMenu() {
  if (menuStack.size() <= 1) return nullptr;
  menuStack.pop_back();
  EditMenu* prev = menuStack.back();
  prev->forceRedraw();
  return prev;
}


// =========================================================
//  MENU ITEM BUILDERS
// =========================================================
// Helper shortcuts for creating label/range/array menu items
MenuItem makeLabel(const String& text, IconType it, const String& iconPath, int iw, int ih) {
  MenuItem m; m.text = text; m.iconType = it; m.iconPath = iconPath; m.iconW = iw; m.iconH = ih;
  return m;
}

MenuItem makeRange(const String& text, long v, long minV, long maxV, long step,
                   IconType it, const String& iconPath, int iw, int ih) {
  MenuItem m = makeLabel(text, it, iconPath, iw, ih);
  m.edit = EditKind::RANGE;
  m.r.value = v; m.r.minV = minV; m.r.maxV = maxV; m.r.step = step;
  return m;
}

MenuItem makeArray(const String& text, const char** choices, uint16_t n, uint16_t idx,
                   IconType it, const String& iconPath, int iw, int ih) {
  MenuItem m = makeLabel(text, it, iconPath, iw, ih);
  m.edit = EditKind::ARRAY;
  m.a.choices = choices; m.a.count = n; m.a.index = idx;
  return m;
}


// =========================================================
//  MENUBASE IMPLEMENTATION
// =========================================================
MenuBase::MenuBase(TFT_eSPI& tft, int16_t w, int16_t h)
  : _tft(tft), _W(w), _H(h) {}

void MenuBase::setTheme(const MenuTheme& th) { _th = th; _dirty = true; }
void MenuBase::setInputMode(InputMode m) { _mode = m; }
InputMode MenuBase::inputMode() const { return _mode; }

bool MenuBase::addItem(const MenuItem& it) {
  if (_count >= MAX_OPT) return false;
  _items[_count++] = it;
  _dirty = true;
  return true;
}

void MenuBase::setItemEnabled(uint16_t idx, bool en) { if (idx < _count) _items[idx].enabled = en; }
void MenuBase::setItemText(uint16_t idx, const String& s) { if (idx < _count) _items[idx].text = s; }
long MenuBase::getItemValue(uint16_t idx) const { return (idx < _count) ? _items[idx].value() : 0; }
void MenuBase::setItemValue(uint16_t idx, long v) { if (idx < _count) _items[idx].setValue(v); }
uint16_t MenuBase::size() const { return _count; }
uint16_t MenuBase::selected() const { return _sel; }


// =========================================================
//  DRAW HELPERS
// =========================================================

// --- Vertical List Mode ---
void MenuBase::drawListToBuffer(TFT_eSprite& spr) {
  spr.fillSprite(_th.bg);
  int16_t y = _th.marginT;

  for (int i = 0; i < _count; ++i) {
    const MenuItem& it = _items[i];
    bool sel = (i == _sel);

    // Highlight selection
    if (sel) {
      spr.fillRoundRect(_th.marginL, y, _W - _th.marginL - _th.marginR, _th.rowH - 4,
                        _th.selectorRadius, _th.selFill);
      spr.drawRoundRect(_th.marginL, y, _W - _th.marginL - _th.marginR, _th.rowH - 4,
                        _th.selectorRadius, _th.selBorder);
    }

    // Text
    spr.setTextFont(_th.textFont);
    spr.setTextDatum(ML_DATUM);
    spr.setTextColor(_th.fg, sel ? _th.selFill : _th.bg);
    spr.drawString(it.text, _th.marginL + _th.textPad, y + _th.rowH / 2);

    y += _th.rowH;
  }
}


// --- Horizontal Carousel Mode (Nintendo-style) ---
void MenuBase::drawCarouselToBuffer(TFT_eSprite& spr) {
  spr.fillSprite(_th.bg);

  int widest = 0;
  for (int i = 0; i < _count; ++i) {
    int w = spr.textWidth(_items[i].text.c_str());
    if (w > widest) widest = w;
  }

  const int spacing = max(180, widest + 40);
  const int centerX = _W / 2;

  for (int i = 0; i < _count; ++i) {
    int x = centerX + (i - _sel) * spacing;
    if (x < -120 || x > _W + 120) continue;

    const MenuItem& it = _items[i];
    bool sel = (i == _sel);

    spr.setTextFont(_th.textFont);
    spr.setTextDatum(MC_DATUM);

    if (sel) {
      int boxW = widest + 60;
      spr.fillRoundRect(x - boxW / 2, _H / 2 - 28, boxW, 56, _th.selectorRadius, _th.selFill);
      spr.drawRoundRect(x - boxW / 2, _H / 2 - 28, boxW, 56, _th.selectorRadius, _th.selBorder);
      spr.setTextColor(_th.fg, _th.selFill);
    } else {
      spr.setTextColor(_th.fg, _th.bg);
    }

    spr.drawString(it.text, x, _H / 2);
  }
}


// --- Scroll Arrows ---
void MenuBase::drawArrowsIfNeededToBuffer(TFT_eSprite& tft) {
  const int rowsFit = max(1, (_H - _th.marginT - _th.marginB) / _th.rowH);
  bool up = (_firstVisible > 0);
  bool dn = (_firstVisible + rowsFit < _count);

  // Clear arrow zones to prevent artifacts
  tft.fillRect(0, 0, _W, _th.marginT, _th.bg);
  tft.fillRect(0, _H - _th.marginB, _W, _th.marginB, _th.bg);

  if (_th.orientation == MenuOrientation::VERTICAL) {
    if (up)
      tft.fillTriangle(
        _W / 2 - 6, _th.marginT - 2,
        _W / 2 + 6, _th.marginT - 2,
        _W / 2, _th.marginT - 14,
        _th.arrow);
    if (dn)
      tft.fillTriangle(
        _W / 2 - 6, _H - _th.marginB + 2,
        _W / 2 + 6, _H - _th.marginB + 2,
        _W / 2, _H - _th.marginB + 14,
        _th.arrow);
  } else {
    bool left = (_sel > 0);
    bool right = (_sel < _count - 1);
    if (left)
      tft.fillTriangle(8, _H / 2 - 8, 8, _H / 2 + 8, 0, _H / 2, _th.arrow);
    if (right)
      tft.fillTriangle(_W - 8, _H / 2 - 8, _W - 8, _H / 2 + 8, _W, _H / 2, _th.arrow);
  }
}


// =========================================================
//  SELECTION & INPUT HANDLING
// =========================================================
void MenuBase::_ensureVisible() {
  if (_th.orientation == MenuOrientation::VERTICAL) {
    if (_sel < _firstVisible) _firstVisible = _sel;
    if (_sel >= _firstVisible + 6) _firstVisible = _sel - 5;
  }
}

void MenuBase::_moveSel(int delta) {
  int newSel = constrain((int)_sel + delta, 0, (int)_count - 1);
  if (newSel != _sel) { _sel = newSel; _dirty = true; }
}

static inline int8_t dirFromOrientation(const MenuTheme& th, bool left, bool right, bool up, bool down) {
  if (th.orientation == MenuOrientation::HORIZONTAL) {
    if (left) return -1;
    if (right) return 1;
  } else {
    if (up) return -1;
    if (down) return 1;
  }
  return 0;
}


// =========================================================
//  INPUT HANDLERS
// =========================================================
void MenuBase::_handleInput() {
  controls.update(_mode);
  switch (_mode) {
    case InputMode::GAMEPAD: _handleGamepad(); break;
    case InputMode::MECH:    _handleMechanical(); break;
    case InputMode::TOUCH:   _handleTouch(); break;
  }
}


// --- Gamepad Navigation ---
void MenuBase::_handleGamepad() {
  if (millis() < inputLockUntil) return;

  int8_t d = dirFromOrientation(_th, controls.left(), controls.right(), controls.up(), controls.down());
  bool any = (d != 0);
  unsigned long now = millis();

  if (!any) { _navActive = false; _navDir = 0; }
  else {
    if (!_navActive || d != _navDir) {
      _moveSel(d);
      _navActive = true;
      _navDir = d;
      _navStart = now;
      _navNext = now + settings.initialRepeatDelay;  // first delay
    } else if (now >= _navNext) {
      _moveSel(d);
      unsigned long elapsed = now - _navStart;
      _navNext = now + (elapsed >= settings.fastRepeatAfter
                        ? settings.fastRepeatDelay
                        : settings.holdRepeatDelay);
    }
  }

  if (controls.confirmPressed()) _activatedIndex = _sel;
  if (controls.backPressed()) {
    if (menuStack.size() > 1) popMenu();
    controls.consumeBack();
    inputLockUntil = millis() + 200;
  }
}


// --- Mechanical (buttons / encoder) ---
void MenuBase::_handleMechanical() {
  if (millis() < inputLockUntil) return;

  int8_t d = dirFromOrientation(_th, controls.left(), controls.right(), controls.up(), controls.down());
  bool any = (d != 0);
  unsigned long now = millis();

  if (!any) { _navActive = false; _navDir = 0; }
  else {
    if (!_navActive || d != _navDir) {
      _moveSel(d);
      _navActive = true;
      _navDir = d;
      _navStart = now;
      _navNext = now + settings.initialRepeatDelay;
    } else if (now >= _navNext) {
      _moveSel(d);
      unsigned long elapsed = now - _navStart;
      _navNext = now + (elapsed >= settings.fastRepeatAfter
                        ? settings.fastRepeatDelay
                        : settings.holdRepeatDelay);
    }
  }

  if (controls.confirmPressed()) { _activatedIndex = _sel; controls.consumeConfirm(); }
  if (controls.backPressed()) {
    if (menuStack.size() > 1) popMenu();
    controls.consumeBack();
    inputLockUntil = millis() + 200;
  }
}


// --- Touch ---
void MenuBase::_handleTouch() {
  if (millis() < inputLockUntil) return;
  int x, y; bool tap = false;
  if (menuGetTouch(x, y, tap)) {
    if (tap) _activatedIndex = _sel;
  }
}


// =========================================================
//  DRAW + UPDATE LOOP
// =========================================================
void MenuBase::draw() {
  if (!_dirty) return;
  if (!spriteA) spriteA = new TFT_eSprite(&_tft);

  spriteA->createSprite(_W, _H);
  if (_th.orientation == MenuOrientation::VERTICAL)
    drawListToBuffer(*spriteA);
  else
    drawCarouselToBuffer(*spriteA);

  drawArrowsIfNeededToBuffer(*spriteA);

  _tft.startWrite();
  spriteA->pushSprite(0, 0);
  _tft.endWrite();

  _dirty = false;
}

int MenuBase::update() {
  _activatedIndex = -1;
  _handleInput();
  if (_dirty) draw();
  int ret = _activatedIndex;
  _activatedIndex = -1;
  return ret;
}


// =========================================================
//  EDITMENU — Editable Menu Extension
// =========================================================
void EditMenu::_editAdjust(int dir) {
  MenuItem& it = _items[_sel];
  long oldVal = it.value();

  if (it.edit == EditKind::RANGE)
    it.r.value = constrain(it.r.value + it.r.step * dir, it.r.minV, it.r.maxV);
  else if (it.edit == EditKind::ARRAY)
    it.a.index = (it.a.index + dir + it.a.count) % it.a.count;

  _dirty = true;

  // Trigger live update callback (if assigned)
  long newVal = it.value();
  if (it.onChange && newVal != oldVal) it.onChange(newVal);

  // Autosave (throttled)
  if (_autosave && _savePath) {
    static unsigned long lastSave = 0;
    unsigned long now = millis();
    if (now - lastSave > 300) {
      saveMenuSettings(*this, _savePath);
      lastSave = now;
    }
  }
}


// =========================================================
//  EDIT INPUT HANDLERS
// =========================================================
void EditMenu::_handleInputEdit() {
  controls.update(_mode);
  switch (_mode) {
    case InputMode::GAMEPAD: _editGamepad(); break;
    case InputMode::MECH:    _editMechanical(); break;
    case InputMode::TOUCH:   _editTouch(); break;
  }
}

void EditMenu::_editGamepad() { // I undastand it now >:)
  if (millis() < inputLockUntil) return;
  bool left = controls.left();
  bool right = controls.right();
  int8_t d = left ? -1 : right ? 1 : 0;
  bool any = (d != 0);
  unsigned long now = millis();

  if (!any) { _editActive = false; _editDir = 0; }
  else {
    if (!_editActive || d != _editDir) {
      _editAdjust(d);
      _editActive = true;
      _editDir = d;
      _editStart = now;
      _editNext = now + settings.initialRepeatDelay;
    } else if (now >= _editNext) {
      _editAdjust(d);
      unsigned long elapsed = now - _editStart;
      _editNext = now + (elapsed >= settings.fastRepeatAfter
                        ? settings.fastRepeatDelay
                        : settings.holdRepeatDelay);
    }
  }

  if (controls.confirmPressed()) {
    _editing = false;
    _dirty = true; // ensure muted color redraw
    controls.consumeConfirm();
  }
  if (controls.backPressed()) {
    _editing = false;
    _dirty = true;
    controls.consumeBack();
  }
}

void EditMenu::_editMechanical() { // No fucking clue if this works yet
  if (millis() < inputLockUntil) return;
  bool left = controls.left();
  bool right = controls.right();
  int8_t d = left ? -1 : right ? 1 : 0;
  bool any = (d != 0);
  unsigned long now = millis();

  if (!any) { _editActive = false; _editDir = 0; }
  else {
    if (!_editActive || d != _editDir) {
      _editAdjust(d);
      _editActive = true;
      _editDir = d;
      _editStart = now;
      _editNext = now + settings.initialRepeatDelay;
    } else if (now >= _editNext) {
      _editAdjust(d);
      unsigned long elapsed = now - _editStart;
      _editNext = now + (elapsed >= settings.fastRepeatAfter
                        ? settings.fastRepeatDelay
                        : settings.holdRepeatDelay);
    }
  }
  if (controls.confirmPressed()) { _editing = false; controls.consumeConfirm(); }
  if (controls.backPressed()) { _editing = false; controls.consumeBack(); }
}

void EditMenu::_editTouch() { // No fucking clue if this works yet
  if (millis() < inputLockUntil) return;
  int x, y; bool tap = false;
  if (menuGetTouch(x, y, tap)) {
    if (tap) _editing = false;
  }
}


// =========================================================
//  EDITMODE DRAW HELPERS (values)
// =========================================================
void EditMenu::drawListWithValues() {
  spriteA->fillSprite(_th.bg);
  int16_t y = _th.marginT;

  for (int i = 0; i < _count; ++i) {
    const MenuItem& it = _items[i];
    bool sel = (i == _sel);

    if (sel) {
      spriteA->fillRoundRect(_th.marginL, y, _W - _th.marginL - _th.marginR, _th.rowH - 4,
                              _th.selectorRadius, _th.selFill);
      spriteA->drawRoundRect(_th.marginL, y, _W - _th.marginL - _th.marginR, _th.rowH - 4,
                              _th.selectorRadius, _th.selBorder);
    }

    spriteA->setTextFont(_th.textFont);
    spriteA->setTextDatum(ML_DATUM);
    spriteA->setTextColor(_th.fg, sel ? _th.selFill : _th.bg);
    spriteA->drawString(it.text, _th.marginL + _th.textPad, y + _th.rowH / 2);

    if (it.edit != EditKind::NONE) {
      spriteA->setTextFont(_th.valueFont);
      spriteA->setTextDatum(MR_DATUM);

      // Default muted color
      uint16_t textCol = _th.muted;
      uint16_t bgCol   = sel ? _th.selFill : _th.bg;

      // Sexy man blink while edit :D
      if (_editing && sel && (millis() / 300 % 2))
        textCol = _th.selBorder;

      spriteA->setTextColor(textCol, bgCol);

      String valStr = (it.edit == EditKind::RANGE)
                        ? String(it.r.value)
                        : String(it.a.choices[it.a.index]);

      spriteA->drawString(valStr, _W - _th.marginR - 4, y + _th.rowH / 2);
    }

    y += _th.rowH;
  }
}

void EditMenu::drawCarouselWithValues() {
  spriteA->fillSprite(_th.bg);

  int widest = 0;
  for (int i = 0; i < _count; ++i) {
    int w = spriteA->textWidth(_items[i].text.c_str());
    if (w > widest) widest = w;
  }

  const int spacing = max(180, widest + 40);

  for (int i = 0; i < _count; ++i) {
    int x = _W / 2 + (i - _sel) * spacing;
    if (x < -150 || x > _W + 150) continue;

    const MenuItem& it = _items[i];
    bool sel = (i == _sel);

    spriteA->setTextDatum(MC_DATUM);

    if (sel) {
      int boxW = widest + 60;
      spriteA->fillRoundRect(x - boxW / 2, _H / 2 - 28, boxW, 56, _th.selectorRadius, _th.selFill);
      spriteA->drawRoundRect(x - boxW / 2, _H / 2 - 28, boxW, 56, _th.selectorRadius, _th.selBorder);
      spriteA->setTextColor(_th.fg, _th.selFill);
    } else {
      spriteA->setTextColor(_th.fg, _th.bg);
    }

    spriteA->setTextFont(_th.textFont);
    spriteA->drawString(it.text, x, _H / 2 - 10);

    if (it.edit != EditKind::NONE) {
      spriteA->setTextFont(_th.valueFont);
      spriteA->setTextDatum(MC_DATUM);

      // Default muted color
      uint16_t textCol = _th.muted;
      uint16_t bgCol   = sel ? _th.selFill : _th.bg;

      // Blink while editing (same logic as vertical)
      if (_editing && sel && (millis() / 300 % 2))
        textCol = _th.selBorder;

      spriteA->setTextColor(textCol, bgCol);

      String valStr = (it.edit == EditKind::RANGE)
                        ? String(it.r.value)
                        : String(it.a.choices[it.a.index]);

      spriteA->drawString(valStr, x, _H / 2 + 14);
    }
  }
}


// =========================================================
//  EDITMENU DRAW + UPDATE
// =========================================================
void EditMenu::draw() {
  if (!_dirty) return;
  if (!spriteA) spriteA = new TFT_eSprite(&_tft);

  spriteA->createSprite(_W, _H);
  if (_th.orientation == MenuOrientation::VERTICAL)
    drawListWithValues();
  else
    drawCarouselWithValues();

  drawArrowsIfNeededToBuffer(*spriteA);

  _tft.startWrite();
  spriteA->pushSprite(0, 0);
  _tft.endWrite();

  _dirty = false;
}

int EditMenu::update() {
  _activatedIndex = -1;
  static unsigned long lastBlink = 0;
  static bool blinkState = false;

  if (_editing) {
    _handleInputEdit();

    unsigned long now = millis();
    bool newState = (now / 300) % 2;
    if (newState != blinkState) {
      blinkState = newState;
      _dirty = true;
    }
  } else {
    _handleInput();
    // Reset blink when editing ends
    if (blinkState) {
      blinkState = false;
      _dirty = true;
    }
  }

  if (_dirty) draw(); // dirty hehe

  int ret = -1;
  if (_activatedIndex >= 0) {
    MenuItem& it = _items[_activatedIndex];
    if (it.submenu) {
      pushMenu(it.submenu);
      inputLockUntil = millis() + 150;
    } else if (it.edit != EditKind::NONE) {
      _editing = true;
    } else {
      ret = _activatedIndex;
    }
  }
  return ret;
}

void EditMenu::enableAutoSave(const char* path) {
  _autosave = true;
  _savePath = path;

  // if this dont work im blowing my brains out
  if (loadMenuSettings(*this, _savePath)) {
    _dirty = true;
    DBG_IF(MENU, "[Menu] Loaded settings from %s\n", _savePath);
  } else {
    DBG_IF(MENU, "[Menu] No existing settings file, will create new.\n");
  }
  // it worked
}


// =========================================================
//  SAVE / LOAD HELPERS (SD / FS)
// =========================================================
bool saveMenuSettings(MenuBase& menu, const char* path) {
  pinMode(TFT_CS, OUTPUT); digitalWrite(TFT_CS, HIGH);
  File f = SD.open(path, FILE_WRITE);
  if (!f) { digitalWrite(TFT_CS, LOW); return false; }

  StaticJsonDocument<512> doc;
  for (int i = 0; i < menu.size(); i++) // please work
    doc[String(i)] = menu.getItemValue(i);

  serializeJsonPretty(doc, f);
  f.close();
  digitalWrite(TFT_CS, LOW);
  return true;
}

bool loadMenuSettings(MenuBase& menu, const char* path) {
  pinMode(TFT_CS, OUTPUT); digitalWrite(TFT_CS, HIGH);
  File f = SD.open(path, FILE_READ);
  if (!f) { digitalWrite(TFT_CS, LOW); return false; }

  StaticJsonDocument<512> doc;
  DeserializationError err = deserializeJson(doc, f);
  f.close();
  digitalWrite(TFT_CS, LOW);

  if (err) return false;

  for (int i = 0; i < menu.size(); i++) { // please work
    if (doc.containsKey(String(i)))
      menu.setItemValue(i, doc[String(i)].as<long>());
  }
  return true;
}

// ======================= End of File =======================
