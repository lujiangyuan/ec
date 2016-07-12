/* Copyright 2016 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Battery pack vendor provided charging profile
 */

#include "battery.h"
#include "battery_smart.h"
#include "util.h"

/* Shutdown mode parameter to write to manufacturer access register */
#define SB_SHUTDOWN_DATA	0x0010

static const struct battery_info BYD_info = {
	.voltage_max = 13200,/* mV */
	.voltage_normal = 11400,
	.voltage_min = 9000,
	.precharge_current = 128,/* mA */
	.start_charging_min_c = 0,
	.start_charging_max_c = 60,
	.charging_min_c = 0,
	.charging_max_c = 60,
	.discharging_min_c = 0,
	.discharging_max_c = 70,
};

static const struct battery_info LGC_info = {
	.voltage_max = 13200,/* mV */
	.voltage_normal = 11400,
	.voltage_min = 9000,
	.precharge_current = 256,/* mA */
	.start_charging_min_c = -3,
	.start_charging_max_c = 50,
	.charging_min_c = -3,
	.charging_max_c = 60,
	.discharging_min_c = -20,
	.discharging_max_c = 70,
};

const struct battery_info *battery_get_info(void)
{
	const struct battery_info *ref = &BYD_info;
	char manuf[4];

	battery_manufacturer_name(manuf, sizeof(manuf));

	if (!strcasecmp(manuf, "LGC")) {
		ref = &LGC_info;
	}

	return ref;
}

int board_cut_off_battery(void)
{
	int rv;

	/* Ship mode command must be sent twice to take effect */
	rv = sb_write(SB_MANUFACTURER_ACCESS, SB_SHUTDOWN_DATA);

	if (rv != EC_SUCCESS)
		return rv;

	return sb_write(SB_MANUFACTURER_ACCESS, SB_SHUTDOWN_DATA);
}
