# Requirements

Derived from the official project brief: *Project 6 — Smart Bracelet for
Elderly Monitoring and Safety with ESP32 and Home Assistant*.

## General objective

Develop an ESP32-based smart bracelet for monitoring the elderly, with fall
detection and remote alerts via Home Assistant.

## Specific objectives

- Detect falls automatically with an accelerometer.
- Provide a manual emergency button.
- Send alerts via MQTT to Home Assistant.
- Trigger an immediate sound alarm in risk situations.
- Provide remote monitoring and a history of events.
- *(Optional)* Monitor basic vital signs (heart rate, oxygen, temperature).

## Functional requirements

| ID | Requirement | Status in this build |
|---|---|---|
| RF01 | Vital-signs monitoring (heart rate, oxygen, temperature) | **Optional — not implemented.** Reserved as a future module (see RNF04). |
| RF02 | Automatic fall detection via accelerometer | Implemented — 3-phase algorithm on the MPU6050 |
| RF03 | Manual emergency button | Implemented — GPIO 18 |
| RF04 | Sound alarm in emergencies | Implemented — buzzer on GPIO 19 |
| RF05 | Communication with Home Assistant via MQTT | Implemented — Mosquitto broker on the Raspberry Pi |

## Non-functional requirements

| ID | Requirement | How it is met |
|---|---|---|
| RNF01 | Reliable and safe operation | Sensor auto-recovery; post-alert lockout to avoid duplicates |
| RNF02 | Simple interface | Configuration and monitoring through Home Assistant |
| RNF03 | Low power consumption | Single ESP32 + sensor; battery/power-bank powered |
| RNF04 | Modular system | Architecture allows adding the optional vital-sign sensors later |
| RNF05 | Real-time notifications | Persistent push notifications to the caregiver's phone |

## Constraints

- Microcontroller: ESP32-WROOM-32.
- Sensor: MPU6050 (accelerometer + gyroscope, I2C).
- Server: Raspberry Pi 3B+ running Home Assistant OS with the Mosquitto broker.
- Messaging: MQTT over WiFi.

> Note on scope: vital-signs monitoring (RF01) was optional in the brief and is
> not part of this build, which focuses on reliable fall detection, the
> emergency button, and the alert chain. The modular design (RNF04) leaves room
> to add it later.
