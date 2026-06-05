# 📦 Components

Bill of materials (BOM) and component datasheets for the smart fall-detection
bracelet.

## Bill of Materials (BOM)

| Ref | Component | Part / Model | Qty | Function in the project |
|---|---|---|---|---|
| U1 | Microcontroller | ESP32-WROOM-32 | 1 | Reads the sensor, runs the algorithm, communicates over WiFi/MQTT |
| U2 | IMU sensor | MPU6050 (GY-521 module) | 1 | Tri-axial accelerometer + gyroscope on the wrist |
| BZ1 | Active buzzer | 5V active buzzer | 1 | Local sound alarm |
| SW1 | Push button | 4-pin tactile button (only 2 pins used) | 1 | Manual emergency trigger |
| — | Server | Raspberry Pi 3B+ | 1 | Hosts Home Assistant + Mosquitto broker |
| — | Storage | 32GB SD card | 1 | Home Assistant OS |
| — | Wiring | Female-female jumpers | ~8 | Solderless connections |
| — | Power | Power bank / LiPo battery | 1 | Portable supply for the bracelet |

## Key electrical notes

- **MPU6050** runs at **3.3V** from the ESP32 3.3V rail.
- **Active buzzer**: sounds as soon as it is energized — no PWM signal needed. It
  is powered from **5V (VIN)** and switched on the GND side through GPIO 19 using
  **inverted logic** (LOW = on).
- **Button**: a 4-pin tactile button, but only 2 pins are wired (the other two
  are internally tied to those). It uses the ESP32 **internal pull-up**
  (`INPUT_PULLUP`), so no external resistor is needed.

## datasheets/

Datasheets of the main components, included for offline reference:

- [`MPU-6050_product_specification_rev3.4.pdf`](datasheets/MPU-6050_product_specification_rev3.4.pdf) — InvenSense, Rev. 3.4 (2013). Source for the ±4g sensitivity (8,192 LSB/g), the ±500°/s gyro sensitivity (65.5 LSB/°/s), the I2C address and the register map used in the firmware.
- [`ESP32_technical_reference_manual.pdf`](datasheets/ESP32_technical_reference_manual.pdf) — Espressif. Reference for the ESP32-WROOM-32.
