# Waveshare HA Wall Panel

ESPHome wall panel for Waveshare ESP32-S3-Touch-LCD-4 Rev4.0.

## Struktur

- `components/wall_panel/` – custom ESPHome component
- `packages/hardware.yaml` – hardware
- `packages/lights.yaml` – lys-side
- `packages/blinds.yaml` – persienner
- `packages/audio.yaml` – lyd
- `packages/scenes.yaml` – scener
- `example.yaml` – top-level ESPHome fil

Første trin er kun et rent skelet. Næste trin er at flytte den dynamiske lyslogik over i C++ uden at ændre UI-adfærden.
