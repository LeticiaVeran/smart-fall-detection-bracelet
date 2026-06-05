# ⚙️ Arduino IDE Setup

Step-by-step guide to install and configure the Arduino IDE 2 to compile and flash the bracelet firmware onto the ESP32.

---

## Requirements

| Item | Details |
|---|---|
| Computer | Windows 10/11, macOS 10.14+, or Ubuntu 18.04+ |
| Arduino IDE | Version 2.x (free) |
| ESP32 board package | Espressif Systems — version 2.x |
| PubSubClient library | Nick O'Leary — version 2.8 or higher |
| USB cable | Micro-USB or USB-C (depends on the ESP32 DevKit model) |

---

## 1. Install the Arduino IDE 2

1. Go to [arduino.cc/en/software](https://www.arduino.cc/en/software)
2. Download the installer for your operating system
3. Run the installer and follow the on-screen steps
4. Open the Arduino IDE after installation

---

## 2. Add the ESP32 board package

The ESP32 is not included in the Arduino IDE by default — you need to add the Espressif board registry.

1. Open **File → Preferences** (Windows/Linux) or **Arduino IDE → Preferences** (macOS)
2. In the **"Additional boards manager URLs"** field, paste:
   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
3. Click **OK**
4. Open **Tools → Board → Boards Manager**
5. Search for `esp32`
6. Find **"esp32 by Espressif Systems"** and click **Install**
7. Wait for the download to finish (~200 MB)

---

## 3. Install the PubSubClient library

PubSubClient is the MQTT client used by the firmware to publish fall alerts.

1. Open **Sketch → Include Library → Manage Libraries**
2. In the search box, type `PubSubClient`
3. Find **"PubSubClient by Nick O'Leary"**
4. Click **Install**

> The `Wire.h` and `WiFi.h` libraries are already bundled with the ESP32 package — no separate installation needed.

---

## 4. Select the board and port

1. Connect the ESP32 to the computer via USB
2. Open **Tools → Board → esp32 → ESP32 Dev Module**
3. Open **Tools → Port** and select the port that appeared after connecting the ESP32:
   - **Windows:** something like `COM3`, `COM4`, …
   - **macOS / Linux:** something like `/dev/ttyUSB0` or `/dev/cu.usbserial-…`

> If no port appears, the USB driver may be missing. Install the **CP210x** or **CH340** driver depending on your ESP32 DevKit model.

---

## 5. Configure credentials

The firmware reads WiFi and MQTT credentials from a local `config.h` file that is **not committed to Git** (it is listed in `.gitignore`).

```bash
# Run this command inside the firmware/main/ folder
cp config.h.example config.h
```

Then open `config.h` and fill in your values:

```cpp
#define WIFI_SSID     "YourNetworkName"
#define WIFI_PASSWORD "YourNetworkPassword"
#define MQTT_SERVER   "192.168.x.x"   // Raspberry Pi IP address
#define MQTT_PORT     1883
#define MQTT_USER     "mqtt_user"
#define MQTT_PASSWORD "your_mqtt_password"
```

> Never commit `config.h` to GitHub — it contains your passwords.

---

## 6. Compile and flash

1. Open `firmware/main/main.ino` in the Arduino IDE
2. Click **✓ Verify** to compile (check for errors before flashing)
3. Click **→ Upload** to flash the firmware onto the ESP32
4. Wait for the message `Hard resetting via RTS pin...` — this means the upload succeeded

On power-up you should hear **one short beep**, confirming the buzzer works. The bracelet then warms up for a moment and starts monitoring.

---

## 7. Monitor the serial output (optional)

To see live sensor readings and algorithm state:

1. Open **Tools → Serial Monitor**
2. Set the baud rate to **115200**
3. You should see output like:

```
MPU6050 initialized.
Connecting to WiFi... Connected! IP: 192.168.0.42
Connecting to MQTT... Connected!
System ready for monitoring.
Accel:8210 Gyro:340 Jerk:120 FF:0 ROT:0 IMP:0
```

A full example of the serial output during a detected fall is in
[`firmware/sensors/MPU6050/MPU6050.md`](../sensors/MPU6050/MPU6050.md).

---

## Troubleshooting

| Problem | Solution |
|---|---|
| Port not listed | Install the CP210x or CH340 USB driver for your DevKit |
| Upload error `Failed to connect to ESP32` | Hold the **BOOT** button on the ESP32 while clicking Upload, release after the upload starts |
| `WiFi.h` not found | The ESP32 board package was not installed correctly — repeat step 2 |
| MQTT connection fails (`rc=-2`) | Check that the Raspberry Pi IP in `config.h` is correct and Mosquitto is running |
| No beep on power-up | Check buzzer wiring — GPIO 19 to buzzer −, VIN (5V) to buzzer + |
