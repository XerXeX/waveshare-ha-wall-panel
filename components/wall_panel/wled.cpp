#include "wall_panel.h"

#include "esphome/core/helpers.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <vector>

namespace esphome {
namespace wall_panel {

namespace {

static const char *const WLED_OPTIONS_SEPARATOR = "\n---WLED-PALETTES---\n";

std::string wled_object_id(const std::string &entity_id) {
  const auto separator = entity_id.find('.');
  if (separator == std::string::npos || separator + 1 >= entity_id.size())
    return {};
  return entity_id.substr(separator + 1);
}

bool parse_wled_float(StringRef value, float &result) {
  const std::string text = value.str();
  char *end = nullptr;
  const float parsed = std::strtof(text.c_str(), &end);
  if (end == text.c_str() || !std::isfinite(parsed))
    return false;
  result = parsed;
  return true;
}

void split_wled_option_bundle(const std::string &bundle, std::string &preset_options, std::string &palette_options) {
  const auto separator = bundle.find(WLED_OPTIONS_SEPARATOR);
  if (separator == std::string::npos) {
    preset_options = bundle;
    palette_options.clear();
    return;
  }
  preset_options = bundle.substr(0, separator);
  palette_options = bundle.substr(separator + std::strlen(WLED_OPTIONS_SEPARATOR));
}

std::vector<std::string> split_wled_lines(const std::string &text) {
  std::vector<std::string> values;
  size_t start = 0;
  while (start <= text.size()) {
    const auto end = text.find('\n', start);
    const std::string value = text.substr(start, end == std::string::npos ? std::string::npos : end - start);
    if (!value.empty())
      values.push_back(value);
    if (end == std::string::npos)
      break;
    start = end + 1;
  }
  return values;
}

bool parse_wled_options(StringRef value, std::string &joined) {
  joined.clear();
  std::string option;
  bool first = true;
  const bool valid = parse_python_list(
      value,
      [&](char c) { option.push_back(c); },
      [&]() {
        if (!first)
          joined.push_back('\n');
        joined += option;
        option.clear();
        first = false;
      });
  if (!valid)
    joined.clear();
  return valid;
}

}  // namespace

void WallPanel::set_active_wled(const std::string &entity_id, const std::string &name) {
  if (entity_id.empty())
    return;

  if (this->active_wled_.entity_id == entity_id) {
    this->active_wled_.name = name;
    this->refresh_ha_subscriptions_();
    return;
  }

  this->active_wled_ = WledState{};
  this->active_wled_.entity_id = entity_id;
  this->active_wled_.name = name;
  this->active_wled_.configured = true;

  const std::string object_id = wled_object_id(entity_id);
  if (object_id.empty())
    return;

  this->active_wled_.preset_entity = "select." + object_id + "_preset";
  this->active_wled_.palette_entity = "select." + object_id + "_color_palette";
  this->active_wled_.playlist_entity = "select." + object_id + "_playlist";
  this->active_wled_.speed_entity = "number." + object_id + "_speed";
  this->active_wled_.intensity_entity = "number." + object_id + "_intensity";

  this->subscribe_active_wled_();
}

void WallPanel::clear_active_wled() { this->active_wled_ = WledState{}; }

void WallPanel::subscribe_active_wled_() {
  const auto &state = this->active_wled_;
  if (!state.configured || state.entity_id.empty())
    return;

  this->subscribe_homeassistant_state(&WallPanel::on_active_wled_state_, state.entity_id);
  this->subscribe_homeassistant_state(&WallPanel::on_active_wled_brightness_, state.entity_id, "brightness");

  // Subscribe to options before state. HA answers subscriptions in order, which
  // means the complete option lists are normally ready before the current value.
  this->subscribe_homeassistant_state(&WallPanel::on_active_wled_select_state_, state.preset_entity, "options");
  this->subscribe_homeassistant_state(&WallPanel::on_active_wled_select_state_, state.palette_entity, "options");
  this->subscribe_homeassistant_state(&WallPanel::on_active_wled_select_state_, state.preset_entity);
  this->subscribe_homeassistant_state(&WallPanel::on_active_wled_select_state_, state.palette_entity);

  this->subscribe_homeassistant_state(&WallPanel::on_active_wled_number_state_, state.speed_entity);
  this->subscribe_homeassistant_state(&WallPanel::on_active_wled_number_state_, state.intensity_entity);
  this->refresh_ha_subscriptions_();
}

void WallPanel::toggle_active_wled() {
  const auto &state = this->active_wled_;
  if (!state.configured || state.entity_id.empty())
    return;
  this->call_homeassistant_service_checked_("homeassistant.toggle", {{"entity_id", state.entity_id}});
}

void WallPanel::set_active_wled_brightness(int brightness_pct) {
  const auto &state = this->active_wled_;
  if (!state.configured || state.entity_id.empty())
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

void WallPanel::active_wled_select_relative(const std::string &control, bool next) {
  const auto &state = this->active_wled_;

  // The existing method is also used by the LVGL dropdowns. Prefixing an exact
  // option keeps the public API backwards compatible while avoiding another YAML
  // facing method solely for direct selection.
  const std::string preset_prefix = "preset_option=";
  const std::string palette_prefix = "palette_option=";
  if (control.rfind(preset_prefix, 0) == 0) {
    const std::string option = control.substr(preset_prefix.size());
    if (!state.preset_entity.empty() && !option.empty())
      this->call_homeassistant_service_checked_(
          "select.select_option", {{"entity_id", state.preset_entity}, {"option", option}});
    return;
  }
  if (control.rfind(palette_prefix, 0) == 0) {
    const std::string option = control.substr(palette_prefix.size());
    if (!state.palette_entity.empty() && !option.empty())
      this->call_homeassistant_service_checked_(
          "select.select_option", {{"entity_id", state.palette_entity}, {"option", option}});
    return;
  }

  std::string preset_options;
  std::string palette_options;
  split_wled_option_bundle(state.playlist, preset_options, palette_options);

  std::string entity_id;
  std::string current;
  std::string option_text;
  if (control == "preset") {
    entity_id = state.preset_entity;
    current = state.preset;
    option_text = preset_options;
  } else if (control == "palette") {
    entity_id = state.palette_entity;
    current = state.palette;
    option_text = palette_options;
  } else {
    return;
  }

  const auto options = split_wled_lines(option_text);
  if (entity_id.empty() || options.empty())
    return;

  auto current_it = std::find(options.begin(), options.end(), current);
  size_t index = 0;
  if (current_it == options.end()) {
    index = next ? 0 : options.size() - 1;
  } else {
    const size_t current_index = static_cast<size_t>(std::distance(options.begin(), current_it));
    index = next ? (current_index + 1) % options.size()
                 : (current_index + options.size() - 1) % options.size();
  }

  this->call_homeassistant_service_checked_(
      "select.select_option", {{"entity_id", entity_id}, {"option", options[index]}});
}

void WallPanel::set_active_wled_number(const std::string &control, int value) {
  const auto &state = this->active_wled_;
  value = std::max(0, std::min(255, value));

  std::string entity_id;
  if (control == "speed")
    entity_id = state.speed_entity;
  else if (control == "intensity")
    entity_id = state.intensity_entity;
  else
    return;

  if (!entity_id.empty())
    this->call_homeassistant_service_checked_("number.set_value",
                                               {{"entity_id", entity_id}, {"value", std::to_string(value)}});
}

void WallPanel::set_active_wled_color(const std::string &color_name) {
  const auto &state = this->active_wled_;
  if (!state.configured || state.entity_id.empty() || color_name.empty())
    return;
  this->call_homeassistant_service_checked_("light.turn_on",
                                             {{"entity_id", state.entity_id}, {"color_name", color_name}});
}

void WallPanel::on_active_wled_state_(const std::string &entity_id, StringRef value) {
  if (this->active_wled_.entity_id != entity_id)
    return;
  const std::string state = value.str();
  this->active_wled_.state = state;
  this->active_wled_.available = state != "unknown" && state != "unavailable";
  this->active_wled_.brightness_pct = state == "on" ? this->active_wled_.reported_brightness_pct : 0;
}

void WallPanel::on_active_wled_brightness_(const std::string &entity_id, StringRef value) {
  if (this->active_wled_.entity_id != entity_id)
    return;

  float raw = 0.0f;
  if (!parse_wled_float(value, raw))
    return;

  int pct = static_cast<int>(std::lround((raw / 255.0f) * 100.0f));
  pct = std::max(0, std::min(100, pct));
  this->active_wled_.has_brightness = true;
  this->active_wled_.reported_brightness_pct = pct;
  this->active_wled_.brightness_pct = this->active_wled_.state == "on" ? pct : 0;
}

void WallPanel::on_active_wled_select_state_(const std::string &entity_id, StringRef value) {
  const std::string text = value.str();
  if (text.empty() || text == "unknown" || text == "unavailable")
    return;

  // The same callback handles both the select state and its `options` attribute.
  // Home Assistant serializes the options attribute as a Python-style list.
  const auto first = text.find_first_not_of(" \t\r\n");
  if (first != std::string::npos && text[first] == '[') {
    std::string joined;
    if (!parse_wled_options(value, joined)) {
      ESP_LOGW("wall_panel", "Could not parse WLED options for %s: %s", entity_id.c_str(), text.c_str());
      return;
    }

    std::string preset_options;
    std::string palette_options;
    split_wled_option_bundle(this->active_wled_.playlist, preset_options, palette_options);
    if (entity_id == this->active_wled_.preset_entity)
      preset_options = joined;
    else if (entity_id == this->active_wled_.palette_entity)
      palette_options = joined;
    else
      return;

    // WledState::playlist is unused by the current UI. Store both complete lists
    // here as a compact transport to YAML without changing the public state struct.
    this->active_wled_.playlist = preset_options + WLED_OPTIONS_SEPARATOR + palette_options;
    return;
  }

  if (entity_id == this->active_wled_.preset_entity)
    this->active_wled_.preset = text;
  else if (entity_id == this->active_wled_.palette_entity)
    this->active_wled_.palette = text;
}

void WallPanel::on_active_wled_number_state_(const std::string &entity_id, StringRef value) {
  float parsed = 0.0f;
  if (!parse_wled_float(value, parsed))
    return;
  const int number = std::max(0, std::min(255, static_cast<int>(std::lround(parsed))));

  if (entity_id == this->active_wled_.speed_entity) {
    this->active_wled_.has_speed = true;
    this->active_wled_.speed = number;
  } else if (entity_id == this->active_wled_.intensity_entity) {
    this->active_wled_.has_intensity = true;
    this->active_wled_.intensity = number;
  }
}

}  // namespace wall_panel
}  // namespace esphome
