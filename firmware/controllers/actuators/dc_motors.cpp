/**
 * @file dc_motors.cpp
 *
 * @date March 3, 2020
 * @author Matthew Kennedy (c) 2020
 */

#include "engine.h"
#include "io_pins.h"
#include "engine_configuration.h"
#include "engine_controller.h"
#include "periodic_task.h"

#include "dc_motors.h"
#include "dc_motor.h"

#include "efi_gpio.h"
#include "pwm_generator_logic.h"

EXTERN_ENGINE;

class EtbHardware {
private:
	OutputPin m_pinEnable;
	OutputPin m_pinDir1;
	OutputPin m_pinDir2;
	OutputPin m_disablePin;

	SimplePwm m_pwmEnable;
	SimplePwm m_pwmDir1;
	SimplePwm m_pwmDir2;

public:
	EtbHardware() : dcMotor(&m_pwmEnable, &m_pwmDir1, &m_pwmDir2, &m_disablePin) {}

	TwoPinDcMotor dcMotor;
	
	void setFrequency(int frequency) {
		m_pwmEnable.setFrequency(frequency);
		m_pwmDir1.setFrequency(frequency);
		m_pwmDir2.setFrequency(frequency);
	}

	void start(bool useTwoWires, 
			brain_pin_e pinEnable,
			brain_pin_e pinDir1,
			brain_pin_e pinDir2,
			brain_pin_e pinDisable,
			ExecutorInterface* executor,
			int frequency) {
		dcMotor.setType(useTwoWires ? TwoPinDcMotor::ControlType::PwmDirectionPins : TwoPinDcMotor::ControlType::PwmEnablePin);

		// Configure the disable pin first - ensure things are in a safe state
		m_disablePin.initPin("ETB Disable", pinDisable);

		// Clamp to >100hz
		int clampedFrequency = maxI(100, frequency);

// no need to complicate event queue with ETB PWM in unit tests
#if ! EFI_UNIT_TEST
		startSimplePwmHard(&m_pwmEnable, "ETB Enable",
			executor,
			pinEnable,
			&m_pinEnable,
			clampedFrequency,
			0
		);

		startSimplePwmHard(&m_pwmDir1, "ETB Dir 1",
			executor,
			pinDir1,
			&m_pinDir1,
			clampedFrequency,
			0
		);

		startSimplePwmHard(&m_pwmDir2, "ETB Dir 2",
			executor,
			pinDir2,
			&m_pinDir2,
			clampedFrequency,
			0
		);
#endif /* EFI_UNIT_TEST */
	}
};

static EtbHardware etbHardware[ETB_COUNT * 2];

// We needed more H-bridge configs - so the IO configs are split
// across two arrays of settings to preserve config compatibility
const etb_io& getConfigForMotor(size_t index DECLARE_ENGINE_PARAMETER_SUFFIX) {
	size_t firstSize = efi::size(engineConfiguration->etbIo);

	if (index < firstSize) {
		return engineConfiguration->etbIo[index];
	}

	return engineConfiguration->etbIo2[index - firstSize];
}

DcMotor* initDcMotor(size_t index, bool useTwoWires DECLARE_ENGINE_PARAMETER_SUFFIX) {
	const auto& io = getConfigForMotor(index PASS_ENGINE_PARAMETER_SUFFIX);
	auto& hw = etbHardware[index];

	hw.start(
		useTwoWires,
		io.controlPin1,
		io.directionPin1,
		io.directionPin2,
		io.disablePin,
		&ENGINE(executor),
		CONFIG(etbFreq)
	);

	return &hw.dcMotor;
}

void setDcMotorFrequency(size_t index, int hz) {
	etbHardware[index].setFrequency(hz);
}

void setDcMotorDuty(size_t index, float duty) {
	etbHardware[index].dcMotor.set(duty);
}


