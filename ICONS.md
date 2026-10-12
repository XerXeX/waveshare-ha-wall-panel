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

All shared images are currently rendered at 42x42 using RGB565 with alpha transparency.

## Included icons

### Home, rooms and lighting

`mdi:home`, `mdi:home-outline`, `mdi:bed`, `mdi:sofa`, `mdi:door-open`, `mdi:door-closed`, `mdi:window-open`, `mdi:window-closed`, `mdi:window-shutter`, `mdi:blinds`, `mdi:garage`, `mdi:garage-open`, `mdi:lightbulb`, `mdi:lightbulb-outline`, `mdi:lightbulb-group`, `mdi:lamp`, `mdi:wall-sconce-flat`, `mdi:power`, `mdi:fire`, `mdi:fireplace`.

### Scenes, media and entertainment

`mdi:palette`, `mdi:palette-outline`, `mdi:party-popper`, `mdi:movie`, `mdi:movie-open`, `mdi:television`, `mdi:music`, `mdi:music-note`, `mdi:speaker`, `mdi:speaker-wireless`, `mdi:volume-high`, `mdi:volume-medium`, `mdi:volume-low`, `mdi:volume-off`, `mdi:microphone`, `mdi:microphone-off`, `mdi:gamepad-variant`, `mdi:play`, `mdi:pause`, `mdi:stop`, `mdi:skip-next`, `mdi:skip-previous`, `mdi:star`, `mdi:star-outline`, `mdi:heart`, `mdi:heart-outline`, `mdi:book-open-page-variant`.

### Weather and environment

`mdi:weather-sunny`, `mdi:weather-night`, `mdi:weather-cloudy`, `mdi:weather-rainy`, `mdi:weather-pouring`, `mdi:weather-snowy`, `mdi:weather-lightning`, `mdi:moon-waning-crescent`, `mdi:moon-waxing-crescent`, `mdi:snowflake`, `mdi:tree`, `mdi:flower`, `mdi:leaf`, `mdi:water`.

### Climate and appliances

`mdi:thermometer`, `mdi:thermostat`, `mdi:radiator`, `mdi:fan`, `mdi:air-conditioner`, `mdi:water-boiler`, `mdi:refrigerator`, `mdi:microwave`, `mdi:kettle`, `mdi:stove`, `mdi:dishwasher`, `mdi:washing-machine`, `mdi:vacuum`, `mdi:robot-vacuum`, `mdi:shower`, `mdi:toilet`, `mdi:coffee`, `mdi:silverware-fork-knife`.

### Security and sensors

`mdi:lock`, `mdi:lock-open`, `mdi:security`, `mdi:shield-check`, `mdi:shield-home`, `mdi:camera`, `mdi:cctv`, `mdi:motion-sensor`, `mdi:bell`, `mdi:bell-outline`, `mdi:alert`, `mdi:alert-circle`, `mdi:information`, `mdi:check`, `mdi:check-circle`, `mdi:close`, `mdi:close-circle`.

### Devices and connectivity

`mdi:wifi`, `mdi:wifi-off`, `mdi:bluetooth`, `mdi:battery`, `mdi:battery-charging`, `mdi:flash`, `mdi:laptop`, `mdi:tablet`, `mdi:desktop-tower`, `mdi:cog`, `mdi:wrench`.

### People, pets and transport

`mdi:account`, `mdi:account-group`, `mdi:cat`, `mdi:paw`, `mdi:car`, `mdi:car-electric`, `mdi:map-marker`.

### Controls

`mdi:plus`, `mdi:minus`, `mdi:menu`, `mdi:arrow-up`, `mdi:arrow-down`, `mdi:arrow-left`, `mdi:arrow-right`.

## Scene icon inheritance

Scene and script cards read the `icon` attribute from Home Assistant. If the MDI icon is present in the shared library, the matching image is shown automatically. Unknown icons fall back to `mdi:palette-outline`.

## Adding custom icons

Additional images can be declared in the user's own ESPHome YAML using the same naming convention. This makes them available for direct use by custom widgets. Automatic Scene icon inheritance is limited to the icons mapped by the packaged `scenes.yaml`, so a custom Scene icon also requires extending or overriding that mapping.
