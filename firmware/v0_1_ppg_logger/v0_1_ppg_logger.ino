// ==========================================
//  DATA-PPG-LOGGER v0.1 (100 Hz RAW LOGGER)
//  - Device   : ESP32 + MAX30102 (SCL-9 ; SDA-8)
//  - Library  : SparkFun MAX30105 (chính hãng)
//  - Function : Log IR / RED @ ~100 Hz ra Serial
//  - No WiFi, No BLE, No OLED, No Firebase
// ==========================================

#include <Arduino.h>
#include <Wire.h>
#include "MAX30105.h"     // SparkFun MAX30105 library
#include "heartRate.h"    // Chưa dùng ở v0.1, nhưng để sẵn cho v2 (HR)

// =========================
// 1. GLOBAL CONFIG
// =========================

// Serial logging
const uint32_t SERIAL_BAUD = 115200;

// PPG sampling config (theoretical)
const int PPG_SAMPLE_RATE_HZ = 100; // Dùng để ghi log/ghi chú
// Lưu ý: tốc độ thực tế phụ thuộc sensor config bên dưới

// MAX30102 config (đã chỉnh để đạt ~100 Hz thực tế)
byte cfg_ledBrightness = 0x3F; // 0x00 - 0xFF (dòng càng cao càng tốn pin, dễ bão hòa)
byte cfg_sampleAverage = 1;    // 1, 2, 4, 8, 16, 32 (>=4 có thể làm giảm tốc độ hiệu dụng)
byte cfg_ledMode       = 2;    // 1 = Red only, 2 = Red + IR
int  cfg_sampleRate    = 100;  // 50, 100, 200, 400, 800, 1000, 1600, 3200
int  cfg_pulseWidth    = 118;  // 69, 118, 215, 411 (118 phù hợp cho 100 Hz)
int  cfg_adcRange      = 16384; // 2048, 4096, 8192, 16384

// Finger detect threshold (IR)
#define FINGER_ON_THRESHOLD 7000

// =========================
// 2. GLOBAL VARIABLES
// =========================

MAX30105 ppgSensor;

uint32_t startTimeMs = 0; // mốc thời gian bắt đầu logging

// Biến debug: đếm số sample/s
uint32_t sampleCount = 0;
uint32_t lastRatePrintMs = 0;

// =========================
// 3. UTILITY: FLUSH FIFO
// =========================
// Xóa toàn bộ dữ liệu cũ trong FIFO trước khi bắt đầu logging

void flushPPGFIFO() {
  ppgSensor.check();
  while (ppgSensor.available()) {
    ppgSensor.getFIFORed();
    ppgSensor.getFIFOIR();
    ppgSensor.nextSample();
  }
}

// =========================
// 4. INIT PPG SENSOR
// =========================

bool initPPGSensor() {
  // Nếu bạn dùng chân I2C custom của ESP32, bật dòng dưới:
  // Wire.begin(SDA_PIN, SCL_PIN);
  // Nếu dùng default (GPIO 21 SDA, 22 SCL) thì Wire.begin() mặc định là đủ.
  Wire.begin();
  Wire.setClock(400000); // 400kHz I2C

  if (!ppgSensor.begin(Wire, I2C_SPEED_FAST)) {
    Serial.println("ERROR: MAX30102 not found. Check wiring/power.");
    return false;
  }

  // Thiết lập cấu hình theo bảng đã tối ưu cho 100 Hz
  ppgSensor.setup(
    cfg_ledBrightness,
    cfg_sampleAverage,
    cfg_ledMode,
    cfg_sampleRate,
    cfg_pulseWidth,
    cfg_adcRange
  );

  // Bật RED & IR ở mức vừa phải
  ppgSensor.setPulseAmplitudeRed(cfg_ledBrightness);
  ppgSensor.setPulseAmplitudeIR(cfg_ledBrightness);
  // Tắt LED Green nếu có
  ppgSensor.setPulseAmplitudeGreen(0);

  // Xóa FIFO cũ
  flushPPGFIFO();

  Serial.println("MAX30102 initialized with 100 Hz config.");
  Serial.print("Configured sampleRate = ");
  Serial.println(cfg_sampleRate);
  Serial.print("Configured pulseWidth = ");
  Serial.println(cfg_pulseWidth);
  Serial.print("Configured sampleAverage = ");
  Serial.println(cfg_sampleAverage);

  return true;
}

// =========================
// 5. READ ALL AVAILABLE SAMPLES
// =========================
//
// Đọc toàn bộ mẫu đang có trong FIFO.
// Với mỗi mẫu: tính timestamp và log ra Serial.
// Tránh đọc 1 mẫu 1 lần loop -> có thể miss nếu FIFO đầy.

void readAndLogPPGSamples() {
  // Cập nhật buffer nội bộ của lib từ FIFO
  ppgSensor.check();

  // Đọc hết các mẫu hiện có
  while (ppgSensor.available()) {
    long red = ppgSensor.getFIFORed();
    long ir  = ppgSensor.getFIFOIR();

    uint32_t now = millis();
    uint32_t tMs = now - startTimeMs;

    // (Optional) finger check – bạn có thể dùng sau này
    // bool fingerOn = (ir > FINGER_ON_THRESHOLD);

    // Log dưới dạng: timestamp_ms,ir,red
    Serial.print(tMs);
    Serial.print(",");
    Serial.print(ir);
    Serial.print(",");
    Serial.println(red);

    sampleCount++;

    // Chuyển đến sample tiếp theo trong FIFO
    ppgSensor.nextSample();
  }
}

// =========================
// 6. SETUP
// =========================

void setup() {
  Serial.begin(SERIAL_BAUD);
  delay(1000); // chờ ổn định USB

  Serial.println();
  Serial.println("=========================================");
  Serial.println("  DATA-PPG-LOGGER v0.1 (100 Hz RAW PPG)  ");
  Serial.println("  - MAX30102 + SparkFun MAX30105 lib     ");
  Serial.println("  - Output: timestamp_ms,ir,red          ");
  Serial.println("=========================================");

  if (!initPPGSensor()) {
    // Nếu sensor lỗi, dừng tại chỗ
    while (1) {
      Serial.println("PPG init failed. Halting.");
      delay(1000);
    }
  }

  // Ghi header cho file CSV
  Serial.println("timestamp_ms,ir,red");

  startTimeMs = millis();
  lastRatePrintMs = millis();
}

// =========================
// 7. LOOP CHÍNH
// =========================

void loop() {
  // Đọc và log tất cả sample đang có trong FIFO
  readAndLogPPGSamples();

  // Mỗi 1 giây in thử số lượng sample để kiểm tra sample rate thực tế
  uint32_t now = millis();
  if (now - lastRatePrintMs >= 1000) {
    Serial.print("# Samples per second ~ ");
    Serial.println(sampleCount);
    sampleCount = 0;
    lastRatePrintMs = now;
  }

  // Không delay dài, để loop chạy đủ nhanh đọc FIFO.
  // Có thể thêm delay(1) nếu muốn MCU "thở" chút.
  // delay(1);
}
