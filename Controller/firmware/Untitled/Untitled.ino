/*
------------------------------------------------------------
UNTITLED – Controller firmware (XIAO ESP32C3)

Artist:          Kitty Kraus
Original code:   Roman Zulek (Arduino)
Port / update:   Fubbi Karlsson, House of North, 2026

Behaviour (same as original):
  Run on       -> release brake, wait brakeRelease, first kick at once
  Kick         -> MOVE (VersiDrive run input) on for random spinMin..spinMax ms
  Rest         -> next kick random dropMin..dropMax ms after kick start
  Pause        -> every hangEvery..hangEveryMax ms (random): next kick in hangTime..hangTimeMax ms (random)
  Run off      -> MOVE off, brake engaged, wait brakeEngage

Added: Wi-Fi AP + mobile web page (tune, save, start/stop, test kick,
stats, OTA upload). Artist page at /, technician page at /tech.
Live changes apply at once; Save stores them, Undo returns to the saved state.
AP turns off apMinutes after it starts (0 = always on),
but not while a phone is connected.

V2 options (1.1.0) – each switched on the web page, saved in flash, OFF by default:
  useVfdOk   VFD "drive OK" relay (VFD_OK_IN). Not OK -> move off, brake engaged.
             Resumes once OK has been stable for okResume ms. Also holds the
             piece after power-up until the drive reports ready.
  autostart  Start on power-up (warning blink, then first push). Off = after a
             power cut the piece stays stopped until RUN is held or Start is pressed.
  softKicks  First N kicks after every start ramp from 50 % to 100 % length (0 = off).

Panel buttons (1.4.0, always active, momentary to GND):
  RUN (RUN_IN)   stopped: hold 2 s = start (warning blink, then first push; may let go).
                 Short press does nothing. Starting/running: any press = stop + brake at once.
  Wi-Fi (BTN_IN) press = Wi-Fi AP on, or restart its timer if already on. Times out
                 after apMinutes. NOTE: GPIO2 is a strapping pin – holding this
                 button during power-up stops the board from booting.
  Web Start/Stop behave the same as the RUN button.

Always active (indication only, cannot stop the piece):
  LED_VFD_OK  on = drive OK, off = not OK, fast blink = piece held by VFD fault
  LED_SYS     Wi-Fi: on while the AP is up, off when Wi-Fi is off, fast blink during OTA
  VFD AO      live VersiDrive analog output + peak per kick on the web page

Board: XIAO_ESP32C3 (esp32 core 3.x).
------------------------------------------------------------
*/

#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include <Update.h>
#include <esp_random.h>
#include "page.h"     // technician page  /tech
#include "artist.h"   // artist page      /

#define FW_VERSION "1.4.0"

// --- Pins (named after the schematic nets) ---
constexpr uint8_t PIN_K1_GATE    = D10; // GPIO10 net K1_GATE   -> Q1 -> K1 : brake coil  (HIGH = brake released)
constexpr uint8_t PIN_K2_GATE    = D7;  // GPIO20 net K2_GATE   -> Q2 -> K2 : VFD_24V/VFD_DI1 contact (HIGH = run impulse)
constexpr uint8_t PIN_LED_BRAKE  = D5;  // GPIO7  net LED_BRAKE : onboard LED + panel LED via PLED_BRAKE
constexpr uint8_t PIN_LED_MOVE   = D6;  // GPIO21 net LED_MOVE  : onboard LED + panel LED via PLED_MOVE
constexpr uint8_t PIN_LED_VFD_OK = D3;  // GPIO5  net LED_VFD_OK: onboard LED3 + panel LED via PLED_VFD_OK
constexpr uint8_t PIN_LED_SYS    = D8;  // GPIO8  net LED_SYS   : LED4 + panel SYSTEM LED, ACTIVE LOW (strapping pin)
constexpr uint8_t PIN_VFD_OK_SIG = D4;  // GPIO6  net VFD_OK_SIG: drive OK relay, active low (10k pull-up)
constexpr uint8_t PIN_VFD_AO_SIG = D1;  // GPIO3  net VFD_AO_SIG: VFD analog out via 68k/10k divider
constexpr uint8_t PIN_RUN_SIG    = D2;  // GPIO4  net RUN_SIG   : RUN button, pressed = LOW (10k pull-up)
constexpr uint8_t PIN_BTN_SIG    = D0;  // GPIO2  net BTN_SIG   : Wi-Fi button, pressed = LOW (R11 pull-up, strapping pin)

constexpr float    AO_SCALE      = 7.8f;  // V at VFD terminal 8 = V at pin * 7.8
constexpr uint32_t AO_TAIL_MS    = 1500;  // keep tracking the peak this long after a kick ends
constexpr uint32_t RUN_HOLD_MS   = 2000;  // hold RUN this long to start

// --- Wi-Fi ---
const char *AP_SSID  = "KK-Untitled";
const char *AP_PASS  = "untitled2026";   // min 8 chars
const char *HOSTNAME = "untitled";       // http://untitled.local


// --- Config ---
struct Config {
  uint32_t spinMin, spinMax;          // ms
  uint32_t dropMin, dropMax;          // ms
  uint32_t hangEvery, hangTime;       // ms (hangEvery 0 = no pause)
  uint32_t brakeRelease, brakeEngage; // ms
  uint32_t ledPct;                    // 0..100
  uint32_t apMinutes;                 // 0 = AP always on
  // --- added in CFG_VERSION 2 (V2 options) ---
  uint32_t useVfdOk;                  // 0/1
  uint32_t okResume;                  // ms drive OK must be stable before resuming
  uint32_t useRun;                    // unused since 1.4.0 (RUN is a button now), kept for layout
  uint32_t useBtn;                    // unused since 1.4.0, kept for layout
  uint32_t softKicks;                 // 0 = off
  // --- added in CFG_VERSION 3: random long pauses ---
  uint32_t hangEveryMax;              // ms, pause interval is random hangEvery..hangEveryMax
  uint32_t hangTimeMax;               // ms, pause length is random hangTime..hangTimeMax
  // --- added in CFG_VERSION 4 ---
  uint32_t warnMs;                    // Move LED speeding-up blink before each push (0 = off)
  // --- added in CFG_VERSION 5 ---
  uint32_t autostart;                 // 0/1 start on power-up
};
const Config DEFAULTS = {200, 900, 10000, 45000, 600000, 60000, 1250, 1250, 50, 15,
                         0, 5000, 0, 0, 0,
                         600000, 60000,
                         3000,
                         0};
constexpr uint32_t CFG_VERSION = 5;
constexpr size_t   CFG_V1_SIZE = 10 * sizeof(uint32_t);
constexpr size_t   CFG_V2_SIZE = 15 * sizeof(uint32_t);
constexpr size_t   CFG_V3_SIZE = 17 * sizeof(uint32_t);
constexpr size_t   CFG_V4_SIZE = 18 * sizeof(uint32_t);

Config cfg;
uint32_t cfgRev = 0;               // bumps on every settings change, pages re-sync on it
Config savedCfg;                   // what is in flash; cfg may hold live (unsaved) changes
Preferences prefs;
WebServer server(80);

// --- State ---
enum Phase : uint8_t { STOPPED, RELEASING, WAITING, KICKING, ENGAGING };
const char *PHASE_NAMES[] = {"stopped", "releasing brake", "resting", "kick", "engaging brake"};

Phase    phase = STOPPED;
bool     userRun = false;     // wanted on/off (web Start/Stop, RUN button)
bool     running = false;     // actually running
bool     inPause = false;     // current rest is a hangout pause
bool     brakeReleased = false, moveOn = false;
uint32_t phaseStart = 0, kickStart = 0, kickLen = 0, nextKick = 0, hangoutAt = 0;
uint32_t lastSpin = 0, lastDrop = 0, kicks = 0, pauses = 0, bootCount = 0;
uint32_t kicksSinceStart = 0, vfdFaults = 0, lastPause = 0;
float    warnPhase = 0;                    // Move LED warning blink phase
bool     warnWas = false;
bool     faultActive = false;
bool     apOn = false, webReady = false;
uint32_t apSince = 0;
bool     otaActive = false;

// AO feedback
float    aoV = 0;                         // smoothed V at terminal 8
float    aoPeak = 0, aoLastPeak = -1, aoAvgPeak = -1;
bool     aoTracking = false;

// ---------- Inputs (all active low, debounced) ----------
struct Input {
  uint8_t  pin;
  uint16_t debounce;
  bool     state, raw;
  uint32_t since;          // when raw last changed
  void begin(uint32_t now) { pinMode(pin, INPUT); raw = state = digitalRead(pin) == LOW; since = now; }
  bool update(uint32_t now) {               // true when state changed
    bool r = digitalRead(pin) == LOW;
    if (r != raw) { raw = r; since = now; return false; }
    if (r != state && now - since >= debounce) { state = r; return true; }
    return false;
  }
  uint32_t stableFor(uint32_t now) const { return raw == state ? now - since : 0; }
};
Input inVfdOk = {PIN_VFD_OK_SIG, 300};   // true = drive OK
Input inRun   = {PIN_RUN_SIG,    30};    // true = RUN pressed
Input inBtn   = {PIN_BTN_SIG,    40};    // true = Wi-Fi pressed
uint32_t runDownAt = 0;
bool     runArmed = false;               // press began while stopped -> may start after RUN_HOLD_MS

// ---------- Outputs ----------
uint32_t ledDuty() { return (255 * cfg.ledPct) / 100; }

void setBrakeReleased(bool on) {
  brakeReleased = on;
  digitalWrite(PIN_K1_GATE, on ? HIGH : LOW);
  ledcWrite(PIN_LED_BRAKE, on ? ledDuty() : 0);
}

void setMove(bool on) {
  moveOn = on;
  digitalWrite(PIN_K2_GATE, on ? HIGH : LOW);
  ledcWrite(PIN_LED_MOVE, on ? ledDuty() : 0);
}

void safeOutputs() { setMove(false); setBrakeReleased(false); }

uint32_t randRange(uint32_t lo, uint32_t hi) {
  if (hi <= lo) return lo;
  return lo + (esp_random() % (hi - lo + 1));
}

inline bool reached(uint32_t now, uint32_t t) { return (int32_t)(now - t) >= 0; }
inline bool blink(uint32_t now, uint32_t period) { return (now / (period / 2)) & 1; }

// ---------- Config storage ----------
void clampConfig(Config &c) {
  auto clamp = [](uint32_t &v, uint32_t lo, uint32_t hi) { v = v < lo ? lo : (v > hi ? hi : v); };
  clamp(c.spinMin, 20, 20000);     clamp(c.spinMax, 20, 20000);
  clamp(c.dropMin, 500, 3600000);  clamp(c.dropMax, 500, 3600000);
  clamp(c.hangEvery, 0, 86400000); clamp(c.hangTime, 0, 3600000);
  clamp(c.brakeRelease, 0, 10000); clamp(c.brakeEngage, 0, 10000);
  clamp(c.ledPct, 0, 100);         clamp(c.apMinutes, 0, 1440);
  clamp(c.useVfdOk, 0, 1);         clamp(c.okResume, 500, 600000);
  clamp(c.useRun, 0, 1);           clamp(c.useBtn, 0, 1);
  clamp(c.autostart, 0, 1);
  clamp(c.softKicks, 0, 50);
  clamp(c.hangEveryMax, 60000, 86400000); clamp(c.hangTimeMax, 0, 3600000);
  clamp(c.warnMs, 0, 10000);
  if (c.hangEvery && c.hangEvery < 60000) c.hangEvery = 60000;   // 0 = no long pauses
  if (c.hangEvery && c.hangEveryMax < c.hangEvery) std::swap(c.hangEvery, c.hangEveryMax);
  if (c.hangTimeMax < c.hangTime) std::swap(c.hangTime, c.hangTimeMax);
  if (c.spinMax < c.spinMin) std::swap(c.spinMin, c.spinMax);
  if (c.dropMax < c.dropMin) std::swap(c.dropMin, c.dropMax);
}

void saveConfig() {
  prefs.putBytes("cfg", &cfg, sizeof(Config));
  prefs.putUInt("ver", CFG_VERSION);
  savedCfg = cfg;
}

bool configDirty() { return memcmp(&cfg, &savedCfg, sizeof(Config)) != 0; }

void loadConfig() {
  cfg = DEFAULTS;
  uint32_t ver = prefs.getUInt("ver", 0);
  size_t   len = prefs.getBytesLength("cfg");
  if (ver == CFG_VERSION && len == sizeof(Config)) {
    prefs.getBytes("cfg", &cfg, sizeof(Config));
  } else if ((ver == 1 && len == CFG_V1_SIZE) || (ver == 2 && len == CFG_V2_SIZE) ||
             (ver == 3 && len == CFG_V3_SIZE) || (ver == 4 && len == CFG_V4_SIZE)) {
    // keep tuned settings; fields added later keep their DEFAULTS
    prefs.getBytes("cfg", &cfg, len);
    if (ver < 3) {                     // pause ranges start as the old fixed values
      cfg.hangEveryMax = cfg.hangEvery ? cfg.hangEvery : DEFAULTS.hangEveryMax;
      cfg.hangTimeMax  = cfg.hangTime;
    }
    clampConfig(cfg);
    saveConfig();
    Serial.printf("Config migrated from v%lu\n", ver);
  }
  clampConfig(cfg);
  savedCfg = cfg;
}

// ---------- Piece logic ----------
void startPiece() {
  if (running) return;
  running = true;
  inPause = false;
  kicksSinceStart = 0;
  setMove(false);
  setBrakeReleased(true);
  phase = RELEASING;
  phaseStart = millis();
  nextKick = phaseStart + max(cfg.brakeRelease, cfg.warnMs);   // first push after release + warning
  Serial.println("RUN: brake released");
}

void stopPiece() {
  if (!running) return;
  running = false;
  inPause = false;
  setMove(false);
  setBrakeReleased(false);
  phase = ENGAGING;
  phaseStart = millis();
  Serial.println("STOP: move off, brake engaged");
}

void startKick(uint32_t now) {
  kickLen  = randRange(cfg.spinMin, cfg.spinMax);
  if (cfg.softKicks && kicksSinceStart < cfg.softKicks)          // ramp 50 % -> 100 %
    kickLen = kickLen * (50 + 50 * kicksSinceStart / cfg.softKicks) / 100;
  kicksSinceStart++;
  lastDrop = randRange(cfg.dropMin, cfg.dropMax);
  lastSpin = kickLen;
  kickStart = now;
  nextKick  = now + lastDrop;      // measured from kick start, as original
  inPause = false;
  kicks++;
  aoPeak = 0;
  aoTracking = true;
  setMove(true);
  phase = KICKING;
  Serial.printf("KICK #%lu: %lu ms, next in %lu ms\n", kicks, kickLen, lastDrop);
}

void tickPiece(uint32_t now) {
  switch (phase) {
    case RELEASING:
      if (now - phaseStart >= cfg.brakeRelease) {
        // nextKick was set in startPiece (brake release + warning)
        hangoutAt = now + randRange(cfg.hangEvery, cfg.hangEveryMax);
        phase = WAITING;
      }
      break;
    case WAITING:
      if (cfg.hangEvery && reached(now, hangoutAt)) {
        hangoutAt = now + randRange(cfg.hangEvery, cfg.hangEveryMax);
        lastPause = randRange(cfg.hangTime, cfg.hangTimeMax);
        nextKick  = now + lastPause;
        inPause = true;
        pauses++;
        Serial.printf("PAUSE: %lu ms, next pause in %lu ms\n", lastPause, hangoutAt - now);
      } else if (reached(now, nextKick)) {
        startKick(now);
      }
      break;
    case KICKING:
      if (now - kickStart >= kickLen) {
        setMove(false);
        phase = WAITING;
      }
      break;
    case ENGAGING:
      if (now - phaseStart >= cfg.brakeEngage) phase = STOPPED;
      break;
    case STOPPED:
      break;
  }
}

// ---------- Inputs decide whether the piece may run ----------
const char *holdReason() {
  if (!userRun)    return "stopped";
  if (faultActive) return "VFD fault";
  return "";
}

void startAP();

void tickInputs(uint32_t now) {
  // VFD drive OK
  if (inVfdOk.update(now)) Serial.printf(">> VFD_OK %s\n", inVfdOk.state ? "OK" : "NOT OK");
  if (cfg.useVfdOk) {
    if (!inVfdOk.state && !faultActive) {
      faultActive = true;
      if (running) { vfdFaults++; Serial.println("VFD FAULT: holding piece"); }
    } else if (faultActive && inVfdOk.state && inVfdOk.stableFor(now) >= cfg.okResume) {
      faultActive = false;
      Serial.println("VFD OK: resuming");
    }
  } else {
    faultActive = false;
  }

  // RUN button: starting/running -> any press stops at once; stopped -> hold RUN_HOLD_MS to start
  if (inRun.update(now) && inRun.state) {
    if (userRun) {
      userRun = false; runArmed = false;
      Serial.println("RUN button: stop");
    } else {
      runDownAt = now; runArmed = true;
    }
  }
  if (!inRun.state) runArmed = false;    // released before 2 s: nothing
  if (runArmed && now - runDownAt >= RUN_HOLD_MS) {
    runArmed = false;
    userRun = true;                      // warning blink starts now, operator may let go
    Serial.println("RUN button: start");
  }

  // Wi-Fi button: AP on, or restart its timer
  if (inBtn.update(now) && inBtn.state) {
    if (!apOn) startAP();
    apSince = now;
    Serial.println("Wi-Fi button: AP on, timer restarted");
  }

  // Apply
  bool want = userRun && !faultActive;
  if (want && !running) startPiece();
  else if (!want && running) stopPiece();
}

void tickAO(uint32_t now) {
  static uint32_t last = 0;
  if (now - last < 20) return;
  last = now;
  float v = analogReadMilliVolts(PIN_VFD_AO_SIG) * AO_SCALE / 1000.0f;
  aoV = aoV * 0.7f + v * 0.3f;
  if (aoTracking) {
    if (aoV > aoPeak) aoPeak = aoV;
    if (phase != KICKING && now - kickStart >= kickLen + AO_TAIL_MS) {
      aoTracking = false;
      aoLastPeak = aoPeak;
      aoAvgPeak  = aoAvgPeak < 0 ? aoPeak : aoAvgPeak * 0.8f + aoPeak * 0.2f;
    }
  }
}

bool moveWarning(uint32_t now) {
  return cfg.warnMs && running && !otaActive && (phase == RELEASING || phase == WAITING) &&
         (int32_t)(nextKick - now) <= (int32_t)cfg.warnMs;
}

void tickIndicators(uint32_t now) {
  // VFD OK LED
  bool vOn = (cfg.useVfdOk && faultActive && userRun) ? blink(now, 250) : inVfdOk.state;
  ledcWrite(PIN_LED_VFD_OK, vOn ? ledDuty() : 0);
  // System LED (active low) = Wi-Fi indicator: on while the AP is up, fast blink during OTA
  bool sOn = otaActive ? blink(now, 100) : apOn;
  ledcWrite(PIN_LED_SYS, sOn ? 255 - ledDuty() : 255);

  // Move LED: solid while pushing; speeding-up blink during the warning before a push
  static uint32_t lastT = 0;
  uint32_t dt = now - lastT; lastT = now;
  bool warn = moveWarning(now);
  if (warn && !warnWas) warnPhase = 0;
  warnWas = warn;
  bool mOn = moveOn;
  if (!moveOn && warn) {
    float f = 1.0f - (float)(int32_t)(nextKick - now) / cfg.warnMs;   // 0 -> 1 over the warning
    if (f < 0) f = 0;
    float period = 700.0f - 620.0f * f;                               // 700 ms -> 80 ms
    warnPhase += dt / period;
    mOn = (warnPhase - (int)warnPhase) < 0.5f;
  }
  ledcWrite(PIN_LED_MOVE, mOn ? ledDuty() : 0);
}

// ---------- Web ----------
void sendJson(const String &s) {
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json", s);
}

String configJson() {
  char b[600];
  snprintf(b, sizeof b,
    "{\"spinMin\":%lu,\"spinMax\":%lu,\"dropMin\":%lu,\"dropMax\":%lu,"
    "\"hangEvery\":%lu,\"hangTime\":%lu,\"brakeRelease\":%lu,\"brakeEngage\":%lu,"
    "\"ledPct\":%lu,\"apMinutes\":%lu,"
    "\"useVfdOk\":%lu,\"okResume\":%lu,\"useRun\":%lu,\"useBtn\":%lu,\"softKicks\":%lu,"
    "\"hangEveryMax\":%lu,\"hangTimeMax\":%lu,\"warnMs\":%lu,\"autostart\":%lu,\"rev\":%lu}",
    cfg.spinMin, cfg.spinMax, cfg.dropMin, cfg.dropMax, cfg.hangEvery, cfg.hangTime,
    cfg.brakeRelease, cfg.brakeEngage, cfg.ledPct, cfg.apMinutes,
    cfg.useVfdOk, cfg.okResume, cfg.useRun, cfg.useBtn, cfg.softKicks,
    cfg.hangEveryMax, cfg.hangTimeMax, cfg.warnMs, cfg.autostart, cfgRev);
  return String(b);
}

String statusJson() {
  uint32_t now = millis();
  long nextKickIn = (running && phase == WAITING) ? (long)(int32_t)(nextKick - now) : -1;
  long pauseIn    = (running && cfg.hangEvery && phase != RELEASING) ? (long)(int32_t)(hangoutAt - now) : -1;
  long apOffIn    = (apOn && cfg.apMinutes) ? max(0L, (long)cfg.apMinutes * 60000L - (long)(now - apSince)) : -1;
  const char *ph  = (phase == WAITING && inPause) ? "pause" : PHASE_NAMES[phase];
  char b[800];
  snprintf(b, sizeof b,
    "{\"fw\":\"%s\",\"running\":%s,\"userRun\":%s,\"hold\":\"%s\",\"phase\":\"%s\",\"brake\":%s,\"move\":%s,"
    "\"uptime\":%lu,\"boots\":%lu,\"kicks\":%lu,\"pauses\":%lu,\"lastSpin\":%lu,\"lastDrop\":%lu,"
    "\"nextKickIn\":%ld,\"pauseIn\":%ld,\"apOffIn\":%ld,\"clients\":%d,\"heap\":%lu,"
    "\"vfdOk\":%s,\"runSw\":%s,\"btn\":%s,\"faults\":%lu,\"ao\":%.2f,\"aoPeak\":%.2f,\"aoAvg\":%.2f,"
    "\"dirty\":%s,\"hangTime\":%lu,\"warn\":%s,\"rev\":%lu}",
    FW_VERSION, running ? "true" : "false", userRun ? "true" : "false",
    running ? "" : holdReason(), ph,
    brakeReleased ? "\"released\"" : "\"engaged\"", moveOn ? "true" : "false",
    now / 1000, bootCount, kicks, pauses, lastSpin, lastDrop,
    nextKickIn, pauseIn, apOffIn, WiFi.softAPgetStationNum(),
    (unsigned long)ESP.getFreeHeap(),
    inVfdOk.state ? "true" : "false", inRun.state ? "true" : "false", inBtn.state ? "true" : "false",
    vfdFaults, aoV, aoLastPeak, aoAvgPeak,
    configDirty() ? "true" : "false", lastPause, moveWarning(now) ? "true" : "false", cfgRev);
  return String(b);
}

// Read config fields from the request (missing ones keep their current value)
Config configFromArgs() {
  Config c = cfg;
  auto rd = [](const char *k, uint32_t &v) {
    if (server.hasArg(k)) { long x = server.arg(k).toInt(); v = x < 0 ? 0 : (uint32_t)x; }
  };
  rd("spinMin", c.spinMin);   rd("spinMax", c.spinMax);
  rd("dropMin", c.dropMin);   rd("dropMax", c.dropMax);
  rd("hangEvery", c.hangEvery); rd("hangTime", c.hangTime);
  rd("brakeRelease", c.brakeRelease); rd("brakeEngage", c.brakeEngage);
  rd("ledPct", c.ledPct);     rd("apMinutes", c.apMinutes);
  rd("useVfdOk", c.useVfdOk); rd("okResume", c.okResume);
  rd("useRun", c.useRun);     rd("useBtn", c.useBtn);
  rd("softKicks", c.softKicks);
  rd("hangEveryMax", c.hangEveryMax); rd("hangTimeMax", c.hangTimeMax);
  rd("warnMs", c.warnMs);
  rd("autostart", c.autostart);
  clampConfig(c);
  return c;
}

// Make a config active right away (not saved)
void applyConfig(const Config &c) {
  Config old = cfg;
  cfg = c;
  if (memcmp(&old, &cfg, sizeof(Config)) != 0) cfgRev++;
  uint32_t now = millis();
  if (running && cfg.hangEvery && (cfg.hangEvery != old.hangEvery || cfg.hangEveryMax != old.hangEveryMax))
    hangoutAt = now + randRange(cfg.hangEvery, cfg.hangEveryMax);
  // New rest range: re-draw the current rest so the change is felt at once
  if (running && phase == WAITING && !inPause &&
      (cfg.dropMin != old.dropMin || cfg.dropMax != old.dropMax)) {
    lastDrop = randRange(cfg.dropMin, cfg.dropMax);
    nextKick = kickStart + lastDrop;
  }
  setBrakeReleased(brakeReleased);   // refresh LED brightness
  setMove(moveOn);
  tickInputs(now);                   // apply option changes at once
}

void setupWeb() {
  server.on("/", HTTP_GET, [] {
    server.sendHeader("Cache-Control", "no-store");
    server.send_P(200, "text/html", ARTIST_HTML);
  });
  server.on("/tech", HTTP_GET, [] {
    server.sendHeader("Cache-Control", "no-store");
    server.send_P(200, "text/html", INDEX_HTML);
  });
  server.on("/api/status", HTTP_GET, [] { sendJson(statusJson()); });
  server.on("/api/config", HTTP_GET, [] { sendJson(configJson()); });
  server.on("/api/config", HTTP_POST, [] {           // apply + save
    applyConfig(configFromArgs()); saveConfig();
    Serial.println("Config saved");
    sendJson(configJson());
  });
  server.on("/api/live", HTTP_POST, [] {             // apply, not saved
    applyConfig(configFromArgs());
    sendJson(configJson());
  });
  server.on("/api/revert", HTTP_POST, [] {           // back to the saved config
    applyConfig(savedCfg);
    sendJson(configJson());
  });
  server.on("/api/defaults", HTTP_POST, [] {
    applyConfig(DEFAULTS); saveConfig();
    sendJson(configJson());
  });
  server.on("/api/start", HTTP_POST, [] { userRun = true;  tickInputs(millis()); sendJson(statusJson()); });
  server.on("/api/stop",  HTTP_POST, [] { userRun = false; tickInputs(millis()); sendJson(statusJson()); });
  server.on("/api/kick",  HTTP_POST, [] {            // push after the warning
    uint32_t now = millis(), due = now + cfg.warnMs;
    if (running && phase == WAITING && (int32_t)(nextKick - due) > 0) {
      nextKick = due; lastDrop = nextKick - kickStart; inPause = false;
    }
    sendJson(statusJson());
  });
  server.on("/api/wifi", HTTP_POST, [] {             // restart the Wi-Fi off timer
    apSince = millis();
    sendJson(statusJson());
  });
  server.on("/api/reboot", HTTP_POST, [] {
    server.send(200, "text/plain", "rebooting");
    safeOutputs(); delay(300); ESP.restart();
  });

  // OTA: upload Untitled.ino.bin
  server.on("/update", HTTP_POST, [] {
    bool ok = !Update.hasError();
    server.sendHeader("Connection", "close");
    server.send(ok ? 200 : 500, "text/plain", ok ? "OK" : Update.errorString());
    otaActive = false;
    if (ok) { delay(500); ESP.restart(); }
  }, [] {
    HTTPUpload &up = server.upload();
    if (up.status == UPLOAD_FILE_START) {
      otaActive = true;
      running = false; phase = STOPPED; safeOutputs();   // piece safe during update
      Serial.printf("OTA start: %s\n", up.filename.c_str());
      if (!Update.begin(UPDATE_SIZE_UNKNOWN)) Update.printError(Serial);
    } else if (up.status == UPLOAD_FILE_WRITE) {
      if (Update.write(up.buf, up.currentSize) != up.currentSize) Update.printError(Serial);
    } else if (up.status == UPLOAD_FILE_END) {
      if (Update.end(true)) Serial.printf("OTA done: %u bytes\n", up.totalSize);
      else Update.printError(Serial);
    } else if (up.status == UPLOAD_FILE_ABORTED) {
      Update.abort(); otaActive = false;
    }
  });

  server.onNotFound([] { server.sendHeader("Location", "/"); server.send(302); });
}

void startAP() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASS);
  WiFi.setSleep(false);              // snappier web page
  MDNS.begin(HOSTNAME);
  MDNS.addService("http", "tcp", 80);
  if (!webReady) { setupWeb(); webReady = true; }
  server.begin();
  apOn = true;
  apSince = millis();
  Serial.printf("AP %s up at %s\n", AP_SSID, WiFi.softAPIP().toString().c_str());
}

void stopAP() {
  server.stop();
  MDNS.end();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_OFF);
  apOn = false;
  Serial.println("AP off");
}

// ---------- Main ----------
void setup() {
  // Safe state first: brake engaged, VersiDrive not running
  pinMode(PIN_K1_GATE, OUTPUT); digitalWrite(PIN_K1_GATE, LOW);
  pinMode(PIN_K2_GATE, OUTPUT); digitalWrite(PIN_K2_GATE, LOW);
  ledcAttach(PIN_LED_BRAKE,  5000, 8);
  ledcAttach(PIN_LED_MOVE,   5000, 8);
  ledcAttach(PIN_LED_VFD_OK, 5000, 8);
  ledcAttach(PIN_LED_SYS,    5000, 8);
  ledcWrite(PIN_LED_SYS, 255);                     // active low: off

  uint32_t now = millis();
  inVfdOk.begin(now); inRun.begin(now); inBtn.begin(now);
  analogSetPinAttenuation(PIN_VFD_AO_SIG, ADC_11db);

  Serial.begin(115200);
  prefs.begin("kk", false);
  bootCount = prefs.getUInt("boots", 0) + 1;
  prefs.putUInt("boots", bootCount);
  loadConfig();
  safeOutputs();
  Serial.printf("\nUNTITLED fw %s, boot #%lu | VFD_OK %s, autostart %s, soft %lu\n",
                FW_VERSION, bootCount, cfg.useVfdOk ? "on" : "off",
                cfg.autostart ? "on" : "off", cfg.softKicks);

  startAP();
  userRun = cfg.autostart;                         // default: stay stopped after a power cut
  faultActive = cfg.useVfdOk && !inVfdOk.state;    // wait for drive ready
  tickInputs(millis());
}

void loop() {
  uint32_t now = millis();
  if (!otaActive) {
    tickInputs(now);
    tickPiece(now);
  }
  tickAO(now);
  tickIndicators(now);

  if (apOn) {
    server.handleClient();
    static uint32_t lastApCheck = 0;
    if (cfg.apMinutes && now - lastApCheck > 1000) {
      lastApCheck = now;
      if (now - apSince > cfg.apMinutes * 60000UL && WiFi.softAPgetStationNum() == 0 && !otaActive) stopAP();
    }
  }
  delay(1);
}
