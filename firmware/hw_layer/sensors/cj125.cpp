/*
 * @file CJ125.cpp
 *
 * https://github.com/rusefi/rusefi/tree/master/hardware/CJ125_board
 *
 * https://github.com/rusefi/hw_modular/tree/master/cj125_Module
 *
 *
 * See vag_18_Turbo for test configuration
 * set engine_type 102
 *
 * @date: Jun 24, 2016
 * @author Andrey Belomutskiy, (c) 2012-2020
 *
 */

#include "engine.h"
#include "cj125.h"
#include "pwm_generator_logic.h"
#include "rpm_calculator.h"

EXTERN_ENGINE;

#if EFI_CJ125
#include "adc_inputs.h"
#include "mpu_util.h"
//#define CJ125_DEBUG
//#define CJ125_DEBUG_SPI

#include "hardware.h"
#include "backup_ram.h"
#include "pin_repository.h"

static uint8_t tx_buff[2] NO_CACHE;
static uint8_t rx_buff[1] NO_CACHE;

static CJ125 globalInstance;

static THD_WORKING_AREA(cj125ThreadStack, UTILITY_THREAD_STACK_SIZE);

static SPIDriver *driver;
static SPIConfig cj125spicfg = {
		false,
		NULL,
		SPI_CJ125_CS_GPIO,
		SPI_CJ125_CS_PIN,
		SPI_CR1_MSTR | SPI_CR1_CPHA | SPI_BAUDRATEPRESCALER_128,
		SPI_CR2_8BIT_MODE
};

static volatile int lastSlowAdcCounter = 0;

// Pump current, mA
static constexpr float pumpCurrentLsu42[] = {
	// LSU 4.2
	-1.85f, -1.08f, -0.76f, -0.47f, 0.0f, 0.34f, 0.68f, 0.95f, 1.4f 
};

static constexpr float pumpCurrentLsu49[] = {
	// LSU 4.9
	-2.0f, -1.602f, -1.243f, -0.927f, -0.8f, -0.652f, -0.405f, -0.183f, -0.106f, -0.04f, 0, 0.015f, 0.097f, 0.193f, 0.250f, 0.329f, 0.671f, 0.938f, 1.150f, 1.385f, 1.700f, 2.000f, 2.150f, 2.250f
};

// Corresponding lambda values for the above pump current
static constexpr float lambdaLsu42[] = {
	// LSU 4.2
	0.7f, 0.8f, 0.85f, 0.9f, 1.009f, 1.18f, 1.43f, 1.7f, 2.42f
};

static constexpr float lambdaLsu49[] = {
	// LSU 4.9
	0.65f, 0.7f, 0.75f, 0.8f, 0.822f, 0.85f, 0.9f, 0.95f, 0.97f, 0.99f, 1.003f, 1.01f, 1.05f, 1.1f, 1.132f, 1.179f, 1.429f, 1.701f, 1.990f, 2.434f, 3.413f, 5.391f, 7.506f, 10.119f
};

static uint8_t cjReadRegister(uint8_t regAddr) {
	spiAcquireBus(driver);
	spiStart(driver, &cj125spicfg);
	spiSelect(driver);
	tx_buff[0] = regAddr;
	spiSend(driver, 1, tx_buff);
	spiReceive(driver, 1, rx_buff);
	spiUnselect(driver);
	spiReleaseBus(driver);
	return rx_buff[0];
}

static void cjWriteRegister(uint8_t regAddr, uint8_t regValue) {
	tx_buff[0] = regAddr;
	tx_buff[1] = regValue;
	spiAcquireBus(driver);
	spiStart(driver, &cj125spicfg);
	spiSelect(driver);
	spiSend(driver, 2, tx_buff);
	spiUnselect(driver);
	spiReleaseBus(driver);
}

static float getUr() {
	if (engine->cj125ur != EFI_ADC_NONE) {
		return getVoltageDivided("cj125ur", engine->cj125ur PASS_ENGINE_PARAMETER_SUFFIX);
	}
	return 0.0f;
}

static float getUa() {
	if (engine->cj125ua != EFI_ADC_NONE) {
		return getVoltageDivided("cj125ua", engine->cj125ua PASS_ENGINE_PARAMETER_SUFFIX);
	}
	return 0.0f;
}

static float getVoltageFrom16bit(uint32_t stored) {
	return ((float)stored) / CJ125_VOLTAGE_TO_16BIT_FACTOR;
}

static uint32_t get16bitFromVoltage(float v) {
	return (uint32_t)(v * CJ125_VOLTAGE_TO_16BIT_FACTOR);
}

class RealSpi : public Cj125SpiStream {
public:
	uint8_t ReadRegister(uint8_t reg) override {
		return cjReadRegister(reg);
	}

	void WriteRegister(uint8_t regAddr, uint8_t regValue) {
		cjWriteRegister(regAddr, regValue);
	}
};

static RealSpi spi;

static void cjUpdateAnalogValues() {
	waitForSlowAdc(lastSlowAdcCounter);
	globalInstance.vUr = getUr();
	globalInstance.vUa = getUa();
    lastSlowAdcCounter = getSlowAdcCounter();

}

void CJ125::calibrate(DECLARE_ENGINE_PARAMETER_SIGNATURE) {
	cjIdentify(PASS_ENGINE_PARAMETER_SIGNATURE);
	cjSetMode(CJ125_MODE_CALIBRATION);
	int init1 = cjReadRegister(INIT_REG1_RD);
	// check if our command has been accepted
	if (init1 != CJ125_INIT1_CALBRT) {
		cjSetMode(CJ125_MODE_NORMAL_17);
		return;
	}
#if EFI_PROD_CODE
	// todo: some testing solution
	// wait for the start of the calibration
	chThdSleepMilliseconds(CJ125_CALIBRATION_DELAY);
#endif /* EFI_PROD_CODE */
	vUaCal = 0.0f;
	vUrCal = 0.0f;
	// wait for some more ADC samples
	for (int i = 0; i < CJ125_CALIBRATE_NUM_SAMPLES; i++) {
		cjUpdateAnalogValues();

		if (engineConfiguration->debugMode == DBG_CJ125) {
			cjPostState(&tsOutputChannels);
		}

		vUaCal += vUa;
		vUrCal += vUr;
	}
	// find average
	vUaCal /= (float)CJ125_CALIBRATE_NUM_SAMPLES;
	vUrCal /= (float)CJ125_CALIBRATE_NUM_SAMPLES;
	// restore normal mode
	cjSetMode(CJ125_MODE_NORMAL_17);

	chThdSleepMilliseconds(CJ125_CALIBRATION_DELAY);
	// check if everything is ok
	diag = cjReadRegister(DIAG_REG_RD);
	cjUpdateAnalogValues();

	// store new calibration data
	uint32_t storedLambda = get16bitFromVoltage(vUaCal);
	uint32_t storedHeater = get16bitFromVoltage(vUrCal);

	CONFIG(storedLambda) = storedLambda;
	CONFIG(storedHeater) = storedHeater;
	state = CJ125_IDLE;
}

static void cjStart(DECLARE_ENGINE_PARAMETER_SIGNATURE) {
	if (!CONFIG(isCJ125Enabled)) {
		return;
	}
	globalInstance.cjIdentify(PASS_ENGINE_PARAMETER_SIGNATURE);

	// Load calibration values

	uint32_t storedLambda = CONFIG(storedLambda);
	uint32_t storedHeater = CONFIG(storedHeater);

	// if no calibration, try to calibrate now and store new values
	if (storedLambda == 0 || storedHeater == 0) {
		/**
		 * open question if we need special considerations for calibration. Some controllers insist on open air calibration
		 */
		globalInstance.calibrate(PASS_ENGINE_PARAMETER_SIGNATURE);
	} else {
		globalInstance.vUaCal = getVoltageFrom16bit(storedLambda);
		globalInstance.vUrCal = getVoltageFrom16bit(storedHeater);
		// Start normal measurement mode
		globalInstance.cjSetMode(CJ125_MODE_NORMAL_17);
	}
	lastSlowAdcCounter = getSlowAdcCounter();
}

void CJ125::setError(cj125_error_e errCode DECLARE_ENGINE_PARAMETER_SUFFIX) {
	errorCode = errCode;
	state = CJ125_ERROR;
	// This is for safety:
	//SetHeater(0 PASS_ENGINE_PARAMETER_SUFFIX);
	// Software-reset of CJ125
	cjWriteRegister(INIT_REG2_WR, CJ125_INIT2_RESET);
}

static bool cjStartSpi(DECLARE_ENGINE_PARAMETER_SIGNATURE) {

	palSetPad(SPI_CJ125_CS_GPIO, SPI_CJ125_CS_PIN);
	palSetPadMode(SPI_CJ125_CS_GPIO, SPI_CJ125_CS_PIN,
	PAL_MODE_OUTPUT_PUSHPULL | PAL_STM32_OSPEED_HIGHEST);
	driver = SPI_CJ125_SPI;
	return true;
}
/**
 * @return true if currently in IDLE or ERROR state
 */
static bool cj125periodic(CJ125 *instance DECLARE_ENGINE_PARAMETER_SUFFIX) {
	{
	efitick_t nowNt = getTimeNowNt();
		bool isStopped = engine->rpmCalculator.isStopped(PASS_ENGINE_PARAMETER_SIGNATURE);
		cjUpdateAnalogValues();
		// If the controller is disabled
		if (instance->state == CJ125_IDLE || instance->state == CJ125_ERROR) {
			return true;
		}

		if (instance->state == CJ125_CALIBRATION) {
			globalInstance.calibrate(PASS_ENGINE_PARAMETER_SIGNATURE);
			// Start normal operation
			instance->state = CJ125_INIT;
			globalInstance.cjSetMode(CJ125_MODE_NORMAL_17);
		}
		
		globalInstance.diag = cjReadRegister(DIAG_REG_RD);
		// check heater state
		if (globalInstance.vUr > CJ125_UR_PREHEAT_THR || instance->heaterDuty < CJ125_PREHEAT_MIN_DUTY) {
			// Check if RPM>0 and it's time to start pre-heating
			if (instance->state == CJ125_INIT && !isStopped) {
				// start preheating
				instance->state = CJ125_PREHEAT;
				instance->startHeatingNt = instance->prevNt = getTimeNowNt();
				globalInstance.cjSetMode(CJ125_MODE_NORMAL_17);
			}
		} else if (instance->vUr > CJ125_UR_GOOD_THR) {
			instance->state = CJ125_HEAT_UP;
		} else if (instance->vUr < CJ125_UR_OVERHEAT_THR) {
			instance->state = CJ125_OVERHEAT;
		} else {
			// This indicates that the heater temperature is optimal for UA measurement
			instance->state = CJ125_READY;
		}

		if (isStopped && instance->isWorkingState()) {
			instance->state = CJ125_INIT;
			instance->SetIdleHeater(PASS_ENGINE_PARAMETER_SIGNATURE);
		}

		if (CONFIG(cj125AutoChangeMode)) {
			// Change amplification if AFR gets lean/rich for better accuracy
			globalInstance.cjSetMode(globalInstance.lambda > 1.0f ? CJ125_MODE_NORMAL_17 : CJ125_MODE_NORMAL_8);
		}

		switch (instance->state) {
		case CJ125_PREHEAT:
			// use constant-speed startup heat-up
			if (nowNt - instance->prevNt >= CJ125_HEATER_PREHEAT_PERIOD) {
				float periodSecs = (float)(nowNt - instance->prevNt) / NT_PER_SECOND;
				// maintain speed at ~0.4V/sec
				float preheatDuty = instance->heaterDuty + periodSecs * CJ125_HEATER_PREHEAT_RATE;
				instance->SetHeater(preheatDuty PASS_ENGINE_PARAMETER_SUFFIX);
				// If we are heating too long, and there's still no result, then something is wrong...
				//if (nowNt - instance->startHeatingNt > NT_PER_SECOND * CJ125_PREHEAT_TIMEOUT) {
				//	instance->setError(CJ125_ERROR_HEATER_MALFUNCTION PASS_ENGINE_PARAMETER_SUFFIX);
				//}

				instance->prevNt = nowNt;
			}
			break;
		case CJ125_HEAT_UP:
		case CJ125_READY:
			// use PID for normal heater control
			if (nowNt - instance->prevNt >= CJ125_HEATER_CONTROL_PERIOD) {

				float duty = globalInstance.heaterPid.getOutput(globalInstance.vUr, globalInstance.vUrCal, MS2SEC(CJ125_TICK_DELAY));
				instance->SetHeater(duty PASS_ENGINE_PARAMETER_SUFFIX);
				instance->prevNt = nowNt;
			}
			break;
		case CJ125_OVERHEAT:
			if (nowNt - instance->prevNt >= CJ125_HEATER_OVERHEAT_PERIOD) {
			//	instance->setError(CJ125_ERROR_OVERHEAT PASS_ENGINE_PARAMETER_SUFFIX);
				instance->prevNt = nowNt;
			}
		default:
			;
		}
}
	return false;
}

static msg_t cjThread(void)
{
	chRegSetThreadName("cj125");
	chThdSleepMilliseconds(500);
	globalInstance.startHeatingNt = 0;
	globalInstance.prevNt = getTimeNowNt();
	while (1) {
		bool needIdleSleep = cj125periodic(&globalInstance PASS_ENGINE_PARAMETER_SUFFIX);
		chThdSleepMilliseconds(needIdleSleep ? 20 : 20);
	}
	return -1;
}

static bool cjCheckConfig(void) {
	if (!CONFIG(isCJ125Enabled)) {
		return false;
	}
	return true;
}

void cjStartCalibration(void) {
	if (!cjCheckConfig())
		return;
//	if (globalInstance.isWorkingState()) {
		// todo: change this later for the normal thread operation (auto pre-heating)
//		return;
//	}
	globalInstance.state = CJ125_CALIBRATION;
}

void cjRestart(void) {
	if (!cjCheckConfig())
		return;
	globalInstance.state = CJ125_INIT;
	globalInstance.errorCode = CJ125_NO_ERROR;
	cjStart(PASS_ENGINE_PARAMETER_SIGNATURE);
}

#ifdef CJ125_DEBUG
static void cjSetInit1(int v) {
	cjWriteRegister(INIT_REG1_WR, v & 0xff);
	v = cjReadRegister(INIT_REG1_RD);
}

static void cjSetInit2(int v) {
	cjWriteRegister(INIT_REG2_WR, v & 0xff);
	v = cjReadRegister(INIT_REG2_RD);

}
#endif /* CJ125_DEBUG */

float cjGetAfr(DECLARE_ENGINE_PARAMETER_SIGNATURE) {
	// See CJ125 datasheet, page 6
	float pumpCurrent = (globalInstance.vUa - globalInstance.vUaCal) * globalInstance.amplCoeff * (CJ125_PUMP_CURRENT_FACTOR / CJ125_PUMP_SHUNT_RESISTOR);
	
	if (engineConfiguration->cj125isLsu49) {
		globalInstance.lambda = interpolate2d("cj125Lsu", pumpCurrent, pumpCurrentLsu49, lambdaLsu49);
		enginePins.cj125ModePin.setValue(0);
	} else {
		globalInstance.lambda = interpolate2d("cj125Lsu", pumpCurrent, pumpCurrentLsu42, lambdaLsu42);
		enginePins.cj125ModePin.setValue(1);
	}
	return globalInstance.lambda * CJ125_STOICH_RATIO;
}

bool cjHasAfrSensor(DECLARE_CONFIG_PARAMETER_SIGNATURE) {
	if (!CONFIG(isCJ125Enabled))
		return false;
	return globalInstance.isValidState();
}

void cjPostState(TunerStudioOutputChannels *tsOutputChannels) {
	tsOutputChannels->debugFloatField1 = globalInstance.heaterDuty;
	tsOutputChannels->debugFloatField2 = globalInstance.heaterPid.getIntegration();
	tsOutputChannels->debugFloatField3 = globalInstance.heaterPid.getPrevError();
	tsOutputChannels->debugFloatField4 = globalInstance.vUa;
	tsOutputChannels->debugFloatField5 = globalInstance.vUr;
	tsOutputChannels->debugFloatField6 = globalInstance.vUaCal;
	tsOutputChannels->debugFloatField7 = globalInstance.vUrCal;
	tsOutputChannels->debugIntField2 = globalInstance.diag;
}

void initCJ125(DECLARE_ENGINE_PARAMETER_SUFFIX) {
	
	globalInstance.spi = &spi;

	if (!CONFIG(isCJ125Enabled)) {
		globalInstance.errorCode = CJ125_ERROR_DISABLED;
		return;
	}

	if (engine->cj125ur == EFI_ADC_NONE || engine->cj125ua == EFI_ADC_NONE) {
		warning(CUSTOM_CJ125_0, "cj ur ua");
		globalInstance.errorCode = CJ125_ERROR_DISABLED;
		return;
	}

	if (engine->wboHeaterPin == GPIO_UNASSIGNED) {
		warning(CUSTOM_CJ125_1, "cj heater");
		globalInstance.errorCode = CJ125_ERROR_DISABLED;
		return;
	}

	globalInstance.cjInitPid(PASS_ENGINE_PARAMETER_SIGNATURE);
	if (!cjStartSpi(PASS_ENGINE_PARAMETER_SIGNATURE))
		return;

	globalInstance.StartHeaterControl((pwm_gen_callback*)applyPinState PASS_ENGINE_PARAMETER_SUFFIX);
	cjStart(PASS_ENGINE_PARAMETER_SIGNATURE);

	chThdCreateStatic(cj125ThreadStack, sizeof(cj125ThreadStack), LOWPRIO, (tfunc_t)(void*) cjThread, NULL);
}

#endif /* EFI_CJ125 && HAL_USE_SPI */

