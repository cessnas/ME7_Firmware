/**
 * @file	rusefi_enums.h
 * @brief	Fundamental rusEfi enumerable types live here
 *
 * @note this file should probably not include any other files
 *
 * @date Jan 14, 2014
 * @author Andrey Belomutskiy, (c) 2012-2020
 */

#pragma once

#include "efifeatures.h"
#include "obd_error_codes.h"
// we do not want to start the search for header from current folder so we use brackets here
// https://stackoverflow.com/questions/21593/what-is-the-difference-between-include-filename-and-include-filename
#include <rusefi_hw_enums.h>

// I believe that TunerStudio curve editor has a bug with F32 support
// because of that bug we cannot have '1.05' for 5% extra multiplier
/**
 * *0.01 because of https://sourceforge.net/p/rusefi/tickets/153/
 */

#define PERCENT_MULT 100.0f
#define PERCENT_DIV 0.01f

/**
 * http://rusefi.com/wiki/index.php?title=Manual:Engine_Type
 */
typedef enum {
	MINIMAL_PINS = 99,
	Force_4_bytes_size_engine_type = ENUM_32_BITS,
} engine_type_e;


/**
 * @see http://rusefi.com/wiki/index.php?title=Manual:Software:Trigger
 */
typedef enum {
	TT_TOOTHED_WHEEL = 0,
	TT_FORD_ASPIRE = 1,
	TT_DODGE_NEON_1995 = 2,
	/**
	 * https://rusefi.com/wiki/index.php?title=Manual:Software:Trigger#Mazda_Miata_NA
	 */
	TT_MAZDA_MIATA_NA = 3,
	/**
	 * NB1 means non-VVT NB, 99 and 00 1.8 engine
	 */
	TT_MAZDA_MIATA_NB1 = 4,
	TT_GM_7X = 5,
	TT_MINI_COOPER_R50 = 6,
	TT_MAZDA_SOHC_4 = 7,
	/**
	 * "60/2"
	 * See also TT_ONE_PLUS_TOOTHED_WHEEL_60_2
	 */
	TT_TOOTHED_WHEEL_60_2 = 8,
	TT_TOOTHED_WHEEL_36_1 = 9,

	TT_HONDA_4_24_1 = 10,

	TT_MITSUBISHI = 11,

	// this makes sense because mechanical spark distribution does not require synchronization
	TT_HONDA_4_24 = 12,

	TT_HONDA_1_4_24 = 13,

	// cam-based
	TT_DODGE_NEON_2003_CAM = 14,

	TT_MAZDA_DOHC_1_4 = 15,

	/**
	 * "1+1" - one tooth on primary channel, one tooth on secondary channel
	 * this trigger is used only by unit tests
	 * see also TT_ONE a bit below
	 */
	TT_ONE_PLUS_ONE = 16,
	// "1+60/2"
	TT_VVT_JZ = 17,
	// just one channel with just one tooth
	TT_ONE = 18,

	TT_DODGE_RAM = 19,
	/**
	 * It looks like this is the VR shape if you have your wires flipped
	 */
	TT_60_2_VW = 20,

	TT_HONDA_1_24 = 21,

	TT_DODGE_STRATUS = 22,

	TT_36_2_2_2 = 23,

	/**
	 * only the 4 tooth signal, without the 360 signal
	 * 8,2,2,2 Nissan pattern
	 * See also TT_NISSAN_SR20VE_360
	 */
	TT_NISSAN_SR20VE = 24,

	TT_2JZ_3_34 = 25,

	TT_ROVER_K = 26,

	TT_GM_LS_24 = 27,

	TT_HONDA_CBR_600 = 28,

	TT_2JZ_1_12 = 29,

	TT_HONDA_CBR_600_CUSTOM = 30,

	// skipped 3/1 with cam sensor for testing
	TT_3_1_CAM = 31,

	// crank-based in case your cam is broken
	TT_DODGE_NEON_2003_CRANK = 32,

	/**
	 * this takes care of crank sensor, VVT sensor should be configured separately
	 * for VVT simulated trigger signal we have https://github.com/rusefi/rusefi/issues/566 gap
	 * See also TT_MAZDA_MIATA_VVT_TEST
	 */
	TT_MIATA_VVT = 33,

	/**
	 * This is a different version of TT_HONDA_ACCORD_1_24
	 * See https://sourceforge.net/p/rusefi/tickets/319/
	 */
	TT_HONDA_ACCORD_1_24_SHIFTED = 34,

	/**
	 * a version of NB1 with shifted CAM, useful for VVT testing & development
	 */
	TT_MAZDA_MIATA_VVT_TEST = 35,

	TT_SUBARU_7_6 = 36,

	// this one is 6 cylinder, see TT_JEEP_4_cyl for 4 cylinders
	TT_JEEP_18_2_2_2 = 37,

	/*
	 * See also TT_NISSAN_SR20VE
	 */
	TT_NISSAN_SR20VE_360 = 38,

	TT_DODGE_NEON_1995_ONLY_CRANK = 39,

	// Jeep XJ 2500cc 4 cylinder. See also TT_JEEP_18_2_2_2 for 6 cylinders
	TT_JEEP_4_CYL = 40,

	// magneti marelli Fiat/Lancia IAW P8 from the 90', 2.0 16 v turbo engine - Lancia Coupe
	// https://rusefi.com/forum/viewtopic.php?f=5&t=1440
	TT_FIAT_IAW_P8 = 41,

	TT_MAZDA_Z5 = 42,

	/**
	 * cam sensor of Mazda Miata NB2 - the VVT signal shape
	 */
	TT_VVT_MIATA_NB2 = 43,

	TT_RENIX_44_2_2 = 44,

	/**
	 * Same as TT_RENIX_44_2_2 but repeated three times, not two.
	 */
	TT_RENIX_66_2_2_2 = 45,

	TT_HONDA_K_12_1 = 46,

	TT_VVT_BOSCH_QUICK_START = 47,

	TT_TOOTHED_WHEEL_36_2 = 48,

	TT_SUBARU_SVX = 49,

	// do not forget to edit "#define trigger_type_e_enum" line in integration/rusefi_config.txt file to propogate new value to rusefi.ini TS project
	// do not forget to invoke "gen_config.bat" once you make changes to integration/rusefi_config.txt
	// todo: one day a hero would integrate some of these things into Makefile in order to reduce manual magic
	//
	// Another point: once you add a new trigger, run get_trigger_images.bat which would run rusefi_test.exe from unit_tests
	//
	TT_UNUSED = 50, // this is used if we want to iterate over all trigger types

	Force_4_bytes_size_trigger_type = ENUM_32_BITS,
} trigger_type_e;

typedef enum {
	ADC_OFF = 0,
	ADC_SLOW = 1,
	ADC_FAST = 2,

	Force_4_bytes_size_adc_channel_mode = ENUM_32_BITS,
} adc_channel_mode_e;

typedef enum {
	TV_FALL = 0,
	TV_RISE = 1
} trigger_value_e;

// see also PWM_PHASE_MAX_WAVE_PER_PWM
// todo: better names?
typedef enum {
	T_PRIMARY = 0,
	T_SECONDARY = 1,
	// todo: I really do not want to call this 'tertiary'. maybe we should rename all of these?
	T_CHANNEL_3 = 2,
	T_NONE = 15
} trigger_wheel_e;

// see also 'HW_EVENT_TYPES'
typedef enum {
	SHAFT_PRIMARY_FALLING = 0,
	SHAFT_PRIMARY_RISING = 1,
	SHAFT_SECONDARY_FALLING = 2,
	SHAFT_SECONDARY_RISING = 3,
	SHAFT_3RD_FALLING = 4,
	SHAFT_3RD_RISING = 5,
} trigger_event_e;

typedef enum {
	/**
	 * This mode is useful for troubleshooting and research - events are logged but no effects on phase synchronization
	 */
	VVT_INACTIVE = 0,

	/**
	 * Single-tooth cam sensor mode where TDC and cam signal happen in opposite 360 degree of 720 degree engine cycle
	 */
	VVT_SECOND_HALF = 1,
	/**
	 * Toyota 2JZ has three cam tooth. We pick one of these three tooth to synchronize based on the expected angle position of the event
	 */
	VVT_2JZ = 2,
	/**
	 * Mazda NB2 has three cam tooth. We synchronize based on gap ratio.
	 */
	MIATA_NB2 = 3,

	/**
	 * Single-tooth cam sensor mode where TDC and cam signal happen in the same 360 degree of 720 degree engine cycle
	 */
	VVT_FIRST_HALF = 4,
	/**
	 * @see TT_VVT_BOSCH_QUICK_START
	 */
	VVT_BOSCH_QUICK_START = 5,

	Force_4_bytes_size_vvt_mode = ENUM_32_BITS,
} vvt_mode_e;

/**
 * This enum is used to select your desired Engine Load calculation algorithm
 */
typedef enum {
	/**
	 * Speed Density algorithm - Engine Load is a function of MAP, VE and target AFR
	 * http://articles.sae.org/8539/
	 */
	LM_SPEED_DENSITY = 3,

	/**
	 * MAF with a known kg/hour function
	 */
	LM_REAL_MAF = 4,

	// todo: rename after LM_ALPHA_N is removed
	LM_ALPHA_N_2 = 5,

	Force_4_bytes_size_engine_load_mode = ENUM_32_BITS,
} engine_load_mode_e;


typedef enum  __attribute__ ((__packed__)){
	TL_AUTO = 0,
	TL_SEMI_AUTO = 1,
	TL_MANUAL = 2,
	TL_HALL = 3,

} tle8888_mode_e;


typedef enum {
	/**
	 * In auto mode we currently have some pid-like-but-not really PID logic which is trying
	 * to get idle RPM to desired value by dynamically adjusting idle valve position.
	 * TODO: convert to PID
	 */
	IM_AUTO = 0,
	/**
	 * Manual idle control is extremely simple: user just specifies desired idle valve position
	 * which could be adjusted according to current CLT
	 */
	IM_MANUAL = 1,
	Force_4_bytes_size_idle_mode = ENUM_32_BITS,
} idle_mode_e;

typedef enum __attribute__ ((__packed__)) {
	/**
	 * GND for logical OFF, VCC for logical ON
	 */
	OM_DEFAULT = 0,
	/**
	 * GND for logical ON, VCC for logical OFF
	 */
	OM_INVERTED = 1,
	/**
	 * logical OFF is floating, logical ON is GND
	 */
	OM_OPENDRAIN = 2,
	OM_OPENDRAIN_INVERTED = 3
} pin_output_mode_e;

typedef enum __attribute__ ((__packed__)) {
	PI_DEFAULT = 0,
	PI_PULLUP = 1,
	PI_PULLDOWN = 2
} pin_input_mode_e;

#define CRANK_MODE_MULTIPLIER 2.0f

// todo: better enum name
typedef enum {
	OM_NONE = 0,
	/**
	 * 720 degree engine cycle but trigger is defined using a 360 cycle which is when repeated.
	 * For historical reasons we have a pretty weird approach where one crank trigger revolution is
	 * defined as if it's stretched to 720 degress. See CRANK_MODE_MULTIPLIER
	 */
	FOUR_STROKE_CRANK_SENSOR = 1,
	/**
	 * 720 degree engine and trigger cycle
	 */
	FOUR_STROKE_CAM_SENSOR = 2,
	/**
	 * 360 degree cycle
	 */
	TWO_STROKE = 3,

	/**
	 * 720 degree engine cycle but trigger is defined using a 180 cycle which is when repeated three more times
	 * In other words, same pattern is repeatet on the crank wheel twice.
	 */
	FOUR_STROKE_SYMMETRICAL_CRANK_SENSOR = 4,

	/**
	 * Same pattern repeated three times on crank wheel. Crazy, I know!
	 */
	FOUR_STROKE_THREE_TIMES_CRANK_SENSOR = 5,

	Force_4_bytes_size_operation_mode_e = ENUM_32_BITS,
} operation_mode_e;

/**
 * @brief Ignition Mode
 */
typedef enum {
	/**
	 * in this mode only SPARKOUT_1_OUTPUT is used
	 */
	IM_ONE_COIL = 0,
	/**
	 * in this mode we use as many coils as we have cylinders
	 */
	IM_INDIVIDUAL_COILS = 1,
	IM_WASTED_SPARK = 2,

	/**
	 * some v12 engines line BMW M70 and M73 run two distributors, one for each bank of cylinders
	 */
	IM_TWO_COILS = 3,

	Force_4_bytes_size_ignition_mode = ENUM_32_BITS,
} ignition_mode_e;

/**
 * @see getNumberOfInjections
 */
typedef enum {
	/**
	 * each cylinder has it's own injector but they all works in parallel
	 */
	IM_SIMULTANEOUS = 0,
	/**
	 * each cylinder has it's own injector, each injector is wired separately
	 */
	IM_SEQUENTIAL = 1,
	/**
	 * each cylinder has it's own injector but these injectors work in pairs. Injectors could be wired in pairs or separately.
	 * Each pair is fired once per engine cycle
	 * todo: we might want to implement one additional mode where each pair of injectors is floating twice per engine cycle.
	 * todo: this could reduce phase offset from injection to stroke but would not work great for large injectors
	 */
	IM_BATCH = 2,
	/**
	 * only one injector located in throttle body
	 */
	IM_SINGLE_POINT = 3,


	Force_4_bytes_size_injection_mode = ENUM_32_BITS,
} injection_mode_e;

/**
 * @brief Ignition Mode while cranking
 */
typedef enum {
	CIM_DEFAULT = 0,
	CIM_FIXED_ANGLE = 1,

	// todo: make this a one byte enum
	Force_4_bytes_size_cranking_ignition_mode = ENUM_32_BITS,
} cranking_ignition_mode_e;

typedef enum __attribute__ ((__packed__)) {
	UART_NONE = 0,
	UART_DEVICE_1 = 1,
	UART_DEVICE_2 = 2,
	UART_DEVICE_3 = 3,
	UART_DEVICE_4 = 4,
} uart_device_e;

typedef enum __attribute__ ((__packed__)) {
	_5MHz,
	_2_5MHz,
	_1_25MHz,
	_150KHz
} spi_speed_e;


/**
 * See pinSpi3Mosi
 * See spi2MisoMode
 */
typedef enum __attribute__ ((__packed__)) {
	SPI_NONE = 0,
	SPI_DEVICE_1 = 1,
	SPI_DEVICE_2 = 2,
	SPI_DEVICE_3 = 3,
	SPI_DEVICE_4 = 4,
} spi_device_e;

typedef enum {
	BMW_e46 = 0,
	W202 = 1,
	VAG = 2,
	Force_4_bytes_size_can_vss_nbc_e = ENUM_32_BITS,
} can_vss_nbc_e;

typedef enum {
	MS_AUTO = 0,
	MS_ALWAYS = 1,
	MS_NEVER = 2,
	Force_4_bytes_size_mass_storage = ENUM_32_BITS,
} mass_storage_e;

typedef enum {
	ES_BPSX_D1 = 0,
	/**
	 * same as innovate LC2
	 * 0v->7.35afr, 5v->22.39
	 */
	ES_Innovate_MTX_L = 1,
	/**
	 * Same as AEM
	 * 0v->10.0afr
	 * 5v->20.0afr
	 */
	ES_14Point7_Free = 2,

	ES_NarrowBand = 3,

	ES_PLX = 4,

	ES_Custom = 5,

	ES_AEM = 6,

	ES_CJ125 = 7,

	Force_4_bytes_size_ego_sensor = ENUM_32_BITS,
} ego_sensor_e;

typedef brain_pin_e output_pin_e;

/**
 * https://rusefi.com//wiki/index.php?title=Manual:Debug_fields
 */
typedef enum {
	DBG_ALTERNATOR_PID = 0,
	DBG_TPS_ACCEL = 1,
	DBG_2 = 2,
	DBG_IDLE_CONTROL = 3,
	DBG_EL_ACCEL = 4,
	DBG_TRIGGER_COUNTERS = 5,
	DBG_FSIO_ADC = 6,
	/**
	 * VVT valve control often uses AUX pid #1
	 */
	DBG_AUX_PID_1 = 7,
	/**
	 * VVT position debugging - not VVT valve control. See AUX pid #1 debug for valve position.
	 */
	DBG_VVT = 8,
	DBG_CRANKING_DETAILS = 9,
	DBG_IGNITION_TIMING = 10,
	DBG_FUEL_PID_CORRECTION = 11,
	DBG_VEHICLE_SPEED_SENSOR = 12,
	DBG_SD_CARD = 13,
	DBG_SR5_PROTOCOL = 14,
	DBG_KNOCK = 15,
	DBG_16 = 16,
	/**
	 * See also DBG_ELECTRONIC_THROTTLE_EXTRA
	 */
	DBG_ELECTRONIC_THROTTLE_PID = 17,
	DBG_EXECUTOR = 18,
	/**
	 * See tunerstudio.cpp
	 */
	DBG_BENCH_TEST = 19,
	DBG_AUX_VALVES = 20,
	/**
	 * ADC
	 * See also DBG_ANALOG_INPUTS2
	 */
	DBG_ANALOG_INPUTS = 21,
	
	DBG_INSTANT_RPM = 22,
	DBG_FSIO_EXPRESSION = 23,
	DBG_STATUS = 24,
	DBG_CJ125 = 25,
	DBG_CAN = 26,
	DBG_MAP = 27,
	DBG_METRICS = 28,
	DBG_ELECTRONIC_THROTTLE_EXTRA = 29,
	DBG_ION = 30,
	DBG_TLE8888 = 31,
	/**
	 * See also DBG_ANALOG_INPUTS
	 */
	DBG_ANALOG_INPUTS2 = 32,
	DBG_DWELL_METRIC = 33,
	DBG_34 = 34,
	DBG_ETB_LOGIC = 35,
	DBG_BOOST = 36,
	DBG_START_STOP = 37,
	DBG_LAUNCH = 38,
	DBG_ETB_AUTOTUNE = 39,
	DBG_COMPOSITE_LOG = 40,
	DBG_FRAM = 41,
	Force_4_bytes_size_debug_mode_e = ENUM_32_BITS,
} debug_mode_e;


typedef enum {
	REVERSE = -1,
	NEUTRAL = 0,
	GEAR_1 = 1,
	GEAR_2 = 2,
	GEAR_3 = 3,
	GEAR_4 = 4,

} gear_e;

typedef enum {
	CUSTOM = 0,
	Bosch0280218037 = 1,
	Bosch0280218004 = 2,
	DensoTODO = 3,
	Internal_ForceMyEnumIntSize_maf_sensor = ENUM_32_BITS,
} maf_sensor_type_e;

typedef enum {
	/**
	 * This is the default mode in which ECU controls timing dynamically
	 */
	TM_DYNAMIC = 0,
	/**
	 * Fixed timing is useful while you are playing with a timing gun - you need to have fixed
	 * timing if you want to install your distributor at some specific angle
	 */
	TM_FIXED = 1,

	Internal_ForceMyEnumIntSize_timing_mode = ENUM_32_BITS,
} timing_mode_e;

typedef enum {
    CS_OPEN = 0,
    CS_CLOSED = 1,
    CS_SWIRL_TUMBLE = 2,

	Internal_ForceMyEnumIntSize_chamber_stype = ENUM_32_BITS,
} chamber_style_e;

/**
 * Net Body Computer types
 */
typedef enum {
	CAN_BUS_NBC_NONE = 0,
	CAN_BUS_NBC_FIAT = 1,
	CAN_BUS_NBC_VAG = 2,
	CAN_BUS_MAZDA_RX8 = 3,
	CAN_BUS_NBC_BMW = 4,
	CAN_BUS_W202_C180 = 5,
    CAN_BUS_BMW_E90 = 6,
	Internal_ForceMyEnumIntSize_can_nbc = ENUM_32_BITS,
} can_nbc_e;


typedef enum {
	TCHARGE_MODE_RPM_TPS = 0,
	TCHARGE_MODE_AIR_INTERP = 1,
	Force_4bytes_size_tChargeMode_e = ENUM_32_BITS,
} tChargeMode_e;

// peak type
typedef enum {
  MINIMUM = -1,
  NOT_A_PEAK = 0,
  MAXIMUM = 1
} PidAutoTune_Peak;

// auto tuner state
typedef enum {
  AUTOTUNER_OFF = 0,
  STEADY_STATE_AT_BASELINE = 1,
  STEADY_STATE_AFTER_STEP_UP = 2,
  RELAY_STEP_UP = 4,
  RELAY_STEP_DOWN = 8,
  CONVERGED = 16,
  FAILED = 128
} PidAutoTune_AutoTunerState;

typedef enum {
	INIT = 0,
	TPS_THRESHOLD = 1,
	RPM_DEAD_ZONE = 2,
	PID_VALUE = 4,
	PID_UPPER = 16,
	BLIP = 64,
	/**
	 * Live Docs reads 4 byte value so we want 4 byte enum
	 */
	Force_4bytes_size_idle_state_e = ENUM_32_BITS,
} idle_state_e;

typedef enum {
	OPEN_LOOP = 0,
	CLOSED_LOOP = 1,
	Force_4bytes_size_boostType_e = ENUM_32_BITS,
} boostType_e;

typedef enum {
	ON_OFF_VVT = 0,
	CLOSED_LOOP_VVT = 1,
	Force_4bytes_size_vvtType_e = ENUM_32_BITS,
} vvtType_e;
typedef enum {
	VVT_LOAD_TPS = 0,
	VVT_LOAD_MAP = 1,
	VVT_LOAD_CLT = 2,
	Force_4bytes_size_vvtLoadAxis_e = ENUM_32_BITS,
} vvtLoadAxis_e;
typedef enum {
	SWITCH_INPUT_LAUNCH = 0,
	CLUTCH_INPUT_LAUNCH = 1,
	ALWAYS_ACTIVE_LAUNCH = 2,
	Force_4bytes_size_launchActivationMode_e = ENUM_32_BITS,
} launchActivationMode_e;

typedef enum {
	SWITCH_INPUT_ANTILAG = 0,
	ALWAYS_ON_ANTILAG = 1,
	Force_4bytes_size_antiLagActivationMode_e = ENUM_32_BITS,
} antiLagActivationMode_e;
typedef enum {
	SOLENOID = 0,
	IDLE_VALVE = 1,
	E_THROTTLE = 2,
	Force_4bytes_size_antiLagAirSupply_e = ENUM_32_BITS,
} antiLagAirSupply_e;

typedef enum __attribute__ ((__packed__)) {
	GPPWM_Tps = 0,
	GPPWM_Map = 1,
	GPPWM_Clt = 2,
	GPPWM_Iat = 3,
	GPPWM_FuelLoad = 4,
	GPPWM_IgnLoad = 5,
} gppwm_channel_e;

typedef enum __attribute__ ((__packed__)) {
	B100KBPS = 0, // 100kbps
	B250KBPS = 1, // 250kbps
	B500KBPS = 2, // 500kbps
	B1MBPS = 3, // 1Mbps
} can_baudrate_e;

typedef enum __attribute__ ((__packed__)) {
	GPPWM_GreaterThan = 0,
	GPPWM_LessThan = 1,
} gppwm_compare_mode_e;
typedef enum {
	F_1_US = 0,
	F_5_US = 1,
	F_10_US = 2,
	F_20_US = 3,
	Force_4bytes_size_tle8888filter_e = ENUM_32_BITS,
} tle8888filter_e;
typedef enum {
	V_50_MV = 0,
	V_150_MV = 1,
	V_350_MV = 2,
	V_550_MV = 3,
	Force_4bytes_size_tle8888detection_voltage_e = ENUM_32_BITS,
} tle8888detection_voltage_e;
typedef enum {
	T_10_US = 0,
	T_250_US = 1,
	Force_4bytes_size_tle8888vrpeak_time_e = ENUM_32_BITS,
} tle8888vrpeak_time_e;
typedef enum {
	TF_NONE = 0,
	TF_LEVEL_1 = 1,
	TF_LEVEL_2 = 2,
	TF_LEVEL_3 = 3,
	Force_4bytes_size_trigger_filter_e = ENUM_32_BITS,
} trigger_filter_e;
