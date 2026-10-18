from pathlib import Path

import yaml

import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import image as image_component
from esphome.const import CONF_ID
from esphome.core import ID

DEPENDENCIES = ["api", "image"]
AUTO_LOAD = ["select"]

CONF_CUSTOM_ICONS = "custom_icons"
CONF_MDI = "mdi"
CONF_IMAGE_ID = "image_id"

wall_panel_ns = cg.esphome_ns.namespace("wall_panel")
WallPanel = wall_panel_ns.class_("WallPanel", cg.Component)


def _load_languages():
    language_dir = Path(__file__).parent / "language"
    languages = []
    seen_codes = set()
    seen_names = set()

    if not language_dir.is_dir():
        raise cv.Invalid(f"Wall panel language directory not found: {language_dir}")

    for path in sorted(language_dir.glob("*.y*ml")):
        with path.open("r", encoding="utf-8") as handle:
            data = yaml.safe_load(handle) or {}

        code = str(data.get("code", "")).strip()
        name = str(data.get("name", "")).strip()
        translations = data.get("translations", {})

        if not code or not name:
            raise cv.Invalid(f"{path.name}: both 'code' and 'name' are required")
        if code in seen_codes:
            raise cv.Invalid(f"Duplicate wall panel language code: {code}")
        if name in seen_names:
            raise cv.Invalid(f"Duplicate wall panel language name: {name}")
        if not isinstance(translations, dict):
            raise cv.Invalid(f"{path.name}: 'translations' must be a mapping")

        normalized = {}
        for key, value in translations.items():
            key = str(key).strip()
            if not key:
                raise cv.Invalid(f"{path.name}: translation keys cannot be empty")
            if value is None:
                continue
            normalized[key] = str(value)

        languages.append({"code": code, "name": name, "translations": normalized})
        seen_codes.add(code)
        seen_names.add(name)

    english = next((item for item in languages if item["code"] == "en"), None)
    if english is None:
        raise cv.Invalid("language/en.yaml with code 'en' is required as the fallback language")
    if not english["translations"]:
        raise cv.Invalid("language/en.yaml must contain the master translation catalog")

    # Keep English first in Home Assistant, then sort the remaining languages
    # by their native display name. Adding a new YAML file is therefore enough
    # to add it to the language selector on the next compile.
    languages.sort(key=lambda item: (item["code"] != "en", item["name"].casefold()))
    return languages


LANGUAGES = _load_languages()
LANGUAGE_OPTIONS = [item["name"] for item in LANGUAGES]


CUSTOM_ICON_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_MDI): cv.string_strict,
        cv.Required(CONF_IMAGE_ID): cv.use_id(image_component.Image_),
    }
)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(WallPanel),
        cv.Optional(CONF_CUSTOM_ICONS, default=[]): cv.ensure_list(CUSTOM_ICON_SCHEMA),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    # The panel uses Home Assistant action status responses to detect when
    # action permissions have not been enabled for the ESPHome device.
    cg.add_define("USE_API_HOMEASSISTANT_ACTION_RESPONSES")

    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    # Translation files are converted to firmware lookup tables at compile time.
    # Missing keys in non-English languages intentionally fall back to English.
    for language in LANGUAGES:
        cg.add(var.register_language(language["code"], language["name"]))
        for key, value in language["translations"].items():
            cg.add(var.register_translation(language["code"], key, value))

    # Shared images follow the convention:
    #   mdi:robot-vacuum -> icon_robot_vacuum
    # Register every compiled image using that convention automatically so
    # Home/Scene cards can inherit Home Assistant's `icon` attribute without
    # maintaining a second manual registry list.
    for image_id in sorted(image_component.get_all_image_metadata()):
        if not image_id.startswith("icon_"):
            continue

        mdi_name = "mdi:" + image_id[len("icon_") :].replace("_", "-")
        image_var = await cg.get_variable(ID(image_id, type=image_component.Image_))
        cg.add(var.register_icon(mdi_name, image_var))

    # Explicit custom mappings are applied last, so users can override an
    # automatically-derived MDI mapping if required.
    for icon_config in config[CONF_CUSTOM_ICONS]:
        image_var = await cg.get_variable(icon_config[CONF_IMAGE_ID])
        cg.add(var.register_icon(icon_config[CONF_MDI], image_var))
