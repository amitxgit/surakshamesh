/*
 * SurakshaMesh Node Firmware — SIH26025
 * Unified sketch for Central Gateway & Mesh Field Nodes with Active Buzzer.
 *
 * Board Configurations:
 *   NODE_INDEX 1, IS_GATEWAY 1 -> Central Gateway Node (NODE-01) with Active Buzzer & USB/Wi-Fi Telemetry
 *   NODE_INDEX 2, IS_GATEWAY 0 -> Mesh Node 02 (NODE-02) field sensor with Active Buzzer
 *   NODE_INDEX 3, IS_GATEWAY 0 -> Mesh Node 03 (NODE-03) field sensor with Active Buzzer
 *
 * Wiring (Identical for all 3 nodes):
 *   MPU6050:      VCC -> 3V3 | GND -> GND | SDA -> GPIO 21 | SCL -> GPIO 22 | AD0 -> GND
 *   Active Buzzer: Pos(+) -> GPIO 14 | Neg(-) -> GND
 *   Status LED:   Onboard LED (GPIO 2)
 *   TP4056 + Batt:OUT+ -> VIN | OUT- -> GND
 */

#include <Wire.h>
#include <esp_now.h>
#include <WiFi.h>
#include <math.h>

// ==================== Node Configuration ====================
#define NODE_INDEX       1          // 1 for NODE-01 (Gateway), 2 for NODE-02, 3 for NODE-03
#define IS_GATEWAY       1          // 1 on Central Gateway (NODE-01), 0 on Field Nodes
#define FLIP_PITCH       0          // Set to 1 if IMU mounted reversed on pitch axis
#define FLIP_ROLL        0          // Set to 1 if IMU mounted reversed on roll axis

// Communication Mode for Gateway
#define USE_WIFI_HTTP    0          // 0 = USB Serial output (for serial-bridge.mjs), 1 = Direct Wi-Fi HTTP POST
#define WIFI_SSID        "SurakshaMesh-Hotspot"
#define WIFI_PASS        "suraksha123"
#define API_URL          "http://192.168.137.1:3000/api/telemetry"

// Peripherals
#define USE_ONBOARD_LED  1
#define USE_BUZZER       1          // Active Buzzer on GPIO 14 (All nodes)

// Pin Definitions
#define MPU_ADDR         0x68
#define SDA_PIN          21
#define SCL_PIN          22
#define PIN_BUZZER       14
#define PIN_ONBOARD      2

// Sampling and Filter Constants
#define SAMPLE_HZ        50
#define SEND_MS          1000
#define VIB_WINDOW       50
#define ALPHA            0.98f      // Complementary filter weighting

#if USE_WIFI_HTTP
#include <HTTPClient.h>
#endif

// Packet structure sent over ESP-NOW
typedef struct __attribute__((packed)) {
  uint8_t  id;           // 1, 2, 3
  float    pitch;        // degrees (relative delta)
  float    roll;         // degrees (relative delta)
  float    vib;          // g RMS
  uint32_t t_ms;         // uptime millis
  uint8_t  risk;         // 0: Normal, 1: Watch, 2: Warning, 3: Critical
} packet_t;

static float pitch_raw = 0, roll_raw = 0;
static float pitch_zero = 0, roll_zero = 0; // Auto-tared resting position
static float delta_pitch = 0, delta_roll = 0;
static bool  tared = false;
static uint32_t tilt_started_at = 0;

static float vib_rms = 0;
static float vib_buf[VIB_WINDOW];
static int   vib_i = 0;
static int   vib_n = 0;
static uint8_t risk = 0;
static uint8_t max_mesh_risk = 0;   // Highest risk heard on mesh
static uint32_t last_mesh_alert = 0;
static uint32_t last_send = 0;
static uint32_t last_sample = 0;
static uint8_t broadcast_mac[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

static const char* getNodeName(uint8_t id) {
  if (id == 1) return "NODE-01";
  if (id == 2) return "NODE-02";
  if (id == 3) return "NODE-03";
  static char customName[16];
  snprintf(customName, sizeof(customName), "NODE-%02d", id);
  return customName;
}

static const char* getNodeRole(uint8_t id) {
  return (id == 1) ? "gateway" : "field";
}

// ==================== Active Buzzer & LED Feedback ====================
static void update_feedback(uint8_t r) {
  uint8_t effective_risk = r;

#if IS_GATEWAY
  // On Gateway, sound buzzer only if own sensor OR incoming mesh packets show warning/critical
  if (millis() - last_mesh_alert < 3000 && max_mesh_risk > effective_risk) {
    effective_risk = max_mesh_risk;
  } else if (millis() - last_mesh_alert >= 3000) {
    max_mesh_risk = 0;
  }
#endif

#if USE_ONBOARD_LED
  digitalWrite(PIN_ONBOARD, (effective_risk >= 1) ? HIGH : LOW);
#endif

#if USE_BUZZER
  uint32_t now_ms = millis();
  if (effective_risk <= 1) {
    // Completely SILENT during Normal (0) and transient Watch (1)
    digitalWrite(PIN_BUZZER, LOW);
  } else if (effective_risk == 2) {
    // Pulsing warning beep on persistent >5° tilt (200ms ON / 200ms OFF)
    bool beep = (now_ms % 400) < 200;
    digitalWrite(PIN_BUZZER, beep ? HIGH : LOW);
  } else if (effective_risk == 3) {
    // Rapid emergency alarm on >8° severe tilt / collapse (80ms ON / 80ms OFF)
    bool alert = (now_ms % 160) < 80;
    digitalWrite(PIN_BUZZER, alert ? HIGH : LOW);
  }
#endif
}

static bool mpu_write(uint8_t reg, uint8_t val) {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(reg);
  Wire.write(val);
  return Wire.endTransmission() == 0;
}

static bool mpu_begin() {
  delay(50);
  if (!mpu_write(0x6B, 0x00)) return false; // Wake up MPU6050
  mpu_write(0x1B, 0x00);                    // Gyro +-250 dps
  mpu_write(0x1C, 0x00);                    // Accel +-2 g
  return true;
}

static bool mpu_read(float *ax, float *ay, float *az, float *gx, float *gy, float *gz) {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom((int)MPU_ADDR, 14) != 14) return false;

  int16_t rax = (Wire.read() << 8) | Wire.read();
  int16_t ray = (Wire.read() << 8) | Wire.read();
  int16_t raz = (Wire.read() << 8) | Wire.read();
  Wire.read(); Wire.read(); // Skip temp
  int16_t rgx = (Wire.read() << 8) | Wire.read();
  int16_t rgy = (Wire.read() << 8) | Wire.read();
  int16_t rgz = (Wire.read() << 8) | Wire.read();

  *ax = rax / 16384.0f;
  *ay = ray / 16384.0f;
  *az = raz / 16384.0f;
  *gx = rgx / 131.0f;
  *gy = rgy / 131.0f;
  *gz = rgz / 131.0f;
  return true;
}

// Exactly matches website's decision engine thresholds and persistence
static uint8_t classify(float dp, float dr, float v, uint32_t now) {
  if (!tared) return 0; // Silent while auto-taring

  float delta_tilt = fabsf(dp) > fabsf(dr) ? fabsf(dp) : fabsf(dr);

  // Track persistent deformation (filters out accidental 1-second bumps)
  if (delta_tilt >= 2.0f) {
    if (tilt_started_at == 0) tilt_started_at = now;
  } else if (delta_tilt < 1.5f) {
    tilt_started_at = 0; // Hysteresis recovery
  }

  bool is_persistent = (tilt_started_at != 0) && ((now - tilt_started_at) >= 3000);

  if (delta_tilt >= 8.0f) {
    return 3; // Critical immediately on severe >8° tilt
  }
  if (delta_tilt >= 5.0f && is_persistent) {
    return 2; // Warning ONLY if >5° tilt is held steady for 3 seconds
  }
  if (delta_tilt >= 2.0f || v >= 0.25f) {
    return 1; // Watch on minor movements (Buzzer stays SILENT)
  }
  return 0;   // Normal (Buzzer stays SILENT)
}

static void emit_json(const packet_t *p) {
  Serial.print("{\"nodeId\":\"");
  Serial.print(getNodeName(p->id));
  Serial.print("\",\"role\":\"");
  Serial.print(getNodeRole(p->id));
  Serial.print("\",\"pitch\":");
  Serial.print(p->pitch, 2);
  Serial.print(",\"roll\":");
  Serial.print(p->roll, 2);
  Serial.print(",\"vibration\":");
  Serial.print(p->vib, 4);
  Serial.print(",\"t\":");
  Serial.print(p->t_ms);
  Serial.print(",\"risk\":");
  Serial.print(p->risk);
  Serial.println("}");
}

#if USE_WIFI_HTTP && IS_GATEWAY
static void post_http_packet(const packet_t *p) {
  if (WiFi.status() != WL_CONNECTED) return;
  HTTPClient http;
  http.begin(API_URL);
  http.addHeader("Content-Type", "application/json");

  char jsonBuf[256];
  snprintf(jsonBuf, sizeof(jsonBuf),
    "{\"packets\":[{\"nodeId\":\"%s\",\"role\":\"%s\",\"pitch\":%.2f,\"roll\":%.2f,\"vibration\":%.4f}]}",
    getNodeName(p->id), getNodeRole(p->id), p->pitch, p->roll, p->vib);

  int httpCode = http.POST((uint8_t*)jsonBuf, strlen(jsonBuf));
  http.end();
}
#endif

static void send_esp_now(const packet_t *p) {
  esp_now_send(broadcast_mac, (const uint8_t *)p, sizeof(*p));
}

#if IS_GATEWAY
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
void on_rx(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  (void)info;
#else
void on_rx(const uint8_t *mac, const uint8_t *data, int len) {
  (void)mac;
#endif
  if (len < (int)sizeof(packet_t)) return;
  packet_t p;
  memcpy(&p, data, sizeof(p));
  
  // Track mesh alert for gateway buzzer
  if (p.risk >= 2) {
    max_mesh_risk = p.risk;
    last_mesh_alert = millis();
  }
  
  emit_json(&p);
#if USE_WIFI_HTTP
  post_http_packet(&p);
#endif
}
#endif

void setup() {
  Serial.begin(115200);
  delay(200);

#if USE_ONBOARD_LED
  pinMode(PIN_ONBOARD, OUTPUT);
  digitalWrite(PIN_ONBOARD, LOW);
#endif

#if USE_BUZZER
  pinMode(PIN_BUZZER, OUTPUT);
  digitalWrite(PIN_BUZZER, LOW);
#endif

  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(400000);
  if (!mpu_begin()) {
    Serial.println("{\"err\":\"mpu6050_not_found\"}");
  }

#if USE_WIFI_HTTP && IS_GATEWAY
  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
#else
  WiFi.mode(WIFI_STA);
#endif

  if (esp_now_init() != ESP_OK) {
    Serial.println("{\"err\":\"espnow_init_failed\"}");
  }

  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, broadcast_mac, 6);
  peer.channel = 0;
  peer.encrypt = false;
  esp_now_add_peer(&peer);

#if IS_GATEWAY
  esp_now_register_recv_cb(on_rx);
#endif

  last_sample = micros();
  Serial.print("{\"boot\":1,\"nodeId\":\"");
  Serial.print(getNodeName(NODE_INDEX));
  Serial.print("\",\"isGateway\":");
  Serial.print(IS_GATEWAY);
  Serial.println("}");
}

void loop() {
  uint32_t now_us = micros();
  uint32_t dt_us = now_us - last_sample;
  if (dt_us < (1000000UL / SAMPLE_HZ)) {
    update_feedback(risk);
    return;
  }
  last_sample = now_us;
  float dt = dt_us / 1000000.0f;
  if (dt > 0.1f) dt = 0.02f;

  float ax, ay, az, gx, gy, gz;
  if (!mpu_read(&ax, &ay, &az, &gx, &gy, &gz)) {
    update_feedback(risk);
    return;
  }

  float acc_pitch = atan2f(-ax, sqrtf(ay * ay + az * az)) * 57.2957795f;
  float acc_roll  = atan2f(ay, az) * 57.2957795f;
  pitch_raw = ALPHA * (pitch_raw + gy * dt) + (1.0f - ALPHA) * acc_pitch;
  roll_raw  = ALPHA * (roll_raw  + gx * dt) + (1.0f - ALPHA) * acc_roll;

#if FLIP_PITCH
  pitch_raw = -pitch_raw;
#endif
#if FLIP_ROLL
  roll_raw = -roll_raw;
#endif

  // Auto-tare baseline zero offset during initial 1.5 seconds after power-on
  static uint32_t tare_samples = 0;
  static float sum_p = 0, sum_r = 0;
  if (!tared) {
    sum_p += pitch_raw;
    sum_r += roll_raw;
    tare_samples++;
    if (tare_samples >= 50) {
      pitch_zero = sum_p / 50.0f;
      roll_zero  = sum_r / 50.0f;
      tared = true;
    }
  }

  delta_pitch = pitch_raw - pitch_zero;
  delta_roll  = roll_raw - roll_zero;

  float mag = sqrtf(ax * ax + ay * ay + az * az);
  float ac = mag - 1.0f;
  vib_buf[vib_i] = ac * ac;
  vib_i = (vib_i + 1) % VIB_WINDOW;
  if (vib_n < VIB_WINDOW) vib_n++;
  float ss = 0;
  for (int i = 0; i < vib_n; i++) ss += vib_buf[i];
  vib_rms = sqrtf(ss / vib_n);

  uint32_t now = millis();
  risk = classify(delta_pitch, delta_roll, vib_rms, now);
  update_feedback(risk);

  if (now - last_send < SEND_MS) return;
  last_send = now;

  packet_t p;
  p.id = NODE_INDEX;
  p.pitch = delta_pitch;
  p.roll = delta_roll;
  p.vib = vib_rms;
  p.t_ms = now;
  p.risk = risk;

  // Broadcast packet to gateway
  send_esp_now(&p);

  // If central gateway, emit own reading locally
#if IS_GATEWAY
  emit_json(&p);
#if USE_WIFI_HTTP
  post_http_packet(&p);
#endif
#endif
}
