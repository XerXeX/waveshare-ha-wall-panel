# Waveshare HA Wall Panel

ESPHome wall panel for the Waveshare ESP32-S3-Touch-LCD-4 Rev4.0.

The project is designed as a reusable Home Assistant wall panel with runtime-configurable entities, persistent settings and a multilingual on-screen UI.

## Structure

- `components/wall_panel/` – custom ESPHome component and runtime logic
- `components/wall_panel/language/` – auto-discovered translation catalogs
- `packages/hardware.yaml` – display, touch, backlight and board hardware
- `packages/icons.yaml` – shared Material Design Icon image library
- `packages/ui.yaml` – shared fonts, language selection and global UI elements
- `packages/units.yaml` – global display-unit preferences
- `packages/home.yaml` – mixed-entity Home dashboard with domain-aware detail controls
- `packages/home_weather.yaml` – weather detail overlay and 10-hour forecast
- `packages/lights.yaml` – dynamic light controls
- `packages/blinds.yaml` – blinds controls
- `packages/scenes.yaml` – scene controls and Home Assistant icon inheritance
- `packages/icon_registry.yaml` – applies user-registered custom icons to Scene/Script cards
- `packages/screensaver.yaml` – screensaver behavior and shared time source
- `packages/header_clock.yaml` – persistent clock shown on normal pages
- `packages/subdevices.yaml` – Home Assistant sub-device organization
- `ICONS.md` – icon naming convention, included icon list and custom icon registry examples
- `DEVELOPMENT.md` – architecture, data flow, translation/unit rules and editing guide
- `example.yaml` – top-level ESPHome example configuration

## Home dashboard

Home is the first main page and provides six configurable slots. Slots can mix Home Assistant entities such as lights, switches, climate devices, covers, scenes, scripts, weather entities, sensors and binary sensors. Cards show a compact domain-aware state and open a detail page with controls appropriate to the entity type when available.

Home configuration is exposed through its own Home Assistant sub-device. Each slot has a Name and Entity setting, and empty slots are hidden automatically.

Names entered by the user are treated as user data and are never translated. Changing panel language therefore changes UI words and known state labels without rewriting names such as room names, device names or scene names.

## Display units

The Settings sub-device exposes global display-unit preferences:

- Temperature: Auto / °C / °F
- Distance: Auto / km / mi
- Speed: Auto / m/s / km/h / mph
- Pressure: Auto / hPa / inHg

`Auto` keeps the unit supplied by Home Assistant. Unit conversion is display-only: the source value and source unit remain unchanged for Home Assistant actions and service calls.

Weather currently consumes Temperature, Speed and Pressure. Home climate/weather cards also use the Temperature preference. Distance is available as the shared preference for future distance/range cards.

## Icons

Shared images use a predictable ID derived from the MDI name. For example, `mdi:weather-night` is exposed as `icon_weather_night`. Users can also compile their own images and register them with the custom component without editing the packaged Scene mapping. Registered custom icons are also available to Home dashboard cards. See `ICONS.md` for the complete built-in list and configuration example.

## Languages

Source code, configuration entity names, comments and documentation are written in English. User-facing display text comes from translation catalogs in `components/wall_panel/language/`.

English (`en.yaml`) is the master catalog and fallback language. The `Language` configuration entity is generated automatically from the language files present at compile time, and the selected language is stored on the panel.

To add another language, copy `en.yaml`, save it with the ISO language code (for example `de.yaml` or `fi.yaml`), set its `code` and native `name`, and translate the values under `translations:`. No C++ code or hard-coded language list needs to be changed. Missing translations automatically fall back to English.

See `components/wall_panel/language/README.md` for the language-pack format and font/glyph notes, and `DEVELOPMENT.md` for the rules that determine which strings should and should not be translated.
