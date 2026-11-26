# isolation-sphere

A 100 mm contact-juggling ball that is a spherical video display: 800 LEDs on flexible PCBs, IMU world-locked, standalone playback.

![status](https://img.shields.io/badge/status-exhibited%20at%20Maker%20Faire%20Tokyo%202026-green)

isolation-sphere is a 100 mm ball that plays video on its own surface. 800 WS2812 LEDs are mounted on ten flexible "Goldberg" PCB panels that form a sphere. A BNO055 IMU re-maps the image every frame, so the picture stays fixed in space as the ball rolls and spins in your hand.

This repository is the **public release** of the project: board designs, bills of materials, firmware releases and documentation. Development happens in a private repository; releases are published here as tagged snapshots.

## Two editions

| Edition | How it plays | Controller | Use case |
|---|---|---|---|
| **Solo** | Fully standalone. One video stored on the ball loops on power-up; swap it from an iPhone over the ball's own Wi-Fi access point. No server, no router, no internet. | Seeed XIAO ESP32S3 | Single-ball performance, exhibitions, the planned crowdfunding product |
| **Server** | A server streams live video over Wi-Fi to one or more balls for synchronised playback. (The Solo firmware also contains this mode; the original server edition is documented here as well.) | Server PC + in-ball controller | Multi-ball synchronised shows, live/VJ use |

## Key features

- 800 × WS2812C-2020-V6 LEDs on 10 flexible PCB panels, 5 strips of 160, driven in parallel
- World-locked display: BNO055 IMU at 100 Hz, image re-mapped every draw pass (~50–60 Hz)
- Standalone playback (Solo): 320×160 / 10 fps MJPEG from LittleFS, auto-loop on power-on
- Video swap from an iPhone: QR sticker → SoftAP → web UI at http://192.168.4.1/
- Custom power system (power-AB): BQ25792 charger, BQ28Z620 fuel gauge/protector, LTC2954 power controller with magnet on/off at the north pole
- Measured 10 h 9 min continuous playback at 50% brightness, maximum temperature 30.9 °C, automatic power-off working as designed

## Specifications

- Diameter: 100 mm
- Battery: 2S 2000 mAh LiPo × 2 (14.8 Wh)
- Charger input: 5–12 V via magnetic contacts at the south pole
- Boards: Goldberg LED FPC ×10, core-XIAO-04, north-pole-04, mother-ring-04, power-AB (A/B), charge-connector-06
- All boards designed in EasyEDA, fabricated and assembled by JLCPCB

See [hardware/](hardware/) for the board list and design links, and [firmware/](firmware/) for the two editions' firmware.

## Links

- Demo video: https://www.youtube.com/watch?v=RMU48gLjnmo
- OSHWLab: *(link to be added when the projects are published)*
- Exhibited at Maker Faire Tokyo 2026; submitted to the Inno-vation Generation Award 2026

## License

- Hardware designs in this repository are licensed under the **CERN Open Hardware Licence Version 2 – Strongly Reciprocal (CERN-OHL-S v2)**. See [LICENSE](LICENSE).
- Firmware is licensed separately; see the license note inside each firmware folder.

## Author

sastle (Yasuo Katano) — https://sastle.com/web/katano/
