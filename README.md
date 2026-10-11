# Waveshare HA Wall Panel

ESPHome wall panel for the Waveshare ESP32-S3-Touch-LCD-4 Rev4.0.

The project is designed as a reusable Home Assistant wall panel with runtime-configurable entities, persistent settings and a multilingual on-screen UI.

## Structure

- `components/wall_panel/` – custom ESPHome component and runtime logic
- `packages/hardware.yaml` – display, touch, backlight and board hardware
- `packages/ui.yaml` – shared fonts, icons, language selection and global UI elements
- `packages/lights.yaml` – dynamic light controls
- `packages/blinds.yaml` – blinds controls
- `packages/audio.yaml` – audio controls
- `packages/scenes.yaml` – scene controls
- `example.yaml` – top-level ESPHome example configuration

## Languages

Source code, configuration entity names, comments and documentation are written in English. User-facing display text is provided through the translation layer in the `wall_panel` component.

English is the default language. Danish can be selected from the `Language` configuration entity in Home Assistant. The selected language is stored on the panel and survives reboots.
