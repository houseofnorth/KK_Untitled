// Chaos Pendulum controller — Seeed XIAO ESP32C3
// Board: "XIAO_ESP32C3" (esp32:esp32:XIAO_ESP32C3)
// Safe default: brake engaged + VFD disabled until firmware commands otherwise.

// --- Pin map (named after the schematic nets) ---
constexpr uint8_t PIN_K1_GATE   = D10; // GPIO10  net K1_GATE   -> Q1 -> K1 brake relay
constexpr uint8_t PIN_K2_GATE   = D7; // GPIO20  net K2_GATE   -> Q2 -> K2 motor/VFD relay
constexpr uint8_t PIN_LED_BRAKE = D5; // GPIO7   net LED_BRAKE : K1 status LED
constexpr uint8_t PIN_LED_MOVE  = D6; // GPIO21  net LED_MOVE  : K2 status LED

// --- LED PWM ---
constexpr uint32_t LED_FREQ = 5000;
constexpr uint8_t  LED_RES  = 8;
constexpr uint8_t  LED_DUTY = 128;   // 50%

// --- Test cycle timing (ms) ---
constexpr uint32_t STEP_MS = 2000;

void setBrakeReleased(bool released) {
  digitalWrite(PIN_K1_GATE, released ? HIGH : LOW);
  ledcWrite(PIN_LED_BRAKE, released ? LED_DUTY : 0);
}

void setMotorEnabled(bool enabled) {
  digitalWrite(PIN_K2_GATE, enabled ? HIGH : LOW);
  ledcWrite(PIN_LED_MOVE, enabled ? LED_DUTY : 0);
}

void safeState() {
  setMotorEnabled(false);
  setBrakeReleased(false);
}

void setup() {
  pinMode(PIN_K1_GATE, OUTPUT);
  pinMode(PIN_K2_GATE, OUTPUT);
  digitalWrite(PIN_K1_GATE, LOW);
  digitalWrite(PIN_K2_GATE, LOW);
  ledcAttach(PIN_LED_BRAKE,  LED_FREQ, LED_RES);
  ledcAttach(PIN_LED_MOVE, LED_FREQ, LED_RES);
  safeState();
  Serial.begin(115200);
  Serial.println("Chaos Pendulum: safe state");
}

void loop() {
  // Relay/LED test cycle: release brake -> enable motor -> disable motor -> engage brake
  setBrakeReleased(true);  delay(STEP_MS);
  setMotorEnabled(true);   delay(STEP_MS);
  setMotorEnabled(false);  delay(STEP_MS);
  setBrakeReleased(false); delay(STEP_MS);
}
