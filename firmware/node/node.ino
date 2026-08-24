/*
 * SurakshaMesh Node Firmware — SIH26025
 * Unified sketch for Central Gateway & Mesh Field Nodes.
 *
 * Board Configurations:
 *   NODE_INDEX 1, IS_GATEWAY 1 -> Central Gateway Node (NODE-01) with MAX98357A I2S Speaker & Serial
 *   NODE_INDEX 2, IS_GATEWAY 0 -> Mesh Node 02 (NODE-02) field sensor with Active Buzzer on power bank
 *   NODE_INDEX 3, IS_GATEWAY 0 -> Mesh Node 03 (NODE-03) field sensor with Active Buzzer on power bank
 *
 * Wiring:
 *   MPU6050:      VCC -> 3V3 | GND -> GND | SDA -> GPIO 21 | SCL -> GPIO 22 | AD0 -> GND
 *   Active Buzzer: Pos(+) -> GPIO 14 | Neg(-) -> GND (on Field Nodes 02 & 03)
 *   MAX98357A:    VCC -> 5V/VIN | GND -> GND | LRC/WS -> GPIO 25 | BCLK -> GPIO 26 | DIN -> GPIO 27 (on Gateway 01)
 *   Status LED:   Anode -> GPIO 25 (Field) or Onboard LED GPIO 2
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
#define USE_BUZZER       1          // Active Buzzer on Field Nodes (GPIO 14)
#define USE_MAX98357A    1          // I2S Audio Amp on Gateway (GPIO 25, 26, 27)

// Pin Definitions
#define MPU_ADDR         0x68
#define SDA_PIN          21
#define SCL_PIN          22
#define PIN_BUZZER       14
#define PIN_ONBOARD      2

// I2S Audio Pins for MAX98357A (Central Gateway)
#if IS_GATEWAY && USE_MAX98357A
#include "driver/i2s.h"
#define I2S_PORT         I2S_NUM_0
#define I2S_LRC_PIN      25         // WS / LRC
#define I2S_BCLK_PIN     26         // BCLK
#define I2S_DOUT_PIN     27         // DIN
#define SAMPLE_RATE      16000
#endif

// Sampling and Thresholds
#define SAMPLE_HZ        50
#define SEND_MS          1000
#define VIB_WINDOW       50

#define WATCH_DEG        2.0f
#define WARN_DEG         5.0f
#define CRIT_DEG         8.0f
#define VIB_G            0.15f
#define WATCH_HOLD_MS    5000
#define GREEN_HOLD_MS    2000
#define GREEN_BACK_DEG   1.5f

#define ALPHA            0.98f      // Complementary filter weighting

#if USE_WIFI_HTTP
#include <HTTPClient.h>
#endif

// Packet structure sent over ESP-NOW
typedef struct __attribute__((packed)) {
  uint8_t  id;           // 1, 2, 3
  float    pitch;        // degrees
  float    roll;         // degrees
  float    vib;          // g RMS
  uint32_t t_ms;         // uptime millis
  uint8_t  risk;         // 0: Normal, 1: Watch, 2: Warning, 3: Critical
} packet_t;

static float pitch_deg = 0, roll_deg = 0;
static float vib_rms = 0;
static float vib_buf[VIB_WINDOW];
static int   vib_i = 0;
static int   vib_n = 0;
static uint8_t risk = 0;
static uint8_t max_mesh_risk = 0;   // Highest risk heard on mesh
static uint32_t watch_since = 0;
static uint32_t green_since = 0;
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

// ==================== I2S Siren Generator (MAX98357A) ====================
#if IS_GATEWAY && USE_MAX98357A
static void i2s_init_audio() {
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = 0,
    .dma_buf_count = 4,
    .dma_buf_len = 256,
    .use_apll = false,
    .tx_desc_auto_clear = true
  };
  i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_BCLK_PIN,
    .ws_io_num = I2S_LRC_PIN,
    .data_out_num = I2S_DOUT_PIN,
    .data_in_num = I2S_PIN_NO_CHANGE
  };
  i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_PORT, &pin_config);
}

static void play_siren_step(uint8_t alert_level) {
  if (alert_level < 2) return; // Silent on normal/watch
  
  static float phase = 0;
  static uint32_t last_tone_swap = 0;
  static bool tone_hi = false;
  
  uint32_t now_ms = millis();
  if (now_ms - last_tone_swap > (alert_level == 3 ? 250 : 500)) {
    last_tone_swap = now_ms;
    tone_hi = !tone_hi;
  }

  float freq = tone_hi ? (alert_level == 3 ? 1200.0f : 880.0f) : (alert_level == 3 ? 750.0f : 587.0f);
  float phase_inc = (2.0f * M_PI * freq) / SAMPLE_RATE;

  int16_t sample_buffer[128 * 2];
  for (int i = 0; i < 128; i++) {
    int16_t sample = (int16_t)(sinf(phase) * 12000.0f); // ~40% volume sine wave
    phase += phase_inc;
    if (phase >= 2.0f * M_PI) phase -= 2.0f * M_PI;
    sample_buffer[i * 2] = sample;     // Left
    sample_buffer[i * 2 + 1] = sample; // Right
  }
  size_t bytes_written;
  i2s_write(I2S_PORT, sample_buffer, sizeof(sample_buffer), &bytes_written, 10);
}
#endif

// ==================== Active Buzzer & LED Feedback ====================
static void update_feedback(uint8_t r) {
#if USE_ONBOARD_LED
  digitalWrite(PIN_ONBOARD, (r >= 1) ? HIGH : LOW);
#endif

#if !IS_GATEWAY && USE_BUZZER
  uint32_t now_ms = millis();
  if (r == 0) {
    digitalWrite(PIN_BUZZER, LOW);
  } else if (r == 1) {
    // Short periodic chirp every 2.5s
    bool chirp = (now_ms % 2500) < 50;
    digitalWrite(PIN_BUZZER, chirp ? HIGH : LOW);
  } else if (r == 2) {
    // Pulsing warning beep (200ms ON / 200ms OFF)
    bool beep = (now_ms % 400) < 200;
    digitalWrite(PIN_BUZZER, beep ? HIGH : LOW);
  } else if (r == 3) {
    // Rapid emergency alarm (80ms ON / 80ms OFF)
    bool alert = (now_ms % 160) < 80;
    digitalWrite(PIN_BUZZER, alert ? HIGH : LOW);
  }
#endif

#if IS_GATEWAY && USE_MAX98357A
  play_siren_step(max(r, max_mesh_risk));
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

static uint8_t classify(float p, float r, float v, uint32_t now) {
  float a = fabsf(p) > fabsf(r) ? fabsf(p) : fabsf(r);
  uint8_t next;
  if (a >= CRIT_DEG)                     next = 3;
  else if (a >= WARN_DEG)                next = 2;
  else if (a >= WATCH_DEG || v >= VIB_G) next = 1;
  else                                   next = 0;

  if (next >= 1) {
    if (watch_since == 0) watch_since = now;
    if (next == 1 && (now - watch_since) >= WATCH_HOLD_MS && v >= VIB_G)
      next = 3;
    green_since = 0;
  } else {
    watch_since = 0;
    if (a > GREEN_BACK_DEG) {
      next = risk >= 1 ? risk : 0;
    } else {
      if (green_since == 0) green_since = now;
      if ((now - green_since) < GREEN_HOLD_MS) next = risk;
    }
  }
  return next;
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
  
  // Track highest risk reported across the entire mesh
  if (p.risk > max_mesh_risk) max_mesh_risk = p.risk;
  
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
#endif

#if !IS_GATEWAY && USE_BUZZER
  pinMode(PIN_BUZZER, OUTPUT);
  digitalWrite(PIN_BUZZER, LOW);
#endif

#if IS_GATEWAY && USE_MAX98357A
  i2s_init_audio();
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
  pitch_deg = ALPHA * (pitch_deg + gy * dt) + (1.0f - ALPHA) * acc_pitch;
  roll_deg  = ALPHA * (roll_deg  + gx * dt) + (1.0f - ALPHA) * acc_roll;

#if FLIP_PITCH
  pitch_deg = -pitch_deg;
#endif
#if FLIP_ROLL
  roll_deg = -roll_deg;
#endif

  float mag = sqrtf(ax * ax + ay * ay + az * az);
  float ac = mag - 1.0f;
  vib_buf[vib_i] = ac * ac;
  vib_i = (vib_i + 1) % VIB_WINDOW;
  if (vib_n < VIB_WINDOW) vib_n++;
  float ss = 0;
  for (int i = 0; i < vib_n; i++) ss += vib_buf[i];
  vib_rms = sqrtf(ss / vib_n);

  uint32_t now = millis();
  risk = classify(pitch_deg, roll_deg, vib_rms, now);
  update_feedback(risk);

  if (now - last_send < SEND_MS) return;
  last_send = now;

  packet_t p;
  p.id = NODE_INDEX;
  p.pitch = pitch_deg;
  p.roll = roll_deg;
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
