/**
 * @file	trigger_mazda.h
 *
 * @date Feb 18, 2014
 * @author Andrey Belomutskiy, (c) 2012-2020
 */

#pragma once

#include "trigger_structure.h"

#define MIATA_NA_GAP 1.4930f

void initializeMazdaMiataNaShape(TriggerWaveform *s);
void initializeMazdaMiataNb1Shape(TriggerWaveform *s);
void initializeMazdaMiataNb2Crank(TriggerWaveform *s);
void initializeMazdaMiataVVtTestShape(TriggerWaveform *s);
void configureMazdaProtegeSOHC(TriggerWaveform *s);
void configureMazdaProtegeLx(TriggerWaveform *s);
void initialize_Mazda_Engine_z5_Shape(TriggerWaveform *s);

/**
 * TT_VVT_MIATA_NB2
 * if you unplug crank sensor you can start & run your engine on just this cam decoder
 * trigger offset "-90" seems to be doing the job
 *
 * same decoder is used for VVT processing
 */
void initializeMazdaMiataVVtCamShape(TriggerWaveform *s);
