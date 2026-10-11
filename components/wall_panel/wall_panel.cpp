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

  for (uint8_t i = 0; i < LIGHT_SLOT_COUNT; i++) {
    const auto &slot = this->light_slots_[i];
    ESP_LOGCONFIG(TAG, "  Slot %u: %s", i + 1,
                  slot.entity_id.empty() ? "<not configured>" : slot.entity_id.c_str());
  }
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

  if (entity_id.empty()) {
    ESP_LOGI(TAG, "Light slot %u cleared", slot);
    return;
  }

  this->subscribe_light_slot_(slot);
}

void WallPanel::subscribe_light_slot_(uint8_t slot) {
  if (!this->valid_slot_(slot))
    return;

  const std::string entity_id = this->light_slots_[slot - 1].entity_id;
  if (entity_id.empty())
    return;

  ESP_LOGI(TAG, "Light slot %u subscribing to %s", slot, entity_id.c_str());

  // The callback receives the entity ID as well as the state. This lets us
  // safely leave an old subscription registered after a slot is reconfigured:
  // callbacks only update slots that still point at that entity.
  this->subscribe_homeassistant_state(&WallPanel::on_entity_state_, entity_id);
  this->subscribe_homeassistant_state(&WallPanel::on_brightness_state_, entity_id, "brightness");
}

void WallPanel::on_entity_state_(const std::string &entity_id, StringRef value) {
  const std::string state_value = value.str();

  for (auto &slot : this->light_slots_) {
    if (slot.entity_id != entity_id)
      continue;

    slot.available = state_value != "unavailable" && state_value != "unknown";
    slot.is_on = state_value == "on";
  }

  ESP_LOGD(TAG, "%s state=%s", entity_id.c_str(), state_value.c_str());
}

void WallPanel::on_brightness_state_(const std::string &entity_id, StringRef value) {
  const std::string brightness_value = value.str();

  char *end = nullptr;
  const float raw = std::strtof(brightness_value.c_str(), &end);

  // A missing/non-numeric brightness attribute means the entity does not
  // currently expose brightness capability.
  if (end == brightness_value.c_str() || !std::isfinite(raw))
    return;

  int pct = static_cast<int>(std::lround((raw / 255.0f) * 100.0f));
  if (pct < 0)
    pct = 0;
  if (pct > 100)
    pct = 100;

  for (auto &slot : this->light_slots_) {
    if (slot.entity_id != entity_id)
      continue;

    slot.has_brightness = true;
    slot.brightness_pct = pct;
  }

  ESP_LOGD(TAG, "%s brightness=%d%%", entity_id.c_str(), pct);
}

}  // namespace wall_panel
}  // namespace esphome
