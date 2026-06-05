# Setup Checklist & Future Work

Everything in this repository is ready to use. To get the bracelet running, the
only things **you** need to provide are your own private settings:

| # | What you provide | Where |
|---|---|---|
| 1 | Your WiFi + MQTT credentials | `cp firmware/main/config.h.example firmware/main/config.h`, then edit |
| 2 | Your phone's device ID in the alerts | replace `mobile_app_YOUR_PHONE` in `home_assistant/automation_*.yaml` |

Full step-by-step instructions are in the main [`README.md`](README.md) (Quick
start) and in [`home_assistant/README.md`](home_assistant/README.md).

## Future work (optional)

- A more compact 3D-printed enclosure (the current prototype case is bulky) and
  a custom PCB to replace the jumper wiring.
- Battery / power management for longer autonomy.
- Status LED and a periodic `bracelet/status: online` heartbeat so Home
  Assistant can flag a disconnected bracelet.
- Optional vital-signs module (RF01 in the brief — heart rate, oxygen,
  temperature), which the modular design leaves room for.
