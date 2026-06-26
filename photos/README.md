# 📸 Photos

Photos of the physical prototype and the alert screens.

## Final assembly (montagem final)

| Photo | What it shows |
|---|---|
| ![Internal wiring](assembled_internal_topdown.jpg) | Top-down view inside the case: ESP32, MPU6050 and jumpers |
| ![Components close-up](assembled_components_closeup.jpg) | Close-up of the MPU6050 (GY-521), the buzzer and the ESP32 |
| ![Open with lid](assembled_open_with_lid.jpg) | Open case next to its 3D-printed lid, with the emergency button on the side |
| ![Closed top](enclosure_closed_top.jpg) | Closed case (vented lid) |
| ![Closed with strap](enclosure_closed_with_strap.jpg) | Closed case with the wrist strap |
| ![Closed side](enclosure_closed_side.jpg) | Side view of the closed case |
| ![On wrist](bracelet_on_wrist.jpg) | Bracelet worn on the wrist |
| ![On wrist side](bracelet_on_wrist_side.jpg) | Worn on the wrist — the emergency button is reachable on the side |

> The enclosure is a deliberately simple 3D-printed prototype case. It came out
> bulky (it houses a full ESP32 DevKit on a breadboard-style wiring); a smaller
> case is listed as future work. Because it was only a quick prototype, no 3D
> model file (STL) is published.

## App screens (telas do sistema)

### Phone notifications

| Screen | What it shows |
|---|---|
| ![Fall detected](notification_fall_detected.jpg) | Persistent push notification on a fall (high confidence) |
| ![Manual emergency](notification_manual_emergency.jpg) | Persistent push notification when the emergency button is pressed |

### Home Assistant (server side)

| Screen | What it shows |
|---|---|
| ![HA automations](home_assistant_automations.jpg) | The two active automations — `Bracelet — Fall Alert` and `Bracelet — Emergency Button` — with their last-triggered times |
| ![MQTT messages](home_assistant_mqtt.jpg) | The MQTT broker receiving live messages: `high_confidence` on `bracelet/fall` and `manual_button` on `bracelet/emergency` |
