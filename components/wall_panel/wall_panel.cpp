#include "wall_panel.h"

#include "esphome/components/api/homeassistant_service.h"
#include "esphome/core/log.h"

#include <cmath>
#include <cstdlib>
#include <cstring>

namespace esphome {
namespace wall_panel {

static const char *const TAG = "wall_panel";
static const char *const ACTION_PERMISSION_TIMEOUT = "ha_action_permission_probe";

void WallPanel::setup() {
  ESP_LOGCONFIG(TAG, "Setting up Wall Panel component...");

  this->set_light_defaults_();
  this->light_slot_pref_ = global_preferences->make_preference<LightSlotPreferences>(LIGHT_SLOT_PREF_KEY, true);
  this->light_slot_pref_ready_ = true;
  this->load_light_configuration_();

  for (uint8_t i = 0; i < LIGHT_SLOT_COUNT; i++) {
    if (this->light_slots_[i].configured)
      this->subscribe_light_slot_(i + 1);
  }
}

void WallPanel::set_light_defaults_() {
  for (uint8_t i = 0; i < LIGHT_SLOT_COUNT; i++) {
    auto &slot = this->light_slots_[i];
    slot.entity_id.clear();
    slot.name = "Light " + std::to_string(i + 1);
    slot.function = "Auto";
    slot.configured = false;
    slot.available = false;
    slot.is_on = false;
    slot.has_brightness = false;
    slot.brightness_pct = 0;
    slot.ignore_all_off = false;
  }

  this->light_slots_[3].name = "Sluk alt";
  this->light_slots_[3].function = "Sluk alt";
}

void WallPanel::load_light_configuration_() {
  LightSlotPreferences prefs{};
  if (this->light_slot_pref_.load(&prefs) && prefs.version == 2) {
    for (uint8_t i = 0; i < LIGHT_SLOT_COUNT; i++) {
      prefs.entity_ids[i][64] = '\0';
      prefs.names[i][32] = '\0';
      prefs.functions[i][15] = '\0';

      auto &slot = this->light_slots_[i];
      slot.entity_id = prefs.entity_ids[i];
      slot.name = prefs.names[i];
      slot.function = prefs.functions[i];
      slot.ignore_all_off = prefs.ignore_all_off[i] != 0;
      slot.configured = !slot.entity_id.empty();

      if (slot.name.empty())
        slot.name = (i == 3) ? "Sluk alt" : "Light " + std::to_string(i + 1);
      if (slot.function != "Auto" && slot.function != "Sluk alt" && slot.function != "Skjult")
        slot.function = (i == 3) ? "Sluk alt" : "Auto";
    }
    ESP_LOGI(TAG, "Loaded persisted light slot configuration");
    return;
  }

  // Migrate the v1 storage used by earlier firmware. That format only stored
  // entity IDs, so the remaining fields keep their defaults on first migration.
  auto legacy_pref = global_preferences->make_preference<LightEntityPreferencesV1>(LEGACY_LIGHT_ENTITY_PREF_KEY, true);
  LightEntityPreferencesV1 legacy{};
  if (legacy_pref.load(&legacy) && legacy.version == 1) {
    for (uint8_t i = 0; i < LIGHT_SLOT_COUNT; i++) {
      legacy.entity_ids[i][64] = '\0';
      auto &slot = this->light_slots_[i];
      slot.entity_id = legacy.entity_ids[i];
      slot.configured = !slot.entity_id.empty();
    }
    ESP_LOGI(TAG, "Migrated legacy light entity configuration to full slot storage");
    this->save_light_configuration_();
    return;
  }

  ESP_LOGI(TAG, "No persisted light slot configuration found; using defaults");
  this->save_light_configuration_();
}

void WallPanel::save_light_configuration_() {
  if (!this->light_slot_pref_ready_)
    return;

  LightSlotPreferences prefs{};
  prefs.version = 2;

  for (uint8_t i = 0; i < LIGHT_SLOT_COUNT; i++) {
    const auto &slot = this->light_slots_[i];
    std::strncpy(prefs.entity_ids[i], slot.entity_id.c_str(), 64);
    prefs.entity_ids[i][64] = '\0';
    std::strncpy(prefs.names[i], slot.name.c_str(), 32);
    prefs.names[i][32] = '\0';
    std::strncpy(prefs.functions[i], slot.function.c_str(), 15);
    prefs.functions[i][15] = '\0';
    prefs.ignore_all_off[i] = slot.ignore_all_off ? 1 : 0;
  }

  if (!this->light_slot_pref_.save(&prefs))
    ESP_LOGW(TAG, "Failed to persist light slot configuration");
}

void WallPanel::dump_config() {
  ESP_LOGCONFIG(TAG, "Wall Panel:");
  ESP_LOGCONFIG(TAG, "  Light slots: %u", LIGHT_SLOT_COUNT);

  for (uint8_t i = 0; i < LIGHT_SLOT_COUNT; i++) {
    const auto &slot = this->light_slots_[i];
    ESP_LOGCONFIG(TAG, "  Slot %u: %s | %s | %s | ignore all off=%s", i + 1,
                  slot.name.c_str(),
                  slot.entity_id.empty() ? "<not configured>" : slot.entity_id.c_str(),
                  slot.function.c_str(), YESNO(slot.ignore_all_off));
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

  auto &state = this->light_slots_[slot - 1];
  if (state.name == name)
    return;

  state.name = name;
  this->save_light_configuration_();
}

void WallPanel::set_light_function(uint8_t slot, const std::string &function) {
  if (!this->valid_slot_(slot))
    return;

  if (function != "Auto" && function != "Sluk alt" && function != "Skjult") {
    ESP_LOGW(TAG, "Ignoring invalid function for light slot %u: %s", slot, function.c_str());
    return;
  }

  auto &state = this->light_slots_[slot - 1];
  if (state.function == function)
    return;

  state.function = function;
  this->save_light_configuration_();
}

void WallPanel::set_ignore_all_off(uint8_t slot, bool ignore) {
  if (!this->valid_slot_(slot))
    return;

  auto &state = this->light_slots_[slot - 1];
  if (state.ignore_all_off == ignore)
    return;

  state.ignore_all_off = ignore;
  this->save_light_configuration_();
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

  this->save_light_configuration_();

  if (entity_id.empty()) {
    ESP_LOGI(TAG, "Light slot %u cleared and persisted", slot);
    return;
  }

  this->subscribe_light_slot_(slot);
}

void WallPanel::dismiss_action_permission_warning() {
  this->action_permission_warning_ = false;
  this->action_permission_confirmed_ = false;
  this->action_permission_probe_pending_ = false;
  this->cancel_timeout(ACTION_PERMISSION_TIMEOUT);
}

void WallPanel::call_homeassistant_service_checked_(const std::string &service_name,
                                                    const std::map<std::string, std::string> &data) {
  if (api::global_api_server == nullptr || !api::global_api_server->is_connected_with_state_subscription()) {
    ESP_LOGW(TAG, "Cannot call %s because Home Assistant is not connected", service_name.c_str());
    return;
  }

  // Once Home Assistant has acknowledged one action, ordinary fire-and-forget
  // calls are sufficient for the rest of this boot.
  if (this->action_permission_confirmed_ || this->action_permission_probe_pending_) {
    this->call_homeassistant_service(service_name, data);
    return;
  }

  api::HomeassistantActionRequest request;
  request.service = StringRef(service_name);
  request.data.init(data.size());
  for (const auto &it : data) {
    auto &kv = request.data.emplace_back();
    kv.key = StringRef(it.first);
    kv.value = StringRef(it.second);
  }

  const uint32_t call_id = this->next_action_call_id_++;
  request.call_id = call_id;
  this->action_permission_probe_call_id_ = call_id;
  this->action_permission_probe_pending_ = true;

  api::global_api_server->register_action_response_callback(
      call_id, [this, call_id](const api::ActionResponse &response) {
        if (!this->action_permission_probe_pending_ || this->action_permission_probe_call_id_ != call_id)
          return;

        this->action_permission_probe_pending_ = false;
        this->cancel_timeout(ACTION_PERMISSION_TIMEOUT);

        if (response.is_success()) {
          this->action_permission_confirmed_ = true;
          this->action_permission_warning_ = false;
          ESP_LOGI(TAG, "Home Assistant action permission confirmed");
        } else {
          this->action_permission_warning_ = true;
          ESP_LOGW(TAG, "Home Assistant action failed: %s", response.get_error_message().c_str());
        }
      });

  api::global_api_server->send_homeassistant_action(request);

  // Home Assistant currently rejects disallowed ESPHome actions without sending
  // an action response. A timeout therefore indicates that the device permission
  // is very likely disabled. The UI shows a helpful message and lets the user retry.
  this->set_timeout(ACTION_PERMISSION_TIMEOUT, 2500, [this, call_id]() {
    if (!this->action_permission_probe_pending_ || this->action_permission_probe_call_id_ != call_id)
      return;

    this->action_permission_probe_pending_ = false;
    this->action_permission_warning_ = true;
    ESP_LOGW(TAG, "No Home Assistant action response received; action permission may be disabled");
  });
}

void WallPanel::activate_light_slot(uint8_t slot) {
  if (!this->valid_slot_(slot))
    return;

  const auto &state = this->light_slots_[slot - 1];
  ESP_LOGD(TAG, "Light slot %u pressed: %s (%s)", slot,
           state.entity_id.empty() ? "<no entity>" : state.entity_id.c_str(), state.function.c_str());

  if (state.function == "Sluk alt") {
    this->all_lights_off();
    return;
  }

  if (state.function != "Auto" || !state.configured || state.entity_id.empty())
    return;

  this->call_homeassistant_service_checked_("homeassistant.toggle", {{"entity_id", state.entity_id}});
}

void WallPanel::set_light_brightness(uint8_t slot, int brightness_pct) {
  if (!this->valid_slot_(slot))
    return;

  const auto &state = this->light_slots_[slot - 1];
  if (state.function != "Auto" || !state.configured || state.entity_id.empty() || !state.has_brightness)
    return;

  if (brightness_pct < 0)
    brightness_pct = 0;
  if (brightness_pct > 100)
    brightness_pct = 100;

  ESP_LOGD(TAG, "Light slot %u brightness -> %d%%", slot, brightness_pct);
  this->call_homeassistant_service_checked_(
      "light.turn_on",
      {{"entity_id", state.entity_id},
       {"brightness_pct", std::to_string(brightness_pct)},
       {"transition", "1"}});
}

void WallPanel::turn_entity_off_(const std::string &entity_id) {
  if (entity_id.empty())
    return;

  this->call_homeassistant_service_checked_("homeassistant.turn_off", {{"entity_id", entity_id}});
}

void WallPanel::all_lights_off() {
  ESP_LOGD(TAG, "All lights off pressed");
  for (const auto &state : this->light_slots_) {
    if (state.function != "Auto" || !state.configured || state.entity_id.empty() || state.ignore_all_off)
      continue;

    this->turn_entity_off_(state.entity_id);
  }
}

void WallPanel::subscribe_light_slot_(uint8_t slot) {
  if (!this->valid_slot_(slot))
    return;

  const std::string entity_id = this->light_slots_[slot - 1].entity_id;
  if (entity_id.empty())
    return;

  ESP_LOGI(TAG, "Light slot %u subscribing to %s", slot, entity_id.c_str());

  // Old subscriptions can safely remain registered. The callback checks the
  // current entity ID before applying updates, so changing an entity does not
  // require a reboot or an unsubscribe operation.
  this->subscribe_homeassistant_state(&WallPanel::on_entity_state_, entity_id);
  this->subscribe_homeassistant_state(&WallPanel::on_brightness_state_, entity_id, "brightness");

  // Home Assistant receives the state-subscription list over the native API.
  // If a slot is configured while that connection is already active, refresh
  // the list immediately so state and attributes begin flowing without a reboot.
  if (api::global_api_server != nullptr && api::global_api_server->is_connected()) {
    ESP_LOGD(TAG, "Refreshing Home Assistant state subscriptions");
    for (const auto &client : api::global_api_server->active_clients()) {
      if (client != nullptr && client->is_connection_setup())
        client->on_subscribe_home_assistant_states_request();
    }
  }
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
