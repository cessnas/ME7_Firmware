/*
 * engine2.cpp
 *
 * @date Jan 5, 2019
 * @author Andrey Belomutskiy, (c) 2012-2020
 */

// todo: move this code to more proper locations

#include "engine.h"
#include "thermistors.h"
#include "speed_density.h"
#include "allsensors.h"
#include "fuel_math.h"
#include "engine_math.h"
#include "advance_map.h"

#include "perf_trace.h"
#include "closed_loop_fuel.h"
#include "sensor.h"


extern fuel_Map3D_t veMap;
extern afr_Map3D_t afrMap;

EXTERN_ENGINE;

// this does not look exactly right


WarningCodeState::WarningCodeState() {
	clear();
}

void WarningCodeState::clear() {
	warningCounter = 0;
	lastErrorCode = 0;
	timeOfPreviousWarning = DEEP_IN_THE_PAST_SECONDS;
	recentWarnings.clear();
}

void WarningCodeState::addWarningCode(obd_code_e code) {
	warningCounter++;
	lastErrorCode = code;
	if (!recentWarnings.contains(code)) {
		recentWarnings.add((int)code);
	}
}

/**
 * @param forIndicator if we want to retrieving value for TS indicator, this case a minimal period is applued
 */
bool WarningCodeState::isWarningNow(efitimesec_t now, bool forIndicator DECLARE_ENGINE_PARAMETER_SUFFIX) const {
	int period = forIndicator ? maxI(3, engineConfiguration->warningPeriod) : engineConfiguration->warningPeriod;
	return absI(now - timeOfPreviousWarning) < period;
}

MockAdcState::MockAdcState() {
	memset(hasMockAdc, 0, sizeof(hasMockAdc));
}

#if EFI_ENABLE_MOCK_ADC
void MockAdcState::setMockVoltage(int hwChannel, float voltage DECLARE_ENGINE_PARAMETER_SUFFIX) {
	efiAssertVoid(OBD_PCM_Processor_Fault, hwChannel >= 0 && hwChannel < MOCK_ADC_SIZE, "hwChannel out of bounds");

	fakeAdcValues[hwChannel] = voltsToAdc(voltage);
	hasMockAdc[hwChannel] = true;
}
#endif /* EFI_ENABLE_MOCK_ADC */

FuelConsumptionState::FuelConsumptionState() {
	accumulatedSecondPrevNt = accumulatedMinutePrevNt = getTimeNowNt();
}

void FuelConsumptionState::addData(float durationMs) {
	if (durationMs > 0.0f) {
		perSecondAccumulator += durationMs;
		perMinuteAccumulator += durationMs;
	}
}

void FuelConsumptionState::update(efitick_t nowNt DECLARE_ENGINE_PARAMETER_SUFFIX) {
	efitick_t deltaNt = nowNt - accumulatedSecondPrevNt;
	if (deltaNt >= NT_PER_SECOND) {
		perSecondConsumption = getFuelRate(perSecondAccumulator, deltaNt PASS_ENGINE_PARAMETER_SUFFIX);
		perSecondAccumulator = 0;
		accumulatedSecondPrevNt = nowNt;
	}

	deltaNt = nowNt - accumulatedMinutePrevNt;
	if (deltaNt >= NT_PER_SECOND * 60) {
		perMinuteConsumption = getFuelRate(perMinuteAccumulator, deltaNt PASS_ENGINE_PARAMETER_SUFFIX);
		perMinuteAccumulator = 0;
		accumulatedMinutePrevNt = nowNt;
	}
}

TransmissionState::TransmissionState() {
}

EngineState::EngineState() {
	timeSinceLastTChargeK = getTimeNowNt();

#if ! EFI_PROD_CODE
	memset(mockPinStates, 0, sizeof(mockPinStates));
#endif /* EFI_PROD_CODE */
}

void EngineState::updateSlowSensors(DECLARE_ENGINE_PARAMETER_SIGNATURE) {
	// this feeds rusEfi console Live Data
	engine->engineState.isCrankingState = ENGINE(rpmCalculator).isCranking(PASS_ENGINE_PARAMETER_SIGNATURE);
}

void EngineState::periodicFastCallback(DECLARE_ENGINE_PARAMETER_SIGNATURE) {
	ScopePerf perf(PE::EngineStatePeriodicFastCallback);


	if (!engine->slowCallBackWasInvoked) {
		warning(CUSTOM_SLOW_NOT_INVOKED, "Slow not invoked yet");
	}
	efitick_t nowNt = getTimeNowNt();
	if (ENGINE(rpmCalculator).isCranking(PASS_ENGINE_PARAMETER_SIGNATURE)) {
		crankingTime = nowNt;
		timeSinceCranking = 0.0f;
	} else {
		timeSinceCranking = nowNt - crankingTime;
	}

	int rpm = ENGINE(rpmCalculator).getRpm(PASS_ENGINE_PARAMETER_SIGNATURE);
	sparkDwell = getSparkDwell(rpm PASS_ENGINE_PARAMETER_SUFFIX);
	dwellAngle = cisnan(rpm) ? NAN :  sparkDwell / getOneDegreeTimeMs(rpm);
	if (hasAfrSensor(PASS_ENGINE_PARAMETER_SIGNATURE)) {
		engine->sensors.currentAfr = getAfr(PASS_ENGINE_PARAMETER_SIGNATURE);
	}

	// todo: move this into slow callback, no reason for IAT corr to be here
	running.intakeTemperatureCoefficient = getIatFuelCorrection(PASS_ENGINE_PARAMETER_SIGNATURE);
	// todo: move this into slow callback, no reason for CLT corr to be here
	running.coolantTemperatureCoefficient = getCltFuelCorrection(PASS_ENGINE_PARAMETER_SIGNATURE);

	running.pidCorrection = fuelClosedLoopCorrection(PASS_ENGINE_PARAMETER_SIGNATURE);

	// update fuel consumption states
	fuelConsumption.update(nowNt PASS_ENGINE_PARAMETER_SUFFIX);

	// Fuel cut-off isn't just 0 or 1, it can be tapered
	fuelCutoffCorrection = getFuelCutOffCorrection(nowNt, rpm PASS_ENGINE_PARAMETER_SUFFIX);

	running.postCrankingFuelCorrection = getAfterStartEnrichment(PASS_ENGINE_PARAMETER_SIGNATURE);



	multispark.count = getMultiSparkCount(rpm PASS_ENGINE_PARAMETER_SUFFIX);

	if (engineConfiguration->fuelAlgorithm == LM_SPEED_DENSITY) {
		auto tps = Sensor::get(SensorType::Tps1);
		updateTChargeK(rpm, tps.value_or(0) PASS_ENGINE_PARAMETER_SUFFIX);
		float map = getMap(PASS_ENGINE_PARAMETER_SIGNATURE);

		/**
		 * *0.01 because of https://sourceforge.net/p/rusefi/tickets/153/
		 */

			currentRawVE = veMap.getValue(rpm, map);

		// get VE from the separate table for Idle
		if (tps.Valid && CONFIG(useSeparateVeForIdle)) {
			float idleVe = interpolate2d("idleVe", rpm, config->idleVeBins, config->idleVe);
			// interpolate between idle table and normal (running) table using TPS threshold
			currentRawVE = interpolateClamped(0.0f, idleVe, CONFIG(idlePidDeactivationTpsThreshold), currentRawVE, tps.Value);
		}
		currentBaroCorrectedVE = 1 * currentRawVE * PERCENT_DIV;
	}

	ENGINE(injectionDuration) = getInjectionDuration(rpm PASS_ENGINE_PARAMETER_SUFFIX);

	float fuelLoad = getFuelingLoad(PASS_ENGINE_PARAMETER_SIGNATURE);
	injectionOffset = getInjectionOffset(rpm, fuelLoad PASS_ENGINE_PARAMETER_SUFFIX);

	float ignitionLoad = getIgnitionLoad(PASS_ENGINE_PARAMETER_SIGNATURE);
	timingAdvance = getAdvance(rpm, ignitionLoad PASS_ENGINE_PARAMETER_SUFFIX);

}

void EngineState::updateTChargeK(int rpm, float tps DECLARE_ENGINE_PARAMETER_SUFFIX) {

	float newTCharge = getTCharge(rpm, tps PASS_ENGINE_PARAMETER_SUFFIX);
	// convert to microsecs and then to seconds
	efitick_t curTime = getTimeNowNt();
	float secsPassed = (float)NT2US(curTime - timeSinceLastTChargeK) / 1000000.0f;
	if (!cisnan(newTCharge)) {
		// control the rate of change or just fill with the initial value
		sd.tCharge = (sd.tChargeK == 0) ? newTCharge : limitRateOfChange(newTCharge, sd.tCharge, CONFIG(tChargeAirIncrLimit), CONFIG(tChargeAirDecrLimit), secsPassed);
		sd.tChargeK = convertCelsiusToKelvin(sd.tCharge);
		timeSinceLastTChargeK = curTime;
	}

}

SensorsState::SensorsState() {
}

int MockAdcState::getMockAdcValue(int hwChannel) const {
	efiAssert(OBD_PCM_Processor_Fault, hwChannel >= 0 && hwChannel < MOCK_ADC_SIZE, "hwChannel out of bounds", -1);
	return fakeAdcValues[hwChannel];
}

StartupFuelPumping::StartupFuelPumping() {
	isTpsAbove50 = false;
	pumpsCounter = 0;
}

void StartupFuelPumping::setPumpsCounter(int newValue) {
	if (pumpsCounter != newValue) {
		pumpsCounter = newValue;

		if (pumpsCounter == PUMPS_TO_PRIME) {
			pumpsCounter = 0;
		}
	}
}

void StartupFuelPumping::update(DECLARE_ENGINE_PARAMETER_SIGNATURE) {
	if (GET_RPM() == 0) {
		bool isTpsAbove50 = Sensor::get(SensorType::DriverThrottleIntent).value_or(0) >= 50;

		if (this->isTpsAbove50 != isTpsAbove50) {
			setPumpsCounter(pumpsCounter + 1);
		}

	} else {
		/**
		 * Engine is not stopped - not priming pumping mode
		 */
		setPumpsCounter(0);
		isTpsAbove50 = false;
	}
}


