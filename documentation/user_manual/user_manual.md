# User Manual

## What the bracelet does

The bracelet watches for falls and lets the wearer call for help. It alerts a
caregiver's phone in two situations:

1. **A fall is detected automatically.**
2. **The wearer presses the emergency button.**

## Powering on

1. Connect the bracelet to its power bank / battery.
2. You will hear **one short beep** — this confirms the device started and the
   buzzer works.
3. Wait a few seconds while the sensor warms up. After warmup the device is
   monitoring silently.

## Buzzer patterns

| Beeps | Meaning |
|---|---|
| 1 beep | Startup / power-on confirmation |
| 3 beeps | Fall confirmed (medium confidence) |
| 5 beeps | Fall confirmed (high confidence) |
| 10 beeps | Emergency button pressed |

## Using the emergency button

Press and hold the button briefly. The bracelet will sound 10 beeps and send an
emergency notification to the caregiver. There is a 3-second cooldown to avoid
accidental repeats.

## What the caregiver sees

A persistent notification arrives on the phone (via the Home Assistant app).
It does not disappear on its own — it stays until the caregiver acknowledges
it, so an alert is never missed.

## Everyday tips

- Keep the bracelet snug on the wrist for accurate readings.
- Recharge the battery regularly.
- If the device keeps re-beeping the startup tone, a cable may be loose — the
  sensor auto-recovers, but check the wiring.
