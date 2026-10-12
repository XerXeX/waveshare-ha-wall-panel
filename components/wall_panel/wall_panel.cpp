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

  this->language_pref_ = global_preferences->make_preference<LanguagePreferences>(LANGUAGE_PREF_KEY, true);
  this->language_pref_ready_ = true;
  this->load_language_();

  this->display_pref_ = global_preferences->make_preference<DisplayPreferences>(DISPLAY_PREF_KEY, true);
  this->display_pref_ready_ = true;
  this->load_display_rotation_();

  this->set_light_defaults_();
  this->light_slot_pref_ = global_preferences->make_preference<LightSlotPreferences>(LIGHT_SLOT_PREF_KEY, true);
  this->light_slot_pref_ready_ = true;
  this->load_light_configuration_();

  this->set_cover_defaults_();
  this->cover_slot_pref_ = global_preferences->make_preference<CoverSlotPreferences>(COVER_SLOT_PREF_KEY, true);
  this->cover_slot_pref_ready_ = true;
  this->load_cover_configuration_();

  for (uint8_t i = 0; i < LIGHT_SLOT_COUNT; i++) {
    if (this->light_slots_[i].configured)
      this->subscribe_light_slot_(i + 1);
  }
  for (uint8_t i = 0; i < COVER_SLOT_COUNT; i++) {
    if (this->cover_slots_[i].configured)
      this->subscribe_cover_slot_(i + 1);
  }
}

void WallPanel::load_language_() {
  LanguagePreferences prefs{};
  if (this->language_pref_.load(&prefs) && prefs.version == 1 &&
      prefs.language <= static_cast<uint8_t>(Language::DANISH)) {
    this->language_ = static_cast<Language>(prefs.language);
    return;
  }
  this->language_ = Language::ENGLISH;
  this->save_language_();
}

void WallPanel::save_language_() {
  if (!this->language_pref_ready_)
    return;
  LanguagePreferences prefs{};
  prefs.version = 1;
  prefs.language = static_cast<uint8_t>(this->language_);
  if (!this->language_pref_.save(&prefs))
    ESP_LOGW(TAG, "Failed to persist language configuration");
}

void WallPanel::set_language(const std::string &language) {
  Language next;
  if (language == "English")
    next = Language::ENGLISH;
  else if (language == "Danish")
    next = Language::DANISH;
  else
    return;

  if (this->language_ == next)
    return;
  this->language_ = next;
  this->save_language_();
}

const char *WallPanel::get_language_option() const {
  return this->language_ == Language::DANISH ? "Danish" : "English";
}

void WallPanel::load_display_rotation_() {
  DisplayPreferences prefs{};
  if (this->display_pref_.load(&prefs) && prefs.version == 1 &&
      (prefs.rotation == 0 || prefs.rotation == 90 || prefs.rotation == 180 || prefs.rotation == 270)) {
    this->display_rotation_ = prefs.rotation;
    return;
  }
  this->display_rotation_ = 0;
  this->save_display_rotation_();
}

void WallPanel::save_display_rotation_() {
  if (!this->display_pref_ready_)
    return;
  DisplayPreferences prefs{};
  prefs.version = 1;
  prefs.rotation = static_cast<uint16_t>(this->display_rotation_);
  if (!this->display_pref_.save(&prefs))
    ESP_LOGW(TAG, "Failed to persist display rotation");
}

void WallPanel::set_display_rotation(int rotation) {
  rotation %= 360;
  if (rotation < 0)
    rotation += 360;
  if (rotation != 0 && rotation != 90 && rotation != 180 && rotation != 270)
    return;
  if (this->display_rotation_ == rotation)
    return;
  this->display_rotation_ = rotation;
  this->save_display_rotation_();
}

const char *WallPanel::get_display_rotation_option() const {
  switch (this->display_rotation_) {
    case 90:
      return "90°";
    case 180:
      return "180°";
    case 270:
      return "270°";
    default:
      return "0°";
  }
}

const char *WallPanel::tr(TranslationKey key) const {
  const bool da = this->language_ == Language::DANISH;
  switch (key) {
    case TranslationKey::LIGHTS:
      return da ? "Lys" : "Lights";
    case TranslationKey::LIGHT:
      return da ? "Lys" : "Light";
    case TranslationKey::BLINDS:
      return da ? "Persienner" : "Blinds";
    case TranslationKey::AUDIO:
      return da ? "Lyd" : "Audio";
    case TranslationKey::SCENES:
      return da ? "Scener" : "Scenes";
    case TranslationKey::ALL_OFF:
      return da ? "Sluk alt" : "All off";
    case TranslationKey::ALL_LIGHTS:
      return da ? "Alle lys" : "All lights";
    case TranslationKey::UNAVAILABLE:
      return da ? "Ikke tilgængelig" : "Unavailable";
    case TranslationKey::ON:
      return da ? "Tændt" : "On";
    case TranslationKey::OFF:
      return da ? "Slukket" : "Off";
    case TranslationKey::ACTION_PERMISSION_MESSAGE:
      return da
                 ? "Panelet kan ikke udføre handlinger i Home Assistant.\n\nAktivér \"Allow device to perform Home Assistant actions\" i ESPHome-enhedens indstillinger."
                 : "This panel cannot perform actions in Home Assistant.\n\nEnable \"Allow device to perform Home Assistant actions\" in the ESPHome device settings.";
    default:
      return "";
  }
}

bool WallPanel::valid_slot_(uint8_t slot) const { return slot >= 1 && slot <= LIGHT_SLOT_COUNT; }
bool WallPanel::valid_cover_slot_(uint8_t slot) const { return slot >= 1 && slot <= COVER_SLOT_COUNT; }

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
    slot.reported_brightness_pct = 0;
    slot.ignore_all_off = false;
  }
  this->light_slots_[3].name = "All off";
  this->light_slots_[3].function = "All off";
}

void WallPanel::load_light_configuration_() {
  LightSlotPreferences prefs{};
  if (this->light_slot_pref_.load(&prefs) && prefs.version == 2) {
    bool migrated = false;
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
      if (slot.function == "Sluk alt") {
        slot.function = "All off";
        migrated = true;
      } else if (slot.function == "Skjult") {
        slot.function = "Hidden";
        migrated = true;
      }
      if (i == 3 && slot.name == "Sluk alt") {
        slot.name = "All off";
        migrated = true;
      }
      if (slot.name.empty())
        slot.name = (i == 3) ? "All off" : "Light " + std::to_string(i + 1);
      if (slot.function != "Auto" && slot.function != "All off" && slot.function != "Hidden") {
        slot.function = (i == 3) ? "All off" : "Auto";
        migrated = true;
      }
    }
    if (migrated)
      this->save_light_configuration_();
    return;
  }

  auto legacy_pref = global_preferences->make_preference<LightEntityPreferencesV1>(LEGACY_LIGHT_ENTITY_PREF_KEY, true);
  LightEntityPreferencesV1 legacy{};
  if (legacy_pref.load(&legacy) && legacy.version == 1) {
    for (uint8_t i = 0; i < LIGHT_SLOT_COUNT; i++) {
      legacy.entity_ids[i][64] = '\0';
      this->light_slots_[i].entity_id = legacy.entity_ids[i];
      this->light_slots_[i].configured = !this->light_slots_[i].entity_id.empty();
    }
  }
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

const LightSlotState &WallPanel::get_light_slot(uint8_t slot) const { return this->light_slots_[slot - 1]; }

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
  std::string normalized = function;
  if (normalized == "Sluk alt")
    normalized = "All off";
  else if (normalized == "Skjult")
    normalized = "Hidden";
  if (normalized != "Auto" && normalized != "All off" && normalized != "Hidden")
    return;
  auto &state = this->light_slots_[slot - 1];
  if (state.function == normalized)
    return;
  state.function = normalized;
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
  state.reported_brightness_pct = 0;
  this->save_light_configuration_();
  if (!entity_id.empty())
    this->subscribe_light_slot_(slot);
}

void WallPanel::set_cover_defaults_() {
  for (uint8_t i = 0; i < COVER_SLOT_COUNT; i++) {
    auto &slot = this->cover_slots_[i];
    slot.entity_id.clear();
    slot.name = "Blind " + std::to_string(i + 1);
    slot.state.clear();
    slot.configured = false;
    slot.available = false;
    slot.has_position = false;
    slot.position_pct = 0;
    slot.supported_features = 0;
  }
}

void WallPanel::load_cover_configuration_() {
  CoverSlotPreferences prefs{};
  if (this->cover_slot_pref_.load(&prefs) && prefs.version == 1) {
    for (uint8_t i = 0; i < COVER_SLOT_COUNT; i++) {
      prefs.entity_ids[i][64] = '\0';
      prefs.names[i][32] = '\0';
      auto &slot = this->cover_slots_[i];
      slot.entity_id = prefs.entity_ids[i];
      slot.name = prefs.names[i];
      slot.configured = !slot.entity_id.empty();
      if (slot.name.empty())
        slot.name = "Blind " + std::to_string(i + 1);
    }
    return;
  }
  this->save_cover_configuration_();
}

void WallPanel::save_cover_configuration_() {
  if (!this->cover_slot_pref_ready_)
    return;
  CoverSlotPreferences prefs{};
  prefs.version = 1;
  for (uint8_t i = 0; i < COVER_SLOT_COUNT; i++) {
    const auto &slot = this->cover_slots_[i];
    std::strncpy(prefs.entity_ids[i], slot.entity_id.c_str(), 64);
    prefs.entity_ids[i][64] = '\0';
    std::strncpy(prefs.names[i], slot.name.c_str(), 32);
    prefs.names[i][32] = '\0';
  }
  if (!this->cover_slot_pref_.save(&prefs))
    ESP_LOGW(TAG, "Failed to persist cover slot configuration");
}

const CoverSlotState &WallPanel::get_cover_slot(uint8_t slot) const { return this->cover_slots_[slot - 1]; }

void WallPanel::set_cover_name(uint8_t slot, const std::string &name) {
  if (!this->valid_cover_slot_(slot))
    return;
  auto &state = this->cover_slots_[slot - 1];
  if (state.name == name)
    return;
  state.name = name;
  this->save_cover_configuration_();
}

void WallPanel::set_cover_entity(uint8_t slot, const std::string &entity_id) {
  if (!this->valid_cover_slot_(slot))
    return;
  auto &state = this->cover_slots_[slot - 1];
  if (state.entity_id == entity_id)
    return;
  state.entity_id = entity_id;
  state.state.clear();
  state.configured = !entity_id.empty();
  state.available = false;
  state.has_position = false;
  state.position_pct = 0;
  state.supported_features = 0;
  this->save_cover_configuration_();
  if (!entity_id.empty())
    this->subscribe_cover_slot_(slot);
}

void WallPanel::open_cover(uint8_t slot) {
  if (!this->valid_cover_slot_(slot))
    return;
  const auto &state = this->cover_slots_[slot - 1];
  if (!state.configured || !state.available || state.entity_id.empty())
    return;
  this->call_homeassistant_service_checked_("cover.open_cover", {{"entity_id", state.entity_id}});
}

void WallPanel::stop_cover(uint8_t slot) {
  if (!this->valid_cover_slot_(slot))
    return;
  const auto &state = this->cover_slots_[slot - 1];
  if (!state.configured || !state.available || state.entity_id.empty())
    return;
  this->call_homeassistant_service_checked_("cover.stop_cover", {{"entity_id", state.entity_id}});
}

void WallPanel::close_cover(uint8_t slot) {
  if (!this->valid_cover_slot_(slot))
    return;
  const auto &state = this->cover_slots_[slot - 1];
  if (!state.configured || !state.available || state.entity_id.empty())
    return;
  this->call_homeassistant_service_checked_("cover.close_cover", {{"entity_id", state.entity_id}});
}

void WallPanel::set_cover_position(uint8_t slot, int position_pct) {
  if (!this->valid_cover_slot_(slot))
    return;
  const auto &state = this->cover_slots_[slot - 1];
  if (!state.configured || !state.available || state.entity_id.empty() || !state.has_position)
    return;
  if (position_pct < 0)
    position_pct = 0;
  if (position_pct > 100)
    position_pct = 100;
  this->call_homeassistant_service_checked_("cover.set_cover_position",
                                             {{"entity_id", state.entity_id},
                                              {"position", std::to_string(position_pct)}});
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
        } else {
          this->action_permission_warning_ = true;
        }
      });

  api::global_api_server->send_homeassistant_action(request);
  this->set_timeout(ACTION_PERMISSION_TIMEOUT, 2500, [this, call_id]() {
    if (!this->action_permission_probe_pending_ || this->action_permission_probe_call_id_ != call_id)
      return;
    this->action_permission_probe_pending_ = false;
    this->action_permission_warning_ = true;
  });
}

void WallPanel::activate_light_slot(uint8_t slot) {
  if (!this->valid_slot_(slot))
    return;
  const auto &state = this->light_slots_[slot - 1];
  if (state.function == "All off") {
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
  if (brightness_pct == 0) {
    this->call_homeassistant_service_checked_("light.turn_off", {{"entity_id", state.entity_id}});
    return;
  }
  this->call_homeassistant_service_checked_(
      "light.turn_on",
      {{"entity_id", state.entity_id}, {"brightness_pct", std::to_string(brightness_pct)}, {"transition", "1"}});
}

void WallPanel::turn_entity_off_(const std::string &entity_id) {
  if (!entity_id.empty())
    this->call_homeassistant_service_checked_("homeassistant.turn_off", {{"entity_id", entity_id}});
}

void WallPanel::all_lights_off() {
  for (const auto &state : this->light_slots_) {
    if (state.function == "Auto" && state.configured && !state.entity_id.empty() && !state.ignore_all_off)
      this->turn_entity_off_(state.entity_id);
  }
}

void WallPanel::refresh_ha_subscriptions_() {
  if (api::global_api_server == nullptr || !api::global_api_server->is_connected())
    return;
  for (const auto &client : api::global_api_server->active_clients()) {
    if (client != nullptr && client->is_connection_setup())
      client->on_subscribe_home_assistant_states_request();
  }
}

void WallPanel::subscribe_light_slot_(uint8_t slot) {
  if (!this->valid_slot_(slot))
    return;
  const std::string entity_id = this->light_slots_[slot - 1].entity_id;
  if (entity_id.empty())
    return;
  this->subscribe_homeassistant_state(&WallPanel::on_entity_state_, entity_id);
  this->subscribe_homeassistant_state(&WallPanel::on_brightness_state_, entity_id, "brightness");
  this->subscribe_homeassistant_state(&WallPanel::on_light_supported_color_modes_state_, entity_id,
                                      "supported_color_modes");
  this->refresh_ha_subscriptions_();
}

void WallPanel::subscribe_cover_slot_(uint8_t slot) {
  if (!this->valid_cover_slot_(slot))
    return;
  const std::string entity_id = this->cover_slots_[slot - 1].entity_id;
  if (entity_id.empty())
    return;
  this->subscribe_homeassistant_state(&WallPanel::on_entity_state_, entity_id);
  this->subscribe_homeassistant_state(&WallPanel::on_cover_position_state_, entity_id, "current_position");
  this->subscribe_homeassistant_state(&WallPanel::on_cover_supported_features_state_, entity_id, "supported_features");
  this->refresh_ha_subscriptions_();
}

void WallPanel::on_entity_state_(const std::string &entity_id, StringRef value) {
  const std::string state_value = value.str();
  for (auto &slot : this->light_slots_) {
    if (slot.entity_id != entity_id)
      continue;
    slot.available = state_value != "unavailable" && state_value != "unknown";
    slot.is_on = state_value == "on";
    slot.brightness_pct = slot.is_on ? slot.reported_brightness_pct : 0;
  }
  for (auto &slot : this->cover_slots_) {
    if (slot.entity_id != entity_id)
      continue;
    slot.available = state_value != "unavailable" && state_value != "unknown";
    slot.state = state_value;
  }
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
    if (slot.entity_id == entity_id) {
      // Keep Home Assistant's last reported level so the previous dimming level is
      // restored when the light turns on, but expose 0% to the UI while it is off.
      slot.has_brightness = true;
      slot.reported_brightness_pct = pct;
      slot.brightness_pct = slot.is_on ? pct : 0;
    }
  }
}

void WallPanel::on_light_supported_color_modes_state_(const std::string &entity_id, StringRef value) {
  const std::string modes = value.str();

  // Home Assistant exposes dimming capability through supported_color_modes even
  // while a light is off, unlike the brightness attribute which can be absent.
  const bool supports_brightness =
      modes.find("brightness") != std::string::npos || modes.find("color_temp") != std::string::npos ||
      modes.find("hs") != std::string::npos || modes.find("xy") != std::string::npos ||
      modes.find("rgb") != std::string::npos || modes.find("rgbw") != std::string::npos ||
      modes.find("rgbww") != std::string::npos || modes.find("white") != std::string::npos;
  const bool explicitly_onoff = modes.find("onoff") != std::string::npos;

  // Ignore missing/unknown representations instead of accidentally removing a
  // capability already learned from a valid brightness update.
  if (!supports_brightness && !explicitly_onoff)
    return;

  for (auto &slot : this->light_slots_) {
    if (slot.entity_id == entity_id)
      slot.has_brightness = supports_brightness;
  }
}

void WallPanel::on_cover_position_state_(const std::string &entity_id, StringRef value) {
  const std::string position_value = value.str();
  char *end = nullptr;
  const float raw = std::strtof(position_value.c_str(), &end);
  if (end == position_value.c_str() || !std::isfinite(raw))
    return;
  int pct = static_cast<int>(std::lround(raw));
  if (pct < 0)
    pct = 0;
  if (pct > 100)
    pct = 100;
  for (auto &slot : this->cover_slots_) {
    if (slot.entity_id == entity_id) {
      slot.has_position = true;
      slot.position_pct = pct;
    }
  }
}

void WallPanel::on_cover_supported_features_state_(const std::string &entity_id, StringRef value) {
  const std::string feature_value = value.str();
  char *end = nullptr;
  const unsigned long raw = std::strtoul(feature_value.c_str(), &end, 10);
  if (end == feature_value.c_str())
    return;
  for (auto &slot : this->cover_slots_) {
    if (slot.entity_id == entity_id)
      slot.supported_features = static_cast<uint32_t>(raw);
  }
}

void WallPanel::dump_config() {
  ESP_LOGCONFIG(TAG, "Wall Panel:");
  ESP_LOGCONFIG(TAG, "  Language: %s", this->get_language_option());
  ESP_LOGCONFIG(TAG, "  Display rotation: %d degrees", this->display_rotation_);
  ESP_LOGCONFIG(TAG, "  Light slots: %u", LIGHT_SLOT_COUNT);
  ESP_LOGCONFIG(TAG, "  Cover slots: %u", COVER_SLOT_COUNT);
  for (uint8_t i = 0; i < LIGHT_SLOT_COUNT; i++) {
    const auto &slot = this->light_slots_[i];
    ESP_LOGCONFIG(TAG, "  Light %u: %s | %s | %s", i + 1, slot.name.c_str(),
                  slot.entity_id.empty() ? "<not configured>" : slot.entity_id.c_str(), slot.function.c_str());
  }
  for (uint8_t i = 0; i < COVER_SLOT_COUNT; i++) {
    const auto &slot = this->cover_slots_[i];
    ESP_LOGCONFIG(TAG, "  Cover %u: %s | %s", i + 1, slot.name.c_str(),
                  slot.entity_id.empty() ? "<not configured>" : slot.entity_id.c_str());
  }
}

}  // namespace wall_panel
}  // namespace esphome
