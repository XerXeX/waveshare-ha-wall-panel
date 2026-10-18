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

        languages.append({"code": code, "name": name, "translations": normalized, "file": path.name})
        seen_codes.add(code)
        seen_names.add(name)

    english = next((item for item in languages if item["code"] == "en"), None)
    if english is None:
        raise cv.Invalid("language/en.yaml with code 'en' is required as the fallback language")
    if not english["translations"]:
        raise cv.Invalid("language/en.yaml must contain the master translation catalog")

    master_keys = set(english["translations"])
    for language in languages:
        if language["code"] == "en":
            continue
        unknown = sorted(set(language["translations"]) - master_keys)
        if unknown:
            raise cv.Invalid(
                f"{language['file']}: translation key(s) not present in en.yaml: {', '.join(unknown)}"
            )

    # English is always first; every other language is shown by its native name.
    # Adding another YAML file is therefore enough to add it to the selector.
    languages.sort(key=lambda item: (item["code"] != "en", item["name"].casefold()))
    return languages


def _load_shared_icons():
    """Read packages/icons.yaml and build the runtime MDI -> image registry.

    Do not rely on image_component.get_all_image_metadata() here. With remote
    packages/external components the wall_panel codegen can run before that
    metadata list contains the images declared by packages/icons.yaml. In that
    case the firmware compiled the icon images correctly, but the runtime
    registry stayed empty and Home cards fell back to domain defaults.

    packages/icons.yaml is therefore the canonical registry source. Any entry
    using an mdi:* file and an icon_* id is automatically inherited by Home
    entities whose Home Assistant `icon` attribute matches it.
    """
    repo_root = Path(__file__).resolve().parents[2]
    icons_path = repo_root / "packages" / "icons.yaml"

    if not icons_path.is_file():
        raise cv.Invalid(f"Wall panel icon catalog not found: {icons_path}")

    with icons_path.open("r", encoding="utf-8") as handle:
        data = yaml.safe_load(handle) or {}

    entries = data.get("image", [])
    if not isinstance(entries, list):
        raise cv.Invalid("packages/icons.yaml: 'image' must be a list")

    icons = []
    seen_mdi = set()
    seen_ids = set()

    for entry in entries:
        if not isinstance(entry, dict):
            continue

        mdi_name = str(entry.get("file", "")).strip()
        image_id = str(entry.get("id", "")).strip()

        # Only shared MDI assets participate in automatic entity inheritance.
        # Other image resources may still be registered explicitly with
        # wall_panel.custom_icons when needed.
        if not mdi_name.startswith("mdi:") or not image_id.startswith("icon_"):
            continue

        if mdi_name in seen_mdi:
            raise cv.Invalid(f"Duplicate shared MDI icon in packages/icons.yaml: {mdi_name}")
        if image_id in seen_ids:
            raise cv.Invalid(f"Duplicate shared icon id in packages/icons.yaml: {image_id}")

        icons.append((mdi_name, image_id))
        seen_mdi.add(mdi_name)
        seen_ids.add(image_id)

    if not icons:
        raise cv.Invalid("packages/icons.yaml did not contain any shared MDI icons")

    return icons


LANGUAGES = _load_languages()
LANGUAGE_OPTIONS = [item["name"] for item in LANGUAGES]
SHARED_ICONS = _load_shared_icons()


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

    # Every MDI asset declared in packages/icons.yaml is registered explicitly.
    # This makes entity icon inheritance deterministic for every Home domain:
    #   HA icon: mdi:robot-vacuum -> image id: icon_robot_vacuum
    # A domain fallback is used by the UI only when the HA icon is missing or the
    # requested MDI asset is not part of the compiled shared icon library.
    for mdi_name, image_id in SHARED_ICONS:
        image_var = await cg.get_variable(ID(image_id, type=image_component.Image_))
        cg.add(var.register_icon(mdi_name, image_var))

    # Explicit custom mappings are applied last, so users can override a shared
    # MDI mapping or register an additional image outside packages/icons.yaml.
    for icon_config in config[CONF_CUSTOM_ICONS]:
        image_var = await cg.get_variable(icon_config[CONF_IMAGE_ID])
        cg.add(var.register_icon(icon_config[CONF_MDI], image_var))
