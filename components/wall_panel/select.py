import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import select
from esphome.const import CONF_ID

from . import LANGUAGES, LANGUAGE_OPTIONS, WallPanel, wall_panel_ns

CONF_WALL_PANEL_ID = "wall_panel_id"

WallPanelLanguageSelect = wall_panel_ns.class_(
    "WallPanelLanguageSelect", select.Select, cg.Component
)

CONFIG_SCHEMA = (
    select.select_schema(WallPanelLanguageSelect)
    .extend(
        {
            cv.GenerateID(CONF_WALL_PANEL_ID): cv.use_id(WallPanel),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):
    parent = await cg.get_variable(config[CONF_WALL_PANEL_ID])
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await select.register_select(var, config, options=LANGUAGE_OPTIONS)
    cg.add(var.set_parent(parent))

    # Convert every language YAML file into lookup tables in firmware.
    for language in LANGUAGES:
        cg.add(var.register_language(language["code"], language["name"]))
        for key, value in language["translations"].items():
            cg.add(var.register_translation(language["code"], key, value))
