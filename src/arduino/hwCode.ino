#include <Wire.h>
#include <DHT.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_TSL2591.h>
#include "HX711.h"
#include <math.h>

#define DHTPIN    12
#define DHTTYPE   DHT22
#define SOIL_PIN  A0
#define RELAY_PIN 8
#define HX_DOUT   2
#define HX_SCK    3

// Set true if relay module is active-low
const bool RELAY_ACTIVE_LOW = false;

inline void pumpOn()  { digitalWrite(RELAY_PIN, RELAY_ACTIVE_LOW ? LOW  : HIGH); }
inline void pumpOff() { digitalWrite(RELAY_PIN, RELAY_ACTIVE_LOW ? HIGH : LOW ); }

DHT dht(DHTPIN, DHTTYPE);
Adafruit_TSL2591 tsl = Adafruit_TSL2591(2591);
bool tslPresent = false;
HX711 scale;

const unsigned long SENSOR_PERIOD_MS = 2000;
const unsigned long WEIGHT_PERIOD_MS = 200;
unsigned long lastSensorMs = 0;
unsigned long lastWeightMs = 0;

// Load cell calibration factor (adjust for your setup)
const float HX711_CAL_FACTOR = -7050.0f;
// Target water amount per watering event
const float WATER_ADD_G = 150.0f;
// Safety timeout to prevent runaway watering
const unsigned long MAX_WATER_MS = 30000;

float weightG = NAN;
float startWeightG = NAN;
float targetWeightG = NAN;

enum State {
  IDLE = 0,
  WATERING = 1
};

State state = IDLE;
unsigned long wateringStartMs = 0;

String cmdLine;

// Command '1' starts autonomous watering cycle
// Arduino completes cycle based on weight target or timeout
void pollSerialCommands() {
  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\n') {
      cmdLine.trim();
      if (cmdLine == "1") {
        if (state == IDLE) {
          state = WATERING;
          wateringStartMs = millis();
          startWeightG = scale.get_units(10);
          targetWeightG = startWeightG + WATER_ADD_G;
          pumpOn();
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

void updateWeightIfNeeded(unsigned long now) {
  if (state != WATERING) return;
  if (now - lastWeightMs < WEIGHT_PERIOD_MS) return;
  lastWeightMs = now;

  weightG = scale.get_units(5);

  if (isfinite(weightG) && isfinite(targetWeightG) && weightG >= targetWeightG) {
    pumpOff();
    state = IDLE;
    return;
  }

  if (now - wateringStartMs > MAX_WATER_MS) {
    pumpOff();
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

  pinMode(RELAY_PIN, OUTPUT);
  pumpOff();

  dht.begin();

  tslPresent = tsl.begin();
  if (tslPresent) {
    tsl.setGain(TSL2591_GAIN_MED);
    tsl.setTiming(TSL2591_INTEGRATIONTIME_100MS);
  }

  scale.begin(HX_DOUT, HX_SCK);
  scale.set_scale(HX711_CAL_FACTOR);
}

void loop() {
  unsigned long now = millis();

  pollSerialCommands();
  updateWeightIfNeeded(now);

  if (now - lastSensorMs >= SENSOR_PERIOD_MS) {
    lastSensorMs = now;

    float soilPct = readSoilPercent();
    float hum = dht.readHumidity();
    float temp = dht.readTemperature();
    float lux = readLuxSafe();

    printCsvLine(temp, hum, lux, soilPct);
  }
}
