/*
 * SurakshaMesh node — SIH26025
 * One sketch for every ESP32.
 *
 *   NODE_ID 1, IS_USB_HUB 1  -> board plugged into the laptop
 *   NODE_ID 2, IS_USB_HUB 0  -> board on the power bank
 *
 * MPU6050: VCC=3V3  GND=GND  SDA=21  SCL=22  ADO=GND
 * RGB CC:  R=25 G=26 B=27
 */

#include <Wire.h>
#include <esp_now.h>
#include <WiFi.h>
#include <math.h>

#define NODE_ID        1
#define IS_USB_HUB     1
#define FLIP_PITCH     0
#define FLIP_ROLL      0
#define USE_RGB        1
#define USE_ONBOARD_LED 1
#define USE_FLEX       0

#define MPU_ADDR       0x68
#define SDA_PIN        21
#define SCL_PIN        22
#define PIN_R          25
#define PIN_G          26
#define PIN_B          27
#define PIN_FLEX       34
#define PIN_ONBOARD    2

#define SAMPLE_HZ      50
#define SEND_MS        1000
#define VIB_WINDOW     50

#define WATCH_DEG      2.0f
#define WARN_DEG       5.0f
#define CRIT_DEG       8.0f
#define VIB_G          0.15f
#define WATCH_HOLD_MS  8000
#define GREEN_HOLD_MS  2000
#define GREEN_BACK_DEG 1.5f

#define ALPHA          0.98f   // complementary filter

typedef struct __attribute__((packed)) {
  uint8_t  id;
  float    pitch;
  float    roll;
  float    vib;
  uint32_t t_ms;
  uint8_t  risk;   // 0 green 1 watch 2 warning 3 critical
} packet_t;

static float pitch_deg = 0, roll_deg = 0;
static float vib_rms = 0;
static float vib_buf[VIB_WINDOW];
static int   vib_i = 0;
static int   vib_n = 0;
static uint8_t risk = 0;
static uint32_t watch_since = 0;
static uint32_t green_since = 0;
static uint32_t last_send = 0;
static uint32_t last_sample = 0;
static uint8_t broadcast[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

static void led_rgb(int r, int g, int b) {
#if USE_RGB
  digitalWrite(PIN_R, r ? HIGH : LOW);
  digitalWrite(PIN_G, g ? HIGH : LOW);
  digitalWrite(PIN_B, b ? HIGH : LOW);
#endif
#if USE_ONBOARD_LED
  digitalWrite(PIN_ONBOARD, (r || g) ? HIGH : LOW);
#endif
}

static void show_risk(uint8_t r) {
  if (r == 0)      led_rgb(0, 1, 0);
  else if (r == 1) led_rgb(1, 1, 0);
  else             led_rgb(1, 0, 0);
}

static bool mpu_write(uint8_t reg, uint8_t val) {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(reg);
  Wire.write(val);
  return Wire.endTransmission() == 0;
}

static bool mpu_begin() {
  delay(50);
  if (!mpu_write(0x6B, 0x00)) return false; // wake
  mpu_write(0x1B, 0x00);                    // gyro ±250 dps
  mpu_write(0x1C, 0x00);                    // accel ±2 g
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
  Wire.read(); Wire.read(); // temp
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
  if (a >= CRIT_DEG)                    next = 3;
  else if (a >= WARN_DEG)               next = 2;
  else if (a >= WATCH_DEG || v >= VIB_G) next = 1;
  else                                  next = 0;

  if (next >= 1) {
    if (watch_since == 0) watch_since = now;
    if (next == 1 && (now - watch_since) >= WATCH_HOLD_MS && v >= VIB_G)
      next = 3;
    green_since = 0;
  } else {
    watch_since = 0;
    if (a > GREEN_BACK_DEG) {
      next = risk >= 1 ? risk : 0; // stay until we are really flat
    } else {
      if (green_since == 0) green_since = now;
      if ((now - green_since) < GREEN_HOLD_MS) next = risk;
    }
  }
  return next;
}

static void emit_json(const packet_t *p) {
  Serial.print("{\"id\":");
  Serial.print(p->id);
  Serial.print(",\"pitch\":");
  Serial.print(p->pitch, 2);
  Serial.print(",\"roll\":");
  Serial.print(p->roll, 2);
  Serial.print(",\"vib\":");
  Serial.print(p->vib, 3);
  Serial.print(",\"t\":");
  Serial.print(p->t_ms);
  Serial.print(",\"risk\":");
  Serial.print(p->risk);
  Serial.println("}");
}

static void send_now(const packet_t *p) {
  esp_now_send(broadcast, (const uint8_t *)p, sizeof(*p));
}

#if IS_USB_HUB
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
  emit_json(&p);
}
#endif

void setup() {
  Serial.begin(115200);
  delay(200);

#if USE_RGB
  pinMode(PIN_R, OUTPUT);
  pinMode(PIN_G, OUTPUT);
  pinMode(PIN_B, OUTPUT);
#endif
#if USE_ONBOARD_LED
  pinMode(PIN_ONBOARD, OUTPUT);
#endif
#if USE_FLEX
  pinMode(PIN_FLEX, INPUT);
#endif

  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(400000);
  if (!mpu_begin()) {
    Serial.println("{\"err\":\"mpu6050\"}");
  }

  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    Serial.println("{\"err\":\"espnow\"}");
  }
  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, broadcast, 6);
  peer.channel = 0;
  peer.encrypt = false;
  esp_now_add_peer(&peer);
#if IS_USB_HUB
  esp_now_register_recv_cb(on_rx);
#endif

  last_sample = micros();
  Serial.println("{\"boot\":1,\"id\":" + String(NODE_ID) + ",\"hub\":" + String(IS_USB_HUB) + "}");
}

void loop() {
  uint32_t now_us = micros();
  uint32_t dt_us = now_us - last_sample;
  if (dt_us < (1000000UL / SAMPLE_HZ)) return;
  last_sample = now_us;
  float dt = dt_us / 1000000.0f;
  if (dt > 0.1f) dt = 0.02f;

  float ax, ay, az, gx, gy, gz;
  if (!mpu_read(&ax, &ay, &az, &gx, &gy, &gz)) return;

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
  show_risk(risk);

  if (now - last_send < SEND_MS) return;
  last_send = now;

  packet_t p;
  p.id = NODE_ID;
  p.pitch = pitch_deg;
  p.roll = roll_deg;
  p.vib = vib_rms;
  p.t_ms = now;
  p.risk = risk;
#if USE_FLEX
  // packed into vib unused high? keep packet stable; print extra on hub only
  if (IS_USB_HUB) {
    int flex = analogRead(PIN_FLEX);
    Serial.print("{\"id\":");
    Serial.print(NODE_ID);
    Serial.print(",\"flex\":");
    Serial.print(flex);
    Serial.println("}");
  }
#endif
  send_now(&p);
#if IS_USB_HUB
  emit_json(&p);
#endif
}
