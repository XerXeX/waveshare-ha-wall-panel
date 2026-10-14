import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import image as image_component
from esphome.const import CONF_ID
from esphome.core import ID

DEPENDENCIES = ["api", "image"]

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
