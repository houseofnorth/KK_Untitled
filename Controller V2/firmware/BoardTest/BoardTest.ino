/*
------------------------------------------------------------
Chaos Pendulum — Controller V2 board test (XIAO ESP32C3)

Bench test with only the XIAO fitted, powered from USB.
Open the serial monitor at 115200 baud, line ending "No line ending"
or "Newline" (both work). Type a key + Enter. "?" shows the menu.

Relay coils sit on the 5V net (IRM side of D9). USB alone does NOT
power them: feed 5 V into MOTOR 1 (+) / MOTOR 2 (GND). USB can stay on.
Relay contacts are checked with a multimeter on continuity:
  K1 (brake):  MOTOR 3 (24V+)    <-> MOTOR 5 (BRAKE+)   closed when K1 on
  K2 (VFD):    MOTOR 7 (VFD_24V) <-> MOTOR 8 (VFD_DI1)  closed when K2 on

Inputs (no connectors fitted: short the header pads with a wire):
  RUN     PANEL 1 -> PANEL 2 (GND)    shorted = ON
  BUTTON  PANEL 3 -> PANEL 4 (GND)    shorted = PRESSED
  VFD OK  MOTOR 9 -> MOTOR 10 (GND)   shorted = DRIVE OK (active low)
  VFD AO  MOTOR 11: apply a known voltage (e.g. MOTOR 1, 5 V).
          Expect ~0.64 V at the pin and "T8 ~5.0 V" in the readout.

Wi-Fi: joins WIFI_SSID with a static IP. Open http://192.168.1.222
in a browser on the same network — same tests as the serial menu.
The serial monitor prints the Wi-Fi state on connect.

Board: XIAO_ESP32C3 (esp32:esp32:XIAO_ESP32C3), esp32 core 3.x
House of North, 2026
------------------------------------------------------------
*/

#include <WiFi.h>
#include <WebServer.h>

// --- Wi-Fi ---
const char *WIFI_SSID = "House of North";
const char *WIFI_PASS = "p3rman3ntr3cord";
const IPAddress WIFI_IP(192, 168, 1, 222);
const IPAddress WIFI_GW(192, 168, 1, 1);
const IPAddress WIFI_MASK(255, 255, 255, 0);
const IPAddress WIFI_DNS(192, 168, 1, 1);

WebServer server(80);
bool wifiUp = false;

// --- Pins (named after the schematic nets) ---
constexpr uint8_t PIN_K1_GATE    = D10; // GPIO10 -> Q1 -> K1 brake relay
constexpr uint8_t PIN_K2_GATE    = D7;  // GPIO20 -> Q2 -> K2 VFD run relay
constexpr uint8_t PIN_LED_BRAKE  = D5;  // GPIO7  onboard LED1 + PLED_BRAKE
constexpr uint8_t PIN_LED_MOVE   = D6;  // GPIO21 onboard LED2 + PLED_MOVE
constexpr uint8_t PIN_LED_VFD_OK = D3;  // GPIO5  onboard LED3 + PLED_VFD_OK
constexpr uint8_t PIN_LED_SYS    = D8;  // GPIO8  onboard LED4 + panel SYS, ACTIVE LOW
constexpr uint8_t PIN_RUN_SIG    = D2;  // GPIO4  RUN switch, active low
constexpr uint8_t PIN_BTN_SIG    = D0;  // GPIO2  button, active low
constexpr uint8_t PIN_VFD_OK_SIG = D4;  // GPIO6  drive OK, active low
constexpr uint8_t PIN_VFD_AO_SIG = D1;  // GPIO3  ADC, 68k/10k divider
constexpr uint8_t PIN_9          = D9;  // GPIO9  test pad only

constexpr float AO_SCALE = 7.8f;        // V at VFD terminal 8 = V at pin * 7.8

// --- LEDs ---
struct Led { const char *name; uint8_t pin; bool activeLow; bool on; };
Led leds[] = {
  {"BRAKE",  PIN_LED_BRAKE,  false, false},
  {"MOVE",   PIN_LED_MOVE,   false, false},
  {"VFD_OK", PIN_LED_VFD_OK, false, false},
  {"SYS",    PIN_LED_SYS,    true,  false},
};
constexpr uint8_t N_LEDS = sizeof(leds) / sizeof(leds[0]);
uint8_t ledPct = 100;

void printWifi();

// --- State ---
bool k1On = false, k2On = false, pin9On = false;
bool monitorAO = false;
uint32_t lastAO = 0;
bool lastRun, lastBtn, lastOk;

// ------------------------------------------------------------
void applyLed(Led &l) {
  uint32_t duty = l.on ? (255UL * ledPct) / 100 : 0;
  ledcWrite(l.pin, l.activeLow ? 255 - duty : duty);
}

void setLed(uint8_t i, bool on) {
  leds[i].on = on;
  applyLed(leds[i]);
  Serial.printf("LED %-6s %s\n", leds[i].name, on ? "ON" : "off");
}

void setAllLeds(bool on) { for (uint8_t i = 0; i < N_LEDS; i++) setLed(i, on); }

void setK1(bool on) {
  k1On = on;
  digitalWrite(PIN_K1_GATE, on ? HIGH : LOW);
  Serial.printf("K1 brake relay %s  -> MOTOR 3-5 %s\n", on ? "ON " : "off", on ? "CLOSED" : "open");
}

void setK2(bool on) {
  k2On = on;
  digitalWrite(PIN_K2_GATE, on ? HIGH : LOW);
  Serial.printf("K2 VFD relay   %s  -> MOTOR 7-8 %s\n", on ? "ON " : "off", on ? "CLOSED" : "open");
}

void setPin9(bool on) {
  pin9On = on;
  digitalWrite(PIN_9, on ? HIGH : LOW);
  Serial.printf("GPIO9 test pad %s (measure %s)\n", on ? "HIGH" : "LOW", on ? "~3.3 V" : "~0 V");
}

void safeState() {
  setK2(false);
  setK1(false);
  setAllLeds(false);
  setPin9(false);
}

// --- Inputs ---
bool runOn()   { return digitalRead(PIN_RUN_SIG) == LOW; }
bool btnDown() { return digitalRead(PIN_BTN_SIG) == LOW; }
bool driveOk() { return digitalRead(PIN_VFD_OK_SIG) == LOW; }

void readAO(uint32_t &pinMv, float &t8V) {
  uint32_t sum = 0;
  for (int i = 0; i < 16; i++) sum += analogReadMilliVolts(PIN_VFD_AO_SIG);
  pinMv = sum / 16;
  t8V = pinMv * AO_SCALE / 1000.0f;
}

void printInputs() {
  uint32_t mv; float v; readAO(mv, v);
  Serial.printf("RUN %-3s | BUTTON %-7s | VFD_OK %-5s | AO pin %4lu mV = T8 %5.2f V\n",
                runOn() ? "ON" : "off", btnDown() ? "PRESSED" : "up",
                driveOk() ? "OK" : "FAULT", (unsigned long)mv, v);
}

void printStatus() {
  Serial.println(F("\n--- status ---"));
  Serial.printf("K1 brake %s | K2 VFD %s | GPIO9 %s | LED level %u%%\n",
                k1On ? "ON" : "off", k2On ? "ON" : "off", pin9On ? "HIGH" : "LOW", ledPct);
  Serial.print("LEDs:");
  for (uint8_t i = 0; i < N_LEDS; i++) Serial.printf(" %s=%s", leds[i].name, leds[i].on ? "ON" : "off");
  Serial.println();
  printInputs();
}

void printMenu() {
  Serial.println(F(
    "\n=== Chaos Pendulum V2 board test ===\n"
    "Relays (click + continuity)\n"
    "  1  toggle K1 brake relay   (MOTOR 3-5)\n"
    "  2  toggle K2 VFD relay     (MOTOR 7-8)\n"
    "LEDs\n"
    "  3  toggle BRAKE   LED      4  toggle MOVE LED\n"
    "  5  toggle VFD_OK  LED      6  toggle SYS  LED (active low)\n"
    "  a  all LEDs on             l  LED level 10/50/100 %\n"
    "Inputs\n"
    "  i  read inputs once        m  live input monitor on/off\n"
    "     (RUN / BUTTON / VFD_OK changes are always reported)\n"
    "Other\n"
    "  9  toggle GPIO9 test pad   w  Wi-Fi status\n"
    "  c  auto sequence (each output 1 s in turn)\n"
    "  0  all off (safe state)    s  status    ?  menu"));
}

void pause(uint32_t ms) {
  uint32_t t = millis();
  while (millis() - t < ms) { if (Serial.available()) return; delay(5); }
}

void autoSequence() {
  Serial.println(F("\n--- auto sequence (any key aborts) ---"));
  safeState();
  for (uint8_t i = 0; i < N_LEDS && !Serial.available(); i++) { setLed(i, true); pause(1000); setLed(i, false); }
  if (!Serial.available()) { setK1(true); pause(1000); setK1(false); pause(300); }
  if (!Serial.available()) { setK2(true); pause(1000); setK2(false); pause(300); }
  if (!Serial.available()) { setPin9(true); pause(1000); setPin9(false); }
  while (Serial.available()) Serial.read();
  safeState();
  Serial.println(F("--- sequence done, safe state ---"));
}

// ------------------------------------------------------------
// One command handler for serial and web
void doCmd(char c) {
  switch (c) {
    case '1': setK1(!k1On); break;
    case '2': setK2(!k2On); break;
    case '3': case '4': case '5': case '6': { uint8_t i = c - '3'; setLed(i, !leds[i].on); break; }
    case 'a': case 'A': setAllLeds(true); break;
    case 'l': case 'L':
      ledPct = ledPct == 100 ? 10 : (ledPct == 10 ? 50 : 100);
      for (uint8_t i = 0; i < N_LEDS; i++) applyLed(leds[i]);
      Serial.printf("LED level %u%%\n", ledPct); break;
    case 'i': case 'I': printInputs(); break;
    case 'm': case 'M': monitorAO = !monitorAO;
      Serial.printf("Live monitor %s\n", monitorAO ? "ON (every 0.5 s)" : "off"); break;
    case '9': setPin9(!pin9On); break;
    case 'c': case 'C': autoSequence(); break;
    case '0': safeState(); break;
    case 's': case 'S': printStatus(); break;
    case 'w': case 'W': printWifi(); break;
    case '?': case 'h': case 'H': printMenu(); break;
    case '\r': case '\n': case ' ': break;
    default: Serial.printf("? unknown key '%c' — type ? for menu\n", c);
  }
}

// --- Web interface ---
const char PAGE[] PROGMEM = R"HTML(<!doctype html><html><head>
<meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>V2 board test</title><style>
body{font-family:system-ui,sans-serif;margin:16px;max-width:520px;background:#111;color:#eee}
h1{font-size:20px}h2{font-size:14px;color:#999;margin:18px 0 6px;text-transform:uppercase}
.g{display:grid;grid-template-columns:1fr 1fr;gap:8px}
button{padding:14px;font-size:16px;border:0;border-radius:8px;background:#333;color:#eee}
button.on{background:#2a7;color:#fff}button.warn{background:#a33}
.r{display:flex;justify-content:space-between;padding:8px 10px;background:#222;border-radius:6px;margin:4px 0}
.ok{color:#4d8}.no{color:#888}small{color:#777}
</style></head><body>
<h1>Chaos Pendulum &mdash; V2 board test</h1>
<h2>Relays</h2><div class="g">
<button id="k1" onclick="c('1')">K1 brake<br><small>MOTOR 3&ndash;5</small></button>
<button id="k2" onclick="c('2')">K2 VFD<br><small>MOTOR 7&ndash;8</small></button></div>
<h2>LEDs</h2><div class="g">
<button id="l0" onclick="c('3')">BRAKE</button><button id="l1" onclick="c('4')">MOVE</button>
<button id="l2" onclick="c('5')">VFD OK</button><button id="l3" onclick="c('6')">SYS</button>
<button onclick="c('a')">All on</button><button id="lv" onclick="c('l')">Level</button></div>
<h2>Inputs</h2>
<div class="r"><span>RUN</span><b id="run"></b></div>
<div class="r"><span>BUTTON</span><b id="btn"></b></div>
<div class="r"><span>VFD OK</span><b id="ok"></b></div>
<div class="r"><span>AO pin / T8</span><b id="ao"></b></div>
<h2>Other</h2><div class="g">
<button id="p9" onclick="c('9')">GPIO9 pad</button><button onclick="c('c')">Auto sequence</button>
<button class="warn" onclick="c('0')" style="grid-column:span 2">ALL OFF</button></div>
<script>
function set(id,on,t){const e=document.getElementById(id);e.classList.toggle('on',on);if(t)e.textContent=t}
function txt(id,t,on){const e=document.getElementById(id);e.textContent=t;e.className=on?'ok':'no'}
function show(s){set('k1',s.k1);set('k2',s.k2);set('p9',s.p9);
 for(let i=0;i<4;i++)set('l'+i,s.led[i]);document.getElementById('lv').textContent='Level '+s.lvl+'%';
 txt('run',s.run?'ON':'off',s.run);txt('btn',s.btn?'PRESSED':'up',s.btn);
 txt('ok',s.ok?'OK':'FAULT',s.ok);txt('ao',s.mv+' mV / '+s.t8.toFixed(2)+' V',s.mv>50)}
const ids={'1':'k1','2':'k2','3':'l0','4':'l1','5':'l2','6':'l3','9':'p9'};let busy=0;
function c(k){if(ids[k])document.getElementById(ids[k]).classList.toggle('on');busy++;
 fetch('/cmd?k='+k).then(r=>r.json()).then(show).catch(()=>{}).finally(()=>busy--)}
function poll(){if(busy){setTimeout(poll,200);return}
 fetch('/status').then(r=>r.json()).then(s=>{if(!busy)show(s)}).catch(()=>{}).finally(()=>setTimeout(poll,500))}
poll();
</script></body></html>)HTML";

String statusJson() {
  uint32_t mv; float v; readAO(mv, v);
  char buf[256];
  snprintf(buf, sizeof(buf),
    "{\"k1\":%d,\"k2\":%d,\"p9\":%d,\"led\":[%d,%d,%d,%d],\"lvl\":%u,"
    "\"run\":%d,\"btn\":%d,\"ok\":%d,\"mv\":%lu,\"t8\":%.2f}",
    k1On, k2On, pin9On, leds[0].on, leds[1].on, leds[2].on, leds[3].on, ledPct,
    runOn(), btnDown(), driveOk(), (unsigned long)mv, v);
  return String(buf);
}

void handleCmd() {
  String k = server.arg("k");
  if (k.length() == 1 && strchr("1234569al0c", k[0])) {
    Serial.printf("[web] %c\n", k[0]);
    doCmd(k[0]);
  }
  server.send(200, "application/json", statusJson());
}

void printWifi() {
  if (WiFi.status() == WL_CONNECTED)
    Serial.printf("Wi-Fi: connected to \"%s\", http://%s  (RSSI %d dBm)\n",
                  WIFI_SSID, WiFi.localIP().toString().c_str(), WiFi.RSSI());
  else
    Serial.printf("Wi-Fi: not connected (joining \"%s\")\n", WIFI_SSID);
}

void setupWifi() {
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);               // no modem sleep: fast web response
  WiFi.setHostname("kk-boardtest");
  WiFi.config(WIFI_IP, WIFI_GW, WIFI_MASK, WIFI_DNS);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASS);   // non-blocking; serial menu works meanwhile

  server.on("/", [] { server.send_P(200, "text/html", PAGE); });
  server.on("/status", [] { server.send(200, "application/json", statusJson()); });
  server.on("/cmd", handleCmd);
  server.begin();
}

// ------------------------------------------------------------
void setup() {
  // Relays off first: brake engaged, VFD disabled
  pinMode(PIN_K1_GATE, OUTPUT); digitalWrite(PIN_K1_GATE, LOW);
  pinMode(PIN_K2_GATE, OUTPUT); digitalWrite(PIN_K2_GATE, LOW);
  pinMode(PIN_9, OUTPUT);       digitalWrite(PIN_9, LOW);

  for (uint8_t i = 0; i < N_LEDS; i++) { ledcAttach(leds[i].pin, 5000, 8); applyLed(leds[i]); }

  pinMode(PIN_RUN_SIG, INPUT);      // external 10k pull-ups on the board
  pinMode(PIN_BTN_SIG, INPUT);
  pinMode(PIN_VFD_OK_SIG, INPUT);
  analogSetPinAttenuation(PIN_VFD_AO_SIG, ADC_11db);

  Serial.begin(115200);
  uint32_t t = millis();
  while (!Serial && millis() - t < 3000) delay(10);   // wait for USB CDC

  lastRun = runOn(); lastBtn = btnDown(); lastOk = driveOk();
  setupWifi();
  printMenu();
  printStatus();
}

void loop() {
  // Report input edges
  bool r = runOn(), b = btnDown(), o = driveOk();
  if (r != lastRun) { Serial.printf(">> RUN    %s\n", r ? "ON" : "off");       lastRun = r; }
  if (b != lastBtn) { Serial.printf(">> BUTTON %s\n", b ? "PRESSED" : "up");   lastBtn = b; }
  if (o != lastOk)  { Serial.printf(">> VFD_OK %s\n", o ? "OK" : "FAULT");     lastOk = o; }

  if (monitorAO && millis() - lastAO >= 500) { lastAO = millis(); printInputs(); }

  // Report Wi-Fi connect / disconnect
  bool up = WiFi.status() == WL_CONNECTED;
  if (up != wifiUp) { wifiUp = up; printWifi(); }

  server.handleClient();
  if (Serial.available()) doCmd(Serial.read());
  delay(5);
}
