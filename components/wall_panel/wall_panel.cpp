#include "wall_panel.h"

#include "esphome/core/log.h"

#include <cmath>
#include <cstdlib>

namespace esphome {
namespace wall_panel {

static const char *const TAG = "wall_panel";

void WallPanel::setup() {
  ESP_LOGCONFIG(TAG, "Setting up Wall Panel component...");
}

void WallPanel::dump_config() {
  ESP_LOGCONFIG(TAG, "Wall Panel:");
  ESP_LOGCONFIG(TAG, "  Light slots: %u", LIGHT_SLOT_COUNT);
}

bool WallPanel::valid_slot_(uint8_t slot) const {
  return slot >= 1 && slot <= LIGHT_SLOT_COUNT;
}

const LightSlotState &WallPanel::get_light_slot(uint8_t slot) const {
  return this->light_slots_[slot - 1];
}

void WallPanel::set_light_name(uint8_t slot, const std::string &name) {
  if (!this->valid_slot_(slot))
    return;
  this->light_slots_[slot - 1].name = name;
}

void WallPanel::set_ignore_all_off(uint8_t slot, bool ignore) {
  if (!this->valid_slot_(slot))
    return;
  this->light_slots_[slot - 1].ignore_all_off = ignore;
}

void WallPanel::set_light_entity(uint8_t slot, const std::string &entity_id) {
  if (!this->valid_slot_(slot))
    return;

  auto &state = this->light_slots_[slot - 1];
  if (state.entity_id == entity_id)
    return;

  state.entity_id = entity_id;
  state.configured = !entity_id.empty();
  state.available = false;
  state.is_on = false;
  state.has_brightness = false;
  state.brightness_pct = 0;

  if (!entity_id.empty())
    this->subscribe_light_slot_(slot);
}

void WallPanel::subscribe_light_slot_(uint8_t slot) {
  if (!this->valid_slot_(slot))
    return;

  const std::string entity_id = this->light_slots_[slot - 1].entity_id;
  if (entity_id.empty())
    return;

  ESP_LOGI(TAG, "Slot %u subscribing to %s", slot, entity_id.c_str());

  this->subscribe_homeassistant_state(
      entity_id,
      [this, slot, entity_id](std::string value) {
        auto &state = this->light_slots_[slot - 1];
        if (state.entity_id != entity_id)
          return;

        state.available = value != "unavailable" && value != "unknown";
        state.is_on = value == "on";
      });

  this->subscribe_homeassistant_state(
      entity_id, "brightness",
      [this, slot, entity_id](std::string value) {
        auto &state = this->light_slots_[slot - 1];
        if (state.entity_id != entity_id)
          return;

        char *end = nullptr;
        const float raw = std::strtof(value.c_str(), &end);
        if (end == value.c_str() || !std::isfinite(raw))
          return;

        int pct = static_cast<int>(std::lround((raw / 255.0f) * 100.0f));
        if (pct < 0) pct = 0;
        if (pct > 100) pct = 100;

        state.has_brightness = true;
        state.brightness_pct = pct;
      });
}

}  // namespace wall_panel
}  // namespace esphome
