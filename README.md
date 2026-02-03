# Smart Pot Irrigation System 🌱

Smart pot irrigation system for automated plant watering based on environmental conditions.

## Overview

Smart pot irrigation system combines Arduino sensor acquisition with LabVIEW supervisory control to automatically water plants based on soil moisture, temperature, humidity, and light levels.

## Hardware

- Arduino Uno
- DHT22 (temperature/humidity)
- TSL25911 (light sensor)
- Capacitive soil moisture sensor
- HX711 + 5kg load cell
- Relay module + submersible pump

![[gallery/test_setup.jpg]]

## Quick Start

**Upload Arduino firmware:**
```bash
arduino-cli compile --fqbn arduino:avr:uno hwCode/
arduino-cli upload -p COM3 --fqbn arduino:avr:uno hwCode/
```

**Required Arduino libraries:** DHT, Adafruit_Sensor, Adafruit_TSL2591, HX711

**Run LabVIEW:** Open `irrigationSystem.vi`, configure serial port (9600 baud), and run.

## How It Works

- Arduino reads sensors and sends CSV data every 2 seconds
- LabVIEW analyzes conditions and sends watering commands
- Watering algorithm adjusts thresholds based on temperature, humidity, and light
- Pump runs until target weight (150g) or 30s timeout

See `shouldWater.txt` for complete algorithm logic.

## Configuration

Edit `hwCode.ino`:
- `HX711_CAL_FACTOR`: Calibrate load cell (default -7050.0)
- `WATER_ADD_G`: Target water amount in grams (default 150)

