# 🎨 Design

Electrical design of the bracelet.

## Wiring schematic

The full wiring diagram is in
[`schematics/wiring_diagram.png`](schematics/wiring_diagram.png).

```
MPU6050 (GY-521)         ESP32
VCC  ─────────────────── 3.3V
GND  ─────────────────── GND
SCL  ─────────────────── GPIO 22
SDA  ─────────────────── GPIO 21

Active buzzer            ESP32
+ (positive) ──────────── VIN (5V)
− (negative) ──────────── GPIO 19   ← inverted logic: LOW = on

Push button              ESP32
Pin 1 ──────────────────  GPIO 18   ← INPUT_PULLUP
Pin 2 ──────────────────  GND
Pins 3 and 4 ───────────  not used
```

## Connection summary

| From | Signal | To (ESP32) | Notes |
|---|---|---|---|
| MPU6050 VCC | Power | 3.3V | — |
| MPU6050 GND | Ground | GND | — |
| MPU6050 SCL | I2C clock | GPIO 22 | — |
| MPU6050 SDA | I2C data | GPIO 21 | — |
| Buzzer + | Power | VIN (5V) | Maximum volume |
| Buzzer − | Switch | GPIO 19 | LOW = on (inverted) |
| Button pin 1 | Input | GPIO 18 | Internal pull-up |
| Button pin 2 | Ground | GND | — |

> The bracelet is built on a solderless (jumper) connection — no PCB. The
> `schematics/` folder holds the wiring diagram.

## Connection & data flow diagram

The communication diagram (device → server → caregiver), required as part of the
schematic deliverables, is in
[`diagrams/connection_flow.svg`](diagrams/connection_flow.svg) (and a rendered
[`connection_flow.png`](diagrams/connection_flow.png)):

![Connection and data flow](diagrams/connection_flow.png)

## Enclosure (3D-printed prototype)

The case is a simple 3D-printed prototype: a two-part box (vented lid + base)
sized to hold a full ESP32 DevKit with the sensor and buzzer on jumper wires.
Because it was only a quick prototype, it came out **bulky** for a wrist device
and **no 3D model (STL) is published** — a slimmer enclosure (and a custom PCB
to replace the jumpers) is listed as future work. See the photos in
[`../photos/`](../photos/README.md).
