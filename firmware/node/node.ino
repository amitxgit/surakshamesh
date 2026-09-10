/*
 * ================================================================================
 *                   SURAKSHAMESH NODE FIRMWARE — SIH26025
 * Unified Sketch for Central Gateway (NODE-01) & Mesh Nodes (NODE-02, NODE-03)
 * ================================================================================
 *
 * Board Roles:
 *   NODE_INDEX 1, IS_GATEWAY 1 -> Central Gateway (NODE-01) with USB Serial / Wi-Fi Telemetry
 *   NODE_INDEX 2, IS_GATEWAY 0 -> Field Sensor Node (NODE-02) on wooden plank
 *   NODE_INDEX 3, IS_GATEWAY 0 -> Field Sensor Node (NODE-03) on wooden plank
 *
 * Hardware Wiring (Identical across all 3 nodes):
 *   MPU6050 IMU:   VCC -> 3V3 | GND -> GND | SDA -> GPIO 21 | SCL -> GPIO 22 | AD0 -> GND
 *   Active Buzzer: Pos(+) -> GPIO 14 | Neg(-) -> GND
 *   Status LED:    Onboard LED (GPIO 2)
 *   Power:         TP4056 + 18650 Battery (OUT+ -> VIN, OUT- -> GND) or USB-C
 *
 * Libraries Required (Built-in with ESP32 Board Package):
 *   Wire.h, esp_now.h, WiFi.h, esp_wifi.h, math.h
 * ================================================================================
 */

#include <Wire.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <WiFi.h>
#include <math.h>

// ==================== Node Configuration ====================
#define NODE_INDEX       1          // 1: NODE-01 (Gateway), 2: NODE-02, 3: NODE-03
#define IS_GATEWAY       1          // 1 on Central Gateway (NODE-01), 0 on Field Nodes
#define FLIP_PITCH       0          // 1 if IMU is mounted reversed on pitch axis
#define FLIP_ROLL        0          // 1 if IMU is mounted reversed on roll axis
#define MESH_CHANNEL     1          // Fixed Wi-Fi channel for 100% reliable ESP-NOW mesh

// Communication Mode for Gateway
#define USE_WIFI_HTTP    0          // 0 = USB Serial (serial-bridge.mjs), 1 = Direct Wi-Fi HTTP POST
#define WIFI_SSID        "SurakshaMesh-Hotspot"
#define WIFI_PASS        "suraksha123"
#define API_URL          "http://192.168.137.1:3000/api/telemetry"

// Peripherals
#define USE_ONBOARD_LED  1
#define USE_BUZZER       1          // Active Buzzer on GPIO 14

// Pin Definitions
#define MPU_ADDR         0x68
#define SDA_PIN          21
#define SCL_PIN          22
#define PIN_BUZZER       14
#define PIN_ONBOARD      2

// Sampling & Filter Constants
#define SAMPLE_HZ        50         // 50 Hz loop rate (20ms interval)
#define SEND_MS          1000       // Broadcast rate: 1 packet per second
#define VIB_WINDOW       50         // 50-sample rolling RMS window (1.0s)
#define ALPHA            0.98f      // Complementary filter weight (98% gyro + 2% accel)

#if USE_WIFI_HTTP
#include <HTTPClient.h>
#endif

// Packet structure sent over ESP-NOW mesh (29 bytes packed)
typedef struct __attribute__((packed)) {
  uint8_t  id;           // Node ID (1, 2, 3)
  uint16_t seq;          // Packet sequence counter (wraps at 65535)
  float    pitch;        // Relative delta pitch (degrees)
  float    roll;         // Relative delta roll (degrees)
  float    vib;          // Dynamic vibration RMS (g)
  float    stalta;       // Current STA/LTA ratio
  float    temp_c;       // Internal MPU6050 temperature (Celsius)
  uint32_t t_ms;         // Node uptime (milliseconds)
  uint8_t  risk;         // Risk Level: 0=Normal, 1=Watch, 2=Warning, 3=Critical
  uint8_t  event_type;   // Event Classification: 0=None, 1=Pending, 2=Subsidence, 3=Blast
} packet_t;

// Sensor & Filter State
static bool  mpu_ready = false;
static bool  first_sample = true;
static bool  tared = false;
static float pitch_raw = 0.0f, roll_raw = 0.0f;
static float pitch_zero = 0.0f, roll_zero = 0.0f; // Auto-tared resting zero
static float delta_pitch = 0.0f, delta_roll = 0.0f;
static uint32_t tilt_started_at = 0;

// Dynamic Vibration State (AC filter eliminates static gravity offset)
static float mag_dc = 1.0f;
static float vib_rms = 0.0f;
static float vib_buf[VIB_WINDOW];
static int   vib_i = 0;
static int   vib_n = 0;

// STA/LTA Event Detector & Blast Classifier (Allen 1978, Earle & Shearer 1994)
#define STA_SAMPLES        25        // 0.5 s at 50 Hz
#define LTA_SAMPLES        1500      // 30.0 s at 50 Hz
#define STALTA_THRESH      4.0f      // Event trigger ratio

static float   sta_buf[STA_SAMPLES];
static float   lta_buf[LTA_SAMPLES];
static int     sta_i   = 0;
static int     lta_i   = 0;
static float   sta_sum = 0.0f;
static float   lta_sum = 0.0f;
static float   stalta_ratio = 0.0f;

// Event State Machine (Blast vs. Ground Subsidence)
#define EVT_TILT_DELTA_DEG 0.5f      // Minimum tilt shift to confirm subsidence (degrees)
#define EVT_CHECK_MS       30000UL   // 30s observation window before classifying
#define EVT_RESET_MS       90000UL   // 90s reset window after classification

typedef enum { EVT_NONE = 0, EVT_PENDING = 1, EVT_SUBSIDENCE = 2, EVT_BLAST = 3 } evt_t;

static evt_t    evt_state          = EVT_NONE;
static uint32_t evt_time           = 0;
static float    evt_pitch_snapshot = 0.0f;
static float    evt_roll_snapshot  = 0.0f;
static uint8_t  event_type         = 0;
static uint16_t seq_counter        = 0;
static float    node_temp_c        = 25.0f;

// Alarm & Mesh State
static uint8_t risk = 0;
static uint8_t max_mesh_risk = 0;
static uint32_t last_mesh_alert = 0;
static uint32_t last_send = 0;
static uint32_t last_sample = 0;
static uint32_t last_mpu_retry = 0;
static uint8_t broadcast_mac[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// Helpers
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
  // Gateway sounds alarm if its own sensor OR incoming mesh packets detect danger
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
    // Rapid emergency siren on >8° severe tilt / collapse (80ms ON / 80ms OFF)
    bool alert = (now_ms % 160) < 80;
    digitalWrite(PIN_BUZZER, alert ? HIGH : LOW);
  }
#endif
}

// ==================== MPU6050 Driver ====================
static bool mpu_write(uint8_t reg, uint8_t val) {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(reg);
  Wire.write(val);
  return (Wire.endTransmission() == 0);
}

static bool mpu_begin() {
  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(400000);
  delay(50);
  if (!mpu_write(0x6B, 0x00)) return false; // Wake up from sleep
  mpu_write(0x1B, 0x00);                    // Gyro range: +-250 dps
  mpu_write(0x1C, 0x00);                    // Accel range: +-2 g
  return true;
}

static bool mpu_read(float *ax, float *ay, float *az, float *gx, float *gy, float *gz, float *temp) {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom((int)MPU_ADDR, 14) != 14) return false;

  int16_t rax = (Wire.read() << 8) | Wire.read();
  int16_t ray = (Wire.read() << 8) | Wire.read();
  int16_t raz = (Wire.read() << 8) | Wire.read();
  int16_t rtemp = (Wire.read() << 8) | Wire.read();
  int16_t rgx = (Wire.read() << 8) | Wire.read();
  int16_t rgy = (Wire.read() << 8) | Wire.read();
  int16_t rgz = (Wire.read() << 8) | Wire.read();

  *ax = rax / 16384.0f;
  *ay = ray / 16384.0f;
  *az = raz / 16384.0f;
  if (temp) *temp = (rtemp / 340.0f) + 36.53f; // MPU6050 internal thermometer formula
  *gx = rgx / 131.0f;
  *gy = rgy / 131.0f;
  *gz = rgz / 131.0f;
  return true;
}

// ==================== Decision Engine ====================
// Matches the Command Center algorithm:
// - ΔTilt < 2.0° : Normal (0)
// - ΔTilt 2.0°–5.0° or vibration >= 0.15g : Watch (1)
// - ΔTilt > 5.0° sustained for 3.0s : Warning (2)
// - ΔTilt > 8.0° : Critical Emergency (3)
static uint8_t classify(float dp, float dr, float v, uint32_t now) {
  if (!tared) return 0; // Remain silent during 1-second startup auto-tare

  float delta_tilt = fabsf(dp) > fabsf(dr) ? fabsf(dp) : fabsf(dr);

  // Track deformation persistence to prevent false alarms from brief 1-second bumps
  if (delta_tilt >= 2.0f) {
    if (tilt_started_at == 0) tilt_started_at = now;
  } else if (delta_tilt < 1.5f) {
    tilt_started_at = 0; // Hysteresis reset
  }

  bool is_persistent = (tilt_started_at != 0) && ((now - tilt_started_at) >= 3000);

  if (delta_tilt >= 8.0f) {
    return 3; // Critical: Immediate alarm on severe tilt
  }
  if (delta_tilt >= 5.0f && is_persistent) {
    return 2; // Warning: 5° tilt held for >3 seconds
  }
  if (delta_tilt >= 2.0f || v >= 0.15f) {
    return 1; // Watch: Minor tilt/vibration (Buzzer stays SILENT)
  }
  return 0;   // Normal: Ground is stable (Buzzer stays SILENT)
}

// ==================== Telemetry Emission ====================
static void emit_json(const packet_t *p) {
  Serial.print("{\"nodeId\":\"");
  Serial.print(getNodeName(p->id));
  Serial.print("\",\"role\":\"");
  Serial.print(getNodeRole(p->id));
  Serial.print("\",\"seq\":");
  Serial.print(p->seq);
  Serial.print(",\"pitch\":");
  Serial.print(p->pitch, 2);
  Serial.print(",\"roll\":");
  Serial.print(p->roll, 2);
  Serial.print(",\"vibration\":");
  Serial.print(p->vib, 4);
  Serial.print(",\"stalta\":");
  Serial.print(p->stalta, 2);
  Serial.print(",\"temp\":");
  Serial.print(p->temp_c, 1);
  Serial.print(",\"t\":");
  Serial.print(p->t_ms);
  Serial.print(",\"risk\":");
  Serial.print(p->risk);
  Serial.print(",\"evt\":");
  Serial.print(p->event_type);
  Serial.println("}");
}

#if USE_WIFI_HTTP && IS_GATEWAY
static void post_http_packet(const packet_t *p) {
  if (WiFi.status() != WL_CONNECTED) return;
  HTTPClient http;
  http.begin(API_URL);
  http.addHeader("Content-Type", "application/json");

  char jsonBuf[320];
  snprintf(jsonBuf, sizeof(jsonBuf),
    "{\"packets\":[{\"nodeId\":\"%s\",\"role\":\"%s\",\"seq\":%u,\"pitch\":%.2f,\"roll\":%.2f,\"vibration\":%.4f,\"stalta\":%.2f,\"temp\":%.1f,\"risk\":%u,\"evt\":%u}]}",
    getNodeName(p->id), getNodeRole(p->id), p->seq, p->pitch, p->roll, p->vib, p->stalta, p->temp_c, p->risk, p->event_type);

  http.POST((uint8_t*)jsonBuf, strlen(jsonBuf));
  http.end();
}
#endif

static void send_esp_now(const packet_t *p) {
  esp_now_send(broadcast_mac, (const uint8_t *)p, sizeof(*p));
}

// ==================== ESP-NOW Receiver Callback ====================
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

  // If incoming mesh packet reports warning or critical, update gateway buzzer
  if (p.risk >= 2) {
    max_mesh_risk = p.risk;
    last_mesh_alert = millis();
  }

  // Output to USB serial for Next.js dashboard
  emit_json(&p);

#if USE_WIFI_HTTP
  post_http_packet(&p);
#endif
}
#endif

// ==================== Setup ====================
void setup() {
  Serial.begin(115200);
  delay(200);

#if USE_ONBOARD_LED
  pinMode(PIN_ONBOARD, OUTPUT);
  digitalWrite(PIN_ONBOARD, HIGH); // Light LED during boot
#endif

#if USE_BUZZER
  pinMode(PIN_BUZZER, OUTPUT);
  // Short 100ms confirmation chirp on startup
  digitalWrite(PIN_BUZZER, HIGH);
  delay(100);
  digitalWrite(PIN_BUZZER, LOW);
#endif

#if USE_ONBOARD_LED
  digitalWrite(PIN_ONBOARD, LOW);
#endif

  // Initialize MPU6050
  mpu_ready = mpu_begin();
  if (!mpu_ready) {
    Serial.println("{\"err\":\"mpu6050_not_found\",\"msg\":\"Check I2C wiring (SDA=21, SCL=22)\"}");
  }

  // Setup Wi-Fi and Radio Channel
#if USE_WIFI_HTTP && IS_GATEWAY
  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
#else
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  esp_wifi_set_channel(MESH_CHANNEL, WIFI_SECOND_CHAN_NONE);
#endif

  // Initialize ESP-NOW
  if (esp_now_init() == ESP_OK) {
    esp_now_peer_info_t peer = {};
    memcpy(peer.peer_addr, broadcast_mac, 6);
    peer.channel = 0; // Send on current Wi-Fi channel
    peer.encrypt = false;
    esp_now_add_peer(&peer);

#if IS_GATEWAY
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
    esp_now_register_recv_cb(on_rx);
#else
    esp_now_register_recv_cb((esp_now_recv_cb_t)on_rx);
#endif
#endif
  } else {
    Serial.println("{\"err\":\"espnow_init_failed\"}");
  }

  last_sample = micros();
  Serial.print("{\"boot\":1,\"nodeId\":\"");
  Serial.print(getNodeName(NODE_INDEX));
  Serial.print("\",\"role\":\"");
  Serial.print(getNodeRole(NODE_INDEX));
  Serial.print("\",\"isGateway\":");
  Serial.print(IS_GATEWAY);
  Serial.println("}");
}

// ==================== Main Loop (50 Hz) ====================
void loop() {
  uint32_t now_us = micros();
  uint32_t dt_us = now_us - last_sample;

  // Enforce 50 Hz loop rate (20,000 µs interval)
  if (dt_us < (1000000UL / SAMPLE_HZ)) {
    update_feedback(risk);
    return;
  }
  last_sample = now_us;
  float dt = dt_us / 1000000.0f;
  if (dt > 0.1f) dt = 0.02f;

  uint32_t now = millis();

  // Retry MPU initialization every 5s if disconnected at boot
  if (!mpu_ready) {
    if (now - last_mpu_retry >= 5000) {
      last_mpu_retry = now;
      mpu_ready = mpu_begin();
    }
  }

  // If MPU is ready, read and process sensor data
  if (mpu_ready) {
    float ax, ay, az, gx, gy, gz;
    if (mpu_read(&ax, &ay, &az, &gx, &gy, &gz, &node_temp_c)) {
      // Calculate gravity tilt angles
      float acc_pitch = atan2f(-ax, sqrtf(ay * ay + az * az)) * 57.2957795f;
      float acc_roll  = atan2f(ay, az) * 57.2957795f;

      // Seed initial angles on first sample to avoid startup filter ramp lag
      if (first_sample) {
        pitch_raw = acc_pitch;
        roll_raw  = acc_roll;
        mag_dc = sqrtf(ax * ax + ay * ay + az * az);
        first_sample = false;
      } else {
        // Complementary Filter: 98% Gyro integration + 2% Accelerometer gravity
        pitch_raw = ALPHA * (pitch_raw + gy * dt) + (1.0f - ALPHA) * acc_pitch;
        roll_raw  = ALPHA * (roll_raw  + gx * dt) + (1.0f - ALPHA) * acc_roll;
      }

#if FLIP_PITCH
      pitch_raw = -pitch_raw;
#endif
#if FLIP_ROLL
      roll_raw = -roll_raw;
#endif

      // Auto-tare resting baseline during first 40 stable samples (approx. 0.8s)
      static uint32_t tare_samples = 0;
      static float sum_p = 0.0f, sum_r = 0.0f;
      if (!tared) {
        sum_p += pitch_raw;
        sum_r += roll_raw;
        tare_samples++;
        if (tare_samples >= 40) {
          pitch_zero = sum_p / 40.0f;
          roll_zero  = sum_r / 40.0f;
          tared = true;
        }
      }

      // Calculate relative deviation from calibrated ground zero
      delta_pitch = pitch_raw - pitch_zero;
      delta_roll  = roll_raw - roll_zero;

      // Dynamic Vibration RMS (Filtered for pure AC vibration, ignoring static gravity)
      float mag = sqrtf(ax * ax + ay * ay + az * az);
      mag_dc = 0.995f * mag_dc + 0.005f * mag; // Slow DC gravity tracker
      float ac = mag - mag_dc;

      vib_buf[vib_i] = ac * ac;
      vib_i = (vib_i + 1) % VIB_WINDOW;
      if (vib_n < VIB_WINDOW) vib_n++;
      float ss = 0.0f;
      for (int i = 0; i < vib_n; i++) ss += vib_buf[i];
      vib_rms = sqrtf(ss / vib_n);

      // ---- STA/LTA Event Detector (Allen 1978, Earle & Shearer 1994) ----
      float ac_abs = fabsf(ac);

      // Short-Term Average (STA: 0.5s window at 50 Hz = 25 samples)
      sta_sum -= sta_buf[sta_i];
      sta_buf[sta_i] = ac_abs;
      sta_sum += ac_abs;
      sta_i = (sta_i + 1) % STA_SAMPLES;

      // Long-Term Average (LTA: 30.0s window at 50 Hz = 1500 samples)
      lta_sum -= lta_buf[lta_i];
      lta_buf[lta_i] = ac_abs;
      lta_sum += ac_abs;
      lta_i = (lta_i + 1) % LTA_SAMPLES;

      float sta_mean = sta_sum / (float)STA_SAMPLES;
      float lta_mean = lta_sum / (float)LTA_SAMPLES;
      stalta_ratio   = (lta_mean > 0.001f) ? (sta_mean / lta_mean) : 0.0f;
      bool event_now = (stalta_ratio > STALTA_THRESH);

      // ---- Event Classification State Machine (Blast vs. Ground Subsidence) ----
      if (event_now && evt_state == EVT_NONE) {
        evt_state          = EVT_PENDING;
        evt_time           = now;
        evt_pitch_snapshot = delta_pitch;
        evt_roll_snapshot  = delta_roll;
        event_type         = 1; // Pending
      }

      if (evt_state == EVT_PENDING && (now - evt_time) >= EVT_CHECK_MS) {
        float dp = fabsf(delta_pitch - evt_pitch_snapshot);
        float dr = fabsf(delta_roll  - evt_roll_snapshot);
        if (dp > EVT_TILT_DELTA_DEG || dr > EVT_TILT_DELTA_DEG) {
          evt_state  = EVT_SUBSIDENCE;
          event_type = 2; // Permanent tilt offset -> Subsidence event
        } else {
          evt_state  = EVT_BLAST;
          event_type = 3; // No permanent tilt offset -> Transient blast vibration
        }
      }

      // Reset after 90s cooldown to re-arm detector
      if ((evt_state == EVT_SUBSIDENCE || evt_state == EVT_BLAST) && (now - evt_time) >= EVT_RESET_MS) {
        evt_state  = EVT_NONE;
        event_type = 0;
      }

      // Classify subsidence risk
      risk = classify(delta_pitch, delta_roll, vib_rms, now);
    } else {
      mpu_ready = false; // I2C communication dropped
    }
  }

  // Update physical Buzzer & LED
  update_feedback(risk);

  // Broadcast telemetry once per second
  if (now - last_send < SEND_MS) return;
  last_send = now;

  packet_t p;
  p.id = NODE_INDEX;
  p.seq = seq_counter++;
  p.pitch = delta_pitch;
  p.roll = delta_roll;
  p.vib = vib_rms;
  p.stalta = stalta_ratio;
  p.temp_c = node_temp_c;
  p.t_ms = now;
  p.risk = risk;
  p.event_type = event_type;

  // Broadcast packet to Gateway over ESP-NOW mesh
  send_esp_now(&p);

  // If Gateway, emit own reading locally to USB Serial / HTTP
#if IS_GATEWAY
  emit_json(&p);
#if USE_WIFI_HTTP
  post_http_packet(&p);
#endif
#endif
}
