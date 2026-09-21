// Chaos Pendulum controller — Seeed XIAO ESP32C3
// Board: "XIAO_ESP32C3" (esp32:esp32:XIAO_ESP32C3)
// Safe default: brake engaged + VFD disabled until firmware commands otherwise.

// --- Pin map (from schematic) ---
constexpr uint8_t PIN_K1_BRAKE = 8;  // K1 brake relay via Q1
constexpr uint8_t PIN_K2_MOTOR = 7;  // K2 motor/VFD enable relay via Q2
constexpr uint8_t PIN_LED_RED  = 5;  // K1 status
constexpr uint8_t PIN_LED_BLUE = 6;  // K2 status

// --- LED PWM ---
constexpr uint32_t LED_FREQ = 5000;
constexpr uint8_t  LED_RES  = 8;
constexpr uint8_t  LED_DUTY = 128;   // 50%

// --- Test cycle timing (ms) ---
constexpr uint32_t STEP_MS = 2000;

void setBrakeReleased(bool released) {
  digitalWrite(PIN_K1_BRAKE, released ? HIGH : LOW);
  ledcWrite(PIN_LED_RED, released ? LED_DUTY : 0);
}

void setMotorEnabled(bool enabled) {
  digitalWrite(PIN_K2_MOTOR, enabled ? HIGH : LOW);
  ledcWrite(PIN_LED_BLUE, enabled ? LED_DUTY : 0);
}

void safeState() {
  setMotorEnabled(false);
  setBrakeReleased(false);
}

void setup() {
  pinMode(PIN_K1_BRAKE, OUTPUT);
  pinMode(PIN_K2_MOTOR, OUTPUT);
  digitalWrite(PIN_K1_BRAKE, LOW);
  digitalWrite(PIN_K2_MOTOR, LOW);
  ledcAttach(PIN_LED_RED,  LED_FREQ, LED_RES);
  ledcAttach(PIN_LED_BLUE, LED_FREQ, LED_RES);
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
