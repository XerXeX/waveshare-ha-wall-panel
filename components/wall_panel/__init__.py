import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import image as image_component
from esphome.const import CONF_ID

DEPENDENCIES = ["api"]

CONF_CUSTOM_ICONS = "custom_icons"
CONF_MDI = "mdi"
CONF_IMAGE_ID = "image_id"

wall_panel_ns = cg.esphome_ns.namespace("wall_panel")
WallPanel = wall_panel_ns.class_("WallPanel", cg.Component)

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

    # User-defined images are resolved at code-generation time and registered
    # with their Home Assistant MDI names for runtime lookup by the UI.
    for icon_config in config[CONF_CUSTOM_ICONS]:
        image_var = await cg.get_variable(icon_config[CONF_IMAGE_ID])
        cg.add(var.register_icon(icon_config[CONF_MDI], image_var))
