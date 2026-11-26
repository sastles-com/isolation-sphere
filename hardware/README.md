# Hardware

All boards were designed in **EasyEDA** and fabricated / assembled by **JLCPCB**.
The editable design sources are published on OSHWLab (links below); this folder
holds the board list, bills of materials and manufacturing exports for each release.

## Boards

| Board | Role | Design |
|---|---|---|
| Goldberg LED FPC (×10 per ball) | Flexible spherical LED panel: 80 × WS2812C-2020-V6 + 80 capacitors per panel, 800 LEDs per ball | *(OSHWLab link TBD)* |
| core-XIAO-04 | Core board carrying the XIAO ESP32S3 (Solo edition) | *(OSHWLab link TBD)* |
| north-pole-04 | North-pole board: pole LED, hall sensor (DRV5032FC) for magnet power on/off | *(OSHWLab link TBD)* |
| mother-ring-04 | Ring board interconnecting the stack | *(OSHWLab link TBD)* |
| power-AB (board A) | Power control: LTC2954, LED and MCU step-down regulators, always-on rail | *(OSHWLab link TBD)* |
| power-AB (board B) | Charging and gauging: BQ25792 charger, BQ28Z620 fuel gauge / protector | *(OSHWLab link TBD)* |
| charge-connector-06 | South-pole magnetic charging contacts | *(OSHWLab link TBD)* |

## Per release

For each tagged release, add here:

- `bom/` — bill of materials per board (CSV)
- `exports/` — Gerber / manufacturing exports actually used for the production run
- A note of the JLCPCB options used (e.g. FPC coverlay colour), so the run is reproducible

## License

Hardware designs: CERN-OHL-S v2 (see the repository root LICENSE).
