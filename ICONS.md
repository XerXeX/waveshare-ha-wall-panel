# Shared icon library

The panel ships with a curated set of commonly used Material Design Icons in `packages/icons.yaml`.

## Naming convention

Image IDs are derived from the MDI name:

- replace the `mdi:` prefix with `icon_`
- replace hyphens with underscores

Examples:

- `mdi:weather-night` -> `icon_weather_night`
- `mdi:moon-waning-crescent` -> `icon_moon_waning_crescent`
- `mdi:window-shutter` -> `icon_window_shutter`
- `mdi:robot-vacuum` -> `icon_robot_vacuum`

All shared images are currently rendered at 42x42 using RGB565 with alpha transparency.

## Automatic runtime registration

The `wall_panel` component automatically discovers every compiled image whose ID starts with `icon_` and registers the corresponding Material Design Icon name at code-generation time.

For example, `icon_robot_vacuum` is automatically registered as `mdi:robot-vacuum`.

This makes `packages/icons.yaml` the single source of truth: adding an icon there is enough for Home and Scene cards to inherit the matching `icon` attribute from Home Assistant. No second manual registry list is required.

If Home Assistant supplies an icon that is not compiled into the firmware, the UI uses its normal domain/page fallback icon.

## Included icons

### Home, rooms and lighting

`mdi:home`, `mdi:home-outline`, `mdi:bed`, `mdi:sofa`, `mdi:door-open`, `mdi:door-closed`, `mdi:window-open`, `mdi:window-closed`, `mdi:window-shutter`, `mdi:blinds`, `mdi:blinds-open`, `mdi:garage`, `mdi:garage-open`, `mdi:lightbulb`, `mdi:lightbulb-outline`, `mdi:lightbulb-group`, `mdi:lamp`, `mdi:wall-sconce-flat`, `mdi:power`, `mdi:fire`, `mdi:fireplace`.

### Sleep and night

`mdi:sleep`, `mdi:sleep-off`, `mdi:power-sleep`, `mdi:bell-sleep`, `mdi:bell-sleep-outline`, `mdi:chat-sleep`, `mdi:chat-sleep-outline`, `mdi:bed`, `mdi:bed-outline`, `mdi:bed-empty`, `mdi:bed-clock`, `mdi:bed-single`, `mdi:bed-single-outline`, `mdi:bed-double`, `mdi:bed-double-outline`, `mdi:bed-queen`, `mdi:bed-queen-outline`, `mdi:bed-king`, `mdi:bed-king-outline`, `mdi:bunk-bed`, `mdi:bunk-bed-outline`, `mdi:lightbulb-night`, `mdi:lightbulb-night-outline`, `mdi:weather-night`, `mdi:weather-night-partly-cloudy`, `mdi:moon-new`, `mdi:moon-full`, `mdi:moon-first-quarter`, `mdi:moon-last-quarter`, `mdi:moon-waning-crescent`, `mdi:moon-waning-gibbous`, `mdi:moon-waxing-crescent`, `mdi:moon-waxing-gibbous`, `mdi:shield-moon`, `mdi:shield-moon-outline`, `mdi:weather-moonset`, `mdi:weather-moonset-up`, `mdi:weather-moonset-down`.

### Scenes, media and entertainment

`mdi:palette`, `mdi:palette-outline`, `mdi:party-popper`, `mdi:movie`, `mdi:movie-open`, `mdi:television`, `mdi:music`, `mdi:music-note`, `mdi:speaker`, `mdi:speaker-wireless`, `mdi:volume-high`, `mdi:volume-medium`, `mdi:volume-low`, `mdi:volume-off`, `mdi:microphone`, `mdi:microphone-off`, `mdi:gamepad-variant`, `mdi:play`, `mdi:pause`, `mdi:stop`, `mdi:skip-next`, `mdi:skip-previous`, `mdi:star`, `mdi:star-outline`, `mdi:heart`, `mdi:heart-outline`, `mdi:book-open-page-variant`.

### Weather and environment

`mdi:weather-sunny`, `mdi:weather-night`, `mdi:weather-night-partly-cloudy`, `mdi:weather-cloudy`, `mdi:weather-rainy`, `mdi:weather-pouring`, `mdi:weather-snowy`, `mdi:weather-lightning`, `mdi:weather-moonset`, `mdi:weather-moonset-up`, `mdi:weather-moonset-down`, `mdi:moon-new`, `mdi:moon-full`, `mdi:moon-first-quarter`, `mdi:moon-last-quarter`, `mdi:moon-waning-crescent`, `mdi:moon-waning-gibbous`, `mdi:moon-waxing-crescent`, `mdi:moon-waxing-gibbous`, `mdi:snowflake`, `mdi:tree`, `mdi:flower`, `mdi:leaf`, `mdi:water`.

### Climate, appliances and cleaning

`mdi:thermometer`, `mdi:thermostat`, `mdi:radiator`, `mdi:fan`, `mdi:air-conditioner`, `mdi:water-boiler`, `mdi:fridge`, `mdi:microwave`, `mdi:kettle`, `mdi:stove`, `mdi:dishwasher`, `mdi:washing-machine`, `mdi:vacuum`, `mdi:vacuum-outline`, `mdi:robot-vacuum`, `mdi:robot-vacuum-alert`, `mdi:robot-vacuum-off`, `mdi:robot-vacuum-variant`, `mdi:robot-vacuum-variant-alert`, `mdi:robot-vacuum-variant-off`, `mdi:shower`, `mdi:toilet`, `mdi:coffee`, `mdi:silverware-fork-knife`.

### Security and sensors

`mdi:lock`, `mdi:lock-open`, `mdi:security`, `mdi:shield-check`, `mdi:shield-home`, `mdi:shield-moon`, `mdi:shield-moon-outline`, `mdi:camera`, `mdi:cctv`, `mdi:motion-sensor`, `mdi:bell`, `mdi:bell-outline`, `mdi:bell-sleep`, `mdi:bell-sleep-outline`, `mdi:alert`, `mdi:alert-circle`, `mdi:information`, `mdi:check`, `mdi:check-circle`, `mdi:close`, `mdi:close-circle`.

### Devices and connectivity

`mdi:wifi`, `mdi:wifi-off`, `mdi:bluetooth`, `mdi:battery`, `mdi:battery-charging`, `mdi:flash`, `mdi:laptop`, `mdi:tablet`, `mdi:desktop-tower`, `mdi:cog`, `mdi:wrench`.

### People, pets and transport

`mdi:account`, `mdi:account-group`, `mdi:cat`, `mdi:paw`, `mdi:car`, `mdi:car-electric`, `mdi:map-marker`.

### Controls

`mdi:plus`, `mdi:minus`, `mdi:menu`, `mdi:arrow-up`, `mdi:arrow-down`, `mdi:arrow-left`, `mdi:arrow-right`.

## Home and Scene icon inheritance

Home cards, the Home detail page, and Scene/Script cards read the `icon` attribute supplied by Home Assistant. If the matching MDI image is compiled into the shared library, the runtime registry returns that image automatically.

Unknown icons keep the existing fallback behavior for the relevant domain or page.

## Adding custom icons

For an icon that follows the normal naming convention, simply declare the image with an `icon_...` ID:

```yaml
image:
  - platform: file
    file: mdi:rocket-launch
    id: icon_rocket_launch
    resize: 42x42
    type: RGB565
    transparency: alpha_channel
```

It is automatically available as `mdi:rocket-launch`.

If an image ID cannot follow the naming convention, or you intentionally want to map a different MDI name, use `wall_panel.custom_icons`:

```yaml
wall_panel:
  custom_icons:
    - mdi: mdi:rocket-launch
      image_id: my_rocket_image
```

Explicit custom mappings are applied after the automatic registrations and can therefore override them.
