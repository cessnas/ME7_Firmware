/**
 * @file	can_dash.cpp
 *
 * This file handles transmission of ECU data to various OE dashboards.
 *
 * @date Mar 19, 2020
 * @author Matthew Kennedy, (c) 2020
 */

#include "globalaccess.h"

#include "engine.h"
#include "can_dash.h"
#include "can_msg_tx.h"

#include "sensor.h"
#include "allsensors.h"
#include "vehicle_speed.h"
#include "rtc_helper.h"

EXTERN_ENGINE;

// CAN Bus ID for broadcast
/**
 * e46 data is from http://forums.bimmerforums.com/forum/showthread.php?1887229
 *
 * Same for Mini Cooper? http://vehicle-reverse-engineering.wikia.com/wiki/MINI
 *
 * All the below packets are using 500kb/s
 *
 */
#define CAN_BMW_E46_SPEED 0x153
#define CAN_BMW_E46_RPM 0x316
#define CAN_BMW_E46_DME2 0x329
#define CAN_BMW_E46_CLUSTER_STATUS 0x613
#define CAN_BMW_E46_CLUSTER_STATUS_2 0x615
#define CAN_FIAT_MOTOR_INFO 0x561
#define CAN_MAZDA_RX_RPM_SPEED 0x201
#define CAN_MAZDA_RX_STEERING_WARNING 0x300
#define CAN_MAZDA_RX_STATUS_1 0x212
#define CAN_MAZDA_RX_STATUS_2 0x420
// https://wiki.openstreetmap.org/wiki/VW-CAN
#define VAG_MOTOR_1 0x280
#define VAG_MOTOR_2 0x288
#define VAG_MOTOR_3 0x380
#define VAG_MOTOR_5 0x480
#define VAG_MOTOR_6 0x488
#define VAG_MOTOR_7 0x588
#define CAN_VAG_IMMO 0x3D0
//w202 DASH
#define W202_STAT_1	0x308
#define W202_STAT_2 0x608
#define W202_ALIVE	0x210
#define W202_STAT_3 0x310

//BMW E90 DASH
#define E90_ABS_COUNTER 0x0C0
#define E90_SEATBELT_COUNTER 0x0D7
#define E90_T15	0x130
#define E90_RPM 0x175
#define E90_BRAKE_COUNTER 0x19E
#define E90_SPEED 0x1A6
#define E90_TEMP 0x1D0
#define E90_GEAR 0x1D2
#define E90_FUEL 0x349
#define E90_EBRAKE 0x34F
#define E90_TIME 0x39E

static uint8_t rpmcounter;
static uint16_t e90msgcounter;
static uint8_t seatbeltcnt;
static uint8_t abscounter = 0xF0;
static uint8_t brakecnt_1 = 0xF0, brakecnt_2 = 0xF0;
static uint8_t mph_a, mph_2a, mph_last, tmp_cnt, gear_cnt;
static uint16_t mph_counter = 0xF000;
static time_msecs_t mph_timer;
static time_msecs_t mph_ctr;


constexpr uint8_t e90_temp_offset = 49;

void canDashboardBMW(void) {
	//BMW Dashboard
	{
		CanTxMessage msg(CAN_BMW_E46_SPEED);
		msg.setShortValue(10 * 8, 1);
	}

	{
		CanTxMessage msg(CAN_BMW_E46_RPM);
		msg.setShortValue((int) (GET_RPM() * 6.4), 2);
	}

	{
		CanTxMessage msg(CAN_BMW_E46_DME2);
		msg.setShortValue((int) ((Sensor::get(SensorType::Clt).value_or(0) + 48.373) / 0.75), 1);
	}
}

void canMazdaRX8(void) {
	{
		CanTxMessage msg(CAN_MAZDA_RX_STEERING_WARNING);
		// todo: something needs to be set here? see http://rusefi.com/wiki/index.php?title=Vehicle:Mazda_Rx8_2004
	}

	{
		CanTxMessage msg(CAN_MAZDA_RX_RPM_SPEED);

		float kph = getVehicleSpeed();

		msg.setShortValue(SWAP_UINT16(GET_RPM() * 4), 0);
		msg.setShortValue(0xFFFF, 2);
		msg.setShortValue(SWAP_UINT16((int )(100 * kph + 10000)), 4);
		msg.setShortValue(0, 6);
	}

	{
		CanTxMessage msg(CAN_MAZDA_RX_STATUS_1);
		msg[0] = 0xFE; //Unknown
		msg[1] = 0xFE; //Unknown
		msg[2] = 0xFE; //Unknown
		msg[3] = 0x34; //DSC OFF in combo with byte 5 Live data only seen 0x34
		msg[4] = 0x00; // B01000000; // Brake warning B00001000;  //ABS warning
		msg[5] = 0x40; // TCS in combo with byte 3
		msg[6] = 0x00; // Unknown
		msg[7] = 0x00; // Unused
	}

	{
		CanTxMessage msg(CAN_MAZDA_RX_STATUS_2);
		auto clt = Sensor::get(SensorType::Clt);
		msg[0] = (uint8_t)(clt.value_or(0) + 69); //temp gauge //~170 is red, ~165 last bar, 152 centre, 90 first bar, 92 second bar
		msg[1] = ((int16_t)(engine->engineState.vssEventCounter*(engineConfiguration->vehicleSpeedCoef*0.277*2.58))) & 0xff;
		msg[2] = 0x00; // unknown
		msg[3] = 0x00; //unknown
		msg[4] = 0x01; //Oil Pressure (not really a gauge)
		msg[5] = 0x00; //check engine light
		msg[6] = 0x00; //Coolant, oil and battery
		if ((GET_RPM()>0) && (engine->sensors.vBatt<13)) {
			msg.setBit(6, 6); // battery light
		}
		if (!clt.Valid || clt.Value > 105) {
			// coolant light, 101 - red zone, light means its get too hot
			// Also turn on the light in case of sensor failure
			msg.setBit(6, 1);
		}
		//oil pressure warning lamp bit is 7
		msg[7] = 0x00; //unused
	}
}

void canDashboardFiat(void) {
	{
		//Fiat Dashboard
		CanTxMessage msg(CAN_FIAT_MOTOR_INFO);
		msg.setShortValue((int) (Sensor::get(SensorType::Clt).value_or(0) - 40), 3); //Coolant Temp
		msg.setShortValue(GET_RPM() / 32, 6); //RPM
	}
}

void canDashboardVAG(void) {

	float clt = Sensor::get(SensorType::Clt).value_or(0);

	{
		CanTxMessage msg(CAN_VAG_IMMO);
		msg.setShortValue(0x80, 1);
	}
	chThdSleepMilliseconds(10);

	  {  	  	  	  	  	  	  	  	// All Torque fields = 0 - 99.06 %  (X * 2.54)
		//  Motor 1 ($280)
	CanTxMessage msg(VAG_MOTOR_1);
			msg[0] = 0x08;
			msg[1] = (int)(Sensor::get(SensorType::DriverThrottleIntent).Value * 2.5);		// inneres Motor-Moment
			msg.setShortValue(GET_RPM() * 4, 2); //RPM Byte 2-3
			msg[4] = (int)(Sensor::get(SensorType::DriverThrottleIntent).Value * 2.5);		// inneres Moment ohne externe Eingriffe
			msg[5] = (int)(Sensor::get(SensorType::DriverThrottleIntent).Value * 2.5);  // Throttle Pedal
			msg[6] = 0x00;     // mechanisches Motor-Verlustmoment
			msg[7] = (int)(Sensor::get(SensorType::DriverThrottleIntent).Value * 2.5);     // Drivers Wish Torque
	  }
	  chThdSleepMilliseconds(10);
	  {						//01100101, 10001111, 11101000, 00001011
		  uint8_t myValues[4] = {0x65, 0x8f, 0xE8, 0x0B};
		  for(int i=0; i<4; i++)
		 	{
	  		  CanTxMessage msg(VAG_MOTOR_2);
	  			  	  	msg[0] = myValues[i];
	  		  			msg.setShortValue((int) ((clt + 48.373) / 0.75), 1); // Coolant Temp
	  		  			msg[2] = 0x00;
	  		  			msg[4] = 0x00;		// GRA target speed for CAN output (GRA = Cruise control?)
	  		  			msg[5] = 0x51; 		// Idle target rpm
	  		  			msg[6] = (int)(Sensor::get(SensorType::DriverThrottleIntent).Value * 2.5);   // max possible torque
	  		  			msg[7] = (int)(Sensor::get(SensorType::DriverThrottleIntent).Value * 2.5);   // indexed engine torque at the latest ignition angle for CAN output
	  		  	  }
	  }
	  chThdSleepMilliseconds(10);

		  {



	  CanTxMessage msg(VAG_MOTOR_6);
	  	  	  	msg[0] = 0x00;
	  			msg[1] = (int)(Sensor::get(SensorType::DriverThrottleIntent).Value * 2.5);
	  			msg[2] = (int)(Sensor::get(SensorType::DriverThrottleIntent).Value * 2.5);
	  			msg[3] = 0x7C;
	  			msg[4] = 0xA6;
	  			msg[5] = 0x00;
	  			msg[6] = 0x00;
	  			msg[7] = 0x00;
	  	  }

		  {
			  CanTxMessage msg(VAG_MOTOR_7);
			  			msg[0] = 0x08;
			  			msg[1] = 0x00;
			  			msg[2] = 0x7C;

			  	  }

}

void canDashboardW202(void) {
	
	uint16_t tmp;
	{
		CanTxMessage msg(W202_STAT_1);
		tmp = GET_RPM();
		msg[0] = 0x08; // Unknown
		msg[1] = (tmp >> 8); //RPM
		msg[2] = (tmp & 0xff); //RPM
		msg[3] = 0x00; // 0x01 - tank blink, 0x02 - EPC
		msg[4] = 0x00; // Unknown
		msg[5] = 0x00; // Unknown
		msg[6] = 0x00; // Unknown - oil info
		msg[7] = 0x00; // Unknown - oil info
	}

	{
		CanTxMessage msg(W202_STAT_2); //dlc 7
		msg[0] = (int)(Sensor::get(SensorType::Clt).value_or(0) + 40); // CLT -40 offset
		msg[1] = 0x3D; // TBD
		msg[2] = 0x63; // Const
		msg[3] = 0x41; // Const
		msg[4] = 0x00; // Unknown
		msg[5] = 0x05; // Const
		msg[6] = 0x50; // TBD
		msg[7] = 0x00; // Unknown
	}

	{
		CanTxMessage msg(W202_ALIVE);
		msg[0] = 0x0A; // Const
		msg[1] = 0x18; // Const
		msg[2] = 0x00; // Const
		msg[3] = 0x00; // Const
		msg[4] = 0xC0; // Const
		msg[5] = 0x00; // Const
		msg[6] = 0x00; // Const
		msg[7] = 0x00; // Const
	}	

	{
		CanTxMessage msg(W202_STAT_3);
		msg[0] = 0x00; // Const
		msg[1] = 0x00; // Const
		msg[2] = 0x6D; // TBD
		msg[3] = 0x7B; // Const
		msg[4] = 0x21; // TBD
		msg[5] = 0x07; // Const
		msg[6] = 0x33; // Const
		msg[7] = 0x05; // Const
	}
}

void canDashboardBMWE90()
{
	if (e90msgcounter == UINT16_MAX)
		e90msgcounter = 0;
	e90msgcounter++;

	{ //T15 'turn-on'
		CanTxMessage msg(E90_T15, 5);
		msg[0] = 0x45;
		msg[1] = 0x41;
		msg[2] = 0x61;
		msg[3] = 0x8F;
		msg[4] = 0xFC;			
	}

	{ //Ebrake light
		CanTxMessage msg(E90_EBRAKE, 2);
		msg[0] = 0xFD;
		msg[1] = 0xFF;
	}

	{ //RPM
		rpmcounter++;
		if (rpmcounter > 0xFE)
			rpmcounter = 0xF0;
		CanTxMessage msg(E90_RPM, 3);
		msg[0] = rpmcounter;
		msg[1] = (GET_RPM() * 4) & 0xFF;
		msg[2] = (GET_RPM() * 4) >> 8;
	}

	{ //oil & coolant temp (all in C, despite gauge being F)
			tmp_cnt++;
			if (tmp_cnt >= 0x0F)
				tmp_cnt = 0x00;
			CanTxMessage msg(E90_TEMP, 8);
			msg[0] = (int)(Sensor::get(SensorType::Clt).value_or(0) + e90_temp_offset); //coolant
			msg[1] = (int)(Sensor::get(SensorType::AuxTemp1).value_or(0) + e90_temp_offset); //oil (AuxTemp1)
			msg[2] = tmp_cnt;
			msg[3] = 0xC8;
			msg[4] = 0xA7;
			msg[5] = 0xD3;
			msg[6] = 0x0D;
			msg[7] = 0xA8;
	}

	{ //Seatbelt counter
		if (e90msgcounter % 2) {
			seatbeltcnt++;
			if (seatbeltcnt > 0xFE)
				seatbeltcnt = 0x00;
			CanTxMessage msg(E90_SEATBELT_COUNTER, 2);
			msg[0] = seatbeltcnt;
			msg[1] = 0xFF;
		}
	}

	{ //Brake counter
		if (e90msgcounter % 2) {
			brakecnt_1 += 16;
			brakecnt_2 += 16;
			if (brakecnt_1 > 0xEF)
				brakecnt_1 = 0x0F;
			if (brakecnt_2 > 0xF0)
				brakecnt_2 = 0xA0;
			CanTxMessage msg(E90_BRAKE_COUNTER, 8);
			msg[0] = 0x00;
			msg[1] = 0xE0;
			msg[2] = brakecnt_1;
			msg[3] = 0xFC;
			msg[4] = 0xFE;
			msg[5] = 0x41;
			msg[6] = 0x00;
			msg[7] = brakecnt_2;
		}
	}

	{ //ABS counter
		if (e90msgcounter % 2) {
			abscounter++;
			if (abscounter > 0xFE)
				abscounter = 0xF0;
			CanTxMessage msg(E90_ABS_COUNTER, 2);
			msg[0] = abscounter;
			msg[1] = 0xFF;
		}
	}

	{ //Fuel gauge
		if (e90msgcounter % 2) {
			CanTxMessage msg(E90_FUEL, 5); //fuel gauge
			msg[0] = 0x76;
			msg[1] = 0x0F;
			msg[2] = 0xBE;
			msg[3] = 0x1A;
			msg[4] = 0x00;
		}
	}

	{ //Gear indicator/counter
		if (e90msgcounter % 2) {
			gear_cnt++;
			if (gear_cnt >= 0x0F)
				gear_cnt = 0x00;
			CanTxMessage msg(E90_GEAR, 6);
			msg[0] = 0x78;
			msg[1] = 0x0F;
			msg[2] = 0xFF;
			msg[3] = (gear_cnt << 4) | 0xC;
			msg[4] = 0xF1;
			msg[5] = 0xFF;
		}
	}

	{ //E90_SPEED
		if (e90msgcounter % 2) {
			float mph = getVehicleSpeed() * 0.6213712;
			mph_ctr = ((TIME_I2MS(chVTGetSystemTime()) - mph_timer) / 50);
			mph_a = (mph_ctr * mph / 2);
			mph_2a = mph_a + mph_last;
			mph_last = mph_2a;
			mph_counter += mph_ctr * 100;
			if(mph_counter >= 0xFFF0)
				mph_counter = 0xF000;
			mph_timer = TIME_I2MS(chVTGetSystemTime());
			CanTxMessage msg(E90_SPEED, 8);
			msg[0] = mph_2a & 0xFF;
			msg[1] = mph_2a >> 8;
			msg[2] = mph_2a & 0xFF;
			msg[3] = mph_2a >> 8;
			msg[4] = mph_2a & 0xFF;
			msg[5] = mph_2a >> 8;
			msg[6] = mph_counter & 0xFF;
			msg[7] = (mph_counter >> 8) | 0xF0;
		}
	}


}