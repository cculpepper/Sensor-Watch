#define BME280_ADDRESS (0x77)
#define BME280_SOFT_RESET_CODE (0xB6)
#define BME280_STATUS_UPDATING_MASK (1 << 3)

typedef enum BME280Register {
    BME280_REGISTER_DIG_T1 = 0x88,
    BME280_REGISTER_DIG_T2 = 0x8A,
    BME280_REGISTER_DIG_T3 = 0x8C,

    BME280_REGISTER_DIG_P1 = 0x8E,
    BME280_REGISTER_DIG_P2 = 0x90,
    BME280_REGISTER_DIG_P3 = 0x92,
    BME280_REGISTER_DIG_P4 = 0x94,
    BME280_REGISTER_DIG_P5 = 0x96,
    BME280_REGISTER_DIG_P6 = 0x98,
    BME280_REGISTER_DIG_P7 = 0x9A,
    BME280_REGISTER_DIG_P8 = 0x9C,
    BME280_REGISTER_DIG_P9 = 0x9E,

    BME280_REGISTER_DIG_H1 = 0xA1,
    BME280_REGISTER_DIG_H2 = 0xE1,
    BME280_REGISTER_DIG_H3 = 0xE3,
    BME280_REGISTER_DIG_H4 = 0xE4,
    BME280_REGISTER_DIG_H5 = 0xE5,
    BME280_REGISTER_DIG_H6 = 0xE7,

    BME280_REGISTER_CHIPID = 0xD0,
    BME280_REGISTER_VERSION = 0xD1,
    BME280_REGISTER_SOFTRESET = 0xE0,

    BME280_REGISTER_CAL26 = 0xE1,

    BME280_REGISTER_CONTROL_HUMID = 0xF2,
    BME280_REGISTER_STATUS = 0XF3,
    BME280_REGISTER_CONTROL = 0xF4,
    BME280_REGISTER_CONFIG = 0xF5,
    BME280_REGISTER_PRESSURE_DATA = 0xF7,
    BME280_REGISTER_TEMP_DATA = 0xFA,
    BME280_REGISTER_HUMID_DATA = 0xFD
} BME280Register;

typedef enum BME280Control {
    BME280_CONTROL_MODE_SLEEP = 0b00,
    BME280_CONTROL_MODE_FORCED = 0b01,
    BME280_CONTROL_MODE_NORMAL = 0b11,
    BME280_CONTROL_PRESSURE_SAMPLING_NONE = 0b000 << 2,
    BME280_CONTROL_PRESSURE_SAMPLING_X1 = 0b001 << 2,
    BME280_CONTROL_PRESSURE_SAMPLING_X2 = 0b010 << 2,
    BME280_CONTROL_PRESSURE_SAMPLING_X4 = 0b011 << 2,
    BME280_CONTROL_PRESSURE_SAMPLING_X8 = 0b100 << 2,
    BME280_CONTROL_PRESSURE_SAMPLING_X16 = 0b101 << 2,
    BME280_CONTROL_TEMPERATURE_SAMPLING_NONE = 0b000 << 5,
    BME280_CONTROL_TEMPERATURE_SAMPLING_X1 = 0b001 << 5,
    BME280_CONTROL_TEMPERATURE_SAMPLING_X2 = 0b010 << 5,
    BME280_CONTROL_TEMPERATURE_SAMPLING_X4 = 0b011 << 5,
    BME280_CONTROL_TEMPERATURE_SAMPLING_X8 = 0b100 << 5,
    BME280_CONTROL_TEMPERATURE_SAMPLING_X16 = 0b101 << 5
} BME280Control;

typedef enum BME280ControlHumidity {
    BME280_CONTROL_HUMID_SAMPLING_NONE = 0b000,
    BME280_CONTROL_HUMID_SAMPLING_X1 = 0b001,
    BME280_CONTROL_HUMID_SAMPLING_X2 = 0b010,
    BME280_CONTROL_HUMID_SAMPLING_X4 = 0b011,
    BME280_CONTROL_HUMID_SAMPLING_X8 = 0b100,
    BME280_CONTROL_HUMID_SAMPLING_X16 = 0b101
} BME280ControlHumidity;

typedef enum BME280Filter {
    BME280_CONFIG_FILTER_OFF = 0b000 << 2,
    BME280_CONFIG_FILTER_X2 = 0b001 << 2,
    BME280_CONFIG_FILTER_X4 = 0b010 << 2,
    BME280_CONFIG_FILTER_X8 = 0b011 << 2,
    BME280_CONFIG_FILTER_X16 = 0b10 << 2,
    BME280_CONFIG_STANDBY_MS_0_5 = 0b000 << 5,
    BME280_CONFIG_STANDBY_MS_10 = 0b110 << 5,
    BME280_CONFIG_STANDBY_MS_20 = 0b111 << 5,
    BME280_CONFIG_STANDBY_MS_62_5 = 0b001 << 5,
    BME280_CONFIG_STANDBY_MS_125 = 0b010 << 5,
    BME280_CONFIG_STANDBY_MS_250 = 0b011 << 5,
    BME280_CONFIG_STANDBY_MS_500 = 0b100 << 5,
    BME280_CONFIG_STANDBY_MS_1000 = 0b101 << 5
} BME280Filter;

inline uint16_t make_le_16(uint16_t val) { return (val >> 8) | (val << 8); }
