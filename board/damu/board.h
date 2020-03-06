/* Copyright 2019 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* Configuration for Kukui */

#ifndef __CROS_EC_BOARD_H
#define __CROS_EC_BOARD_H

#define VARIANT_KUKUI_BATTERY_SMART
#define VARIANT_KUKUI_CHARGER_ISL9238

#ifndef SECTION_IS_RW
#define VARIANT_KUKUI_NO_SENSORS
#endif /* SECTION_IS_RW */

#include "baseboard.h"

#undef CONFIG_CHIPSET_POWER_SEQ_VERSION
#define CONFIG_CHIPSET_POWER_SEQ_VERSION 1

/* TODO(b:135086465) led implementation */
#undef CONFIG_LED_COMMON

#define CONFIG_BATTERY_HW_PRESENT_CUSTOM

#define CONFIG_CHARGER_PSYS

#define CONFIG_BC12_DETECT_PI3USB9201

#define CONFIG_EXTPOWER_GPIO
#undef CONFIG_EXTPOWER_DEBOUNCE_MS
#define CONFIG_EXTPOWER_DEBOUNCE_MS 200

#undef CONFIG_SYSTEM_UNLOCKED

#define CONFIG_USB_PD_TCPM_FUSB302
#define CONFIG_USB_PD_DISCHARGE_GPIO
#define CONFIG_USB_PD_TCPC_LOW_POWER

#define CONFIG_USB_MUX_IT5205

#define CONFIG_LED_ONOFF_STATES
#define CONFIG_LED_POWER_LED
#define CONFIG_LED_COMMON

/* Motion Sensors */
#ifndef VARIANT_KUKUI_NO_SENSORS
#define CONFIG_ACCEL_LIS2DWL	/* Lid accel */
#define CONFIG_ACCELGYRO_BMI160 /* Base accel */
#define CONFIG_ACCEL_INTERRUPTS
#define CONFIG_ACCELGYRO_BMI160_INT_EVENT \
	TASK_EVENT_MOTION_SENSOR_INTERRUPT(BASE_ACCEL)
#define CONFIG_CMD_ACCEL_INFO

#define CONFIG_LID_ANGLE
#define CONFIG_LID_ANGLE_SENSOR_BASE BASE_ACCEL
#define CONFIG_LID_ANGLE_SENSOR_LID LID_ACCEL

#define CONFIG_ACCEL_FORCE_MODE_MASK BIT(LID_ACCEL)

#endif /* VARIANT_KUKUI_NO_SENSORS */

/* I2C ports */
#define I2C_PORT_BC12               0
#define I2C_PORT_TCPC0              0
#define I2C_PORT_USB_MUX            0
#define I2C_PORT_BATTERY            1
#define I2C_PORT_CHARGER            board_get_charger_i2c()
#define I2C_PORT_SENSORS            1
#define I2C_PORT_IO_EXPANDER_IT8801 1
#define I2C_PORT_VIRTUAL_BATTERY    I2C_PORT_BATTERY

/* Enable Accel over SPI */
#define CONFIG_SPI_ACCEL_PORT    0  /* The first SPI master port (SPI2) */

#define CONFIG_KEYBOARD_PROTOCOL_MKBP
#define CONFIG_MKBP_EVENT
#define CONFIG_MKBP_USE_GPIO
/* Define the MKBP events which are allowed to wakeup AP in S3. */
#define CONFIG_MKBP_HOST_EVENT_WAKEUP_MASK \
		(EC_HOST_EVENT_MASK(EC_HOST_EVENT_LID_OPEN) |\
		 EC_HOST_EVENT_MASK(EC_HOST_EVENT_POWER_BUTTON))

#define CONFIG_GMR_TABLET_MODE
#define GMR_TABLET_MODE_GPIO_L GPIO_TABLET_MODE_L
#define CONFIG_TABLET_MODE_SWITCH

#define CONFIG_IO_EXPANDER
#define CONFIG_IO_EXPANDER_IT8801
#define CONFIG_IO_EXPANDER_PORT_COUNT 1
#define CONFIG_KEYBOARD_DEBUG
#define CONFIG_KEYBOARD_NOT_RAW
#define CONFIG_KEYBOARD_BOARD_CONFIG
#define CONFIG_KEYBOARD_COL2_INVERTED

#define PD_OPERATING_POWER_MW 30000

#ifndef __ASSEMBLER__

enum adc_channel {
	/* Real ADC channels begin here */
	ADC_BOARD_ID = 0,
	ADC_EC_SKU_ID,
	ADC_BATT_ID,
	ADC_CH_COUNT
};

/* power signal definitions */
enum power_signal {
	AP_IN_S3_L,
	PMIC_PWR_GOOD,

	/* Number of signals */
	POWER_SIGNAL_COUNT,
};

/* Motion sensors */
enum sensor_id {
	LID_ACCEL = 0,
	BASE_ACCEL,
	BASE_GYRO,
	SENSOR_COUNT,
};

enum charge_port {
	CHARGE_PORT_USB_C,
};

enum battery_type {
	BATTERY_C235,
	BATTERY_TYPE_COUNT,
};

#include "gpio_signal.h"
#include "registers.h"

#ifdef SECTION_IS_RO
/* Interrupt handler for emmc task */
void emmc_cmd_interrupt(enum gpio_signal signal);
#endif

void bc12_interrupt(enum gpio_signal signal);
void board_reset_pd_mcu(void);
int board_get_version(void);
int board_is_sourcing_vbus(int port);

/* returns the i2c port number of charger */
int board_get_charger_i2c(void);

#endif /* !__ASSEMBLER__ */

#endif /* __CROS_EC_BOARD_H */
