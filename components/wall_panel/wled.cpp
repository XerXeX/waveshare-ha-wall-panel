#include "wall_panel.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace esphome {
namespace wall_panel {

namespace {

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
  this->subscribe_homeassistant_state(&WallPanel::on_active_wled_select_state_, state.preset_entity);
  this->subscribe_homeassistant_state(&WallPanel::on_active_wled_select_state_, state.palette_entity);
  this->subscribe_homeassistant_state(&WallPanel::on_active_wled_select_state_, state.playlist_entity);
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
  std::string entity_id;
  if (control == "preset")
    entity_id = state.preset_entity;
  else if (control == "palette")
    entity_id = state.palette_entity;
  else if (control == "playlist")
    entity_id = state.playlist_entity;
  else
    return;

  if (entity_id.empty())
    return;

  this->call_homeassistant_service_checked_(next ? "select.select_next" : "select.select_previous",
                                             {{"entity_id", entity_id}, {"cycle", "true"}});
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
  const std::string state = value.str();
  if (state.empty() || state == "unknown" || state == "unavailable")
    return;

  if (entity_id == this->active_wled_.preset_entity)
    this->active_wled_.preset = state;
  else if (entity_id == this->active_wled_.palette_entity)
    this->active_wled_.palette = state;
  else if (entity_id == this->active_wled_.playlist_entity)
    this->active_wled_.playlist = state;
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
