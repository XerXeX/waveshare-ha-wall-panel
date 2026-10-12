#pragma once

#include "esphome/core/component.h"
#include "esphome/core/preferences.h"
#include "esphome/components/api/custom_api_device.h"

#include <array>
#include <cctype>
#include <map>
#include <string>

namespace esphome {
namespace wall_panel {

enum class Language : uint8_t {
  ENGLISH = 0,
  DANISH = 1,
};

enum class TranslationKey : uint8_t {
  LIGHTS,
  LIGHT,
  BLINDS,
  AUDIO,
  SCENES,
  ALL_OFF,
  ALL_LIGHTS,
  UNAVAILABLE,
  ON,
  OFF,
  ACTION_PERMISSION_MESSAGE,
};

struct LightSlotState {
  std::string entity_id;
  std::string name;
  std::string function{"Auto"};

  bool configured{false};
  bool available{false};
  bool is_on{false};

  bool has_brightness{false};
  int brightness_pct{0};
  int reported_brightness_pct{0};

  bool ignore_all_off{false};
};

struct CoverSlotState {
  std::string entity_id;
  std::string name;
  std::string state;

  bool configured{false};
  bool available{false};
  bool has_position{false};
  int position_pct{0};
  uint32_t supported_features{0};
};

// Legacy v1 storage. Kept only so existing entity selections can be migrated.
struct LightEntityPreferencesV1 {
  uint8_t version{1};
  char entity_ids[4][65]{};
};

struct LightSlotPreferences {
  uint8_t version{2};
  char entity_ids[4][65]{};
  char names[4][33]{};
  char functions[4][16]{};
  uint8_t ignore_all_off[4]{};
};

struct CoverSlotPreferences {
  uint8_t version{1};
  char entity_ids[4][65]{};
  char names[4][33]{};
};

struct LanguagePreferences {
  uint8_t version{1};
  uint8_t language{static_cast<uint8_t>(Language::ENGLISH)};
};

struct DisplayPreferences {
  uint8_t version{1};
  uint16_t rotation{0};
};

class WallPanel : public Component, public api::CustomAPIDevice {
 public:
  void setup() override;
  void dump_config() override;

  void set_language(const std::string &language);
  const char *get_language_option() const;
  const char *tr(TranslationKey key) const;

  void set_display_rotation(int rotation);
  int get_display_rotation() const { return this->display_rotation_; }
  const char *get_display_rotation_option() const;

  void set_light_entity(uint8_t slot, const std::string &entity_id);
  void set_light_name(uint8_t slot, const std::string &name);
  void set_light_function(uint8_t slot, const std::string &function);
  void set_ignore_all_off(uint8_t slot, bool ignore);

  void activate_light_slot(uint8_t slot);
  void set_light_brightness(uint8_t slot, int brightness_pct);
  void all_lights_off();

  const LightSlotState &get_light_slot(uint8_t slot) const;

  void set_cover_entity(uint8_t slot, const std::string &entity_id);
  void set_cover_name(uint8_t slot, const std::string &name);
  void open_cover(uint8_t slot);
  void stop_cover(uint8_t slot);
  void close_cover(uint8_t slot);
  void set_cover_position(uint8_t slot, int position_pct);
  const CoverSlotState &get_cover_slot(uint8_t slot) const;

  // Scene/script cards are configured in YAML, but the component subscribes to
  // their Home Assistant icon attribute so the UI can mirror the source entity.
  void set_scene_icon_entity(uint8_t slot, const std::string &entity_id) {
    if (slot < 1 || slot > SCENE_SLOT_COUNT)
      return;
    auto &current_entity = this->scene_icon_entities_[slot - 1];
    if (current_entity == entity_id)
      return;
    current_entity = entity_id;
    this->scene_icons_[slot - 1].clear();
    if (entity_id.empty())
      return;
    this->subscribe_homeassistant_state(&WallPanel::on_scene_icon_state_, entity_id, "icon");
    this->refresh_ha_subscriptions_();
  }

  const std::string &get_scene_icon(uint8_t slot) const {
    static const std::string empty;
    if (slot < 1 || slot > SCENE_SLOT_COUNT)
      return empty;
    return this->scene_icons_[slot - 1];
  }

  // Execute a compact, runtime-editable list of Home Assistant actions.
  // Format, one command per line or separated by semicolons:
  //   light.turn_on light.living_room brightness_pct=40
  //   cover.close_cover cover.living_room
  void execute_local_actions(const std::string &actions) {
    auto trim = [](std::string value) {
      while (!value.empty() && std::isspace(static_cast<unsigned char>(value.front())))
        value.erase(value.begin());
      while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back())))
        value.pop_back();
      return value;
    };

    size_t start = 0;
    while (start < actions.size()) {
      size_t end = actions.find_first_of(";\n", start);
      if (end == std::string::npos)
        end = actions.size();

      std::string command = trim(actions.substr(start, end - start));
      start = end + 1;
      if (command.empty())
        continue;

      size_t cursor = 0;
      auto next_token = [&command, &cursor]() -> std::string {
        while (cursor < command.size() && std::isspace(static_cast<unsigned char>(command[cursor])))
          cursor++;
        if (cursor >= command.size())
          return {};
        const size_t token_start = cursor;
        while (cursor < command.size() && !std::isspace(static_cast<unsigned char>(command[cursor])))
          cursor++;
        return command.substr(token_start, cursor - token_start);
      };

      const std::string service = next_token();
      if (service.empty() || service.find('.') == std::string::npos)
        continue;

      std::map<std::string, std::string> data;
      bool entity_assigned = false;
      while (true) {
        const std::string token = next_token();
        if (token.empty())
          break;
        const size_t separator = token.find('=');
        if (separator != std::string::npos) {
          if (separator > 0 && separator + 1 < token.size())
            data[token.substr(0, separator)] = token.substr(separator + 1);
        } else if (!entity_assigned) {
          data["entity_id"] = token;
          entity_assigned = true;
        }
      }

      this->call_homeassistant_service_checked_(service, data);
    }
  }

  bool has_action_permission_warning() const { return this->action_permission_warning_; }
  void dismiss_action_permission_warning();

 protected:
  static constexpr uint8_t LIGHT_SLOT_COUNT = 4;
  static constexpr uint8_t COVER_SLOT_COUNT = 4;
  static constexpr uint8_t SCENE_SLOT_COUNT = 4;
  static constexpr uint32_t LEGACY_LIGHT_ENTITY_PREF_KEY = 0x57504C45U;
  static constexpr uint32_t LIGHT_SLOT_PREF_KEY = 0x57504C32U;
  static constexpr uint32_t COVER_SLOT_PREF_KEY = 0x57504331U;
  static constexpr uint32_t LANGUAGE_PREF_KEY = 0x57504C4CU;
  static constexpr uint32_t DISPLAY_PREF_KEY = 0x57504452U;

  std::array<LightSlotState, LIGHT_SLOT_COUNT> light_slots_{};
  ESPPreferenceObject light_slot_pref_;
  bool light_slot_pref_ready_{false};

  std::array<CoverSlotState, COVER_SLOT_COUNT> cover_slots_{};
  ESPPreferenceObject cover_slot_pref_;
  bool cover_slot_pref_ready_{false};

  std::array<std::string, SCENE_SLOT_COUNT> scene_icon_entities_{};
  std::array<std::string, SCENE_SLOT_COUNT> scene_icons_{};

  Language language_{Language::ENGLISH};
  ESPPreferenceObject language_pref_;
  bool language_pref_ready_{false};

  int display_rotation_{0};
  ESPPreferenceObject display_pref_;
  bool display_pref_ready_{false};

  bool action_permission_confirmed_{false};
  bool action_permission_probe_pending_{false};
  bool action_permission_warning_{false};
  uint32_t action_permission_probe_call_id_{0};
  uint32_t next_action_call_id_{1};

  bool valid_slot_(uint8_t slot) const;
  bool valid_cover_slot_(uint8_t slot) const;
  void load_language_();
  void save_language_();
  void load_display_rotation_();
  void save_display_rotation_();
  void set_light_defaults_();
  void load_light_configuration_();
  void save_light_configuration_();
  void subscribe_light_slot_(uint8_t slot);
  void set_cover_defaults_();
  void load_cover_configuration_();
  void save_cover_configuration_();
  void subscribe_cover_slot_(uint8_t slot);
  void refresh_ha_subscriptions_();
  void turn_entity_off_(const std::string &entity_id);

  void call_homeassistant_service_checked_(const std::string &service_name,
                                           const std::map<std::string, std::string> &data);

  void on_scene_icon_state_(const std::string &entity_id, StringRef value) {
    const std::string icon = value.str();
    for (uint8_t i = 0; i < SCENE_SLOT_COUNT; i++) {
      if (this->scene_icon_entities_[i] == entity_id)
        this->scene_icons_[i] = icon;
    }
  }

  void on_entity_state_(const std::string &entity_id, StringRef value);
  void on_brightness_state_(const std::string &entity_id, StringRef value);
  void on_light_supported_color_modes_state_(const std::string &entity_id, StringRef value);
  void on_cover_position_state_(const std::string &entity_id, StringRef value);
  void on_cover_supported_features_state_(const std::string &entity_id, StringRef value);
};

}  // namespace wall_panel
}  // namespace esphome
