# Waveshare HA Wall Panel

ESPHome wall panel for the Waveshare ESP32-S3-Touch-LCD-4 Rev4.0.

The project is designed as a reusable Home Assistant wall panel with runtime-configurable entities, persistent settings and a multilingual on-screen UI.

## Structure

- `components/wall_panel/` – custom ESPHome component and runtime logic
- `packages/hardware.yaml` – display, touch, backlight and board hardware
- `packages/icons.yaml` – shared Material Design Icon image library
- `packages/ui.yaml` – shared fonts, language selection and global UI elements
- `packages/lights.yaml` – dynamic light controls
- `packages/blinds.yaml` – blinds controls
- `packages/audio.yaml` – audio controls
- `packages/scenes.yaml` – scene controls and Home Assistant icon inheritance
- `packages/icon_registry.yaml` – applies user-registered custom icons to Scene/Script cards
- `packages/screensaver.yaml` – screensaver behavior and shared time source
- `packages/header_clock.yaml` – persistent clock shown on normal pages
- `packages/subdevices.yaml` – Home Assistant sub-device organization
- `ICONS.md` – icon naming convention, included icon list and custom icon registry examples
- `example.yaml` – top-level ESPHome example configuration

## Icons

Shared images use a predictable ID derived from the MDI name. For example, `mdi:weather-night` is exposed as `icon_weather_night`. Users can also compile their own images and register them with the custom component without editing the packaged Scene mapping. See `ICONS.md` for the complete built-in list and configuration example.

## Languages

Source code, configuration entity names, comments and documentation are written in English. User-facing display text is provided through the translation layer in the `wall_panel` component.

English is the default language. Danish can be selected from the `Language` configuration entity in Home Assistant. The selected language is stored on the panel and survives reboots.
