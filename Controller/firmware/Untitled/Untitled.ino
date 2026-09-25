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
  Pause        -> every hangEvery ms: next kick in exactly hangTime ms
  Run off      -> MOVE off, brake engaged, wait brakeEngage

Added: Wi-Fi AP + mobile web page (tune, save, start/stop, test kick,
stats, OTA upload). AP turns off apMinutes after boot (0 = always on),
but not while a phone is connected.
Board: XIAO_ESP32C3 (esp32 core 3.x). Board LED names: D1 red = brake, D5 green = move.
------------------------------------------------------------
*/

#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include <Update.h>
#include <esp_random.h>
#include "page.h"

#define FW_VERSION "1.0.0"

// --- Pins (named after the schematic nets) ---
constexpr uint8_t PIN_K1_GATE   = D10; // GPIO10  net K1_GATE    -> Q1 -> K1 : brake coil  (HIGH = brake released)
constexpr uint8_t PIN_K2_GATE   = D7; // GPIO20  net K2_GATE    -> Q2 -> K2 : VFD_24V/VFD_DI1 contact (HIGH = run impulse)
constexpr uint8_t PIN_LED_BRAKE = D5; // GPIO7   net LED_BRAKE  : onboard LED1 green + panel LED via PLED_BRAKE
constexpr uint8_t PIN_LED_MOVE  = D6; // GPIO21  net LED_MOVE   : onboard LED2 red + panel LED via PLED_MOVE

// Present on V2 hardware, not used by this firmware yet:
constexpr uint8_t PIN_VFD_OK_SIG  = D4; // GPIO6   net VFD_OK_SIG : VersiDrive "drive OK" relay feedback (via VFD_OK_IN, active low)
constexpr uint8_t PIN_VFD_AO_SIG  = D1; // GPIO3   net VFD_AO_SIG : VersiDrive analog out (terminal 8, via VFD_AO_IN, 68k/10k divider). Volts at T8 = mV * 7.8 / 1000
constexpr uint8_t PIN_LED_VFD_OK  = D3; // GPIO5   net LED_VFD_OK : onboard LED3 green + panel LED via PLED_VFD_OK
constexpr uint8_t PIN_RUN_SIG   = D2; // GPIO4   net RUN_SIG    : RUN switch (via RUN_IN)
constexpr uint8_t PIN_BTN_SIG   = D0; // GPIO2   net BTN_SIG    : Wi-Fi / test button (via BTN_IN). Strapping pin: R11 pull-up keeps it high at reset.
constexpr uint8_t PIN_LED_SYS   = D8; // GPIO8   net LED_SYS : blue LED4 + panel SYSTEM LED (3V3 -> R20 -> PLED_SYS -> LED -> LED_SYS), ACTIVE-LOW. Strapping pin: LED pulls it high at reset.

// --- Wi-Fi ---
const char *AP_SSID  = "KK-Untitled";
const char *AP_PASS  = "untitled2026";   // min 8 chars
const char *HOSTNAME = "untitled";       // http://untitled.local

// --- Config ---
struct Config {
  uint32_t spinMin, spinMax;       // ms
  uint32_t dropMin, dropMax;       // ms
  uint32_t hangEvery, hangTime;    // ms (hangEvery 0 = no pause)
  uint32_t brakeRelease, brakeEngage; // ms
  uint32_t ledPct;                 // 0..100
  uint32_t apMinutes;              // 0 = AP always on
};
const Config DEFAULTS = {200, 900, 10000, 45000, 600000, 60000, 1250, 1250, 50, 15};
constexpr uint32_t CFG_VERSION = 1;

Config cfg;
Preferences prefs;
WebServer server(80);

// --- State ---
enum Phase : uint8_t { STOPPED, RELEASING, WAITING, KICKING, ENGAGING };
const char *PHASE_NAMES[] = {"stopped", "releasing brake", "resting", "kick", "engaging brake"};

Phase    phase = STOPPED;
bool     running = false;     // software on/off
bool     inPause = false;     // current rest is a hangout pause
bool     brakeReleased = false, moveOn = false;
uint32_t phaseStart = 0, kickStart = 0, kickLen = 0, nextKick = 0, hangoutAt = 0;
uint32_t lastSpin = 0, lastDrop = 0, kicks = 0, pauses = 0, bootCount = 0;
bool     apOn = false;
bool     otaActive = false;

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

// ---------- Config storage ----------
void clampConfig(Config &c) {
  auto clamp = [](uint32_t &v, uint32_t lo, uint32_t hi) { v = v < lo ? lo : (v > hi ? hi : v); };
  clamp(c.spinMin, 20, 20000);     clamp(c.spinMax, 20, 20000);
  clamp(c.dropMin, 500, 3600000);  clamp(c.dropMax, 500, 3600000);
  clamp(c.hangEvery, 0, 86400000); clamp(c.hangTime, 0, 3600000);
  clamp(c.brakeRelease, 0, 10000); clamp(c.brakeEngage, 0, 10000);
  clamp(c.ledPct, 0, 100);         clamp(c.apMinutes, 0, 1440);
  if (c.spinMax < c.spinMin) std::swap(c.spinMin, c.spinMax);
  if (c.dropMax < c.dropMin) std::swap(c.dropMin, c.dropMax);
}

void loadConfig() {
  cfg = DEFAULTS;
  if (prefs.getUInt("ver", 0) == CFG_VERSION && prefs.getBytesLength("cfg") == sizeof(Config))
    prefs.getBytes("cfg", &cfg, sizeof(Config));
  clampConfig(cfg);
}

void saveConfig() {
  prefs.putBytes("cfg", &cfg, sizeof(Config));
  prefs.putUInt("ver", CFG_VERSION);
}

// ---------- Piece logic ----------
void startPiece() {
  if (running) return;
  running = true;
  inPause = false;
  setMove(false);
  setBrakeReleased(true);
  phase = RELEASING;
  phaseStart = millis();
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
  lastDrop = randRange(cfg.dropMin, cfg.dropMax);
  lastSpin = kickLen;
  kickStart = now;
  nextKick  = now + lastDrop;      // measured from kick start, as original
  inPause = false;
  kicks++;
  setMove(true);
  phase = KICKING;
  Serial.printf("KICK #%lu: %lu ms, next in %lu ms\n", kicks, kickLen, lastDrop);
}

void tickPiece(uint32_t now) {
  switch (phase) {
    case RELEASING:
      if (now - phaseStart >= cfg.brakeRelease) {
        nextKick  = now;                              // first kick immediately
        hangoutAt = now + cfg.hangEvery;
        phase = WAITING;
      }
      break;
    case WAITING:
      if (cfg.hangEvery && reached(now, hangoutAt)) {
        hangoutAt = now + cfg.hangEvery;
        nextKick  = now + cfg.hangTime;
        inPause = true;
        pauses++;
        Serial.printf("PAUSE: %lu ms\n", cfg.hangTime);
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

// ---------- Web ----------
void sendJson(const String &s) {
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json", s);
}

String configJson() {
  char b[400];
  snprintf(b, sizeof b,
    "{\"spinMin\":%lu,\"spinMax\":%lu,\"dropMin\":%lu,\"dropMax\":%lu,"
    "\"hangEvery\":%lu,\"hangTime\":%lu,\"brakeRelease\":%lu,\"brakeEngage\":%lu,"
    "\"ledPct\":%lu,\"apMinutes\":%lu}",
    cfg.spinMin, cfg.spinMax, cfg.dropMin, cfg.dropMax, cfg.hangEvery, cfg.hangTime,
    cfg.brakeRelease, cfg.brakeEngage, cfg.ledPct, cfg.apMinutes);
  return String(b);
}

String statusJson() {
  uint32_t now = millis();
  long nextKickIn = (running && phase == WAITING) ? (long)(int32_t)(nextKick - now) : -1;
  long pauseIn    = (running && cfg.hangEvery && phase != RELEASING) ? (long)(int32_t)(hangoutAt - now) : -1;
  long apOffIn    = (apOn && cfg.apMinutes) ? max(0L, (long)cfg.apMinutes * 60000L - (long)now) : -1;
  const char *ph  = (phase == WAITING && inPause) ? "pause" : PHASE_NAMES[phase];
  char b[420];
  snprintf(b, sizeof b,
    "{\"fw\":\"%s\",\"running\":%s,\"phase\":\"%s\",\"brake\":%s,\"move\":%s,"
    "\"uptime\":%lu,\"boots\":%lu,\"kicks\":%lu,\"pauses\":%lu,\"lastSpin\":%lu,\"lastDrop\":%lu,"
    "\"nextKickIn\":%ld,\"pauseIn\":%ld,\"apOffIn\":%ld,\"clients\":%d,\"heap\":%lu}",
    FW_VERSION, running ? "true" : "false", ph,
    brakeReleased ? "\"released\"" : "\"engaged\"", moveOn ? "true" : "false",
    now / 1000, bootCount, kicks, pauses, lastSpin, lastDrop,
    nextKickIn, pauseIn, apOffIn, WiFi.softAPgetStationNum(),
    (unsigned long)ESP.getFreeHeap());
  return String(b);
}

void handleSaveConfig() {
  Config c = cfg;
  auto rd = [](const char *k, uint32_t &v) {
    if (server.hasArg(k)) { long x = server.arg(k).toInt(); v = x < 0 ? 0 : (uint32_t)x; }
  };
  rd("spinMin", c.spinMin);   rd("spinMax", c.spinMax);
  rd("dropMin", c.dropMin);   rd("dropMax", c.dropMax);
  rd("hangEvery", c.hangEvery); rd("hangTime", c.hangTime);
  rd("brakeRelease", c.brakeRelease); rd("brakeEngage", c.brakeEngage);
  rd("ledPct", c.ledPct);     rd("apMinutes", c.apMinutes);
  clampConfig(c);
  bool hangChanged = c.hangEvery != cfg.hangEvery;
  cfg = c;
  saveConfig();
  if (running && hangChanged && cfg.hangEvery) hangoutAt = millis() + cfg.hangEvery;
  setBrakeReleased(brakeReleased);   // refresh LED brightness
  setMove(moveOn);
  Serial.println("Config saved");
  sendJson(configJson());
}

void setupWeb() {
  server.on("/", HTTP_GET, [] {
    server.sendHeader("Cache-Control", "no-store");
    server.send_P(200, "text/html", INDEX_HTML);
  });
  server.on("/api/status", HTTP_GET, [] { sendJson(statusJson()); });
  server.on("/api/config", HTTP_GET, [] { sendJson(configJson()); });
  server.on("/api/config", HTTP_POST, handleSaveConfig);
  server.on("/api/defaults", HTTP_POST, [] {
    cfg = DEFAULTS; saveConfig(); setBrakeReleased(brakeReleased); setMove(moveOn);
    sendJson(configJson());
  });
  server.on("/api/start", HTTP_POST, [] { startPiece(); sendJson(statusJson()); });
  server.on("/api/stop",  HTTP_POST, [] { stopPiece();  sendJson(statusJson()); });
  server.on("/api/kick",  HTTP_POST, [] {
    if (running && phase == WAITING) startKick(millis());
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
  server.begin();
}

void startAP() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASS);
  MDNS.begin(HOSTNAME);
  MDNS.addService("http", "tcp", 80);
  setupWeb();
  apOn = true;
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
  pinMode(PIN_K2_GATE,  OUTPUT); digitalWrite(PIN_K2_GATE,  LOW);
  ledcAttach(PIN_LED_BRAKE, 5000, 8);
  ledcAttach(PIN_LED_MOVE,  5000, 8);

  Serial.begin(115200);
  prefs.begin("kk", false);
  bootCount = prefs.getUInt("boots", 0) + 1;
  prefs.putUInt("boots", bootCount);
  loadConfig();
  safeOutputs();
  Serial.printf("\nUNTITLED fw %s, boot #%lu\n", FW_VERSION, bootCount);

  startAP();
  startPiece();   // always run on power-up
}

void loop() {
  uint32_t now = millis();
  if (!otaActive) tickPiece(now);

  if (apOn) {
    server.handleClient();
    static uint32_t lastApCheck = 0;
    if (cfg.apMinutes && now - lastApCheck > 1000) {
      lastApCheck = now;
      if (now > cfg.apMinutes * 60000UL && WiFi.softAPgetStationNum() == 0 && !otaActive) stopAP();
    }
  }
  delay(1);
}
