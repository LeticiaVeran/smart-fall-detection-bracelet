# System Architecture

## Overview

The system has three layers: the wearable device, the messaging layer, and the
server/notification layer.

```
┌─────────────────────────┐
│  BRACELET (wearable)     │
│  ESP32 + MPU6050         │
│  + buzzer + button       │
│  - 3-phase detection     │
│  - local buzzer alarm    │
└───────────┬─────────────┘
            │ WiFi (MQTT publish)
            ▼
┌─────────────────────────┐
│  MESSAGING LAYER         │
│  Mosquitto MQTT broker   │
│  (on the Raspberry Pi)   │
└───────────┬─────────────┘
            │ MQTT subscribe
            ▼
┌─────────────────────────┐
│  SERVER / NOTIFICATION   │
│  Home Assistant          │
│  - automations           │
│  - persistent push       │
└───────────┬─────────────┘
            │ push notification
            ▼
     Caregiver's phone
```

## Design decisions

- **Detection runs on the device, not the server.** The ESP32 confirms the
  fall locally, so the buzzer fires instantly and an alert is sent even under
  marginal connectivity. The server is only responsible for notifying.
- **Threshold-based, three-phase model** rather than a machine-learning
  classifier — chosen for explainability and to run without a training
  pipeline. See the theoretical background for the rationale.
- **MQTT publish/subscribe** decouples the bracelet from the consumer; the
  server side can change without reflashing the firmware.
- **Credentials in a git-ignored `config.h`** keep secrets out of the repo.
- **Auto-recovery** of the sensor handles the MPU6050 entering sleep after
  current spikes during impacts.

## Data flow per cycle (~50 ms)

1. Keep the MQTT connection alive.
2. Check the emergency button (with debounce).
3. If inside the post-fall lockout window, refresh state and return.
4. Read the sensor (auto-recover on repeated failures).
5. Compute magnitude, gyroscope magnitude, and jerk.
6. Run the three-phase state machine.
7. On confirmation, fire the buzzer and publish the MQTT alert.
