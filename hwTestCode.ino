#include <Wire.h>
#include <DHT.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_TSL2591.h>
#include <math.h>

#define DHTPIN    12
#define DHTTYPE   DHT22
#define SOIL_PIN  A0  // PLACEHOLDER: Potentiometer simulating soil moisture sensor (A0)
#define LED_PIN   LED_BUILTIN  // PLACEHOLDER: Onboard LED simulating relay/pump (Pin 13)

inline void ledOn()  { digitalWrite(LED_PIN, HIGH); }
inline void ledOff() { digitalWrite(LED_PIN, LOW); }

DHT dht(DHTPIN, DHTTYPE);
Adafruit_TSL2591 tsl = Adafruit_TSL2591(2591);
bool tslPresent = false;

const unsigned long SENSOR_PERIOD_MS = 2000;
unsigned long lastSensorMs = 0;

// PLACEHOLDER: 5-second delay simulating weight-based watering (replaces load cell)
const unsigned long TEST_WATER_DURATION_MS = 5000;

enum State {
  IDLE = 0,
  WATERING = 1
};

State state = IDLE;
unsigned long wateringStartMs = 0;

String cmdLine;

// Only handle '1' command - autonomous operation, no force-stop
void pollSerialCommands() {
  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\n') {
      cmdLine.trim();
      if (cmdLine == "1") {
        if (state == IDLE) {
          state = WATERING;
          wateringStartMs = millis();
          ledOn();
        }
      }
      // No '0' command - autonomous operation only
      cmdLine = "";
    } else if (c != '\r' && cmdLine.length() < 64) {
      cmdLine += c;
    }
  }
}

float readSoilPercent() {
  int raw = analogRead(SOIL_PIN);
  return (raw * 100.0f) / 1023.0f;
}

float readLuxSafe() {
  if (!tslPresent) return NAN;
  uint32_t lum = tsl.getFullLuminosity();
  uint16_t ir = lum >> 16;
  uint16_t full = lum & 0xFFFF;
  float lux = tsl.calculateLux(full, ir);
  return isfinite(lux) ? lux : NAN;
}

void updateWateringIfNeeded(unsigned long now) {
  if (state != WATERING) return;

  if (now - wateringStartMs >= TEST_WATER_DURATION_MS) {
    ledOff();
    state = IDLE;
  }
}

// LabVIEW expects only 4 fields: temp,hum,lux,soilPct
void printCsvLine(float temp, float hum, float lux, float soilPct) {
  if (isnan(temp)) Serial.print("nan"); else Serial.print(temp, 2);
  Serial.print(",");
  if (isnan(hum)) Serial.print("nan"); else Serial.print(hum, 2);
  Serial.print(",");
  if (isnan(lux)) Serial.print("nan"); else Serial.print(lux, 2);
  Serial.print(",");
  Serial.println(soilPct, 1);  // Last field uses println
}

void setup() {
  Serial.begin(9600);
  Wire.begin();

  pinMode(LED_PIN, OUTPUT);
  ledOff();

  dht.begin();

  tslPresent = tsl.begin();
  if (tslPresent) {
    tsl.setGain(TSL2591_GAIN_MED);
    tsl.setTiming(TSL2591_INTEGRATIONTIME_100MS);
  }
}

void loop() {
  unsigned long now = millis();

  pollSerialCommands();
  updateWateringIfNeeded(now);

  if (now - lastSensorMs >= SENSOR_PERIOD_MS) {
    lastSensorMs = now;

    float soilPct = readSoilPercent();
    float hum = dht.readHumidity();
    float temp = dht.readTemperature();
    float lux = readLuxSafe();

    printCsvLine(temp, hum, lux, soilPct);
  }
}
