import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID

DEPENDENCIES = ["api"]

wall_panel_ns = cg.esphome_ns.namespace("wall_panel")
WallPanel = wall_panel_ns.class_("WallPanel", cg.Component)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(WallPanel),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    # The panel uses Home Assistant action status responses to detect when
    # action permissions have not been enabled for the ESPHome device.
    cg.add_define("USE_API_HOMEASSISTANT_ACTION_RESPONSES")

    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
