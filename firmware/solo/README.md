# Solo edition firmware

Target: Seeed XIAO ESP32S3 on the core-XIAO-04 board.

Behaviour on power-up: the ball starts its own Wi-Fi access point, loads the
MJPEG video stored in LittleFS (320×160, 10 fps) and loops it with IMU
world-locking. Scan the QR sticker on the ball and open
http://192.168.4.1/ to upload a different video. The same binary also
implements the server mode (live UDP stream) with automatic fallback to
local playback.

Release snapshots will be placed here. Each snapshot gets its own folder:

```
solo/
  vX.Y.Z/
    README.md      <- version, hardware revision, build notes, license
    src/           <- source snapshot
    bin/           <- prebuilt binary (if provided)
```

License: *(to be fixed at first release — the hardware CERN-OHL-S v2 does not
automatically cover this code.)*
