# Development guide

This file explains how the project is wired together and, more importantly, where to look when something on the panel needs changing.

## Mental model

The project is split into three layers:

1. **Hardware layer** – display, touch, backlight and buzzer.
2. **Runtime/data layer** – the custom `wall_panel` ESPHome component subscribes to Home Assistant entities and stores their latest values.
3. **UI layer** – the YAML packages render those values with LVGL and send actions back to Home Assistant.

A useful debugging rule is:

```text
Home Assistant entity
        ↓
components/wall_panel/*
        ↓
HomeSlotState / LightSlotState / CoverSlotState
        ↓
packages/*.yaml interval/script
        ↓
LVGL widget
```

If a value is wrong, first ask which step in that chain is wrong instead of changing the widget immediately.

## File map

### `components/wall_panel/__init__.py`

ESPHome/Python code-generation entry point for the custom component.

It currently does two especially important jobs:

- registers compiled MDI images with the C++ component so Home Assistant entity icons can be reused at runtime;
- scans `components/wall_panel/language/*.yaml` at compile time and builds the available language list.

If a feature must be discovered automatically at compile time, this is one of the first files to inspect.

### `components/wall_panel/select.py`

Implements the custom Language select platform.

Every language YAML file is converted into in-firmware lookup tables here. The C++ `WallPanelLanguageSelect` receives:

- language code;
- native language name;
- key/value translation pairs.

### `components/wall_panel/wall_panel.h`

Data structures and public API for the custom component.

Important structures:

- `HomeSlotState`
- `LightSlotState`
- `CoverSlotState`

This file also contains the runtime translation lookup class and icon registry.

### `components/wall_panel/*.cpp`

Runtime behaviour and Home Assistant subscriptions.

The component listens for state and attribute updates, parses them and updates the slot structures. YAML UI files should normally read the prepared slot state instead of creating duplicate Home Assistant subscriptions.

### `components/wall_panel/language/`

Translation catalogs.

`en.yaml` is the master catalog and fallback language. A new language file is automatically added to the Language selector at compile time.

Rule: panel-owned user-facing text belongs here. User-entered names do not.

### `packages/hardware.yaml`

Physical board configuration:

- MIPI RGB display;
- GT911 touch;
- CH32V003 IO expander;
- backlight PWM;
- buzzer/click sound.

If the problem exists before LVGL is involved, start here.

### `packages/icons.yaml`

All compiled MDI image resources.

Convention:

```text
mdi:weather-sunny → icon_weather_sunny
mdi:navigation    → icon_navigation
```

Keeping this naming convention allows the custom component to auto-register icons.

### `packages/ui.yaml`

Shared UI infrastructure:

- fonts;
- Language selector;
- display rotation;
- default page;
- bottom navigation;
- global overlays;
- common page-changing scripts.

This file should contain UI behaviour that belongs to the whole panel rather than one page.

### `packages/units.yaml`

Global display-unit preferences.

Current settings:

- Temperature Unit: Auto / °C / °F
- Distance Unit: Auto / km / mi
- Speed Unit: Auto / m/s / km/h / mph
- Pressure Unit: Auto / hPa / inHg

These selects change **display formatting only**. Home Assistant remains the source of truth and service calls continue to use source values.

`Auto` means: keep the unit received from Home Assistant.

### `packages/home.yaml`

Mixed-domain Home dashboard.

Six slots may point at different HA domains. This package:

- lays out configured cards;
- chooses fallback icons;
- translates known HA states;
- formats temperatures using the global unit preference;
- opens a generic detail page;
- exposes domain-specific controls.

Important rule: `slot.name` is user data and is never translated.

### `packages/home_weather.yaml`

Weather-specific overlay for the Home detail page.

Current conditions come from `HomeSlotState`. Forecast data comes from `weather.get_forecasts`.

The five forecast columns use approximately +2, +4, +6, +8 and +10 hour entries.

This file also applies the Temperature, Speed and Pressure unit preferences.

### `packages/lights.yaml`

Dedicated light page and light-detail controls.

The custom component owns the actual light slot configuration and HA subscriptions. This YAML file renders the cards, brightness slider and "All off" behaviour.

### `packages/blinds.yaml`

Dedicated cover/blinds page.

Supports both covers with a numeric position and simpler open/stop/close covers. Runtime states such as `open`, `closed`, `opening` and `closing` are translated through the language catalog.

### `packages/scenes.yaml`

Scene/script cards.

The scene Name field is user content. If no custom name is supplied, an English configuration fallback may be used; runtime page headings come from the translation system.

### `packages/icon_registry.yaml`

Applies entity icons discovered by the custom component to scene cards.

This is deliberately small: icon discovery belongs in the component, while this package only updates LVGL images.

### `packages/screensaver.yaml`

Clock screensaver:

- digital/analog modes;
- 12/24-hour formatting;
- localized weekdays and months;
- timeout and brightness behaviour.

### `packages/header_clock.yaml`

Small persistent clock shown on normal pages.

### `packages/subdevices.yaml`

Groups configuration entities into logical Home Assistant child devices:

- Home
- Lights
- Blinds
- Scenes
- Settings

This does not create new physical devices. It only keeps the HA device page organized.

## Translation rules

Translate:

- page headings;
- buttons/actions;
- known HA states (`open`, `closed`, `heating`, etc.);
- explanatory UI messages;
- weather conditions;
- weekdays/months.

Do **not** translate:

- text entered into a `... Name` configuration entity;
- entity IDs;
- arbitrary sensor state strings from third-party integrations;
- numeric values;
- units themselves when they are symbols (`°C`, `%`, `m/s`, ...).

Typical runtime lookup:

```cpp
id(wall_panel_language).tr("pressure")
```

When adding new panel-owned text:

1. add the English key to `language/en.yaml`;
2. optionally add translations to other language files;
3. use the key in runtime UI code;
4. if another language omits it, English is shown automatically.

## Unit conversion rules

The panel receives a numeric value plus a source unit from Home Assistant.

When the corresponding unit select is `Auto`, display the source value unchanged.

When a unit override is selected:

```text
source value + source unit
        ↓
normalize to a known base unit
        ↓
convert to selected display unit
        ↓
format label
```

Important: never overwrite the stored source value with the converted display value. For example, a climate entity may report 21 °C while the panel displays 69.8 °F. A service call must still use the source-unit value expected by Home Assistant.

## LVGL update pattern

Most frequently refreshed labels use a small helper like:

```cpp
auto set_label = [](lv_obj_t *label, const std::string &text) {
  const char *current = lv_label_get_text(label);
  if (current == nullptr || text != current)
    lv_label_set_text(label, text.c_str());
};
```

This matters because many intervals run four times per second. Avoid rewriting identical LVGL text on every pass.

The same principle applies to images, positions and hidden flags: compare before changing when practical.

## Home names and defaults

There are two different concepts that look similar:

```yaml
name: "Home · Slot 1 · 1 Name"
initial_value: "Home 1"
```

The first is the Home Assistant **configuration entity name**. The second is the initial value of the user's card name.

Once the user edits the Name entity, that value is user-owned content and must be preserved exactly. Changing language must never rewrite it.

## Where to make common changes

### "I want to change how a Home card looks"

Start in `packages/home.yaml`.

### "The value reaching Home is wrong"

Start in the relevant subscription/parser in `components/wall_panel/*.cpp`.

### "I want another translated word"

Add it to `components/wall_panel/language/en.yaml`, then use `wall_panel_language.tr("key")`.

### "I want another language"

Copy `language/en.yaml`; no C++ language list is required.

### "I want another global unit option"

Add the select to `packages/units.yaml`, attach it to `settings_device` in `packages/subdevices.yaml`, then make the relevant UI formatter consume it.

### "An MDI icon is missing"

Add it to `packages/icons.yaml` using the `icon_<mdi-name>` convention.

## Compile-time vs runtime

A useful distinction:

**Compile-time**

- which language files exist;
- which fonts/glyphs exist;
- which MDI images exist;
- YAML structure and LVGL widgets.

**Runtime**

- selected language;
- selected display units;
- HA states and attributes;
- card names/entity IDs;
- page navigation;
- forecast responses.

Adding a new language file therefore requires a compile, but switching between already compiled languages does not.

## Before committing a UI change

Check these points:

- Is the text panel-owned or user-owned?
- Does it need a translation key?
- Does the source unit need conversion?
- Is a value being converted only for display, not for actions?
- Is the update loop rewriting LVGL unnecessarily?
- Does the change still work with `Auto` unit mode?
- Does an unknown HA state fall back gracefully instead of becoming blank?

That checklist prevents most of the hard-to-find bugs in this project.
