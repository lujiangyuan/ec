# Copyright 2019 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Build for FPC fingerprint drivers

# Note that this variable includes the trailing "/"
_fpc_cur_dir:=$(dir $(lastword $(MAKEFILE_LIST)))

ifeq ($(CONFIG_FP_SENSOR_FPC1145),rw)
include $(_fpc_cur_dir)libfp/build.mk
else ifeq ($(CONFIG_FP_SENSOR_FPC1025),rw)
include $(_fpc_cur_dir)bep/build.mk
else ifeq ($(CONFIG_FP_SENSOR_FPC1035),rw)
include $(_fpc_cur_dir)bep/build.mk
endif

ifneq ($(CONFIG_FINGERPRINT_MCU),)
# Make sure output directory is created (in build directory)
dirs-rw+="$(_fpc_cur_dir)"

# Only build these objects for the RW image
all-obj-rw+=$(_fpc_cur_dir)fpc_sensor.o
endif
