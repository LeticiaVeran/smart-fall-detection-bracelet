# 💾 Firmware

ESP32 firmware for the smart fall-detection bracelet, written in C++ for the
Arduino IDE 2.

## Structure

```
firmware/
├── main/
│   ├── main.ino            ← the sketch (fall detection + MQTT)
│   ├── config.h.example    ← credentials template (copy to config.h)
│   └── config.h            ← your credentials (you create it; git-ignored)
└── libraries.txt           ← required libraries and versions
```

The sketch lives in a folder named `main` so the Arduino IDE accepts it (the IDE
requires the folder name to match the `.ino` file name). `config.h` sits in the
same folder, so the `#include "config.h"` in the sketch resolves with no extra
setup.

## Build and flash

1. Install the ESP32 board package and the `PubSubClient` library (see
   [`libraries.txt`](libraries.txt)).
2. Create your credentials file (in the same `main/` folder):
   ```bash
   cp main/config.h.example main/config.h
   ```
   and fill in your WiFi + MQTT details. `config.h` is in `.gitignore`, so your
   passwords never reach GitHub.
3. Open `main/main.ino`, select **ESP32 Dev Module** and the correct COM port,
   then click **Upload**.

On power-up the device gives **one short beep** to confirm the buzzer works,
then warms up for a moment before it starts monitoring.

## Code overview

The sketch is organized into small functions:

- `initMPU()` — configures the MPU6050 registers (±4g scale, ~94 Hz filter, wake-up)
- `readSensor()` — reads all 14 bytes in one I2C transaction
- `triggerBuzzer(n)` — sounds the buzzer n times
- `connectWiFi()` / `connectMQTT()` — connectivity with auto-reconnect
- `resetState()` — clears the algorithm state machine
- `loop()` — runs the 3-phase detection ~20×/second

The detection logic and parameter calibration are explained in
[`../documentation/literature/theoretical-background.md`](../documentation/literature/theoretical-background.md).
