/*
 * @file	trigger_central.h
 *
 * @date Feb 23, 2014
 * @author Andrey Belomutskiy, (c) 2012-2020
 */

#pragma once

#include "rusefi_enums.h"
#include "listener_array.h"
#include "trigger_decoder.h"
#include "trigger_central_generated.h"

class Engine;
typedef void (*ShaftPositionListener)(trigger_event_e signal, uint32_t index, efitick_t edgeTimestamp DECLARE_ENGINE_PARAMETER_SUFFIX);

#define HAVE_CAM_INPUT() engineConfiguration->pinCam[0] != GPIO_UNASSIGNED

class TriggerNoiseFilter {
public:
	void resetAccumSignalData();
	bool noiseFilter(efitick_t nowNt,
			TriggerState * triggerState,
			trigger_event_e signal DECLARE_ENGINE_PARAMETER_SUFFIX);

	efitick_t lastSignalTimes[HW_EVENT_TYPES];
	efitick_t accumSignalPeriods[HW_EVENT_TYPES];
	efitick_t accumSignalPrevPeriods[HW_EVENT_TYPES];
};

/**
 * Maybe merge TriggerCentral and TriggerState classes into one class?
 * Probably not: we have an instance of TriggerState which is used for trigger initialization,
 * also composition probably better than inheritance here
 */
class TriggerCentral : public trigger_central_s {
public:
	TriggerCentral();
	void addEventListener(ShaftPositionListener handler, const char *name, Engine *engine);
	void handleShaftSignal(trigger_event_e signal, efitick_t timestamp DECLARE_ENGINE_PARAMETER_SUFFIX);
	int getHwEventCounter(int index) const;
	void resetCounters();
	void validateCamVvtCounters();

	TriggerNoiseFilter noiseFilter;

	trigger_type_e vvtTriggerType;
	angle_t getVVTPosition();

	// latest VVT event position (could be not synchronization event)
	angle_t currentVVTEventPosition = 0;
	// synchronization event position
	angle_t vvtPosition = 0;

	/**
	 * this is similar to TriggerState#startOfCycleNt but with the crank-only sensor magic
	 */
	efitick_t timeAtVirtualZeroNt = 0;

	efitick_t vvtSyncTimeNt = 0;

	TriggerStateWithRunningStatistics triggerState;
	TriggerWaveform triggerShape;

	TriggerState vvtState;
	TriggerWaveform vvtShape;

	TriggerFormDetails triggerFormDetails;

private:
	IntListenerArray<15> triggerListeneres;
};

void triggerInfo(void);
void hwHandleShaftSignal(trigger_event_e signal, efitick_t timestamp);
void hwHandleVvtCamSignal(trigger_value_e front, efitick_t timestamp DECLARE_ENGINE_PARAMETER_SUFFIX);

void initTriggerCentral();
void printAllTriggers();

void addTriggerEventListener(ShaftPositionListener handler, const char *name, Engine *engine);
int isSignalDecoderError(void);
void resetMaxValues();

void onConfigurationChangeTriggerCallback(DECLARE_ENGINE_PARAMETER_SIGNATURE);
bool checkIfTriggerConfigChanged(DECLARE_ENGINE_PARAMETER_SIGNATURE);
bool isTriggerConfigChanged(DECLARE_ENGINE_PARAMETER_SIGNATURE);

bool isTriggerDecoderError(DECLARE_ENGINE_PARAMETER_SIGNATURE);

#define SYMMETRICAL_CRANK_SENSOR_DIVIDER 4
