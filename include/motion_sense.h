/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* Header for motion_sense.c */

#ifndef __CROS_EC_MOTION_SENSE_H
#define __CROS_EC_MOTION_SENSE_H

#include "chipset.h"
#include "common.h"
#include "ec_commands.h"
#include "gpio.h"
#include "math_util.h"
#include "queue.h"
#include "timer.h"

enum sensor_state {
	SENSOR_NOT_INITIALIZED = 0,
	SENSOR_INITIALIZED = 1,
	SENSOR_INIT_ERROR = 2
};

enum sensor_config {
	SENSOR_CONFIG_AP, /* Configuration requested/for the AP */
	SENSOR_CONFIG_EC_S0, /* Configuration from the EC while device in S0 */
	SENSOR_CONFIG_EC_S3, /* from the EC when device sleep */
	SENSOR_CONFIG_EC_S5, /* from the EC when device powered off */
	SENSOR_CONFIG_MAX,
};

#define SENSOR_ACTIVE_S5 (CHIPSET_STATE_SOFT_OFF | CHIPSET_STATE_HARD_OFF)
#define SENSOR_ACTIVE_S3 CHIPSET_STATE_ANY_SUSPEND
#define SENSOR_ACTIVE_S0 CHIPSET_STATE_ON
#define SENSOR_ACTIVE_S0_S3 (SENSOR_ACTIVE_S3 | SENSOR_ACTIVE_S0)
#define SENSOR_ACTIVE_S0_S3_S5 (SENSOR_ACTIVE_S0_S3 | SENSOR_ACTIVE_S5)

/* Events the motion sense task may have to process.*/
#define TASK_EVENT_MOTION_FLUSH_PENDING     TASK_EVENT_CUSTOM(1)
#define TASK_EVENT_MOTION_ODR_CHANGE        TASK_EVENT_CUSTOM(2)
/* Next 8 events for sensor interrupt lines */
#define TASK_EVENT_MOTION_INTERRUPT_MASK    (0xff << 2)

#define ROUND_UP_FLAG (1 << 31)
#define BASE_ODR(_odr) ((_odr) & ~ROUND_UP_FLAG)
#define BASE_RANGE(_range) ((_range) & ~ROUND_UP_FLAG)

#ifdef CONFIG_ACCEL_FIFO
#define MAX_FIFO_EVENT_COUNT CONFIG_ACCEL_FIFO
#else
#define MAX_FIFO_EVENT_COUNT 0
#endif

/*
 * Define the frequency to use in max_frequency based on the maximal frequency
 * the sensor support and what the EC can provide.
 * Return a frequency the sensor supports.
 * Trigger a compilation error when the EC way to slow for the sensor.
 */
#define MOTION_MAX_SENSOR_FREQUENCY(_max, _step) GENERIC_MIN( \
	(_max) / (CONFIG_EC_MAX_SENSOR_FREQ_MILLIHZ >= (_step)), \
	(_step) << __fls(CONFIG_EC_MAX_SENSOR_FREQ_MILLIHZ / (_step)))

struct motion_data_t {
	/*
	 * data rate the sensor will measure, in mHz: 0 suspended.
	 * MSB is used to know if we are rounding up.
	 */
	unsigned int odr;

	/*
	 * delay between collection by EC, in us.
	 * For non FIFO sensor, should be near 1e9/odr to
	 * collect events.
	 * For sensor with FIFO, can be much longer.
	 * 0: no collection.
	 */
	unsigned int ec_rate;
};

struct motion_sensor_t {
	/* RO fields */
	uint32_t active_mask;
	char *name;
	enum motionsensor_chip chip;
	enum motionsensor_type type;
	enum motionsensor_location location;
	const struct accelgyro_drv *drv;
	struct mutex *mutex;
	void *drv_data;

	/* i2c port */
	uint8_t port;
	/* i2c address or SPI slave logic GPIO. */
	uint8_t addr;

	/*
	 * When non-zero, spoof mode will allow the EC to report arbitrary
	 * values for any of the components.
	 */
	uint8_t in_spoof_mode;

	const mat33_fp_t *rot_standard_ref;

	/*
	 * default_range: set by default by the EC.
	 * The host can change it, but rarely does.
	 */
	int default_range;

	/*
	 * There are 4 configuration parameters to deal with different
	 * configuration
	 *
	 * Power   |         S0        |            S3     |      S5
	 * --------+-------------------+-------------------+-----------------
	 * From AP | <------- SENSOR_CONFIG_AP ----------> |
	 *         | Use for normal    | While sleeping    | Always disabled
	 *         | operation: game,  | For Activity      |
	 *         | screen rotation   | Recognition       |
	 * --------+-------------------+-------------------+------------------
	 * From EC |SENSOR_CONFIG_EC_S0|SENSOR_CONFIG_EC_S3|SENSOR_CONFIG_EC_S5
	 *         | Background        | Gesture  Recognition (Double tap, ...)
	 *         | Activity: compass,|
	 *         | ambient light)|
	 */
	struct motion_data_t config[SENSOR_CONFIG_MAX];

	/* state parameters */
	enum sensor_state state;
	intv3_t raw_xyz;
	intv3_t xyz;
	intv3_t spoof_xyz;

	/* How many flush events are pending */
	uint32_t flush_pending;

	/*
	 * Allow EC to request an higher frequency for the sensors than the AP.
	 * We will downsample according to oversampling_ratio, or ignore the
	 * samples altogether if oversampling_ratio is 0.
	 */
	uint16_t oversampling;
	uint16_t oversampling_ratio;

	/*
	 * How many vector events are lost in the FIFO since last time
	 * FIFO info has been transmitted.
	 */
	uint16_t lost;

	/*
	 * Time since last collection:
	 * For sensor with hardware FIFO,  time since last sample
	 * has move from the hardware FIFO to the FIFO (used if fifo rate != 0).
	 * For sensor without FIFO, time since the last event was collect
	 * from sensor registers.
	 */
	uint32_t last_collection;

	/* Minimum supported sampling frequency in miliHertz for this sensor */
	uint32_t min_frequency;

	/* Maximum supported sampling frequency in miliHertz for this sensor */
	uint32_t max_frequency;
};

/* Defined at board level. */
extern struct motion_sensor_t motion_sensors[];
#ifdef CONFIG_DYNAMIC_MOTION_SENSOR_COUNT
extern unsigned motion_sensor_count;
#else
extern const unsigned motion_sensor_count;
#endif
#if (!defined HAS_TASK_ALS) && (defined CONFIG_ALS)
/* Needed if reading ALS via LPC is needed */
extern const struct motion_sensor_t *motion_als_sensors[];
#endif

/* optionally defined at board level */
extern unsigned int motion_min_interval;

/*
 * Priority of the motion sense resume/suspend hooks, to be sure associated
 * hooks are scheduled properly.
 */
#define MOTION_SENSE_HOOK_PRIO (HOOK_PRIO_DEFAULT)

#ifdef CONFIG_ACCEL_FIFO
extern struct queue motion_sense_fifo;

/**
 * Add new actual data to the fifo, including a timestamp.
 *
 * @param data data to insert in the FIFO
 * @param sensor sensor the data comes from
 * @param valid_data data should be copied into the public sensor vector
 * @param time accurate time (ideally measured in an interrupt) the sample
 *             was taken at
 */
void motion_sense_fifo_add_data(struct ec_response_motion_sensor_data *data,
				struct motion_sensor_t *sensor,
				int valid_data,
				uint32_t time);

#endif

/**
 * Take actions at end of sensor initialization:
 * - print init done status to console,
 * - set default range.
 *
 * @param sensor sensor which was just initialized
 */
int sensor_init_done(const struct motion_sensor_t *sensor);

/**
 * Board specific function that is called when a double_tap event is detected.
 *
 */
void sensor_board_proc_double_tap(void);

#ifdef CONFIG_ORIENTATION_SENSOR
enum motionsensor_orientation motion_sense_remap_orientation(
		const struct motion_sensor_t *s,
		enum motionsensor_orientation orientation);
#endif

#if defined(CONFIG_GESTURE_HOST_DETECTION) || defined(CONFIG_ORIENTATION_SENSOR)
/* Add an extra sensor. We may need to add more */
#define MOTION_SENSE_ACTIVITY_SENSOR_ID (motion_sensor_count)
#define ALL_MOTION_SENSORS (MOTION_SENSE_ACTIVITY_SENSOR_ID + 1)
#else
#define ALL_MOTION_SENSORS motion_sensor_count
#endif

#ifdef CONFIG_ALS_LIGHTBAR_DIMMING
#ifdef TEST_BUILD
#define MOTION_SENSE_LUX 0
#else
#define MOTION_SENSE_LUX motion_sensors[CONFIG_ALS_LIGHTBAR_DIMMING].raw_xyz[0]
#endif
#endif

#endif /* __CROS_EC_MOTION_SENSE_H */
