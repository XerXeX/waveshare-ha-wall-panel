#pragma once

#include "esphome/core/component.h"
#include "esphome/core/preferences.h"
#include "esphome/components/api/custom_api_device.h"

#include <array>
#include <string>

namespace esphome {
namespace wall_panel {

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

class WallPanel : public Component, public api::CustomAPIDevice {
 public:
  void setup() override;
  void dump_config() override;

  void set_light_entity(uint8_t slot, const std::string &entity_id);
  void set_light_name(uint8_t slot, const std::string &name);
  void set_light_function(uint8_t slot, const std::string &function);
  void set_ignore_all_off(uint8_t slot, bool ignore);

  void activate_light_slot(uint8_t slot);
  void set_light_brightness(uint8_t slot, int brightness_pct);
  void all_lights_off();

  const LightSlotState &get_light_slot(uint8_t slot) const;

 protected:
  static constexpr uint8_t LIGHT_SLOT_COUNT = 4;
  static constexpr uint32_t LEGACY_LIGHT_ENTITY_PREF_KEY = 0x57504C45U;
  static constexpr uint32_t LIGHT_SLOT_PREF_KEY = 0x57504C32U;

  std::array<LightSlotState, LIGHT_SLOT_COUNT> light_slots_{};
  ESPPreferenceObject light_slot_pref_;
  bool light_slot_pref_ready_{false};

  bool valid_slot_(uint8_t slot) const;
  void set_light_defaults_();
  void load_light_configuration_();
  void save_light_configuration_();
  void subscribe_light_slot_(uint8_t slot);
  void turn_entity_off_(const std::string &entity_id);

  void on_entity_state_(const std::string &entity_id, StringRef value);
  void on_brightness_state_(const std::string &entity_id, StringRef value);
};

}  // namespace wall_panel
}  // namespace esphome
