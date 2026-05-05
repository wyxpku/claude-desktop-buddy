#include <M5Unified.h>
#include <LittleFS.h>
#include <stdarg.h>
#include "ble_bridge.h"
#include "data.h"
#include "buddy.h"

using namespace lgfx::fonts;

M5Canvas spr(&M5.Display);

// Advertise as "Claude-XXXX" (last two BT MAC bytes) so multiple sticks
// in one room are distinguishable in the desktop picker. Name persists in
// btName for the BLUETOOTH info page.
static char btName[16] = "Claude";
static void startBt() {
  uint8_t mac[6] = {0};
  esp_read_mac(mac, ESP_MAC_BT);
  snprintf(btName, sizeof(btName), "Claude-%02X%02X", mac[4], mac[5]);
  bleInit(btName);
}

#include "character.h"
#include "stats.h"
#include "i18n.h"
const int W = 135, H = 240;
const int CX = W / 2;
const int CY_BASE = 120;
const int FH = 12;         // font height (efontCN_12)
const int FW = 12;         // approximate font width
const int CHARS_PER_LINE = 11;  // W / FW

// Colors used across multiple UI surfaces
const uint16_t HOT   = 0xFA20;   // red-orange: warnings, impatience, deny
const uint16_t PANEL = 0x2104;   // overlay panel background

enum PersonaState { P_SLEEP, P_IDLE, P_BUSY, P_ATTENTION, P_CELEBRATE, P_DIZZY, P_HEART };
const char* stateNames[] = { "sleep", "idle", "busy", "attention", "celebrate", "dizzy", "heart" };

TamaState    tama;
PersonaState baseState   = P_SLEEP;
PersonaState activeState = P_SLEEP;
uint32_t     oneShotUntil = 0;
uint32_t     lastShakeCheck = 0;
float        accelBaseline = 1.0f;
unsigned long t = 0;

// Menu
bool    menuOpen    = false;
uint8_t menuSel     = 0;
uint8_t brightLevel = 4;           // 0..4
bool    btnALong    = false;

enum DisplayMode { DISP_NORMAL, DISP_PET, DISP_INFO, DISP_COUNT };
uint8_t displayMode = DISP_NORMAL;
uint8_t infoPage = 0;
uint8_t petPage = 0;
const uint8_t PET_PAGES = 2;
uint8_t msgScroll = 0;
uint16_t lastLineGen = 0;
char     lastPromptId[40] = "";
uint32_t lastInteractMs = 0;
bool     dimmed = false;
bool     screenOff = false;
bool     swallowBtnA = false;
bool     swallowBtnB = false;
bool     buddyMode = false;
bool     gifAvailable = false;
const uint8_t SPECIES_GIF = 0xFF;   // species NVS sentinel: use the installed GIF

// Cycle GIF (if installed) → ASCII species 0..N-1 → GIF. Persisted to the
// existing "species" NVS key; 0xFF means GIF mode.
static void nextPet() {
  uint8_t n = buddySpeciesCount();
  if (!buddyMode) {                          // GIF → species 0
    buddyMode = true;
    buddySetSpeciesIdx(0);
    speciesIdxSave(0);
  } else if (buddySpeciesIdx() + 1 >= n && gifAvailable) {  // last species → GIF
    buddyMode = false;
    speciesIdxSave(SPECIES_GIF);
  } else {                                   // species i → species i+1
    buddyNextSpecies();
  }
  characterInvalidate();
  if (buddyMode) buddyInvalidate();
}
uint32_t wakeTransitionUntil = 0;
const uint32_t SCREEN_OFF_MS = 30000;

bool     napping = false;
uint32_t napStartMs = 0;
uint32_t promptArrivedMs = 0;

// Face-down = Z-axis dominant and negative. Debounced so a toss doesn't count.
static bool isFaceDown() {
  float ax, ay, az;
  M5.Imu.getAccel(&ax, &ay, &az);
  return az < -0.7f && fabsf(ax) < 0.4f && fabsf(ay) < 0.4f;
}

static void applyBrightness() { M5.Display.setBrightness(51 + brightLevel * 51); }

static void wake() {
  lastInteractMs = millis();
  if (screenOff) {
    M5.Display.wakeup();
    applyBrightness();
    screenOff = false;
    wakeTransitionUntil = millis() + 12000;
  }
  if (dimmed) { applyBrightness(); dimmed = false; }
}
bool     responseSent = false;

static void beep(uint16_t freq, uint16_t dur) {
  if (settings().sound) M5.Speaker.tone(freq, dur);
}

static void sendCmd(const char* json) {
  Serial.println(json);
  size_t n = strlen(json);
  bleWrite((const uint8_t*)json, n);
  bleWrite((const uint8_t*)"\n", 1);
}
const uint8_t INFO_PAGES = 6;
const uint8_t INFO_PG_BUTTONS = 1;
const uint8_t INFO_PG_CREDITS = 5;

static uint8_t  _pageScroll;

void applyDisplayMode() {
  bool peek = displayMode != DISP_NORMAL;
  characterSetPeek(peek);
  buddySetPeek(peek);
  _pageScroll = 0;
  spr.fillSprite(0x0000);
  characterInvalidate();
}

static const StrID menuIds[] = {
  S_MENU_SETTINGS, S_MENU_TURN_OFF, S_MENU_HELP, S_MENU_ABOUT, S_MENU_DEMO, S_MENU_CLOSE
};
const uint8_t MENU_N = 6;

bool    settingsOpen = false;
uint8_t settingsSel  = 0;
static const StrID settingsIds[] = {
  S_SET_BRIGHTNESS, S_SET_SOUND, S_SET_BLUETOOTH, S_SET_WIFI,
  S_SET_LED, S_SET_TRANSCRIPT, S_SET_LANGUAGE, S_SET_CLOCK_ROT,
  S_SET_ASCII_PET, S_SET_RESET, S_SET_BACK
};
const uint8_t SETTINGS_N = 11;

bool    resetOpen = false;
uint8_t resetSel  = 0;
static const StrID resetIds[] = { S_RESET_DELETE, S_RESET_FACTORY, S_RESET_BACK };
const uint8_t RESET_N = 3;
static uint32_t resetConfirmUntil = 0;
static uint8_t  resetConfirmIdx = 0xFF;

uint8_t settingsSubPage = 0;   // 0=main, 1=bluetooth
uint8_t btSel  = 0;
static const uint8_t BT_MENU_N = 3;  // toggle, clear bonds, back
static uint32_t btConfirmUntil = 0;
static bool     btClearArmed = false;

static void applySetting(uint8_t idx) {
  Settings& s = settings();
  switch (idx) {
    case 0:
      brightLevel = (brightLevel + 1) % 5;
      applyBrightness();
      return;
    case 1: s.sound = !s.sound; break;
    case 2:
      settingsSubPage = 1; btSel = 0; btClearArmed = false;
      return;
    case 3: s.wifi = !s.wifi; break;
    case 4: s.led = !s.led; break;
    case 5: s.hud = !s.hud; break;
    case 6:
      s.lang = (s.lang + 1) % LANG_COUNT;
      langSet((Lang)s.lang);
      break;
    case 7: s.clockRot = (s.clockRot + 1) % 3; break;
    case 8: nextPet(); return;
    case 9: resetOpen = true; resetSel = 0; resetConfirmIdx = 0xFF; return;
    case 10: settingsOpen = false; settingsSubPage = 0; characterInvalidate(); return;
  }
  settingsSave();
}

static void applyReset(uint8_t idx) {
  uint32_t now = millis();
  bool armed = (resetConfirmIdx == idx) && (int32_t)(now - resetConfirmUntil) < 0;

  if (idx == 2) { resetOpen = false; return; }

  if (!armed) {
    resetConfirmIdx = idx;
    resetConfirmUntil = now + 3000;
    beep(1400, 60);
    return;
  }

  beep(800, 200);
  if (idx == 0) {
    File d = LittleFS.open("/characters");
    if (d && d.isDirectory()) {
      File e;
      while ((e = d.openNextFile())) {
        char path[80];
        snprintf(path, sizeof(path), "/characters/%s", e.name());
        if (e.isDirectory()) {
          File f;
          while ((f = e.openNextFile())) {
            char fp[128];
            snprintf(fp, sizeof(fp), "%s/%s", path, f.name());
            f.close();
            LittleFS.remove(fp);
          }
          e.close();
          LittleFS.rmdir(path);
        } else {
          e.close();
          LittleFS.remove(path);
        }
      }
      d.close();
    }
  } else {
    _prefs.begin("buddy", false);
    _prefs.clear();
    _prefs.end();
    LittleFS.format();
    bleClearBonds();
  }
  delay(300);
  ESP.restart();
}

static void applyBtSetting(uint8_t idx) {
  if (idx == 0) {
    settings().bt = !settings().bt;
    settingsSave();
  } else if (idx == 1) {
    uint32_t now = millis();
    if (btClearArmed && (int32_t)(now - btConfirmUntil) < 0) {
      bleClearBonds();
      btClearArmed = false;
      beep(2400, 60);
    } else {
      btClearArmed = true;
      btConfirmUntil = now + 3000;
      beep(1400, 60);
    }
  } else {
    settingsSubPage = 0;
  }
}

const int MENU_HINT_H = 14;
static void drawMenuHints(const Palette& p, int mx, int mw, int hy,
                          const char* downLbl = "A", const char* rightLbl = "B") {
  spr.drawFastHLine(mx + 6, hy - 4, mw - 12, p.textDim);
  spr.setTextColor(p.textDim, PANEL);
  int x = mx + 8;
  spr.setCursor(x, hy); spr.print(downLbl);
  x += spr.textWidth(downLbl) + 4;
  spr.fillTriangle(x, hy + 1, x + 6, hy + 1, x + 3, hy + 6, p.textDim);
  x = mx + mw / 2 + 4;
  spr.setCursor(x, hy); spr.print(rightLbl);
  x += spr.textWidth(rightLbl) + 4;
  spr.fillTriangle(x, hy, x, hy + 6, x + 5, hy + 3, p.textDim);
}

static void drawSettings() {
  const Palette& p = characterPalette();
  int mw = 118, mh = 16 + SETTINGS_N * FH + MENU_HINT_H;
  int mx = (W - mw) / 2, my = (H - mh) / 2;
  spr.fillRoundRect(mx, my, mw, mh, 4, PANEL);
  spr.drawRoundRect(mx, my, mw, mh, 4, p.textDim);
  spr.setTextSize(1);

  if (settingsSubPage == 1) {
    // BT sub-page — same panel, different content
    uint8_t bondMacs[8][6];
    int bondCount = settings().bt ? bleGetBonds(bondMacs, 8) : 0;
    int y = my + 8;

    // Section: title (selectable — btSel 0 toggles BT)
    bool titleSel = (btSel == 0);
    spr.setTextColor(titleSel ? p.text : p.textDim, PANEL);
    spr.setCursor(mx + 6, y);
    spr.print(titleSel ? "> " : "  ");
    spr.print(T(S_SET_BLUETOOTH));
    // Status indicator (connected / on / off)
    const char* st = bleConnected() ? T(S_LINKED) : (settings().bt ? T(S_ON) : T(S_OFF));
    uint16_t stCol = bleConnected() ? GREEN : (settings().bt ? GREEN : p.textDim);
    spr.setTextColor(stCol, PANEL);
    spr.setCursor(mx + mw - spr.textWidth(st) - 6, y);
    spr.print(st);
    y += FH;
    // Separator line
    spr.drawFastHLine(mx + 6, y, mw - 12, p.textDim);
    y += 4;

    // Section: paired devices
    spr.setTextColor(p.textDim, PANEL);
    spr.setCursor(mx + 6, y);
    spr.print(T(S_BT_DEVICES));
    // Device count on the right
    char cntBuf[8];
    snprintf(cntBuf, sizeof(cntBuf), "%d", bondCount);
    spr.setCursor(mx + mw - spr.textWidth(cntBuf) - 6, y);
    spr.print(cntBuf);
    y += FH;
    if (bondCount == 0) {
      spr.setTextColor(p.textDim, PANEL);
      spr.setCursor(mx + 10, y);
      spr.print(T(S_BT_NO_DEVICE));
      y += FH;
    } else {
      for (int d = 0; d < bondCount && d < 4; d++) {
        spr.setTextColor(p.textDim, PANEL);
        spr.setCursor(mx + 10, y);
        spr.printf("%02X:%02X:%02X:%02X:%02X:%02X",
          bondMacs[d][0], bondMacs[d][1], bondMacs[d][2],
          bondMacs[d][3], bondMacs[d][4], bondMacs[d][5]);
        y += FH;
      }
    }
    y += 2;
    // Separator
    spr.drawFastHLine(mx + 6, y, mw - 12, p.textDim);
    y += 4;

    // Selectable actions: btSel 1=clear, 2=back
    for (int i = 1; i <= 2; i++) {
      bool sel = (btSel == i);
      spr.setTextColor(sel ? p.text : p.textDim, PANEL);
      spr.setCursor(mx + 6, y);
      spr.print(sel ? "> " : "  ");
      if (i == 1) {
        bool armed = btClearArmed && (int32_t)(millis() - btConfirmUntil) < 0;
        if (armed) spr.setTextColor(HOT, PANEL);
        spr.print(armed ? T(S_REALLY) : T(S_BT_CLEAR));
      } else {
        spr.print(T(S_BT_BACK));
      }
      y += FH;
    }
    drawMenuHints(p, mx, mw, my + mh - 12, T(S_NEXT), T(S_CHANGE));
    return;
  }

  Settings& s = settings();
  bool vals[] = { s.sound, s.bt, s.wifi, s.led, s.hud };
  for (int i = 0; i < SETTINGS_N; i++) {
    bool sel = (i == settingsSel);
    int iy = my + 8 + i * FH;
    spr.setTextColor(sel ? p.text : p.textDim, PANEL);
    spr.setCursor(mx + 6, iy);
    spr.print(sel ? "> " : "  ");
    spr.print(T(settingsIds[i]));
    char vb[16] = ""; uint16_t vc = p.textDim;
    if (i == 0) {
      snprintf(vb, sizeof(vb), "%u/4", brightLevel);
    } else if (i >= 1 && i <= 5) {
      vc = vals[i-1] ? GREEN : p.textDim;
      strncpy(vb, vals[i-1] ? T(S_ON) : T(S_OFF), sizeof(vb)-1);
    } else if (i == 6) {
      strncpy(vb, s.lang == LANG_EN ? T(S_ENGLISH) : T(S_CHINESE), sizeof(vb)-1);
    } else if (i == 7) {
      static const StrID rotIds[] = { S_AUTO, S_PORT, S_LAND };
      strncpy(vb, T(rotIds[s.clockRot]), sizeof(vb)-1);
    } else if (i == 8) {
      uint8_t total = buddySpeciesCount() + (gifAvailable ? 1 : 0);
      uint8_t pos   = buddyMode ? buddySpeciesIdx() + 1 : total;
      snprintf(vb, sizeof(vb), "%u/%u", pos, total);
    }
    if (vb[0]) {
      spr.setTextColor(vc, PANEL);
      spr.setCursor(mx + mw - spr.textWidth(vb) - 6, iy);
      spr.print(vb);
    }
  }
  drawMenuHints(p, mx, mw, my + mh - 12, T(S_NEXT), T(S_CHANGE));
}

static void drawReset() {
  const Palette& p = characterPalette();
  int mw = 118, mh = 16 + RESET_N * FH + MENU_HINT_H;
  int mx = (W - mw) / 2, my = (H - mh) / 2;
  spr.fillRoundRect(mx, my, mw, mh, 4, PANEL);
  spr.drawRoundRect(mx, my, mw, mh, 4, HOT);
  spr.setTextSize(1);
  for (int i = 0; i < RESET_N; i++) {
    bool sel = (i == resetSel);
    spr.setTextColor(sel ? p.text : p.textDim, PANEL);
    spr.setCursor(mx + 6, my + 8 + i * FH);
    spr.print(sel ? "> " : "  ");
    bool armed = (i == resetConfirmIdx) &&
                 (int32_t)(millis() - resetConfirmUntil) < 0;
    if (armed) spr.setTextColor(HOT, PANEL);
    spr.print(armed ? T(S_REALLY) : T(resetIds[i]));
  }
  drawMenuHints(p, mx, mw, my + mh - 12);
}

void menuConfirm() {
  switch (menuSel) {
    case 0: settingsOpen = true; menuOpen = false; settingsSel = 0; settingsSubPage = 0; break;
    case 1: M5.Power.powerOff(); break;
    case 2:
    case 3:
      menuOpen = false;
      displayMode = DISP_INFO;
      infoPage = (menuSel == 2) ? INFO_PG_BUTTONS : INFO_PG_CREDITS;
      applyDisplayMode();
      characterInvalidate();
      break;
    case 4: dataSetDemo(!dataDemo()); break;
    case 5: menuOpen = false; characterInvalidate(); break;
  }
}

void drawMenu() {
  const Palette& p = characterPalette();
  int mw = 118, mh = 16 + MENU_N * FH + MENU_HINT_H;
  int mx = (W - mw) / 2, my = (H - mh) / 2;
  spr.fillRoundRect(mx, my, mw, mh, 4, PANEL);
  spr.drawRoundRect(mx, my, mw, mh, 4, p.textDim);
  spr.setTextSize(1);
  for (int i = 0; i < MENU_N; i++) {
    bool sel = (i == menuSel);
    int iy = my + 8 + i * FH;
    spr.setTextColor(sel ? p.text : p.textDim, PANEL);
    spr.setCursor(mx + 6, iy);
    spr.print(sel ? "> " : "  ");
    spr.print(T(menuIds[i]));
    if (i == 4) {
      const char* dv = dataDemo() ? T(S_ON) : T(S_OFF);
      spr.setTextColor(dataDemo() ? GREEN : p.textDim, PANEL);
      spr.setCursor(mx + mw - spr.textWidth(dv) - 6, iy);
      spr.print(dv);
    }
  }
  drawMenuHints(p, mx, mw, my + mh - 12);
}

// Clock orientation: gravity along the in-plane X axis means the stick is
// on its side. Signed counter for hysteresis on both transitions.
//   0 = portrait (sprite path, pet sleeps underneath)
//   1 = landscape, BtnA-side down (rotation 1)
//   3 = landscape, USB-side down (rotation 3)
static uint8_t clockOrient   = 0;
static int8_t  orientFrames  = 0;
static uint8_t paintedOrient = 0;
// Cache the time once per second.
static m5::rtc_time_t _clkTm;
static m5::rtc_date_t _clkDt;
uint32_t               _clkLastRead = 0;
static bool            _onUsb       = false;
static void clockRefreshRtc() {
  if (millis() - _clkLastRead < 1000) return;
  _clkLastRead = millis();
  _onUsb = M5.Power.isCharging() != m5::Power_Class::is_discharging;
  _clkTm = M5.Rtc.getTime();
  _clkDt = M5.Rtc.getDate();
}

static void clockUpdateOrient() {
  float ax, ay, az;
  M5.Imu.getAccel(&ax, &ay, &az);
  uint8_t lock = settings().clockRot;
  if (lock == 1) { clockOrient = 0; return; }
  if (lock == 2) {
    if (clockOrient == 0) clockOrient = (ax >= 0) ? 1 : 3;
    if      (ax >  0.5f && clockOrient != 1) clockOrient = 1;
    else if (ax < -0.5f && clockOrient != 3) clockOrient = 3;
    return;
  }
  bool side = (clockOrient == 0)
    ? fabsf(ax) > 0.7f && fabsf(ay) < 0.5f && fabsf(az) < 0.5f
    : fabsf(ax) > 0.4f;
  if (side) { if (orientFrames < 20) orientFrames++; }
  else      { if (orientFrames > -10) orientFrames--; }
  if (clockOrient == 0 && orientFrames >= 15) {
    clockOrient = (ax > 0) ? 1 : 3;
  } else if (clockOrient != 0 && orientFrames <= -8) {
    clockOrient = 0;
  } else if (clockOrient != 0 && side) {
    static int8_t swapFrames = 0;
    uint8_t want = (ax > 0) ? 1 : 3;
    if (want != clockOrient) { if (++swapFrames >= 8) { clockOrient = want; swapFrames = 0; } }
    else swapFrames = 0;
  }
}

static const StrID monIds[] = {
  S_JAN, S_FEB, S_MAR, S_APR, S_MAY, S_JUN,
  S_JUL, S_AUG, S_SEP, S_OCT, S_NOV, S_DEC
};
static const StrID dowIds[] = { S_SUN, S_MON, S_TUE, S_WED, S_THU, S_FRI, S_SAT };

static uint8_t clockDow() { return _clkDt.weekDay % 7; }
static void drawClock() {
  const Palette& p = characterPalette();
  char hm[6]; snprintf(hm, sizeof(hm), "%02u:%02u", _clkTm.hours, _clkTm.minutes);
  char ss[4]; snprintf(ss, sizeof(ss), ":%02u", _clkTm.seconds);
  uint8_t mi = (_clkDt.month >= 1 && _clkDt.month <= 12) ? _clkDt.month - 1 : 0;
  char dl[16]; snprintf(dl, sizeof(dl), "%s %02u", T(monIds[mi]), _clkDt.date);

  if (clockOrient == 0) {
    paintedOrient = 0;
    spr.fillRect(0, 90, W, H - 90, p.bg);
    spr.setTextDatum(MC_DATUM);
    spr.setTextSize(3); spr.setTextColor(p.text, p.bg);    spr.drawString(hm, CX, 140);
    spr.setTextSize(1); spr.setTextColor(p.textDim, p.bg); spr.drawString(ss, CX, 170);
                                                         spr.drawString(dl, CX, 190);
    spr.setTextDatum(TL_DATUM);
    return;
  }

  M5.Display.setRotation(clockOrient);
  M5.Display.setFont(&efontCN_12);
  static uint8_t lastSec = 0xFF;
  bool repaint = paintedOrient != clockOrient;
  if (repaint) { M5.Display.fillScreen(p.bg); paintedOrient = clockOrient; lastSec = 0xFF; }

  if (repaint || _clkTm.seconds != lastSec) {
    lastSec = _clkTm.seconds;
    char wdl[16]; snprintf(wdl, sizeof(wdl), "%s %s %02u", T(dowIds[clockDow()]), T(monIds[mi]), _clkDt.date);
    char ssl[3]; snprintf(ssl, sizeof(ssl), "%02u", _clkTm.seconds);
    M5.Display.setTextDatum(MC_DATUM);
    M5.Display.setTextSize(2); M5.Display.setTextColor(p.text, p.bg);    M5.Display.drawString(hm, 170, 42);
    M5.Display.setTextSize(1); M5.Display.setTextColor(p.textDim, p.bg); M5.Display.drawString(ssl, 170, 72);
                                                                          M5.Display.drawString(wdl, 170, 102);
    M5.Display.setTextDatum(TL_DATUM);
  }

  static uint32_t lastPetTick = 0;
  if (millis() - lastPetTick >= 200) {
    lastPetTick = millis();
    if (buddyMode) {
      M5.Display.fillRect(0, 0, 115, 90, p.bg);
      buddyRenderTo(&M5.Display, activeState);
    } else {
      characterSetState(activeState);
      characterRenderTo(&M5.Display, 57, 45);
    }
  }
  M5.Display.setRotation(0);
}

PersonaState derive(const TamaState& s) {
  if (!s.connected)            return P_IDLE;
  if (s.sessionsWaiting > 0)   return P_ATTENTION;
  if (s.recentlyCompleted)     return P_CELEBRATE;
  if (s.sessionsRunning >= 3)  return P_BUSY;
  return P_IDLE;
}

void triggerOneShot(PersonaState s, uint32_t durMs) {
  activeState = s;
  oneShotUntil = millis() + durMs;
}

bool checkShake() {
  float ax, ay, az;
  M5.Imu.getAccel(&ax, &ay, &az);
  float mag = sqrtf(ax*ax + ay*ay + az*az);
  float delta = fabsf(mag - accelBaseline);
  accelBaseline = accelBaseline * 0.95f + mag * 0.05f;
  return delta > 0.8f;
}

// Advance past one UTF-8 codepoint, return its byte length
static uint8_t utf8cp(const char* p, uint8_t& bytes) {
  uint8_t b = *p;
  if (b == 0) { bytes = 0; return 0; }
  if (b < 0x80) { bytes = 1; }
  else if ((b & 0xE0) == 0xC0) { bytes = 2; }
  else if ((b & 0xF0) == 0xE0) { bytes = 3; }
  else if ((b & 0xF8) == 0xF0) { bytes = 4; }
  else { bytes = 1; }
  return bytes;
}

// Estimate pixel width of a UTF-8 codepoint (avoids calling textWidth per char)
static int charPx(uint8_t byteLen) {
  return byteLen == 1 ? 8 : 12;  // ASCII ~8px, CJK/wide ~12px
}

static uint8_t wrapInto(const char* in, char out[][36], uint8_t maxRows, uint8_t width) {
  const int maxPx = W - 8;
  uint8_t row = 0, col = 0;
  int px = 0;
  const char* p = in;
  while (*p && row < maxRows) {
    if (*p == ' ' && col > 0) {
      const char* wp = p + 1;
      // If next word starts with CJK, skip lookahead — CJK can break
      // anywhere. Word lookahead would orphan short English words (e.g.
      // "Claude") on their own line between CJK runs.
      if ((*wp & 0x80) == 0) {
        int wpx = 0;
        while (*wp && *wp != ' ') {
          uint8_t bl; utf8cp(wp, bl); wpx += charPx(bl); wp += bl ? bl : 1;
        }
        if (px + charPx(1) + wpx > maxPx) {
          out[row][col] = 0;
          if (++row >= maxRows) return row;
          col = 0; px = 0;
          p++; continue;
        }
      }
      out[row][col++] = ' '; px += charPx(1);
      p++; continue;
    }
    uint8_t bl;
    utf8cp(p, bl);
    if (bl == 0) break;
    int cpx = charPx(bl);
    if (col + bl >= 36 || (col > 0 && px + cpx > maxPx)) {
      out[row][col] = 0;
      if (++row >= maxRows) return row;
      col = 0; px = 0;
    }
    memcpy(&out[row][col], p, bl);
    col += bl; px += cpx;
    out[row][col] = 0;
    p += bl;
  }
  if (col > 0 && row < maxRows) { out[row][col] = 0; row++; }
  return row;
}

// --- Auto-layout system ---------------------------------------------------
struct TextItem { const char* text; uint16_t color; const char* value; };
static TextItem _infoItems[30];
static uint8_t  _infoItemCount;

static char     _pageRows[40][36];
static uint16_t _pageRowColor[40];
static uint8_t  _pageRowCount;
static const char* _pageRowValue[40];

static char     _fmtBuf[10][320];
static uint8_t  _fmtIdx;

static const char* _fmt(const char* fmt, ...) {
  char* b = _fmtBuf[_fmtIdx % 10];
  va_list a; va_start(a, fmt); vsnprintf(b, sizeof(_fmtBuf[0]), fmt, a); va_end(a);
  return _fmtBuf[_fmtIdx++ % 10];
}

static void layoutItems(const TextItem* items, uint8_t n) {
  _pageRowCount = 0;
  for (uint8_t i = 0; i < n && _pageRowCount < 40; i++) {
    if (!items[i].text || !items[i].text[0]) continue;
    uint8_t room = 40 - _pageRowCount;
    if (room == 0) break;
    if (items[i].value) {
      // Stat line: one row, label left + value right
      strncpy(_pageRows[_pageRowCount], items[i].text, 35);
      _pageRows[_pageRowCount][35] = 0;
      _pageRowColor[_pageRowCount] = items[i].color;
      _pageRowValue[_pageRowCount] = items[i].value;
      _pageRowCount++;
    } else {
      uint8_t got = wrapInto(items[i].text, &_pageRows[_pageRowCount], room, 0);
      for (uint8_t j = 0; j < got; j++) {
        _pageRowColor[_pageRowCount + j] = items[i].color;
        _pageRowValue[_pageRowCount + j] = nullptr;
      }
      _pageRowCount += got;
    }
  }
}

// --- Info page builders ---------------------------------------------------

static void buildAboutPage(const Palette& p) {
  auto& it = _infoItems; uint8_t i = 0;
  it[i++] = {T(S_ABOUT_1), p.textDim};
  it[i++] = {T(S_ABOUT_2), p.textDim};
  _infoItemCount = i;
}

static void buildButtonsPage(const Palette& p) {
  auto& it = _infoItems; uint8_t i = 0;
  it[i++] = {_fmt("%s:", T(S_A_FRONT)), p.text};
  it[i++] = {_fmt("  %s, %s", T(S_NEXT_SCREEN), T(S_APPROVE_PROMPT)), p.textDim};
  it[i++] = {_fmt("%s:", T(S_B_RIGHT)), p.text};
  it[i++] = {_fmt("  %s, %s", T(S_NEXT_PAGE), T(S_DENY_PROMPT)), p.textDim};
  it[i++] = {_fmt("%s:", T(S_HOLD_A)), p.text};
  it[i++] = {_fmt("  %s", T(S_MENU)), p.textDim};
  it[i++] = {_fmt("%s:", T(S_POWER)), p.text};
  it[i++] = {_fmt("  %s", T(S_TAP_OFF_HOLD_PWR)), p.textDim};
  _infoItemCount = i;
}

static void buildClaudePage(const Palette& p) {
  auto& it = _infoItems; uint8_t i = 0;
  it[i++] = {T(S_SESSIONS), p.textDim, _fmt("%u", tama.sessionsTotal)};
  it[i++] = {T(S_RUNNING), p.textDim, _fmt("%u", tama.sessionsRunning)};
  it[i++] = {T(S_WAITING), p.textDim, _fmt("%u", tama.sessionsWaiting)};
  it[i++] = {T(S_LINK), p.text};
  const char* sec = dataScenarioName();
  it[i++] = {T(S_VIA), p.textDim, sec};
  const char* bleState = !bleConnected() ? "-" : bleSecure() ? T(S_ENCRYPTED) : T(S_OPEN);
  it[i++] = {T(S_BLE), p.textDim, bleState};
  uint32_t age = (millis() - tama.lastUpdated) / 1000;
  it[i++] = {T(S_LAST_MSG), p.textDim, _fmt("%lus", (unsigned long)age)};
  it[i++] = {T(S_STATE), p.textDim, stateNames[activeState]};
  _infoItemCount = i;
}

static void buildDevicePage(const Palette& p) {
  auto& it = _infoItems; uint8_t i = 0;
  int vBat_mV = M5.Power.getBatteryVoltage();
  int iBat_mA = M5.Power.getBatteryCurrent();
  int pct = (vBat_mV - 3200) / 10;
  if (pct < 0) pct = 0; if (pct > 100) pct = 100;
  bool usb = _onUsb;
  bool charging = usb && iBat_mA > 1;
  bool full = usb && vBat_mV > 4100 && abs(iBat_mA) < 10;
  const char* batState = full ? T(S_FULL) : (charging ? T(S_CHARGING) : (usb ? T(S_USB) : T(S_BATTERY_STATE)));
  it[i++] = {_fmt("%d%% %s", pct, batState), full ? GREEN : (charging ? HOT : p.textDim)};
  it[i++] = {T(S_BATTERY), p.textDim, _fmt("%d.%02dV", vBat_mV/1000, (abs(vBat_mV)%1000)/10)};
  it[i++] = {T(S_CURRENT), p.textDim, _fmt("%+dmA", iBat_mA)};
  it[i++] = {T(S_SYSTEM), p.text};
  if (ownerName()[0]) it[i++] = {T(S_OWNER), p.textDim, ownerName()};
  uint32_t up = millis() / 1000;
  it[i++] = {T(S_UPTIME), p.textDim, _fmt("%luh %02lum", up / 3600, (up / 60) % 60)};
  it[i++] = {T(S_HEAP), p.textDim, _fmt("%uKB", ESP.getFreeHeap() / 1024)};
  it[i++] = {T(S_BRIGHT), p.textDim, _fmt("%u/4", brightLevel)};
  it[i++] = {T(S_BT), p.textDim, settings().bt ? (dataBtActive() ? T(S_LINKED) : "on") : T(S_OFF)};
  _infoItemCount = i;
}

static void buildBluetoothPage(const Palette& p) {
  auto& it = _infoItems; uint8_t i = 0;
  bool linked = settings().bt && dataBtActive();
  it[i++] = {linked ? T(S_LINKED) : (settings().bt ? T(S_DISCOVER) : T(S_OFF)),
             linked ? GREEN : (settings().bt ? HOT : p.textDim)};
  it[i++] = {btName, p.text};
  uint8_t mac[6] = {0}; esp_read_mac(mac, ESP_MAC_BT);
  it[i++] = {_fmt("%02X:%02X:%02X:%02X:%02X:%02X",
    mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]), p.textDim};
  if (linked) {
    uint32_t age = (millis() - tama.lastUpdated) / 1000;
    it[i++] = {T(S_LAST_MSG), p.textDim, _fmt("%lus", (unsigned long)age)};
  } else if (settings().bt) {
    it[i++] = {T(S_TO_PAIR), p.text};
    it[i++] = {T(S_OPEN_CLAUDE), p.textDim};
    it[i++] = {T(S_DEV), p.textDim};
    it[i++] = {T(S_HW_BUDDY), p.textDim};
    it[i++] = {T(S_AUTO_CONNECT), p.textDim};
  }
  _infoItemCount = i;
}

static void buildCreditsPage(const Palette& p) {
  auto& it = _infoItems; uint8_t i = 0;
  it[i++] = {_fmt("%s %s", T(S_MADE_BY), T(S_FELIX)), p.textDim};
  it[i++] = {_fmt("%s %s", T(S_SOURCE), "github.com/anthropics/claude-desktop-buddy"), p.textDim};
  it[i++] = {_fmt("%s %s", T(S_HARDWARE), "M5StickS3, ESP32-S3"), p.textDim};
  _infoItemCount = i;
}

// --- Info header + render -------------------------------------------------

static const char* _infoSectionTitle() {
  static const StrID sectionIds[] = {S_H_ABOUT, S_H_BUTTONS, S_H_CLAUDE, S_H_DEVICE, S_H_BLUETOOTH, S_H_CREDITS};
  return T(sectionIds[infoPage]);
}

void drawPasskey() {
  const Palette& p = characterPalette();
  spr.fillSprite(p.bg);
  spr.setTextColor(p.textDim, p.bg);
  spr.setCursor(8, 56);  spr.print(T(S_BT_PAIRING));
  spr.setCursor(8, 184); spr.print(T(S_ENTER_DESKTOP));
  spr.setTextColor(p.text, p.bg);
  char b[8]; snprintf(b, sizeof(b), "%06lu", (unsigned long)blePasskey());
  int tw = spr.textWidth(b);
  spr.setCursor((W - tw) / 2, 110);
  spr.print(b);
}

void drawInfo() {
  const Palette& p = characterPalette();
  const int TOP = 70;
  spr.fillRect(0, TOP, W, H - TOP, p.bg);
  spr.setTextSize(1);
  int y = TOP + 2;

  // Header: "Info" + page number + section title
  spr.setTextColor(p.text, p.bg);
  spr.setCursor(4, y); spr.print(T(S_INFO));
  spr.setTextColor(p.textDim, p.bg);
  spr.setCursor(W - 36, y); spr.printf("%u/%u", infoPage + 1, INFO_PAGES);
  y += FH;
  spr.setTextColor(p.body, p.bg);
  spr.setCursor(4, y); spr.print(_infoSectionTitle());
  y += FH + 2;

  // Build page content
  _fmtIdx = 0;
  switch (infoPage) {
    case 0: buildAboutPage(p); break;
    case 1: buildButtonsPage(p); break;
    case 2: buildClaudePage(p); break;
    case 3: buildDevicePage(p); break;
    case 4: buildBluetoothPage(p); break;
    case 5: buildCreditsPage(p); break;
  }
  layoutItems(_infoItems, _infoItemCount);

  // Render visible rows with scroll
  uint8_t maxVis = (H - y - 2) / FH;
  if (_pageScroll >= _pageRowCount) _pageScroll = 0;
  if (maxVis > _pageRowCount - _pageScroll) maxVis = _pageRowCount - _pageScroll;

  for (uint8_t r = 0; r < maxVis; ) {
    uint8_t ri = _pageScroll + r;
    int rowY = y + r * FH;

    if (_pageRowValue[ri]) {
      // Find consecutive stat group
      uint8_t gEnd = r + 1;
      while (gEnd < maxVis && _pageRowValue[_pageScroll + gEnd]) gEnd++;
      // Max label width → value alignment point
      int maxLW = 0;
      for (uint8_t g = r; g < gEnd; g++) {
        int w = spr.textWidth(_pageRows[_pageScroll + g]);
        if (w > maxLW) maxLW = w;
      }
      int valX = 4 + maxLW + 10;
      for (uint8_t g = r; g < gEnd; g++) {
        uint8_t gi = _pageScroll + g;
        spr.setTextColor(_pageRowColor[gi], p.bg);
        spr.setCursor(4, y + g * FH);
        spr.print(_pageRows[gi]);
        spr.setCursor(valX, y + g * FH);
        spr.print(_pageRowValue[gi]);
      }
      r = gEnd;
    } else {
      spr.setTextColor(_pageRowColor[ri], p.bg);
      spr.setCursor(4, rowY);
      bool isLastVis = (r == maxVis - 1);
      bool hasMore = (ri + 1 < _pageRowCount);
      if (isLastVis && hasMore) {
        char tmp[36]; memcpy(tmp, _pageRows[ri], 36);
        int px = 0; uint8_t j = 0;
        while (tmp[j] && px < 127 - 24) {
          uint8_t bl; utf8cp(&tmp[j], bl); if (!bl) break;
          px += charPx(bl); j += bl;
        }
        tmp[j] = 0;
        spr.print(tmp); spr.print("...");
      } else {
        spr.print(_pageRows[ri]);
      }
      r++;
    }
  }

  // Scroll indicator
  if (_pageRowCount > maxVis + _pageScroll) {
    spr.setTextColor(p.textDim, p.bg);
    spr.setCursor(W - 8, H - FH - 2);
    spr.print("v");
  }
}


static void drawApproval() {
  const Palette& p = characterPalette();
  const int AREA = FH * 5 + 8;
  spr.fillRect(0, H - AREA, W, AREA, p.bg);
  spr.drawFastHLine(0, H - AREA, W, p.textDim);

  spr.setTextColor(p.textDim, p.bg);
  spr.setCursor(4, H - AREA + 4);
  uint32_t waited = (millis() - promptArrivedMs) / 1000;
  if (waited >= 10) spr.setTextColor(HOT, p.bg);
  spr.printf(T(S_APPROVE_FMT), (unsigned long)waited);

  spr.setTextColor(p.text, p.bg);
  spr.setCursor(4, H - AREA + 4 + FH);
  spr.print(tama.promptTool);

  spr.setTextColor(p.textDim, p.bg);
  int y = H - AREA + 4 + FH * 2;
  // print hint text, breaking into two lines by estimated pixel width
  const char* h = tama.promptHint;
  const int maxPx = W - 8;
  const char* start = h;
  for (int line = 0; line < 2 && *start; line++) {
    int px = 0;
    const char* end = start;
    while (*end) {
      uint8_t bl; utf8cp(end, bl); if (!bl) break;
      int cpx = charPx(bl);
      if (px + cpx > maxPx) break;
      px += cpx;
      end += bl;
    }
    int take = end - start;
    if (take == 0 && *end) { uint8_t bl; utf8cp(end, bl); take = bl ? bl : 1; }
    spr.setCursor(4, y + line * FH);
    spr.printf("%.*s", take, start);
    start += take;
  }

  if (responseSent) {
    spr.setTextColor(p.textDim, p.bg);
    spr.setCursor(4, H - FH - 2);
    spr.print(T(S_SENT));
  } else {
    spr.setTextColor(GREEN, p.bg);
    spr.setCursor(4, H - FH - 2);
    spr.print(T(S_A_YES));
    spr.setTextColor(HOT, p.bg);
    spr.setCursor(W - 36, H - FH - 2);
    spr.print(T(S_B_NO));
  }
}

static void tinyHeart(int x, int y, bool filled, uint16_t col) {
  if (filled) {
    spr.fillCircle(x - 2, y, 2, col);
    spr.fillCircle(x + 2, y, 2, col);
    spr.fillTriangle(x - 4, y + 1, x + 4, y + 1, x, y + 5, col);
  } else {
    spr.drawCircle(x - 2, y, 2, col);
    spr.drawCircle(x + 2, y, 2, col);
    spr.drawLine(x - 4, y + 1, x, y + 5, col);
    spr.drawLine(x + 4, y + 1, x, y + 5, col);
  }
}

static void drawPetStats(const Palette& p) {
  const int TOP = 70 + FH;  // below pet name header
  spr.fillRect(0, TOP, W, H - TOP, p.bg);
  spr.setTextSize(1);
  int y = TOP + 4;
  // y is the top of each row; text cursor goes at y, graphics centered at y + FH/2
  auto textY = [](int rowY) -> int { return rowY; };
  auto iconY = [](int rowY) -> int { return rowY + FH / 2; };

  spr.setTextColor(p.textDim, p.bg);
  spr.setCursor(6, textY(y)); spr.print(T(S_MOOD));
  uint8_t mood = statsMoodTier();
  uint16_t moodCol = (mood >= 3) ? RED : (mood >= 2) ? HOT : p.textDim;
  int icy = iconY(y);
  for (int i = 0; i < 4; i++) tinyHeart(54 + i * 16, icy, i < mood, moodCol);

  y += FH + 4;
  spr.setCursor(6, textY(y)); spr.print(T(S_FED));
  uint8_t fed = statsFedProgress();
  icy = iconY(y);
  for (int i = 0; i < 10; i++) {
    int px = 38 + i * 9;
    if (i < fed) spr.fillCircle(px, icy, 2, p.body);
    else spr.drawCircle(px, icy, 2, p.textDim);
  }

  y += FH + 4;
  spr.setCursor(6, textY(y)); spr.print(T(S_ENERGY));
  uint8_t en = statsEnergyTier();
  uint16_t enCol = (en >= 4) ? 0x07FF : (en >= 2) ? 0xFFE0 : HOT;
  icy = iconY(y);
  for (int i = 0; i < 5; i++) {
    int px = 54 + i * 13;
    if (i < en) spr.fillRect(px, icy - 3, 9, 6, enCol);
    else spr.drawRect(px, icy - 3, 9, 6, p.textDim);
  }

  y += FH + 6;
  int badgeH = FH + 4;
  spr.fillRoundRect(6, y, 44, badgeH, 3, p.body);
  spr.setTextColor(p.bg, p.body);
  spr.setCursor(11, y + 2); spr.printf(T(S_LV), stats().level);

  y += badgeH + 6;
  spr.setTextColor(p.textDim, p.bg);
  // Compute value alignment: max label width in stats block
  const char* sLabels[] = { T(S_APPROVED), T(S_DENIED), T(S_NAPPED), T(S_TOKENS), T(S_TODAY) };
  int maxLW = 0;
  for (int j = 0; j < 5; j++) { int w = spr.textWidth(sLabels[j]); if (w > maxLW) maxLW = w; }
  int valX = 6 + maxLW + 10;

  spr.setCursor(6, y);          spr.print(T(S_APPROVED));
  spr.setCursor(valX, y);       spr.printf("%u", stats().approvals);
  spr.setCursor(6, y + FH);     spr.print(T(S_DENIED));
  spr.setCursor(valX, y + FH);  spr.printf("%u", stats().denials);
  uint32_t nap = stats().napSeconds;
  spr.setCursor(6, y + FH * 2);     spr.print(T(S_NAPPED));
  spr.setCursor(valX, y + FH * 2);  spr.printf("%luh%02lum", nap/3600, (nap/60)%60);
  auto tokFmt = [&](const char* label, uint32_t v, int yPx) {
    spr.setCursor(6, yPx); spr.print(label);
    spr.setCursor(valX, yPx);
    if (v >= 1000000)   spr.printf("%lu.%luM", v/1000000, (v/100000)%10);
    else if (v >= 1000) spr.printf("%lu.%luK", v/1000, (v/100)%10);
    else                spr.printf("%lu", v);
  };
  tokFmt(T(S_TOKENS), stats().tokens, y + FH * 3);
  tokFmt(T(S_TODAY), tama.tokensToday, y + FH * 4);
}

static void drawPetHowTo(const Palette& p) {
  const int TOP = 70 + FH;
  spr.fillRect(0, TOP, W, H - TOP, p.bg);
  spr.setTextSize(1);

  auto& it = _infoItems; uint8_t i = 0;
  it[i++] = {T(S_HOW_MOOD), p.body};
  it[i++] = {T(S_APPROVE_FAST), p.textDim};
  it[i++] = {T(S_DENY_LOTS), p.textDim};
  it[i++] = {T(S_HOW_FED), p.body};
  it[i++] = {T(S_50K_TOKENS), p.textDim};
  it[i++] = {T(S_LEVEL_UP), p.textDim};
  it[i++] = {T(S_HOW_ENERGY), p.body};
  it[i++] = {T(S_FACE_DOWN), p.textDim};
  it[i++] = {T(S_REFILLS), p.textDim};
  it[i++] = {T(S_IDLE_OFF), p.textDim};
  it[i++] = {T(S_BTN_WAKE), p.textDim};
  it[i++] = {T(S_A_SCREEN_B_PAGE), p.textDim};
  it[i++] = {T(S_HOLD_A_MENU), p.textDim};
  layoutItems(_infoItems, i);

  int y = TOP + 4;
  uint8_t maxVis = (H - y - 2) / FH;
  if (_pageScroll >= _pageRowCount) _pageScroll = 0;
  if (maxVis > _pageRowCount - _pageScroll) maxVis = _pageRowCount - _pageScroll;

  for (uint8_t r = 0; r < maxVis; ) {
    uint8_t ri = _pageScroll + r;
    int rowY = y + r * FH;

    if (_pageRowValue[ri]) {
      uint8_t gEnd = r + 1;
      while (gEnd < maxVis && _pageRowValue[_pageScroll + gEnd]) gEnd++;
      int maxLW = 0;
      for (uint8_t g = r; g < gEnd; g++) {
        int w = spr.textWidth(_pageRows[_pageScroll + g]);
        if (w > maxLW) maxLW = w;
      }
      int valX = 4 + maxLW + 10;
      for (uint8_t g = r; g < gEnd; g++) {
        uint8_t gi = _pageScroll + g;
        spr.setTextColor(_pageRowColor[gi], p.bg);
        spr.setCursor(4, y + g * FH);
        spr.print(_pageRows[gi]);
        spr.setCursor(valX, y + g * FH);
        spr.print(_pageRowValue[gi]);
      }
      r = gEnd;
    } else {
      spr.setTextColor(_pageRowColor[ri], p.bg);
      spr.setCursor(4, rowY);
      bool isLastVis = (r == maxVis - 1);
      bool hasMore = (ri + 1 < _pageRowCount);
      if (isLastVis && hasMore) {
        char tmp[36]; memcpy(tmp, _pageRows[ri], 36);
        int px = 0; uint8_t j = 0;
        while (tmp[j] && px < 127 - 24) {
          uint8_t bl; utf8cp(&tmp[j], bl); if (!bl) break;
          px += charPx(bl); j += bl;
        }
        tmp[j] = 0;
        spr.print(tmp); spr.print("...");
      } else {
        spr.print(_pageRows[ri]);
      }
      r++;
    }
  }

  if (_pageRowCount > maxVis + _pageScroll) {
    spr.setTextColor(p.textDim, p.bg);
    spr.setCursor(W - 8, H - FH - 2);
    spr.print("v");
  }
}

void drawPet() {
  const Palette& p = characterPalette();
  int y = 70;

  // pet name header
  spr.fillRect(0, y, W, FH, p.bg);
  spr.setTextSize(1);
  spr.setTextColor(p.text, p.bg);
  spr.setCursor(4, y);
  if (ownerName()[0]) {
    spr.printf("%s's %s", ownerName(), petName());
  } else {
    spr.print(petName());
  }
  spr.setTextColor(p.textDim, p.bg);
  char pg[8]; snprintf(pg, sizeof(pg), "%u/%u", petPage + 1, PET_PAGES);
  spr.setCursor(W - spr.textWidth(pg) - 4, y);
  spr.print(pg);

  if (petPage == 0) drawPetStats(p);
  else drawPetHowTo(p);
}

// Print a line, rendering `code` spans in highlight color without backticks
static void printColored(const char* s, uint16_t baseColor, uint16_t codeColor, uint16_t bgColor) {
  const char* p = s;
  spr.setTextColor(baseColor, bgColor);
  while (*p) {
    if (*p == '\x01') {
      p++;
      const char* end = strchr(p, '\x02');
      if (end) {
        spr.setTextColor(codeColor, bgColor);
        while (p < end) { spr.print(*p); p++; }
        p++; // skip \x02
        spr.setTextColor(baseColor, bgColor);
      }
    } else if ((uint8_t)*p >= 0x20) {
      spr.print(*p); p++;
    } else {
      p++; // skip other control chars
    }
  }
}

// Replace `code` with \x01code\x02 markers (same pixel width as ASCII,
// so wrapInto computes correct breaks; printColored detects them).
static void markCodeSpans(const char* src, char* dst, size_t dstLen) {
  size_t j = 0;
  while (*src && j < dstLen - 2) {
    if (*src == '`') {
      src++;
      const char* end = strchr(src, '`');
      if (end) {
        dst[j++] = '\x01';
        while (src < end && j < dstLen - 2) dst[j++] = *src++;
        dst[j++] = '\x02';
        src++; // skip closing backtick
      }
      // unmatched backtick: skip it
    } else {
      dst[j++] = *src++;
    }
  }
  dst[j] = 0;
}

void drawHUD() {
  if (tama.promptId[0]) { drawApproval(); return; }
  const Palette& p = characterPalette();
  const int SHOW = 8, LH = FH, WIDTH = CHARS_PER_LINE;
  const int AREA = SHOW * LH + 4;
  spr.fillRect(0, H - AREA, W, AREA, p.bg);
  spr.setTextSize(1);

  if (tama.lineGen != lastLineGen) { msgScroll = 0; lastLineGen = tama.lineGen; wake(); }

  if (tama.nLines == 0) {
    char mb[96];
    markCodeSpans(tama.msg, mb, sizeof(mb));
    spr.setCursor(4, H - LH - 2);
    printColored(mb, p.text, p.body, p.bg);
    return;
  }

  static char disp[32][36];
  static uint8_t srcOf[32];
  char markBuf[96];
  uint8_t nDisp = 0;
  for (uint8_t i = 0; i < tama.nLines && nDisp < 32; i++) {
    markCodeSpans(tama.lines[i], markBuf, sizeof(markBuf));
    uint8_t got = wrapInto(markBuf, &disp[nDisp], 32 - nDisp, WIDTH);
    for (uint8_t j = 0; j < got; j++) srcOf[nDisp + j] = i;
    nDisp += got;
  }

  uint8_t maxBack = (nDisp > SHOW) ? (nDisp - SHOW) : 0;
  if (msgScroll > maxBack) msgScroll = maxBack;

  int end = (int)nDisp - msgScroll;
  int start = end - SHOW; if (start < 0) start = 0;
  uint8_t newest = tama.nLines - 1;
  for (int i = 0; start + i < end; i++) {
    uint8_t row = start + i;
    bool fresh = (srcOf[row] == newest) && (msgScroll == 0);
    uint16_t base = fresh ? p.text : p.textDim;
    spr.setCursor(4, H - AREA + 2 + i * LH);
    printColored(disp[row], base, p.body, p.bg);
  }
  if (msgScroll > 0) {
    spr.setTextColor(p.body, p.bg);
    spr.setCursor(W - 24, H - FH - 2);
    spr.printf("-%u", msgScroll);
  }
}

void setup() {
  Serial.begin(115200);
  // Pre-mount LittleFS: format if corrupted (first boot on new hardware)
  if (!LittleFS.begin(false)) {
    Serial.println("[main] LittleFS corrupted, formatting...");
    LittleFS.begin(true);
  }

  auto cfg = M5.config();
  cfg.internal_imu = true;
  cfg.internal_rtc = true;
  cfg.internal_spk = true;
  M5.begin(cfg);

  M5.Display.setRotation(0);
  spr.setFont(&efontCN_12);
  startBt();
  applyBrightness();
  lastInteractMs = millis();
  statsLoad();
  settingsLoad();
  langSet((Lang)settings().lang);
  petNameLoad();
  buddyInit();

  spr.createSprite(W, H);
  characterInit(nullptr);
  gifAvailable = characterLoaded();
  buddyMode = !(gifAvailable && speciesIdxLoad() == SPECIES_GIF);
  applyDisplayMode();

  {
    const Palette& p = characterPalette();
    spr.setFont(&efontCN_12);
    spr.fillSprite(p.bg);
    spr.setTextDatum(MC_DATUM);
    if (ownerName()[0]) {
      char line[40];
      snprintf(line, sizeof(line), "%s's", ownerName());
      spr.setTextSize(2);
      spr.setTextColor(p.text, p.bg);   spr.drawString(line, W/2, H/2 - 14);
      spr.setTextColor(p.body, p.bg);   spr.drawString(petName(), W/2, H/2 + 14);
    } else {
      spr.setTextSize(2);
      spr.setTextColor(p.body, p.bg);   spr.drawString(T(S_HELLO), W/2, H/2 - 14);
      spr.setTextSize(1);
      spr.setTextColor(p.textDim, p.bg);
      spr.drawString(T(S_BUDDY_APPEARS), W/2, H/2 + 14);
    }
    spr.setTextDatum(TL_DATUM); spr.setTextSize(1);
    spr.pushSprite(0, 0);
    delay(1800);
  }

  Serial.printf("buddy: %s\n", buddyMode ? "ASCII mode" : "GIF character loaded");
}

void loop() {
  M5.update();
  t++;
  uint32_t now = millis();

  dataPoll(&tama);
  if (statsPollLevelUp()) triggerOneShot(P_CELEBRATE, 3000);
  baseState = derive(tama);

  if (baseState == P_IDLE && (int32_t)(now - wakeTransitionUntil) < 0) baseState = P_SLEEP;

  if ((int32_t)(now - oneShotUntil) >= 0) activeState = baseState;

  // LED: pulse on attention
  if (activeState == P_ATTENTION && settings().led) {
    M5.Power.setLed((now / 400) % 2 ? 255 : 0);
  } else {
    M5.Power.setLed(0);
  }

  // shake → dizzy
  if (now - lastShakeCheck > 50) {
    lastShakeCheck = now;
    if (!menuOpen && !screenOff && checkShake() && (int32_t)(now - oneShotUntil) >= 0) {
      wake();
      triggerOneShot(P_DIZZY, 2000);
      Serial.println("shake: dizzy");
    }
  }

  // Prompt arrival
  if (strcmp(tama.promptId, lastPromptId) != 0) {
    strncpy(lastPromptId, tama.promptId, sizeof(lastPromptId)-1);
    lastPromptId[sizeof(lastPromptId)-1] = 0;
    responseSent = false;
    if (tama.promptId[0]) {
      promptArrivedMs = millis();
      wake();
      beep(1200, 80);
      displayMode = DISP_NORMAL;
      menuOpen = settingsOpen = resetOpen = false; settingsSubPage = 0;
      applyDisplayMode();
      characterInvalidate();
      if (buddyMode) buddyInvalidate();
    }
  }

  bool inPrompt = tama.promptId[0] && !responseSent;

  // Button-press wake
  if (M5.BtnA.isPressed() || M5.BtnB.isPressed()) {
    if (screenOff) {
      if (M5.BtnA.isPressed()) swallowBtnA = true;
      if (M5.BtnB.isPressed()) swallowBtnB = true;
    }
    wake();
  }

  // Power button: short-press toggles screen off
  {
    uint8_t key = M5.Power.getKeyState();
    if (key == 2) {  // short click
      if (screenOff) {
        wake();
      } else {
        M5.Display.sleep();
        screenOff = true;
      }
    }
  }

  if (M5.BtnA.pressedFor(600) && !btnALong && !swallowBtnA) {
    btnALong = true;
    beep(800, 60);
    if (settingsOpen && settingsSubPage == 1) { settingsSubPage = 0; }
    else if (resetOpen) { resetOpen = false; }
    else if (settingsOpen) { settingsOpen = false; settingsSubPage = 0; characterInvalidate(); }
    else {
      menuOpen = !menuOpen;
      menuSel = 0;
      if (!menuOpen) characterInvalidate();
    }
    Serial.println(menuOpen ? "menu open" : "menu close");
  }
  if (M5.BtnA.wasReleased()) {
    if (!btnALong && !swallowBtnA) {
      if (inPrompt) {
        char cmd[96];
        snprintf(cmd, sizeof(cmd), "{\"cmd\":\"permission\",\"id\":\"%s\",\"decision\":\"once\"}", tama.promptId);
        sendCmd(cmd);
        responseSent = true;
        uint32_t tookS = (millis() - promptArrivedMs) / 1000;
        statsOnApproval(tookS);
        beep(2400, 60);
        if (tookS < 5) triggerOneShot(P_HEART, 2000);
      } else if (settingsOpen && settingsSubPage == 1) {
        beep(1800, 30);
        btSel = (btSel + 1) % BT_MENU_N;
        btClearArmed = false;
      } else if (resetOpen) {
        beep(1800, 30);
        resetSel = (resetSel + 1) % RESET_N;
        resetConfirmIdx = 0xFF;
      } else if (settingsOpen) {
        beep(1800, 30);
        settingsSel = (settingsSel + 1) % SETTINGS_N;
      } else if (menuOpen) {
        beep(1800, 30);
        menuSel = (menuSel + 1) % MENU_N;
      } else {
        beep(1800, 30);
        displayMode = (displayMode + 1) % DISP_COUNT;
        applyDisplayMode();
      }
    }
    btnALong = false;
    swallowBtnA = false;
  }

  // BtnB
  if (M5.BtnB.wasPressed()) {
    if (swallowBtnB) { swallowBtnB = false; }
    else
    if (inPrompt) {
      char cmd[96];
      snprintf(cmd, sizeof(cmd), "{\"cmd\":\"permission\",\"id\":\"%s\",\"decision\":\"deny\"}", tama.promptId);
      sendCmd(cmd);
      responseSent = true;
      statsOnDenial();
      beep(600, 60);
    } else if (settingsOpen && settingsSubPage == 1) {
      beep(2400, 30);
      applyBtSetting(btSel);
    } else if (resetOpen) {
      beep(2400, 30);
      applyReset(resetSel);
    } else if (settingsOpen) {
      beep(2400, 30);
      applySetting(settingsSel);
    } else if (menuOpen) {
      beep(2400, 30);
      menuConfirm();
    } else if (displayMode == DISP_INFO) {
      beep(2400, 30);
      {
        uint8_t maxVis = _pageRowCount > _pageScroll ? _pageRowCount - _pageScroll : 0;
        if (maxVis > 10) maxVis = 10;
        if (_pageScroll + maxVis < _pageRowCount) {
          _pageScroll += maxVis;
        } else {
          _pageScroll = 0;
          infoPage = (infoPage + 1) % INFO_PAGES;
        }
      }
    } else if (displayMode == DISP_PET) {
      beep(2400, 30);
      if (petPage == 1 && _pageRowCount > 0) {
        uint8_t maxVis = _pageRowCount > _pageScroll ? _pageRowCount - _pageScroll : 0;
        if (maxVis > 10) maxVis = 10;
        if (_pageScroll + maxVis < _pageRowCount) {
          _pageScroll += maxVis;
        } else {
          _pageScroll = 0;
          petPage = (petPage + 1) % PET_PAGES;
          applyDisplayMode();
        }
      } else {
        petPage = (petPage + 1) % PET_PAGES;
        applyDisplayMode();
      }
    } else {
      beep(2400, 30);
      msgScroll = (msgScroll >= 30) ? 0 : msgScroll + 1;
    }
  }

  // Charging clock — debounce _onUsb to prevent rapid clock/HUD flicker
  clockRefreshRtc();
  static bool stableUsb = false;
  static uint8_t usbDebounce = 0;
  if (_onUsb != stableUsb) { if (++usbDebounce >= 5) { stableUsb = _onUsb; usbDebounce = 0; } }
  else usbDebounce = 0;

  bool clocking = displayMode == DISP_NORMAL
               && !menuOpen && !settingsOpen && !resetOpen && !inPrompt
               && tama.sessionsRunning == 0 && tama.sessionsWaiting == 0
               && dataRtcValid() && stableUsb;
  if (clocking) clockUpdateOrient();
  else { clockOrient = 0; orientFrames = 0; paintedOrient = 0; }
  bool landscapeClock = clocking && clockOrient != 0;

  static bool wasClocking = false;
  static bool wasLandscape = false;
  if (clocking != wasClocking || landscapeClock != wasLandscape) {
    if (clocking && !landscapeClock) characterSetPeek(true);
    else applyDisplayMode();
    characterInvalidate();
    if (buddyMode) buddyInvalidate();
    wasClocking = clocking;
    wasLandscape = landscapeClock;
  }
  if (clocking) {
    uint8_t dow = clockDow();
    bool weekend = (dow == 0 || dow == 6);
    bool friday  = (dow == 5);

    uint8_t h = _clkTm.hours;
    if (h >= 1 && h < 7)             activeState = P_SLEEP;
    else if (weekend)                activeState = (now/8000 % 6 == 0) ? P_HEART : P_SLEEP;
    else if (h < 9)                  activeState = (now/6000 % 4 == 0) ? P_IDLE  : P_SLEEP;
    else if (h == 12)                activeState = (now/5000 % 3 == 0) ? P_HEART : P_IDLE;
    else if (friday && h >= 15)      activeState = (now/4000 % 3 == 0) ? P_CELEBRATE : P_IDLE;
    else if (h >= 22 || h == 0)      activeState = (now/7000 % 3 == 0) ? P_DIZZY : P_SLEEP;
    else                             activeState = (now/10000 % 5 == 0) ? P_SLEEP : P_IDLE;
  }

  static uint32_t lastPasskey = 0;
  uint32_t pk = blePasskey();
  if (pk && !lastPasskey) { wake(); beep(1800, 60); }
  lastPasskey = pk;

  if (napping || screenOff || landscapeClock) {
    // skip sprite render
  } else if (buddyMode) {
    buddyTick(activeState);
  } else if (characterLoaded()) {
    characterSetState(activeState);
    characterTick();
  } else {
    const Palette& p = characterPalette();
    spr.fillSprite(p.bg);
    spr.setTextColor(p.textDim, p.bg);
    spr.setTextSize(1);
    if (xferActive()) {
      uint32_t done = xferProgress(), total = xferTotal();
      spr.setCursor(8, 90);
      spr.print(T(S_INSTALLING));
      spr.setCursor(8, 102);
      spr.printf("%luK / %luK", done/1024, total/1024);
      int barW = W - 16;
      spr.drawRect(8, 116, barW, 8, p.textDim);
      if (total > 0) {
        int fill = (int)((uint64_t)barW * done / total);
        if (fill > 1) spr.fillRect(9, 117, fill - 1, 6, p.body);
      }
    } else {
      spr.setCursor(8, 100);
      spr.print(T(S_NO_CHAR));
    }
  }
  if (landscapeClock) {
    drawClock();
  } else if (!napping && !screenOff) {
    if (blePasskey()) drawPasskey();
    else if (clocking) drawClock();
    else if (displayMode == DISP_INFO) drawInfo();
    else if (displayMode == DISP_PET) drawPet();
    else if (settings().hud) drawHUD();
    if (resetOpen) drawReset();
    else if (settingsOpen) drawSettings();
    else if (menuOpen) drawMenu();
    spr.pushSprite(0, 0);
  }

  // Face-down nap
  static int8_t faceDownFrames = 0;
  if (!inPrompt) {
    bool down = isFaceDown();
    if (down)       { if (faceDownFrames < 20) faceDownFrames++; }
    else            { if (faceDownFrames > -10) faceDownFrames--; }
  }

  if (!napping && faceDownFrames >= 15) {
    napping = true;
    napStartMs = now;
    M5.Display.setBrightness(13);  // ~5% brightness for nap
    dimmed = true;
  } else if (napping && faceDownFrames <= -8) {
    napping = false;
    statsOnNapEnd((now - napStartMs) / 1000);
    statsOnWake();
    wake();
  }

  // Auto screen off (not on USB)
  if (!screenOff && !inPrompt && !_onUsb
      && millis() - lastInteractMs > SCREEN_OFF_MS) {
    M5.Display.sleep();
    screenOff = true;
  }

  delay(screenOff ? 100 : 16);
}
