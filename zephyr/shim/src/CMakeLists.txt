# Copyright 2020 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

zephyr_sources(console.c)
zephyr_sources(gpio.c)
zephyr_sources(util.c)
zephyr_sources(crc.c)

zephyr_sources_ifdef(CONFIG_PLATFORM_EC_ESPI  espi.c)
zephyr_sources_ifdef(CONFIG_PLATFORM_EC_HOOKS hooks.c)
zephyr_sources_ifdef(CONFIG_PLATFORM_EC_TIMER hwtimer.c)
zephyr_sources_ifdef(CONFIG_PLATFORM_EC_I2C   i2c.c)
zephyr_sources_ifdef(CONFIG_CROS_EC           system.c)
zephyr_sources_ifdef(CONFIG_SHIMMED_TASKS     tasks.c)
