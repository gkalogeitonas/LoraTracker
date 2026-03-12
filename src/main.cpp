#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <LoRa.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <TinyGPSPlus.h>

// ---------------------------------------------------------------------------
// Configuration
// ---------------------------------------------------------------------------
static const unsigned long DISPLAY_INTERVAL_MS = 2000;
static const unsigned long TX_INTERVAL_MS      = 60000;
static const long          LORA_FREQ           = 868E6;
static const int           LORA_SF             = 10;
static const uint8_t       LORA_SYNC_WORD      = 0xF3;

// ---------------------------------------------------------------------------
// Pin definitions — TTGO LoRa32 V2.1.6 + LC76G GPS
// ---------------------------------------------------------------------------
// GPS (LC76G)
static const int GPS_RX_PIN  = 34;
static const int GPS_TX_PIN  = 4;
static const int GPS_RST_PIN = 25;
static const int GPS_PPS_PIN = 36;
static const long GPS_BAUD   = 115200;

// LoRa SX1276
static const int PIN_LORA_SCK  = 5;
static const int PIN_LORA_MISO = 19;
static const int PIN_LORA_MOSI = 27;
static const int PIN_LORA_CS   = 18;
static const int PIN_LORA_RST  = 23;
static const int PIN_LORA_DIO0 = 26;

// OLED SSD1306
static const int PIN_OLED_SDA    = 21;
static const int PIN_OLED_SCL    = 22;
static const int SCREEN_W    = 128;
static const int SCREEN_H    = 64;
static const uint8_t OLED_ADDR = 0x3C;

// ---------------------------------------------------------------------------
// Globals
// ---------------------------------------------------------------------------
HardwareSerial gpsSerial(1);
TinyGPSPlus    gps;
Adafruit_SSD1306 display(SCREEN_W, SCREEN_H, &Wire, -1);

unsigned long lastDisplayMs = 0;
unsigned long lastTxMs      = 0;
uint32_t        packetCount   = 0;   // number of LoRa packets sent


// ---------------------------------------------------------------------------
// setup()
// ---------------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  Serial.println(F("[BOOT] Starting..."));

  // Reset GPS module
  pinMode(GPS_RST_PIN, OUTPUT);
  pinMode(GPS_PPS_PIN, INPUT);
  digitalWrite(GPS_RST_PIN, LOW);
  delay(100);
  digitalWrite(GPS_RST_PIN, HIGH);
  delay(500);

  // GPS serial
  gpsSerial.begin(GPS_BAUD, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
  Serial.println(F("[GPS] Serial initialised"));

  // OLED
  Wire.begin(PIN_OLED_SDA, PIN_OLED_SCL);
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println(F("[OLED] Init failed — check wiring"));
  } else {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println(F("Booting..."));
    display.display();
    Serial.println(F("[OLED] Initialised"));
  }

  // LoRa
  SPI.begin(PIN_LORA_SCK, PIN_LORA_MISO, PIN_LORA_MOSI, PIN_LORA_CS);
  LoRa.setPins(PIN_LORA_CS, PIN_LORA_RST, PIN_LORA_DIO0);
  if (!LoRa.begin(LORA_FREQ)) {
    Serial.println(F("[LoRa] Init failed — check wiring"));
  } else {
    LoRa.setSpreadingFactor(LORA_SF);
    LoRa.setSyncWord(LORA_SYNC_WORD);
    Serial.println(F("[LoRa] Initialised"));
  }

  Serial.println(F("[BOOT] Ready"));
}

// ---------------------------------------------------------------------------
// loop()
// ---------------------------------------------------------------------------
void loop() {
  // Feed GPS data continuously
  while (gpsSerial.available()) {
    gps.encode(gpsSerial.read());
  }

  unsigned long now = millis();

  // --- Display task ---
  if (now - lastDisplayMs >= DISPLAY_INTERVAL_MS) {
    lastDisplayMs = now;

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);

    if (gps.location.isValid()) {
      display.print(F("Lat: "));
      display.println(gps.location.lat(), 6);
      display.print(F("Lon: "));
      display.println(gps.location.lng(), 6);
      display.print(F("Alt: "));
      display.print(gps.altitude.meters(), 1);
      display.println(F(" m"));
      display.print(F("Sats: "));
      display.println(gps.satellites.value());
    } else {
      display.println(F("Waiting for Fix..."));
    }
    // packet counter
    display.print(F("Sent: "));
    display.println(packetCount);


    display.display();
  }

  // --- LoRa transmit task ---
  if (now - lastTxMs >= TX_INTERVAL_MS || lastTxMs == 0) {
    lastTxMs = now;

    String payload;
    if (gps.location.isValid()) {
      payload = "LAT:" + String(gps.location.lat(), 6) +
                ",LON:" + String(gps.location.lng(), 6);
    } else {
      payload = "LAT:0.000000,LON:0.000000";
    }

    LoRa.beginPacket();
    LoRa.print(payload);
    LoRa.endPacket();

    packetCount++;

    Serial.print(F("[LoRa] TX: "));
    Serial.print(payload);
    Serial.print(F(" (count="));
    Serial.print(packetCount);
    Serial.println(F(")"));
  }
}