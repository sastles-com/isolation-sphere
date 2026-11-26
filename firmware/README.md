# Firmware

Two editions live side by side. Development happens in a private repository;
this folder receives **release snapshots only**, one subfolder per edition,
each tagged with the repository release.

## Editions

- [solo/](solo/) — Solo edition firmware (XIAO ESP32S3). Standalone playback
  from LittleFS, SoftAP + web UI for video upload, and a built-in server mode
  that receives a live UDP video stream.
- [server/](server/) — Server edition: the in-ball firmware and the PC-side
  streaming server for synchronised multi-ball playback.

## Release policy

- A snapshot is added here when a version is declared a release (same tag as
  the GitHub release).
- Each snapshot folder carries its own `README.md` with: version, target
  hardware revision, build environment, and the license for that code.
