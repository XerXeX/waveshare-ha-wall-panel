#include "wall_panel.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>

namespace esphome {
namespace wall_panel {

namespace {

std::string entity_domain(const std::string &entity_id) {
  const auto separator = entity_id.find('.');
  if (separator == std::string::npos)
    return {};
  return entity_id.substr(0, separator);
}

std::string entity_object_id(const std::string &entity_id) {
  const auto separator = entity_id.find('.');
  if (separator == std::string::npos || separator + 1 >= entity_id.size())
    return {};
  return entity_id.substr(separator + 1);
}

bool parse_float(StringRef value, float &result) {
  const std::string text = value.str();
  char *end = nullptr;
  const float parsed = std::strtof(text.c_str(), &end);
  if (end == text.c_str() || !std::isfinite(parsed))
    return false;
  result = parsed;
  return true;
}

void infer_climate_temperature_unit(HomeSlotState &slot, float value) {
  if (slot.domain != "climate" || !slot.unit.empty())
    return;

  // Home Assistant climate entities commonly omit a temperature-unit attribute
  // even though their temperature values are already expressed in HA's native
  // unit system. Typical climate setpoints make a safe fallback discriminator:
  // Celsius values are normally well below 45, while Fahrenheit setpoints are
  // normally above it. A real unit attribute, when supplied later, still wins.
  slot.unit = value >= 45.0f ? "°F" : "°C";
}

}  // namespace

bool WallPanel::valid_home_slot_(uint8_t slot) const { return slot >= 1 && slot <= HOME_SLOT_COUNT; }

const HomeSlotState &WallPanel::get_home_slot(uint8_t slot) const {
  static const HomeSlotState empty;
  if (!this->valid_home_slot_(slot))
    return empty;
  return this->home_slots_[slot - 1];
}

void WallPanel::set_home_name(uint8_t slot, const std::string &name) {
  if (!this->valid_home_slot_(slot))
    return;
  this->home_slots_[slot - 1].name = name;
}

void WallPanel::set_home_entity(uint8_t slot, const std::string &entity_id) {
  if (!this->valid_home_slot_(slot))
    return;

  auto &state = this->home_slots_[slot - 1];
  if (state.entity_id == entity_id)
    return;

  const std::string configured_name = state.name;
  state = HomeSlotState{};
  state.name = configured_name;
  state.entity_id = entity_id;
  state.domain = entity_domain(entity_id);
  state.configured = !entity_id.empty() && !state.domain.empty();

  if (state.configured)
    this->subscribe_home_slot_(slot);
}

void WallPanel::subscribe_home_wled_(uint8_t slot) {
  if (!this->valid_home_slot_(slot))
    return;

  auto &state = this->home_slots_[slot - 1];
  if (state.domain != "light")
    return;

  const std::string object_id = entity_object_id(state.entity_id);
  if (object_id.empty())
    return;

  // WLED's HA integration uses a consistent object-id family for companion
  // entities. We can therefore probe for them without any extra user settings.
  // Normal lights simply never report states for these derived IDs, so they keep
  // the generic light detail page.
  state.wled_preset_entity = "select." + object_id + "_preset";
  state.wled_palette_entity = "select." + object_id + "_color_palette";
  state.wled_playlist_entity = "select." + object_id + "_playlist";
  state.wled_speed_entity = "number." + object_id + "_speed";
  state.wled_intensity_entity = "number." + object_id + "_intensity";

  this->subscribe_homeassistant_state(&WallPanel::on_home_wled_select_state_, state.wled_preset_entity);
  this->subscribe_homeassistant_state(&WallPanel::on_home_wled_select_state_, state.wled_palette_entity);
  this->subscribe_homeassistant_state(&WallPanel::on_home_wled_select_state_, state.wled_playlist_entity);
  this->subscribe_homeassistant_state(&WallPanel::on_home_wled_number_state_, state.wled_speed_entity);
  this->subscribe_homeassistant_state(&WallPanel::on_home_wled_number_state_, state.wled_intensity_entity);
}

void WallPanel::subscribe_home_slot_(uint8_t slot) {
  if (!this->valid_home_slot_(slot))
    return;

  const std::string entity_id = this->home_slots_[slot - 1].entity_id;
  if (entity_id.empty())
    return;

  this->subscribe_homeassistant_state(&WallPanel::on_home_state_, entity_id);
  this->subscribe_homeassistant_state(&WallPanel::on_home_icon_state_, entity_id, "icon");
  this->subscribe_homeassistant_state(&WallPanel::on_home_unit_state_, entity_id, "unit_of_measurement");
  this->subscribe_homeassistant_state(&WallPanel::on_home_temperature_unit_state_, entity_id, "temperature_unit");
  this->subscribe_homeassistant_state(&WallPanel::on_home_brightness_state_, entity_id, "brightness");
  this->subscribe_homeassistant_state(&WallPanel::on_home_supported_color_modes_state_, entity_id,
                                      "supported_color_modes");
  this->subscribe_homeassistant_state(&WallPanel::on_home_position_state_, entity_id, "current_position");
  this->subscribe_homeassistant_state(&WallPanel::on_home_temperature_state_, entity_id, "temperature");
  this->subscribe_homeassistant_state(&WallPanel::on_home_current_temperature_state_, entity_id,
                                      "current_temperature");
  this->subscribe_homeassistant_state(&WallPanel::on_home_humidity_state_, entity_id, "humidity");
  this->subscribe_homeassistant_state(&WallPanel::on_home_hvac_action_state_, entity_id, "hvac_action");
  this->subscribe_homeassistant_state(&WallPanel::on_home_pressure_state_, entity_id, "pressure");
  this->subscribe_homeassistant_state(&WallPanel::on_home_pressure_unit_state_, entity_id, "pressure_unit");
  this->subscribe_homeassistant_state(&WallPanel::on_home_wind_speed_state_, entity_id, "wind_speed");
  this->subscribe_homeassistant_state(&WallPanel::on_home_wind_speed_unit_state_, entity_id, "wind_speed_unit");
  this->subscribe_homeassistant_state(&WallPanel::on_home_wind_bearing_state_, entity_id, "wind_bearing");
  this->subscribe_home_wled_(slot);
  this->refresh_ha_subscriptions_();
}

void WallPanel::toggle_home_entity(uint8_t slot) {
  if (!this->valid_home_slot_(slot))
    return;
  const auto &state = this->home_slots_[slot - 1];
  if (!state.configured || !state.available || state.entity_id.empty())
    return;

  if (state.domain == "light" || state.domain == "switch" || state.domain == "input_boolean" ||
      state.domain == "fan") {
    this->call_homeassistant_service_checked_("homeassistant.toggle", {{"entity_id", state.entity_id}});
  }
}

void WallPanel::activate_home_entity(uint8_t slot) {
  if (!this->valid_home_slot_(slot))
    return;
  const auto &state = this->home_slots_[slot - 1];
  if (!state.configured || !state.available || state.entity_id.empty())
    return;

  if (state.domain == "scene") {
    this->call_homeassistant_service_checked_("scene.turn_on", {{"entity_id", state.entity_id}});
  } else if (state.domain == "script") {
    this->call_homeassistant_service_checked_("script.turn_on", {{"entity_id", state.entity_id}});
  } else if (state.domain == "button") {
    this->call_homeassistant_service_checked_("button.press", {{"entity_id", state.entity_id}});
  } else if (state.domain == "input_button") {
    this->call_homeassistant_service_checked_("input_button.press", {{"entity_id", state.entity_id}});
  } else if (state.domain == "automation") {
    this->call_homeassistant_service_checked_("automation.trigger", {{"entity_id", state.entity_id}});
  }
}

void WallPanel::set_home_light_brightness(uint8_t slot, int brightness_pct) {
  if (!this->valid_home_slot_(slot))
    return;
  const auto &state = this->home_slots_[slot - 1];
  if (!state.configured || !state.available || state.domain != "light" || !state.has_brightness)
    return;

  brightness_pct = std::max(0, std::min(100, brightness_pct));
  if (brightness_pct == 0) {
    this->call_homeassistant_service_checked_("light.turn_off", {{"entity_id", state.entity_id}});
    return;
  }

  this->call_homeassistant_service_checked_(
      "light.turn_on",
      {{"entity_id", state.entity_id}, {"brightness_pct", std::to_string(brightness_pct)}, {"transition", "1"}});
}

void WallPanel::home_wled_select_relative(uint8_t slot, const std::string &control, bool next) {
  if (!this->valid_home_slot_(slot))
    return;
  const auto &state = this->home_slots_[slot - 1];
  if (!state.wled_detected)
    return;

  std::string entity_id;
  if (control == "preset")
    entity_id = state.wled_preset_entity;
  else if (control == "palette")
    entity_id = state.wled_palette_entity;
  else if (control == "playlist")
    entity_id = state.wled_playlist_entity;
  else
    return;

  if (entity_id.empty())
    return;

  this->call_homeassistant_service_checked_(next ? "select.select_next" : "select.select_previous",
                                             {{"entity_id", entity_id}, {"cycle", "true"}});
}

void WallPanel::set_home_wled_number(uint8_t slot, const std::string &control, int value) {
  if (!this->valid_home_slot_(slot))
    return;
  const auto &state = this->home_slots_[slot - 1];
  if (!state.wled_detected)
    return;

  value = std::max(0, std::min(255, value));
  std::string entity_id;
  if (control == "speed")
    entity_id = state.wled_speed_entity;
  else if (control == "intensity")
    entity_id = state.wled_intensity_entity;
  else
    return;

  if (!entity_id.empty())
    this->call_homeassistant_service_checked_("number.set_value",
                                               {{"entity_id", entity_id}, {"value", std::to_string(value)}});
}

void WallPanel::set_home_wled_color(uint8_t slot, const std::string &color_name) {
  if (!this->valid_home_slot_(slot) || color_name.empty())
    return;
  const auto &state = this->home_slots_[slot - 1];
  if (!state.wled_detected || state.entity_id.empty())
    return;

  this->call_homeassistant_service_checked_("light.turn_on",
                                             {{"entity_id", state.entity_id}, {"color_name", color_name}});
}

void WallPanel::open_home_cover(uint8_t slot) {
  if (!this->valid_home_slot_(slot))
    return;
  const auto &state = this->home_slots_[slot - 1];
  if (state.available && state.domain == "cover")
    this->call_homeassistant_service_checked_("cover.open_cover", {{"entity_id", state.entity_id}});
}

void WallPanel::stop_home_cover(uint8_t slot) {
  if (!this->valid_home_slot_(slot))
    return;
  const auto &state = this->home_slots_[slot - 1];
  if (state.available && state.domain == "cover")
    this->call_homeassistant_service_checked_("cover.stop_cover", {{"entity_id", state.entity_id}});
}

void WallPanel::close_home_cover(uint8_t slot) {
  if (!this->valid_home_slot_(slot))
    return;
  const auto &state = this->home_slots_[slot - 1];
  if (state.available && state.domain == "cover")
    this->call_homeassistant_service_checked_("cover.close_cover", {{"entity_id", state.entity_id}});
}

void WallPanel::set_home_cover_position(uint8_t slot, int position_pct) {
  if (!this->valid_home_slot_(slot))
    return;
  const auto &state = this->home_slots_[slot - 1];
  if (!state.available || state.domain != "cover" || !state.has_position)
    return;

  position_pct = std::max(0, std::min(100, position_pct));
  this->call_homeassistant_service_checked_("cover.set_cover_position",
                                             {{"entity_id", state.entity_id},
                                              {"position", std::to_string(position_pct)}});
}

void WallPanel::set_home_climate_temperature(uint8_t slot, float temperature) {
  if (!this->valid_home_slot_(slot))
    return;
  const auto &state = this->home_slots_[slot - 1];
  if (!state.available || state.domain != "climate")
    return;

  char value[16];
  snprintf(value, sizeof(value), "%.1f", temperature);
  this->call_homeassistant_service_checked_("climate.set_temperature",
                                             {{"entity_id", state.entity_id}, {"temperature", value}});
}

void WallPanel::on_home_state_(const std::string &entity_id, StringRef value) {
  const std::string state_value = value.str();
  for (auto &slot : this->home_slots_) {
    if (slot.entity_id != entity_id)
      continue;
    slot.state = state_value;
    slot.available = state_value != "unavailable" && state_value != "unknown";
    if (slot.domain == "light")
      slot.brightness_pct = state_value == "on" ? slot.reported_brightness_pct : 0;
  }
}

void WallPanel::on_home_icon_state_(const std::string &entity_id, StringRef value) {
  const std::string icon = value.str();
  for (auto &slot : this->home_slots_) {
    if (slot.entity_id == entity_id)
      slot.icon = icon;
  }
}

void WallPanel::on_home_unit_state_(const std::string &entity_id, StringRef value) {
  const std::string unit = value.str();
  for (auto &slot : this->home_slots_) {
    if (slot.entity_id == entity_id && !unit.empty())
      slot.unit = unit;
  }
}

void WallPanel::on_home_temperature_unit_state_(const std::string &entity_id, StringRef value) {
  const std::string unit = value.str();
  for (auto &slot : this->home_slots_) {
    if (slot.entity_id == entity_id && !unit.empty())
      slot.unit = unit;
  }
}

void WallPanel::on_home_brightness_state_(const std::string &entity_id, StringRef value) {
  float raw = 0.0f;
  if (!parse_float(value, raw))
    return;

  int pct = static_cast<int>(std::lround((raw / 255.0f) * 100.0f));
  pct = std::max(0, std::min(100, pct));
  for (auto &slot : this->home_slots_) {
    if (slot.entity_id != entity_id)
      continue;
    slot.has_brightness = true;
    slot.reported_brightness_pct = pct;
    slot.brightness_pct = slot.state == "on" ? pct : 0;
  }
}

void WallPanel::on_home_supported_color_modes_state_(const std::string &entity_id, StringRef value) {
  const std::string modes = value.str();
  const bool supports_brightness =
      modes.find("brightness") != std::string::npos || modes.find("color_temp") != std::string::npos ||
      modes.find("hs") != std::string::npos || modes.find("xy") != std::string::npos ||
      modes.find("rgb") != std::string::npos || modes.find("rgbw") != std::string::npos ||
      modes.find("rgbww") != std::string::npos || modes.find("white") != std::string::npos;
  const bool explicitly_onoff = modes.find("onoff") != std::string::npos;
  if (!supports_brightness && !explicitly_onoff)
    return;

  for (auto &slot : this->home_slots_) {
    if (slot.entity_id == entity_id)
      slot.has_brightness = supports_brightness;
  }
}

void WallPanel::on_home_position_state_(const std::string &entity_id, StringRef value) {
  float raw = 0.0f;
  if (!parse_float(value, raw))
    return;

  const int pct = std::max(0, std::min(100, static_cast<int>(std::lround(raw))));
  for (auto &slot : this->home_slots_) {
    if (slot.entity_id == entity_id) {
      slot.has_position = true;
      slot.position_pct = pct;
    }
  }
}

void WallPanel::on_home_temperature_state_(const std::string &entity_id, StringRef value) {
  float parsed = 0.0f;
  if (!parse_float(value, parsed))
    return;

  for (auto &slot : this->home_slots_) {
    if (slot.entity_id != entity_id)
      continue;
    if (slot.domain == "climate") {
      infer_climate_temperature_unit(slot, parsed);
      slot.has_target_temperature = true;
      slot.target_temperature = parsed;
    } else if (slot.domain == "weather") {
      slot.has_current_temperature = true;
      slot.current_temperature = parsed;
    }
  }
}

void WallPanel::on_home_current_temperature_state_(const std::string &entity_id, StringRef value) {
  float parsed = 0.0f;
  if (!parse_float(value, parsed))
    return;

  for (auto &slot : this->home_slots_) {
    if (slot.entity_id == entity_id) {
      infer_climate_temperature_unit(slot, parsed);
      slot.has_current_temperature = true;
      slot.current_temperature = parsed;
    }
  }
}

void WallPanel::on_home_humidity_state_(const std::string &entity_id, StringRef value) {
  float parsed = 0.0f;
  if (!parse_float(value, parsed))
    return;

  for (auto &slot : this->home_slots_) {
    if (slot.entity_id == entity_id) {
      slot.has_humidity = true;
      slot.humidity = parsed;
    }
  }
}

void WallPanel::on_home_hvac_action_state_(const std::string &entity_id, StringRef value) {
  const std::string action = value.str();
  for (auto &slot : this->home_slots_) {
    if (slot.entity_id == entity_id)
      slot.hvac_action = action;
  }
}

void WallPanel::on_home_pressure_state_(const std::string &entity_id, StringRef value) {
  float parsed = 0.0f;
  if (!parse_float(value, parsed))
    return;

  for (auto &slot : this->home_slots_) {
    if (slot.entity_id == entity_id) {
      slot.has_pressure = true;
      slot.pressure = parsed;
    }
  }
}

void WallPanel::on_home_pressure_unit_state_(const std::string &entity_id, StringRef value) {
  const std::string unit = value.str();
  for (auto &slot : this->home_slots_) {
    if (slot.entity_id == entity_id && !unit.empty())
      slot.pressure_unit = unit;
  }
}

void WallPanel::on_home_wind_speed_state_(const std::string &entity_id, StringRef value) {
  float parsed = 0.0f;
  if (!parse_float(value, parsed))
    return;

  for (auto &slot : this->home_slots_) {
    if (slot.entity_id == entity_id) {
      slot.has_wind_speed = true;
      slot.wind_speed = parsed;
    }
  }
}

void WallPanel::on_home_wind_speed_unit_state_(const std::string &entity_id, StringRef value) {
  const std::string unit = value.str();
  for (auto &slot : this->home_slots_) {
    if (slot.entity_id == entity_id && !unit.empty())
      slot.wind_speed_unit = unit;
  }
}

void WallPanel::on_home_wind_bearing_state_(const std::string &entity_id, StringRef value) {
  const std::string bearing = value.str();
  for (auto &slot : this->home_slots_) {
    if (slot.entity_id == entity_id)
      slot.wind_bearing = bearing;
  }
}

void WallPanel::on_home_wled_select_state_(const std::string &entity_id, StringRef value) {
  const std::string state_value = value.str();
  if (state_value.empty() || state_value == "unknown" || state_value == "unavailable")
    return;

  for (auto &slot : this->home_slots_) {
    bool matched = false;
    if (slot.wled_preset_entity == entity_id) {
      slot.wled_preset = state_value;
      matched = true;
    } else if (slot.wled_palette_entity == entity_id) {
      slot.wled_palette = state_value;
      matched = true;
    } else if (slot.wled_playlist_entity == entity_id) {
      slot.wled_playlist = state_value;
      matched = true;
    }
    if (matched)
      slot.wled_detected = true;
  }
}

void WallPanel::on_home_wled_number_state_(const std::string &entity_id, StringRef value) {
  float parsed = 0.0f;
  if (!parse_float(value, parsed))
    return;
  const int number = std::max(0, std::min(255, static_cast<int>(std::lround(parsed))));

  for (auto &slot : this->home_slots_) {
    bool matched = false;
    if (slot.wled_speed_entity == entity_id) {
      slot.has_wled_speed = true;
      slot.wled_speed = number;
      matched = true;
    } else if (slot.wled_intensity_entity == entity_id) {
      slot.has_wled_intensity = true;
      slot.wled_intensity = number;
      matched = true;
    }
    if (matched)
      slot.wled_detected = true;
  }
}

}  // namespace wall_panel
}  // namespace esphome
