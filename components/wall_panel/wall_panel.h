#pragma once

#include "esphome/core/component.h"
#include "esphome/core/preferences.h"
#include "esphome/components/api/custom_api_device.h"

#include <array>
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

  bool has_action_permission_warning() const { return this->action_permission_warning_; }
  void dismiss_action_permission_warning();

 protected:
  static constexpr uint8_t LIGHT_SLOT_COUNT = 4;
  static constexpr uint8_t COVER_SLOT_COUNT = 4;
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

  void on_entity_state_(const std::string &entity_id, StringRef value);
  void on_brightness_state_(const std::string &entity_id, StringRef value);
  void on_cover_position_state_(const std::string &entity_id, StringRef value);
  void on_cover_supported_features_state_(const std::string &entity_id, StringRef value);
};

}  // namespace wall_panel
}  // namespace esphome
