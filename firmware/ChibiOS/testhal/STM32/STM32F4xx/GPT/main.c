/*
    ChibiOS - Copyright (C) 2006..2018 Giovanni Di Sirio

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#include "ch.h"
#include "hal.h"

/*
 * GPT4 callback.
 */
static void gpt4cb(GPTDriver *gptp) {

  (void)gptp;
  palSetPad(GPIOD, GPIOD_LED5);
  chSysLockFromISR();
  gptStartOneShotI(&GPTD3, 1000);   /* 0.1 second pulse.*/
  chSysUnlockFromISR();
}

/*
 * GPT3 callback.
 */
static void gpt3cb(GPTDriver *gptp) {

  (void)gptp;
  palClearPad(GPIOD, GPIOD_LED5);
}

/*
 * GPT4 configuration.
 */
static const GPTConfig gpt4cfg = {
  10000,    /* 10kHz timer clock.*/
  gpt4cb,   /* Timer callback.*/
  0,
  0
};

/*
 * GPT3 configuration.
 */
static const GPTConfig gpt3cfg = {
  10000,    /* 10kHz timer clock.*/
  gpt3cb,   /* Timer callback.*/
  0,
  0
};

/*
 * Application entry point.
 */
int main(void) {

  /*
   * System initializations.
   * - HAL initialization, this also initializes the configured device drivers
   *   and performs the board-specific initializations.
   * - Kernel initialization, the main() function becomes a thread and the
   *   RTOS is active.
   */
  halInit();
  chSysInit();

  /*
   * Initializes the GPT drivers 4 and 3.
   */
  gptStart(&GPTD4, &gpt4cfg);
  gptPolledDelay(&GPTD4, 10); /* Small delay.*/
  gptStart(&GPTD3, &gpt3cfg);
  gptPolledDelay(&GPTD3, 10); /* Small delay.*/

  /*
   * Normal main() thread activity, it changes the GPT4 period every
   * five seconds.
   */
  while (true) {
    palSetPad(GPIOD, GPIOD_LED4);
    gptStartContinuous(&GPTD4, 5000);
    chThdSleepMilliseconds(5000);
    gptStopTimer(&GPTD4);
    palClearPad(GPIOD, GPIOD_LED4);
    gptStartContinuous(&GPTD4, 2500);
    chThdSleepMilliseconds(5000);
    gptStopTimer(&GPTD4);
  }
}
