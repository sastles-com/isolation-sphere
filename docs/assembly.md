# Assembly — overview

This is the assembly overview for one ball. Board details and the
OSHWLab design links are in `hardware/README.md`.

> Status: overview only. Step-by-step photographs and the checks for
> each step are added with the first tagged release.

## Parts of one ball

| Part | Qty | Notes |
|---|---|---|
| Goldberg LED FPC panel | 10 | 80 × WS2812C-2020-V6 + 80 capacitors per panel, 800 LEDs per ball |
| core-XIAO-04 | 1 | Carries the Seeed XIAO ESP32S3 (Solo edition) |
| north-pole-04 | 1 | Pole LED, hall sensor for magnet power on/off |
| mother-ring-04 | 1 | Ring board interconnecting the stack |
| power-AB, board A | 1 | LTC2954 power control, LED and MCU step-down regulators |
| power-AB, board B | 1 | BQ25792 charger, BQ28Z620 fuel gauge / protector |
| charge-connector-06 | 1 | South-pole magnetic charging contacts |
| LiPo pack, 2S 2000 mAh | 2 | 14.8 Wh total |
| BNO055 IMU module | 1 | External module, 100 Hz |

## Order of assembly

1. **Incoming check of each Goldberg panel** — check continuity and
   light every LED on a panel before it goes into the sphere. A dead
   LED found after the sphere is closed is the most expensive defect
   to fix.
2. **Mount and connect the 10 Goldberg panels.** The panel
   connections use headers, so no hand soldering is needed at this
   step.
3. **Stack the core** — batteries, power-AB (A/B), mother-ring,
   core-XIAO and the IMU, in the order shown in the release
   photographs (to be added).
4. **Fit the pole boards** — north-pole board at the north pole,
   charge-connector at the south pole.
5. **Close the sphere.**
6. **Write the firmware** and apply the individual QR sticker
   (the sticker carries the access-point information for video
   upload).

## Final inspection

- Full lighting: all 800 LEDs light, no dead or wrong-colour pixels
- IMU: the image stays fixed in space while the ball is rolled
- Charging: charging starts from the south-pole contacts and the
  over-temperature stop/resume behaves as set (45 °C / 40 °C)
- Gesture: triple-shake opens the menu, shake-off powers down
- Playback: the stored video loops from power-up with no network
