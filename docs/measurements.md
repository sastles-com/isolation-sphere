# Measurements

Measured data behind the figures published in the README and in the
EasyEDA Spark 2026 entry. Every figure below is stated with its test
conditions; figures without conditions are not published.

## Discharge test — Solo edition

| Item | Value |
|---|---|
| Continuous playback time | 10 h 9 min |
| Brightness | 50% (the normal operating setting) |
| Maximum temperature | 30.9 °C (fuel-gauge temperature) |
| End of test | Automatic power-off operated as designed |
| Battery | 2S 2000 mAh LiPo × 2 (14.8 Wh) |
| Playback | Stored 320×160 / 10 fps MJPEG, looping from LittleFS |

Source: discharge test recorded in the development repository
(`docs/battery_report_2026-11-24.md`, Solo / XIAO ESP32S3 build).

## Not yet measured

- **Full-load operation (white, maximum brightness) has not been
  verified yet.** No full-load runtime, temperature or current figures
  are published for the power-AB boards until that test is done.
- Charging inside the closed sphere has been observed to raise the
  fuel-gauge temperature to 47.4 °C under wired 5 V charging
  (about 7 W). The BQ28Z620 over-temperature charge protection is set
  to stop charging at 45 °C and resume at 40 °C, and that stop/resume
  cycle has been confirmed on the real hardware. This is a protection
  behaviour, not a recommended operating condition.
