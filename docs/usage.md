# Usage — Solo edition

## Power on / off

- Hold a magnet to the north pole to switch the ball on. The power
  controller (LTC2954) and the hall sensor (DRV5032FC) on the
  north-pole board handle the toggle; there is no mechanical switch.
- The north-pole LED blinks red / green / blue at boot.
- Power off from the gesture menu (below), or let the battery run
  down: automatic power-off operates when the packs are empty.

## Playback

- On power-up, the video stored in LittleFS (320×160 / 10 fps MJPEG)
  loops automatically. No server, router or internet is needed.
- The BNO055 IMU re-maps the image every frame, so the picture stays
  fixed in space while the ball rolls and spins.

## Changing the video

1. Scan the QR sticker on the ball with an iPhone.
2. Join the ball's own Wi-Fi access point.
3. Open the web UI at http://192.168.4.1/ and upload a new video.

Note: an iPhone cannot join a Wi-Fi network from an NFC tap, so the
QR code is the supported route on iPhone.

## Gesture control

- Triple-shake the ball to open the rotation menu.
- Rotate the ball: rotation toward Z− toggles the Wi-Fi access point,
  rotation toward Z+ powers the ball off.

## Charging

- Charge through the magnetic contacts at the south pole
  (charge-connector-06). Input is 5–12 V; the charger is a BQ25792
  and the fuel gauge / protector is a BQ28Z620.
- Charging stops automatically at a fuel-gauge temperature of 45 °C
  and resumes at 40 °C (over-temperature protection setting).

## Server mode

The same firmware also contains a server mode that receives a live
video stream over UDP and falls back to local playback when the
stream is interrupted. See `firmware/server/` for the server edition.
