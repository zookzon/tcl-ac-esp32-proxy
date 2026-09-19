from esphome import automation, pins
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import climate, sensor, uart
from esphome.const import (
    CONF_ID,
    CONF_LEVEL,
    CONF_BEEPER,
    CONF_VISUAL,
    CONF_MAX_TEMPERATURE,
    CONF_MIN_TEMPERATURE,
    CONF_SUPPORTED_MODES,
    CONF_TEMPERATURE_STEP,
    CONF_SUPPORTED_PRESETS,
    CONF_TARGET_TEMPERATURE,
    CONF_SUPPORTED_FAN_MODES,
    CONF_SUPPORTED_SWING_MODES,
)

from esphome.components.climate import (
    ClimateMode,
    ClimatePreset,
    ClimateSwingMode,
    CONF_CURRENT_TEMPERATURE,
)

AUTO_LOAD = ["climate", "sensor"]
CODEOWNERS = ["@I-am-nightingale", "@xaxexa", "@junkfix"]
DEPENDENCIES = ["api", "climate", "uart"]

TCLAC_MIN_TEMPERATURE = 16.0
TCLAC_MAX_TEMPERATURE = 31.0
TCLAC_TARGET_TEMPERATURE_STEP = 1.0
TCLAC_CURRENT_TEMPERATURE_STEP = 0.1

CONF_RX_LED = "rx_led"
CONF_TX_LED = "tx_led"
CONF_DISPLAY = "show_display"
CONF_FORCE_MODE = "force_mode"
CONF_VERTICAL_AIRFLOW = "vertical_airflow"
CONF_MODULE_DISPLAY = "show_module_display"
CONF_HORIZONTAL_AIRFLOW = "horizontal_airflow"
CONF_VERTICAL_SWING_MODE = "vertical_swing_mode"
CONF_HORIZONTAL_SWING_MODE = "horizontal_swing_mode"
CONF_ERROR_CODE = "error_code"
CONF_ROOM_TEMPERATURE = "room_temperature"
CONF_INDOOR_COIL_TEMPERATURE = "indoor_coil_temperature"
CONF_INDOOR_COIL_RAW = "indoor_coil_raw"
CONF_PIPE_OUT_TEMPERATURE = "pipe_out_temperature"
CONF_PIPE_IN_TEMPERATURE = "pipe_in_temperature"
CONF_OUTDOOR_AMBIENT_CANDIDATE = "outdoor_ambient_candidate"
CONF_OUTDOOR_EXHAUST_CANDIDATE = "outdoor_exhaust_candidate"
CONF_OUTDOOR_CONDENSER_CANDIDATE = "outdoor_condenser_candidate"
CONF_COMPRESSOR_FREQUENCY_CANDIDATE = "compressor_frequency_candidate"
CONF_EXTERNAL_TEMPERATURE_CANDIDATE_RAW_37 = "external_temperature_candidate_raw_37"
CONF_EXTERNAL_TEMPERATURE_CANDIDATE_RAW_38 = "external_temperature_candidate_raw_38"
CONF_COMPRESSOR_CURRENT = "compressor_current"
CONF_COMPRESSOR_STATE = "compressor_state"
CONF_AIRMAX_FAULT_CODE = "airmax_fault_code"
CONF_RAW_BYTE_45 = "raw_byte_45"
CONF_ACTIVE_SUPPLY_VOLTAGE = "active_supply_voltage"
CONF_A5_INPUT_POWER = "a5_input_power"
CONF_A5_COMPRESSOR_ACTUAL = "a5_compressor_actual"
CONF_A5_COMPRESSOR_TARGET = "a5_compressor_target"
CONF_OUTSIDE_MOTOR = "outside_motor"
CONF_VERTICAL_VANE_POSITION = "vertical_vane_position"
CONF_HORIZONTAL_VANE_POSITION = "horizontal_vane_position"
CONF_DONGLE_UART_ID = "dongle_uart_id"
CONF_PROXY_MODE = "proxy_mode"
CONF_A5_ACK_ONLY = "a5_ack_only"
CONF_PROXY_LOG_PACKETS = "proxy_log_packets"
CONF_PROXY_IDLE_TIME = "proxy_idle_time"
CONF_PROXY_FALLBACK_POLL_INTERVAL = "proxy_fallback_poll_interval"

tclac_ns = cg.esphome_ns.namespace("tclac")
tclacClimate = tclac_ns.class_("tclacClimate", uart.UARTDevice, climate.Climate, cg.PollingComponent)

SUPPORTED_FAN_MODES_OPTIONS = {
    "AUTO": ClimateMode.CLIMATE_FAN_AUTO,  # Доступен всегда
    "QUIET": ClimateMode.CLIMATE_FAN_QUIET,
    "LOW": ClimateMode.CLIMATE_FAN_LOW,
    "MIDDLE": ClimateMode.CLIMATE_FAN_MIDDLE,
    "MEDIUM": ClimateMode.CLIMATE_FAN_MEDIUM,
    "HIGH": ClimateMode.CLIMATE_FAN_HIGH,
    "FOCUS": ClimateMode.CLIMATE_FAN_FOCUS,
    "DIFFUSE": ClimateMode.CLIMATE_FAN_DIFFUSE,
}

SUPPORTED_SWING_MODES_OPTIONS = {
    "OFF": ClimateSwingMode.CLIMATE_SWING_OFF,  # Доступен всегда
    "VERTICAL": ClimateSwingMode.CLIMATE_SWING_VERTICAL,
    "HORIZONTAL": ClimateSwingMode.CLIMATE_SWING_HORIZONTAL,
    "BOTH": ClimateSwingMode.CLIMATE_SWING_BOTH,
}

SUPPORTED_CLIMATE_MODES_OPTIONS = {
    "OFF": ClimateMode.CLIMATE_MODE_OFF,  # Доступен всегда
    "AUTO": ClimateMode.CLIMATE_MODE_AUTO,  # Доступен всегда
    "COOL": ClimateMode.CLIMATE_MODE_COOL,
    "HEAT": ClimateMode.CLIMATE_MODE_HEAT,
    "DRY": ClimateMode.CLIMATE_MODE_DRY,
    "FAN_ONLY": ClimateMode.CLIMATE_MODE_FAN_ONLY,
}

SUPPORTED_CLIMATE_PRESETS_OPTIONS = {
    "NONE": ClimatePreset.CLIMATE_PRESET_NONE, # Доступен всегда
    "ECO": ClimatePreset.CLIMATE_PRESET_ECO,
    "SLEEP": ClimatePreset.CLIMATE_PRESET_SLEEP,
    "COMFORT": ClimatePreset.CLIMATE_PRESET_COMFORT,
}

VerticalSwingDirection = tclac_ns.enum("VerticalSwingDirection", True)
VERTICAL_SWING_DIRECTION_OPTIONS = {
    "UP_DOWN": VerticalSwingDirection.UP_DOWN,
    "UPSIDE": VerticalSwingDirection.UPSIDE,
    "DOWNSIDE": VerticalSwingDirection.DOWNSIDE,
}

HorizontalSwingDirection = tclac_ns.enum("HorizontalSwingDirection", True)
HORIZONTAL_SWING_DIRECTION_OPTIONS = {
    "LEFT_RIGHT": HorizontalSwingDirection.LEFT_RIGHT,
    "LEFTSIDE": HorizontalSwingDirection.LEFTSIDE,
    "CENTER": HorizontalSwingDirection.CENTER,
    "RIGHTSIDE": HorizontalSwingDirection.RIGHTSIDE,
}

AirflowVerticalDirection = tclac_ns.enum("AirflowVerticalDirection", True)
AIRFLOW_VERTICAL_DIRECTION_OPTIONS = {
    "LAST": AirflowVerticalDirection.LAST,
    "MAX_UP": AirflowVerticalDirection.MAX_UP,
    "UP": AirflowVerticalDirection.UP,
    "CENTER": AirflowVerticalDirection.CENTER,
    "DOWN": AirflowVerticalDirection.DOWN,
    "MAX_DOWN": AirflowVerticalDirection.MAX_DOWN,
}

AirflowHorizontalDirection = tclac_ns.enum("AirflowHorizontalDirection", True)
AIRFLOW_HORIZONTAL_DIRECTION_OPTIONS = {
    "LAST": AirflowHorizontalDirection.LAST,
    "MAX_LEFT": AirflowHorizontalDirection.MAX_LEFT,
    "LEFT": AirflowHorizontalDirection.LEFT,
    "CENTER": AirflowHorizontalDirection.CENTER,
    "RIGHT": AirflowHorizontalDirection.RIGHT,
    "MAX_RIGHT": AirflowHorizontalDirection.MAX_RIGHT,
}

# Проверка конфигурации интерфейса и принятие значений по умолчанию
def validate_visual(config):
    if CONF_VISUAL in config:
        visual_config = config[CONF_VISUAL]
        if CONF_MIN_TEMPERATURE in visual_config:
            min_temp = visual_config[CONF_MIN_TEMPERATURE]
            if min_temp < TCLAC_MIN_TEMPERATURE:
                raise cv.Invalid(f"Указанная интерфейсная минимальная температура в {min_temp} ниже допустимой {TCLAC_MIN_TEMPERATURE} для кондиционера")
        else:
            config[CONF_VISUAL][CONF_MIN_TEMPERATURE] = TCLAC_MIN_TEMPERATURE
        if CONF_MAX_TEMPERATURE in visual_config:
            max_temp = visual_config[CONF_MAX_TEMPERATURE]
            if max_temp > TCLAC_MAX_TEMPERATURE:
                raise cv.Invalid(f"Указанная интерфейсная максимальная температура в {max_temp} выше допустимой {TCLAC_MAX_TEMPERATURE} для кондиционера")
        else:
            config[CONF_VISUAL][CONF_MAX_TEMPERATURE] = TCLAC_MAX_TEMPERATURE
        if CONF_TEMPERATURE_STEP in visual_config:
            temp_step = config[CONF_VISUAL][CONF_TEMPERATURE_STEP][CONF_TARGET_TEMPERATURE]
            if ((int)(temp_step * 2)) / 2 != temp_step:
                raise cv.Invalid(f"Указанный шаг температуры {temp_step} не корректен, должен быть кратен 1")
        else:
            config[CONF_VISUAL][CONF_TEMPERATURE_STEP] = {CONF_TARGET_TEMPERATURE: TCLAC_TARGET_TEMPERATURE_STEP,CONF_CURRENT_TEMPERATURE: TCLAC_CURRENT_TEMPERATURE_STEP,}
    else:
        config[CONF_VISUAL] = {CONF_MIN_TEMPERATURE: TCLAC_MIN_TEMPERATURE,CONF_MAX_TEMPERATURE: TCLAC_MAX_TEMPERATURE,CONF_TEMPERATURE_STEP: {CONF_TARGET_TEMPERATURE: TCLAC_TARGET_TEMPERATURE_STEP,CONF_CURRENT_TEMPERATURE: TCLAC_CURRENT_TEMPERATURE_STEP,},}
    return config

def validate_proxy(config):
    if config.get(CONF_PROXY_MODE, False) and CONF_DONGLE_UART_ID not in config:
        raise cv.Invalid("proxy_mode requires dongle_uart_id")
    return config

# Проверка конфигурации компонента и принятие значений по умолчанию
CONFIG_SCHEMA = cv.All(
    climate.climate_schema(tclacClimate)
    .extend(
        {
            cv.Optional(CONF_BEEPER, default=True): cv.boolean,
            cv.Optional(CONF_DISPLAY, default=True): cv.boolean,
            cv.Optional(CONF_RX_LED): pins.gpio_output_pin_schema,
            cv.Optional(CONF_TX_LED): pins.gpio_output_pin_schema,
            cv.Optional(CONF_FORCE_MODE, default=True): cv.boolean,
            cv.Optional(CONF_MODULE_DISPLAY, default=True): cv.boolean,
            cv.Optional(CONF_VERTICAL_AIRFLOW, default="CENTER"): cv.ensure_list(cv.enum(AIRFLOW_VERTICAL_DIRECTION_OPTIONS, upper=True)),
            cv.Optional(CONF_VERTICAL_SWING_MODE, default="UP_DOWN"): cv.ensure_list(cv.enum(VERTICAL_SWING_DIRECTION_OPTIONS, upper=True)),
            cv.Optional(CONF_HORIZONTAL_AIRFLOW, default="CENTER"): cv.ensure_list(cv.enum(AIRFLOW_HORIZONTAL_DIRECTION_OPTIONS, upper=True)),
            cv.Optional(CONF_HORIZONTAL_SWING_MODE, default="LEFT_RIGHT"): cv.ensure_list(cv.enum(HORIZONTAL_SWING_DIRECTION_OPTIONS, upper=True)),
            cv.Optional(CONF_SUPPORTED_PRESETS,default=["NONE","ECO","SLEEP","COMFORT",],): cv.ensure_list(cv.enum(SUPPORTED_CLIMATE_PRESETS_OPTIONS, upper=True)),
            cv.Optional(CONF_SUPPORTED_SWING_MODES,default=["OFF","VERTICAL","HORIZONTAL","BOTH",],): cv.ensure_list(cv.enum(SUPPORTED_SWING_MODES_OPTIONS, upper=True)),
            cv.Optional(CONF_SUPPORTED_MODES,default=["OFF","AUTO","COOL","HEAT","DRY","FAN_ONLY",],): cv.ensure_list(cv.enum(SUPPORTED_CLIMATE_MODES_OPTIONS, upper=True)),
            cv.Optional(CONF_SUPPORTED_FAN_MODES,default=["AUTO","QUIET","LOW","MIDDLE","MEDIUM","HIGH","FOCUS","DIFFUSE",],): cv.ensure_list(cv.enum(SUPPORTED_FAN_MODES_OPTIONS, upper=True)),
            cv.Optional(CONF_DONGLE_UART_ID): cv.use_id(uart.UARTComponent),
            cv.Optional(CONF_PROXY_MODE, default=False): cv.boolean,
            cv.Optional(CONF_A5_ACK_ONLY, default=False): cv.boolean,
            cv.Optional(CONF_PROXY_LOG_PACKETS, default=False): cv.boolean,
            cv.Optional(CONF_PROXY_IDLE_TIME, default="150ms"): cv.positive_time_period_milliseconds,
            cv.Optional(CONF_PROXY_FALLBACK_POLL_INTERVAL, default="15s"): cv.positive_time_period_milliseconds,
            cv.Optional(CONF_ERROR_CODE): sensor.sensor_schema(
                accuracy_decimals=0,
                entity_category="diagnostic",
            ),
            cv.Optional(CONF_ROOM_TEMPERATURE): sensor.sensor_schema(
                unit_of_measurement="°C",
                accuracy_decimals=1,
                device_class="temperature",
                state_class="measurement",
            ),
            cv.Optional(CONF_INDOOR_COIL_TEMPERATURE): sensor.sensor_schema(
                unit_of_measurement="°C",
                accuracy_decimals=1,
                device_class="temperature",
                state_class="measurement",
            ),
            cv.Optional(CONF_INDOOR_COIL_RAW): sensor.sensor_schema(
                accuracy_decimals=0,
                entity_category="diagnostic",
            ),
            cv.Optional(CONF_PIPE_OUT_TEMPERATURE): sensor.sensor_schema(
                unit_of_measurement="°C", accuracy_decimals=0,
                device_class="temperature", state_class="measurement",
                entity_category="diagnostic",
            ),
            cv.Optional(CONF_PIPE_IN_TEMPERATURE): sensor.sensor_schema(
                unit_of_measurement="°C", accuracy_decimals=0,
                device_class="temperature", state_class="measurement",
                entity_category="diagnostic",
            ),
            cv.Optional(CONF_OUTDOOR_AMBIENT_CANDIDATE): sensor.sensor_schema(
                unit_of_measurement="°C", accuracy_decimals=0,
                device_class="temperature", state_class="measurement",
            ),
            cv.Optional(CONF_OUTDOOR_EXHAUST_CANDIDATE): sensor.sensor_schema(
                unit_of_measurement="°C", accuracy_decimals=0,
                device_class="temperature", state_class="measurement",
                entity_category="diagnostic",
            ),
            cv.Optional(CONF_OUTDOOR_CONDENSER_CANDIDATE): sensor.sensor_schema(
                unit_of_measurement="°C", accuracy_decimals=0,
                device_class="temperature", state_class="measurement",
                entity_category="diagnostic",
            ),
            cv.Optional(CONF_COMPRESSOR_FREQUENCY_CANDIDATE): sensor.sensor_schema(
                unit_of_measurement="Hz", accuracy_decimals=0,
                device_class="frequency", state_class="measurement",
                entity_category="diagnostic",
            ),
            cv.Optional(CONF_EXTERNAL_TEMPERATURE_CANDIDATE_RAW_37): sensor.sensor_schema(
                accuracy_decimals=0, entity_category="diagnostic",
            ),
            cv.Optional(CONF_EXTERNAL_TEMPERATURE_CANDIDATE_RAW_38): sensor.sensor_schema(
                accuracy_decimals=0, entity_category="diagnostic",
            ),
            cv.Optional(CONF_COMPRESSOR_CURRENT): sensor.sensor_schema(
                unit_of_measurement="A", accuracy_decimals=1,
                device_class="current", state_class="measurement",
            ),
            cv.Optional(CONF_COMPRESSOR_STATE): sensor.sensor_schema(
                accuracy_decimals=0, entity_category="diagnostic",
            ),
            cv.Optional(CONF_AIRMAX_FAULT_CODE): sensor.sensor_schema(
                accuracy_decimals=0, entity_category="diagnostic",
            ),
            cv.Optional(CONF_RAW_BYTE_45): sensor.sensor_schema(
                accuracy_decimals=0, entity_category="diagnostic",
            ),
            cv.Optional(CONF_ACTIVE_SUPPLY_VOLTAGE): sensor.sensor_schema(
                unit_of_measurement="V", accuracy_decimals=0,
                device_class="voltage", state_class="measurement",
            ),
            cv.Optional(CONF_A5_INPUT_POWER): sensor.sensor_schema(
                unit_of_measurement="W", accuracy_decimals=0,
                device_class="power", state_class="measurement",
            ),
            cv.Optional(CONF_A5_COMPRESSOR_ACTUAL): sensor.sensor_schema(
                unit_of_measurement="%", accuracy_decimals=0,
                state_class="measurement",
            ),
            cv.Optional(CONF_A5_COMPRESSOR_TARGET): sensor.sensor_schema(
                unit_of_measurement="%", accuracy_decimals=0,
                state_class="measurement",
            ),
            cv.Optional(CONF_OUTSIDE_MOTOR): sensor.sensor_schema(
                accuracy_decimals=0, entity_category="diagnostic",
            ),
            cv.Optional(CONF_VERTICAL_VANE_POSITION): sensor.sensor_schema(
                accuracy_decimals=0, entity_category="diagnostic",
            ),
            cv.Optional(CONF_HORIZONTAL_VANE_POSITION): sensor.sensor_schema(
                accuracy_decimals=0, entity_category="diagnostic",
            ),
        }
    )
    .extend(uart.UART_DEVICE_SCHEMA)
    .extend(cv.COMPONENT_SCHEMA),
    validate_visual,
    validate_proxy,
)

ForceOnAction = tclac_ns.class_("ForceOnAction", automation.Action)
ForceOffAction = tclac_ns.class_("ForceOffAction", automation.Action)
BeeperOnAction = tclac_ns.class_("BeeperOnAction", automation.Action)
BeeperOffAction = tclac_ns.class_("BeeperOffAction", automation.Action)
DisplayOnAction = tclac_ns.class_("DisplayOnAction", automation.Action)
DisplayOffAction = tclac_ns.class_("DisplayOffAction", automation.Action)
ModuleDisplayOnAction = tclac_ns.class_("ModuleDisplayOnAction", automation.Action)
VerticalAirflowAction = tclac_ns.class_("VerticalAirflowAction", automation.Action)
ModuleDisplayOffAction = tclac_ns.class_("ModuleDisplayOffAction", automation.Action)
HorizontalAirflowAction = tclac_ns.class_("HorizontalAirflowAction", automation.Action)
VerticalSwingDirectionAction = tclac_ns.class_("VerticalSwingDirectionAction", automation.Action)
HorizontalSwingDirectionAction = tclac_ns.class_("HorizontalSwingDirectionAction", automation.Action)

TCLAC_ACTION_BASE_SCHEMA = automation.maybe_simple_id({cv.GenerateID(CONF_ID): cv.use_id(tclacClimate),})

# Регистрация событий включения и отключения дисплея кондиционера
@automation.register_action(
    "climate.tclac.display_on", DisplayOnAction, cv.Schema, synchronous=True
)
@automation.register_action(
    "climate.tclac.display_off", DisplayOffAction, cv.Schema, synchronous=True
)
async def display_action_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, paren)
    return var

# Регистрация событий включения и отключения пищалки кондиционера
@automation.register_action(
    "climate.tclac.beeper_on", BeeperOnAction, cv.Schema, synchronous=True
)
@automation.register_action(
    "climate.tclac.beeper_off", BeeperOffAction, cv.Schema, synchronous=True
)
async def beeper_action_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, paren)
    return var

# Регистрация событий включения и отключения светодиодов связи модуля
@automation.register_action(
    "climate.tclac.module_display_on", ModuleDisplayOnAction, cv.Schema, synchronous=True
)
@automation.register_action(
    "climate.tclac.module_display_off", ModuleDisplayOffAction, cv.Schema, synchronous=True
)
async def module_display_action_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, paren)
    return var
    
# Регистрация событий включения и отключения принудительного применения настроек
@automation.register_action(
    "climate.tclac.force_mode_on", ForceOnAction, cv.Schema, synchronous=True
)
@automation.register_action(
    "climate.tclac.force_mode_off", ForceOffAction, cv.Schema, synchronous=True
)
async def force_mode_action_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, paren)
    return var

# Регистрация события установки вертикальной фиксации заслонки
@automation.register_action(
    "climate.tclac.set_vertical_airflow",
    VerticalAirflowAction,
    cv.Schema(
        {
            cv.GenerateID(): cv.use_id(tclacClimate),
            cv.Required(CONF_VERTICAL_AIRFLOW): cv.templatable(cv.enum(AIRFLOW_VERTICAL_DIRECTION_OPTIONS, upper=True)),
        }
    ),
    synchronous=True,
)
async def tclac_set_vertical_airflow_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, paren)
    template_ = await cg.templatable(
        config[CONF_VERTICAL_AIRFLOW], args, AirflowVerticalDirection
    )
    cg.add(var.set_direction(template_))
    return var


# Регистрация события установки горизонтальной фиксации заслонок
@automation.register_action(
    "climate.tclac.set_horizontal_airflow",
    HorizontalAirflowAction,
    cv.Schema(
        {
            cv.GenerateID(): cv.use_id(tclacClimate),
            cv.Required(CONF_HORIZONTAL_AIRFLOW): cv.templatable(cv.enum(AIRFLOW_HORIZONTAL_DIRECTION_OPTIONS, upper=True)),
        }
    ),
    synchronous=True,
)
async def tclac_set_horizontal_airflow_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, paren)
    template_ = await cg.templatable(config[CONF_HORIZONTAL_AIRFLOW], args, AirflowHorizontalDirection)
    cg.add(var.set_direction(template_))
    return var


# Регистрация события установки вертикального качания шторки
@automation.register_action(
    "climate.tclac.set_vertical_swing_direction",
    VerticalSwingDirectionAction,
    cv.Schema(
        {
            cv.GenerateID(): cv.use_id(tclacClimate),
            cv.Required(CONF_VERTICAL_SWING_MODE): cv.templatable(cv.enum(VERTICAL_SWING_DIRECTION_OPTIONS, upper=True)),
        }
    ),
    synchronous=True,
)
async def tclac_set_vertical_swing_direction_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, paren)
    template_ = await cg.templatable(config[CONF_VERTICAL_SWING_MODE], args, VerticalSwingDirection)
    cg.add(var.set_swing_direction(template_))
    return var


# Регистрация события установки горизонтального качания шторок
@automation.register_action(
    "climate.tclac.set_horizontal_swing_direction",
    HorizontalSwingDirectionAction,
    cv.Schema(
        {
            cv.GenerateID(): cv.use_id(tclacClimate),
            cv.Required(CONF_HORIZONTAL_SWING_MODE): cv.templatable(cv.enum(HORIZONTAL_SWING_DIRECTION_OPTIONS, upper=True)),
        }
    ),
    synchronous=True,
)
async def tclac_set_horizontal_swing_direction_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, paren)
    template_ = await cg.templatable(config[CONF_HORIZONTAL_SWING_MODE], args, HorizontalSwingDirection)
    cg.add(var.set_swing_direction(template_))
    return var


# Добавление конфигурации в код
def to_code(config):
    cg.add_define("USE_API_HOMEASSISTANT_STATES")
    var = cg.new_Pvariable(config[CONF_ID])
    yield cg.register_component(var, config)
    yield uart.register_uart_device(var, config)
    yield climate.register_climate(var, config)

    if CONF_DONGLE_UART_ID in config:
        dongle_uart = yield cg.get_variable(config[CONF_DONGLE_UART_ID])
        cg.add(var.set_dongle_uart(dongle_uart))
    cg.add(var.set_proxy_mode(config[CONF_PROXY_MODE]))
    cg.add(var.set_a5_ack_only(config[CONF_A5_ACK_ONLY]))
    cg.add(var.set_proxy_log_packets(config[CONF_PROXY_LOG_PACKETS]))
    cg.add(var.set_proxy_idle_time(config[CONF_PROXY_IDLE_TIME].total_milliseconds))
    cg.add(var.set_proxy_fallback_poll_interval(config[CONF_PROXY_FALLBACK_POLL_INTERVAL].total_milliseconds))

    if CONF_BEEPER in config:
        cg.add(var.set_beeper_state(config[CONF_BEEPER]))
    if CONF_DISPLAY in config:
        cg.add(var.set_display_state(config[CONF_DISPLAY]))
    if CONF_FORCE_MODE in config:
        cg.add(var.set_force_mode_state(config[CONF_FORCE_MODE]))
    if CONF_SUPPORTED_MODES in config:
        cg.add(var.set_supported_modes(config[CONF_SUPPORTED_MODES]))
    if CONF_SUPPORTED_PRESETS in config:
        cg.add(var.set_supported_presets(config[CONF_SUPPORTED_PRESETS]))
    if CONF_MODULE_DISPLAY in config:
        cg.add(var.set_module_display_state(config[CONF_MODULE_DISPLAY]))
    if CONF_SUPPORTED_FAN_MODES in config:
        cg.add(var.set_supported_fan_modes(config[CONF_SUPPORTED_FAN_MODES]))
    if CONF_SUPPORTED_SWING_MODES in config:
        cg.add(var.set_supported_swing_modes(config[CONF_SUPPORTED_SWING_MODES]))

    if CONF_ERROR_CODE in config:
        sens = yield sensor.new_sensor(config[CONF_ERROR_CODE])
        cg.add(var.set_error_code_sensor(sens))
    if CONF_ROOM_TEMPERATURE in config:
        sens = yield sensor.new_sensor(config[CONF_ROOM_TEMPERATURE])
        cg.add(var.set_room_temperature_sensor(sens))
    if CONF_INDOOR_COIL_TEMPERATURE in config:
        sens = yield sensor.new_sensor(config[CONF_INDOOR_COIL_TEMPERATURE])
        cg.add(var.set_indoor_coil_temperature_sensor(sens))
    if CONF_INDOOR_COIL_RAW in config:
        sens = yield sensor.new_sensor(config[CONF_INDOOR_COIL_RAW])
        cg.add(var.set_indoor_coil_raw_sensor(sens))
    if CONF_PIPE_OUT_TEMPERATURE in config:
        sens = yield sensor.new_sensor(config[CONF_PIPE_OUT_TEMPERATURE])
        cg.add(var.set_pipe_out_temperature_sensor(sens))
    if CONF_PIPE_IN_TEMPERATURE in config:
        sens = yield sensor.new_sensor(config[CONF_PIPE_IN_TEMPERATURE])
        cg.add(var.set_pipe_in_temperature_sensor(sens))
    if CONF_OUTDOOR_AMBIENT_CANDIDATE in config:
        sens = yield sensor.new_sensor(config[CONF_OUTDOOR_AMBIENT_CANDIDATE])
        cg.add(var.set_outdoor_ambient_candidate_sensor(sens))
    if CONF_OUTDOOR_EXHAUST_CANDIDATE in config:
        sens = yield sensor.new_sensor(config[CONF_OUTDOOR_EXHAUST_CANDIDATE])
        cg.add(var.set_outdoor_exhaust_candidate_sensor(sens))
    if CONF_OUTDOOR_CONDENSER_CANDIDATE in config:
        sens = yield sensor.new_sensor(config[CONF_OUTDOOR_CONDENSER_CANDIDATE])
        cg.add(var.set_outdoor_condenser_candidate_sensor(sens))
    if CONF_COMPRESSOR_FREQUENCY_CANDIDATE in config:
        sens = yield sensor.new_sensor(config[CONF_COMPRESSOR_FREQUENCY_CANDIDATE])
        cg.add(var.set_compressor_frequency_candidate_sensor(sens))
    if CONF_EXTERNAL_TEMPERATURE_CANDIDATE_RAW_37 in config:
        sens = yield sensor.new_sensor(config[CONF_EXTERNAL_TEMPERATURE_CANDIDATE_RAW_37])
        cg.add(var.set_external_temperature_candidate_raw_37_sensor(sens))
    if CONF_EXTERNAL_TEMPERATURE_CANDIDATE_RAW_38 in config:
        sens = yield sensor.new_sensor(config[CONF_EXTERNAL_TEMPERATURE_CANDIDATE_RAW_38])
        cg.add(var.set_external_temperature_candidate_raw_38_sensor(sens))
    if CONF_COMPRESSOR_CURRENT in config:
        sens = yield sensor.new_sensor(config[CONF_COMPRESSOR_CURRENT])
        cg.add(var.set_compressor_current_sensor(sens))
    if CONF_COMPRESSOR_STATE in config:
        sens = yield sensor.new_sensor(config[CONF_COMPRESSOR_STATE])
        cg.add(var.set_compressor_state_sensor(sens))
    if CONF_AIRMAX_FAULT_CODE in config:
        sens = yield sensor.new_sensor(config[CONF_AIRMAX_FAULT_CODE])
        cg.add(var.set_airmax_fault_code_sensor(sens))
    if CONF_RAW_BYTE_45 in config:
        sens = yield sensor.new_sensor(config[CONF_RAW_BYTE_45])
        cg.add(var.set_raw_byte_45_sensor(sens))
    if CONF_ACTIVE_SUPPLY_VOLTAGE in config:
        sens = yield sensor.new_sensor(config[CONF_ACTIVE_SUPPLY_VOLTAGE])
        cg.add(var.set_active_supply_voltage_sensor(sens))
    if CONF_A5_INPUT_POWER in config:
        sens = yield sensor.new_sensor(config[CONF_A5_INPUT_POWER])
        cg.add(var.set_a5_input_power_sensor(sens))
    if CONF_A5_COMPRESSOR_ACTUAL in config:
        sens = yield sensor.new_sensor(config[CONF_A5_COMPRESSOR_ACTUAL])
        cg.add(var.set_a5_compressor_actual_sensor(sens))
    if CONF_A5_COMPRESSOR_TARGET in config:
        sens = yield sensor.new_sensor(config[CONF_A5_COMPRESSOR_TARGET])
        cg.add(var.set_a5_compressor_target_sensor(sens))
    if CONF_OUTSIDE_MOTOR in config:
        sens = yield sensor.new_sensor(config[CONF_OUTSIDE_MOTOR])
        cg.add(var.set_outside_motor_sensor(sens))
    if CONF_VERTICAL_VANE_POSITION in config:
        sens = yield sensor.new_sensor(config[CONF_VERTICAL_VANE_POSITION])
        cg.add(var.set_vertical_vane_position_sensor(sens))
    if CONF_HORIZONTAL_VANE_POSITION in config:
        sens = yield sensor.new_sensor(config[CONF_HORIZONTAL_VANE_POSITION])
        cg.add(var.set_horizontal_vane_position_sensor(sens))

    if CONF_TX_LED in config:
        cg.add_define("CONF_TX_LED")
        tx_led_pin = yield cg.gpio_pin_expression(config[CONF_TX_LED])
        cg.add(var.set_tx_led_pin(tx_led_pin))
    if CONF_RX_LED in config:
        cg.add_define("CONF_RX_LED")
        rx_led_pin = yield cg.gpio_pin_expression(config[CONF_RX_LED])
        cg.add(var.set_rx_led_pin(rx_led_pin))
