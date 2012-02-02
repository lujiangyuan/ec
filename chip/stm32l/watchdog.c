/* Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* Watchdog driver */

#include <stdint.h>

#include "board.h"
#include "common.h"
#include "config.h"
#include "registers.h"
#include "gpio.h"
#include "task.h"
#include "timer.h"
#include "uart.h"
#include "util.h"

/* LSI oscillator frequency is typically 38 kHz
 * but might vary from 28 to 56kHz.
 * So let's pick 56kHz to ensure we reload
 * early enough.
 */
#define LSI_CLOCK 56000

/* Prescaler divider = /256 */
#define IWDG_PRESCALER 6
#define IWDG_PRESCALER_DIV (1 << ((IWDG_PRESCALER) + 2))

void watchdog_reload(void)
{
	/* Reload the watchdog */
	STM32L_IWDG_KR = 0xaaaa;
}

int watchdog_init(int period_ms)
{
	uint32_t watchdog_period;

	/* set the time-out period */
	watchdog_period = period_ms * (LSI_CLOCK / IWDG_PRESCALER_DIV) / 1000;

	/* Unlock watchdog registers */
	STM32L_IWDG_KR = 0x5555;

	/* Set the prescaler between the LSI clock and the watchdog counter */
	STM32L_IWDG_PR = IWDG_PRESCALER & 7;
	/* Set the reload value of the watchdog counter */
	STM32L_IWDG_RLR = watchdog_period & 0x7FF ;

	/* Start the watchdog (and re-lock registers) */
	STM32L_IWDG_KR = 0xcccc;

	return EC_SUCCESS;
}

/* Low priority task to reload the watchdog */
void watchdog_task(void)
{
	while (1) {
#ifdef BOARD_discovery
		/* TODO use GPIO API: gpio_set_level(GPIO_GREEN_LED, 1); */
		STM32L_GPIO_ODR(B) |= (1 << 7) ;
#endif
		usleep(500000);
		watchdog_reload();
#ifdef BOARD_discovery
		/* TODO use GPIO API: gpio_set_level(GPIO_GREEN_LED, 0); */
		STM32L_GPIO_ODR(B) &= ~(1 << 7) ;
#endif
		usleep(500000);
		watchdog_reload();
	}
}
