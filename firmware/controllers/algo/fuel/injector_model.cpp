#include "injector_model.h"

EXTERN_ENGINE;

void InjectorModelBase::prepare() {
	m_massFlowRate = getInjectorMassFlowRate();
	m_deadtime = getDeadtime();
}

constexpr float convertToGramsPerSecond(float ccPerMinute) {
	float ccPerSecond = ccPerMinute / 60;
	return ccPerSecond * 0.72f;	// 0.72g/cc fuel density
}

float InjectorModel::getInjectorMassFlowRate() const {
	// TODO: injector flow dependent upon rail pressure (and temperature/ethanol content?)
	auto injectorVolumeFlow = CONFIG(injector.size);
	return convertToGramsPerSecond(injectorVolumeFlow);
}

float InjectorModel::getDeadtime() const {
	return interpolate2d(
		"lag",
		ENGINE(sensors.vBatt),
		engineConfiguration->injector.battLagCorrBins,
		engineConfiguration->injector.battLagCorr
	);
}

float InjectorModelBase::getInjectionDuration(float fuelMassGram) const {
	// TODO: support injector nonlinearity correction

	floatms_t baseDuration = fuelMassGram / m_massFlowRate * 1000;
	return baseDuration + m_deadtime;
}
