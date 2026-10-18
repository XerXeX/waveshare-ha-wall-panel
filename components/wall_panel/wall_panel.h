#pragma once

#include "esphome/core/component.h"
#include "esphome/core/preferences.h"
#include "esphome/components/api/custom_api_device.h"
#include "esphome/components/image/image.h"
#include "esphome/components/select/select.h"

#include <array>
#include <cstring>
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

struct HomeSlotState {
  std::string entity_id;
  std::string name;
  std::string domain;
  std::string state;
  std::string icon;
  std::string unit;
  std::string hvac_action;
  std::string pressure_unit;
  std::string wind_speed_unit;
  std::string wind_bearing;

  bool configured{false};
  bool available{false};
  bool has_brightness{false};
  bool has_position{false};
  bool has_current_temperature{false};
  bool has_target_temperature{false};
  bool has_humidity{false};
  bool has_pressure{false};
  bool has_wind_speed{false};

  int brightness_pct{0};
  int reported_brightness_pct{0};
  int position_pct{0};
  float current_temperature{0.0f};
  float target_temperature{0.0f};
  float humidity{0.0f};
  float pressure{0.0f};
  float wind_speed{0.0f};
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

struct DynamicLanguagePreferences {
  uint8_t version{1};
  char code[16]{};
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

  void set_home_entity(uint8_t slot, const std::string &entity_id);
  void set_home_name(uint8_t slot, const std::string &name);
  const HomeSlotState &get_home_slot(uint8_t slot) const;
  void toggle_home_entity(uint8_t slot);
  void activate_home_entity(uint8_t slot);
  void set_home_light_brightness(uint8_t slot, int brightness_pct);
  void open_home_cover(uint8_t slot);
  void stop_home_cover(uint8_t slot);
  void close_home_cover(uint8_t slot);
  void set_home_cover_position(uint8_t slot, int position_pct);
  void set_home_climate_temperature(uint8_t slot, float temperature);

  void register_icon(const std::string &mdi_name, image::Image *image) {
    if (mdi_name.empty() || image == nullptr)
      return;
    this->icon_registry_[mdi_name] = image;
  }

  image::Image *get_registered_icon(const std::string &mdi_name) const {
    const auto it = this->icon_registry_.find(mdi_name);
    return it == this->icon_registry_.end() ? nullptr : it->second;
  }

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

  bool has_action_permission_warning() const { return this->action_permission_warning_; }
  void dismiss_action_permission_warning();

 protected:
  static constexpr uint8_t LIGHT_SLOT_COUNT = 4;
  static constexpr uint8_t COVER_SLOT_COUNT = 4;
  static constexpr uint8_t HOME_SLOT_COUNT = 6;
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

  std::array<HomeSlotState, HOME_SLOT_COUNT> home_slots_{};

  std::map<std::string, image::Image *> icon_registry_{};
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
  bool valid_home_slot_(uint8_t slot) const;
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
  void subscribe_home_slot_(uint8_t slot);
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

  void on_home_state_(const std::string &entity_id, StringRef value);
  void on_home_icon_state_(const std::string &entity_id, StringRef value);
  void on_home_unit_state_(const std::string &entity_id, StringRef value);
  void on_home_temperature_unit_state_(const std::string &entity_id, StringRef value);
  void on_home_brightness_state_(const std::string &entity_id, StringRef value);
  void on_home_supported_color_modes_state_(const std::string &entity_id, StringRef value);
  void on_home_position_state_(const std::string &entity_id, StringRef value);
  void on_home_temperature_state_(const std::string &entity_id, StringRef value);
  void on_home_current_temperature_state_(const std::string &entity_id, StringRef value);
  void on_home_humidity_state_(const std::string &entity_id, StringRef value);
  void on_home_hvac_action_state_(const std::string &entity_id, StringRef value);
  void on_home_pressure_state_(const std::string &entity_id, StringRef value);
  void on_home_pressure_unit_state_(const std::string &entity_id, StringRef value);
  void on_home_wind_speed_state_(const std::string &entity_id, StringRef value);
  void on_home_wind_speed_unit_state_(const std::string &entity_id, StringRef value);
  void on_home_wind_bearing_state_(const std::string &entity_id, StringRef value);
};

// The language selector owns the scalable translation system. The original
// WallPanel English/Danish methods above are kept for backwards compatibility
// with existing configurations while packages migrate to string translation keys.
class WallPanelLanguageSelect : public select::Select, public Component {
 public:
  void set_parent(WallPanel *parent) { this->parent_ = parent; }

  void register_language(const std::string &code, const std::string &name) {
    if (!code.empty() && !name.empty())
      this->language_names_[code] = name;
  }

  void register_translation(const std::string &code, const std::string &key, const std::string &value) {
    if (!code.empty() && !key.empty())
      this->translations_[code][key] = value;
  }

  const char *get_language_code() const { return this->language_code_.c_str(); }

  const char *get_language_option() const {
    const auto it = this->language_names_.find(this->language_code_);
    if (it != this->language_names_.end())
      return it->second.c_str();
    const auto english = this->language_names_.find("en");
    return english != this->language_names_.end() ? english->second.c_str() : "English";
  }

  const char *tr(const std::string &key) const {
    const auto language = this->translations_.find(this->language_code_);
    if (language != this->translations_.end()) {
      const auto value = language->second.find(key);
      if (value != language->second.end())
        return value->second.c_str();
    }

    const auto english = this->translations_.find("en");
    if (english != this->translations_.end()) {
      const auto value = english->second.find(key);
      if (value != english->second.end())
        return value->second.c_str();
    }
    return "";
  }

  void setup() override {
    this->language_pref_ = global_preferences->make_preference<DynamicLanguagePreferences>(LANGUAGE_SELECT_PREF_KEY, true);
    this->language_pref_ready_ = true;

    DynamicLanguagePreferences prefs{};
    if (this->language_pref_.load(&prefs) && prefs.version == 1) {
      prefs.code[15] = '\0';
      const std::string stored = prefs.code;
      if (this->language_names_.find(stored) != this->language_names_.end())
        this->language_code_ = stored;
    }

    if (this->language_names_.find(this->language_code_) == this->language_names_.end())
      this->language_code_ = "en";

    this->sync_legacy_parent_();
    this->publish_state(this->get_language_option());
  }

 protected:
  static constexpr uint32_t LANGUAGE_SELECT_PREF_KEY = 0x57504C53U;

  void control(const std::string &value) override {
    for (const auto &language : this->language_names_) {
      if (language.second != value)
        continue;
      if (this->language_code_ != language.first) {
        this->language_code_ = language.first;
        this->save_language_();
        this->sync_legacy_parent_();
      }
      this->publish_state(this->get_language_option());
      return;
    }
  }

  void save_language_() {
    if (!this->language_pref_ready_)
      return;
    DynamicLanguagePreferences prefs{};
    prefs.version = 1;
    std::strncpy(prefs.code, this->language_code_.c_str(), sizeof(prefs.code) - 1);
    prefs.code[sizeof(prefs.code) - 1] = '\0';
    this->language_pref_.save(&prefs);
  }

  void sync_legacy_parent_() {
    if (this->parent_ == nullptr)
      return;
    if (this->language_code_ == "da")
      this->parent_->set_language("Danish");
    else
      this->parent_->set_language("English");
  }

  WallPanel *parent_{nullptr};
  std::map<std::string, std::string> language_names_{};
  std::map<std::string, std::map<std::string, std::string>> translations_{};
  std::string language_code_{"en"};
  ESPPreferenceObject language_pref_;
  bool language_pref_ready_{false};
};

}  // namespace wall_panel
}  // namespace esphome
