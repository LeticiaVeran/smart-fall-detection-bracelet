# Theoretical Background

> The scientific and engineering literature behind the code.
> This document explains *why* the fall-detection algorithm and the hardware
> choices work the way they do, linking each decision to a published source.
> The full PDFs of the two core papers are in [`references/`](references/);
> the component datasheets are in [`../../components/datasheets/`](../../components/datasheets/).

---

## 1. The problem: falls in the elderly

Falls are a leading cause of serious injury and accidental death in older
adults. The World Health Organization estimates roughly 684,000 fatal falls per
year worldwide (WHO, 2021), and Bourke & Lyons (2008) note that about one in
three adults aged 65+ falls each year.

The clinical danger is not only the impact but the **"long-lie"** — remaining on
the floor unable to get up. Bourke & Lyons (2008) report that this is a common
outcome and is strongly associated with deterioration and death, even when the
fall itself caused no direct injury. A system that detects the fall and alerts a
caregiver directly attacks that delay.

This motivates the two complementary mechanisms in the project:

1. **Automatic detection** via an inertial sensor, for falls where the person
   cannot call for help (the classic weakness of a push-button pendant, which
   Bourke & Lyons note is often not activated during a faint or loss of
   consciousness).
2. **Manual activation** via an emergency button, for situations the person can
   signal themselves.

---

## 2. Inertial sensing with the MPU6050

### 2.1 The sensor

The MPU6050 integrates a tri-axial accelerometer and a tri-axial gyroscope in a
single MEMS chip, each with 16-bit analog-to-digital conversion, and
communicates over I2C (InvenSense, 2013). Al-Dahan et al. (2016) use exactly
this sensor for fall detection, which makes it a well-validated choice for this
project.

### 2.2 Acceleration magnitude

Body orientation during a fall is unpredictable, so the algorithm does not track
individual axes. Following Al-Dahan et al. (2016), it uses the **total
acceleration vector** — the Euclidean norm of the three axes:

```
Acc = √(ax² + ay² + az²)
```

This single scalar is orientation-independent: at rest it equals ~1g (the
gravity vector) regardless of how the wrist is turned. The gyroscope magnitude
is computed the same way from the three angular-rate axes.

### 2.3 Why the ±4g scale

The accelerometer scale is set to ±4g rather than the default ±2g. The
MPU6050 datasheet gives a sensitivity of 16,384 LSB/g at ±2g and 8,192 LSB/g at
±4g (InvenSense, 2013). At ±2g the 16-bit output saturates at 2g — but real fall
impacts reach roughly 2.5g–3.8g, so a ±2g scale would clip exactly at the
impact, hiding the event. ±4g keeps the impact peak inside the measurable range
while preserving enough resolution for the low-acceleration free-fall phase.

### 2.4 Why the digital filter is set to ~94 Hz

The MPU6050 has a programmable internal digital low-pass filter (DLPF)
(InvenSense, 2013). It is configured for a ~94 Hz bandwidth. An impact is a short
event (~50–100 ms), so its energy lies at relatively high frequencies; a more
aggressive filter would smooth the signal and attenuate the impact peak. This
mirrors Bourke & Lyons (2008), who low-pass filtered their gyroscope signal at a
100 Hz cut-off to remove noise without losing the fall transient.

---

## 3. The detection algorithm

The algorithm is **threshold-based**, the approach validated by both core
papers. The central lesson from the literature is that a real fall is not a
single threshold crossing: Bourke & Lyons (2008) show that the peak values of
ordinary Activities of Daily Living (ADL) — sitting down hard, etc. — *overlap*
with fall peaks, so a single threshold cannot separate them. Their solution is
to **cascade several thresholds**; only when all of them are exceeded, in order,
is a fall declared. Al-Dahan et al. (2016) apply the same idea on the MPU6050,
combining a low-acceleration check, a high-acceleration (impact) check and an
angular-velocity check.

This project follows that cascade and adds a rest-confirmation phase, giving
three sequential phases plus a gyroscope bonus.

### Phase 1 — Free fall (with a jerk gate)

During the descent the wrist is briefly in near free fall, so the acceleration
magnitude drops well below 1g. The code flags free fall when the magnitude falls
below ~0.30g **and** the *jerk* (rate of change of acceleration) is high. The
jerk gate is the project's own addition: a low magnitude alone could just mean a
slowly tilted sensor, whereas a sharp transition indicates the abrupt onset of a
fall. This is the equivalent of Al-Dahan's "lower fall threshold" (LFT), made
more robust.

### Phase 2 — Impact

Free fall is followed, within a short time window (≤1 s), by a sharp impact: the
magnitude spikes above ~1.16g as the body hits the ground (Al-Dahan's "upper
fall threshold"). If no impact arrives inside the window, the candidate is
treated as a false positive and the state machine resets. This temporal coupling
between free fall and impact is the heart of the discrimination.

### Phase 3 — Confirmation by rest

After the impact the algorithm waits ~3 s and checks that the body is **still**.
This is the decisive step that separates a real fall from a vigorous gesture: a
real fall ends with the person motionless on the floor, while a sharp gesture
keeps the body moving afterward. This phase is an engineering addition beyond
the cited papers, motivated directly by Bourke & Lyons's finding that ADL are
the main source of false positives.

### The gyroscope confidence bonus

High angular velocity during the free-fall phase indicates body rotation, which
is typical of a genuine fall. Both papers use the gyroscope: it is the only
sensor in Bourke & Lyons (2008) and the final confirmation check in Al-Dahan et
al. (2016). Here it is used as a **confidence bonus** rather than a hard
requirement, so the algorithm reports:

- **High confidence** — all three phases *plus* detected rotation.
- **Medium confidence** — all three phases *without* significant rotation.

Rotation is deliberately optional, because slow elderly falls may involve little
rotation; making it mandatory would risk missed detections (false negatives),
which are far more dangerous here than false positives.

---

## 4. Calibrated parameters

Bourke & Lyons (2008) set each threshold just below the lowest recorded fall
peak; Al-Dahan et al. (2016) likewise tune the upper/lower thresholds against
recorded events. Following that methodology, every threshold here was calibrated
from **real readings captured on the device** (±4g scale, ~8,192 LSB/g, rest
≈ 8,192 LSB):

| Parameter | Value | Justification |
|---|---|---|
| `FREE_FALL_THRESHOLD` | 2,500 LSB | Real free-fall data: 1,691–2,833 LSB |
| `IMPACT_THRESHOLD` | 9,500 LSB | Real impact data: 10,047–15,994 LSB |
| `REST_THRESHOLD` | 9,500 LSB | Must be > 8,192 (1g rest value) |
| `GYRO_THRESHOLD` | 10,000 LSB | ~152°/s on the ±500°/s scale (65.5 LSB/°/s) |
| `JERK_THRESHOLD` | 35,000 LSB/s | Sharp onset of the fall |
| `TIME_WINDOW` | 1,000 ms | Maximum gap from free fall to impact |
| `REST_DURATION` | 3,000 ms | Final confirmation of stillness |

---

## 5. Communication and notification layer

### 5.1 I2C between the ESP32 and the MPU6050

The sensor talks to the ESP32 (Espressif, 2023) over I2C, a two-wire bus (SDA + SCL). All 14 data
bytes — accelerometer, temperature and gyroscope — are read in a single
transaction; the temperature bytes are discarded but **must** be read, otherwise
the gyroscope registers stay misaligned (a direct consequence of the MPU6050
register layout in InvenSense, 2013).

### 5.2 MQTT and Home Assistant

Alerts are published with MQTT (Banks & Gupta, 2014), a lightweight
publish/subscribe protocol for constrained devices. The ESP32 publishes to a
Mosquitto broker; Home Assistant (Nabu Casa, 2023) subscribes and delivers a
**persistent push notification** to the caregiver's phone — appropriate for a
safety alert, since it does not silently disappear before being acknowledged.

---

## References

- AL-DAHAN, Z. T.; BACHACHE, N. K.; BACHACHE, L. N. Design and Implementation of Fall Detection System Using MPU6050 Arduino. In: *Inclusive Smart Cities and Digital Health* (ICOST 2016). Lecture Notes in Computer Science, v. 9677. Springer, Cham, 2016. DOI: 10.1007/978-3-319-39601-9_16.
- BANKS, A.; GUPTA, R. *MQTT Version 3.1.1*. OASIS Standard, 2014.
- BOURKE, A. K.; LYONS, G. M. A threshold-based fall-detection algorithm using a bi-axial gyroscope sensor. *Medical Engineering & Physics*, v. 30, n. 1, p. 84–90, 2008. DOI: 10.1016/j.medengphy.2006.12.001.
- ESPRESSIF SYSTEMS. *ESP32 Technical Reference Manual*. Espressif Systems, 2023.
- HOME ASSISTANT. *Home Assistant Documentation*. Nabu Casa, 2023.
- INVENSENSE. *MPU-6000/MPU-6050 Product Specification, Revision 3.4*. InvenSense, 2013.
- WORLD HEALTH ORGANIZATION. *Falls*. WHO, 2021.
