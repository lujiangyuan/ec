/* Copyright 2020 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Alternate Mode Downstream Facing Port (DFP) USB-PD module.
 */

#include "console.h"
#include "usb_pd.h"
#include "util.h"

#ifdef CONFIG_COMMON_RUNTIME
#define CPRINTS(format, args...) cprints(CC_USBPD, format, ## args)
#define CPRINTF(format, args...) cprintf(CC_USBPD, format, ## args)
#else
#define CPRINTS(format, args...)
#define CPRINTF(format, args...)
#endif

static int pd_get_mode_idx(int port, uint16_t svid)
{
	int i;
	struct pd_policy *pe = pd_get_am_policy(port);

	for (i = 0; i < PD_AMODE_COUNT; i++) {
		if (pe->amodes[i].fx &&
		    (pe->amodes[i].fx->svid == svid))
			return i;
	}
	return -1;
}

static int pd_allocate_mode(int port, uint16_t svid)
{
	int i, j;
	struct svdm_amode_data *modep;
	int mode_idx = pd_get_mode_idx(port, svid);
	struct pd_policy *pe = pd_get_am_policy(port);

	if (mode_idx != -1)
		return mode_idx;

	/* There's no space to enter another mode */
	if (pe->amode_idx == PD_AMODE_COUNT) {
		CPRINTF("ERR:NO AMODE SPACE\n");
		return -1;
	}

	/* Allocate ...  if SVID == 0 enter default supported policy */
	for (i = 0; i < supported_modes_cnt; i++) {
		for (j = 0; j < pe->svid_cnt; j++) {
			struct svdm_svid_data *svidp = &pe->svids[j];

			if ((svidp->svid != supported_modes[i].svid) ||
			    (svid && (svidp->svid != svid)))
				continue;

			modep = &pe->amodes[pe->amode_idx];
			modep->fx = &supported_modes[i];
			modep->data = &pe->svids[j];
			pe->amode_idx++;
			return pe->amode_idx - 1;
		}
	}
	return -1;
}

/*
 * This algorithm defaults to choosing higher pin config over lower ones in
 * order to prefer multi-function if desired.
 *
 *  NAME | SIGNALING | OUTPUT TYPE | MULTI-FUNCTION | PIN CONFIG
 * -------------------------------------------------------------
 *  A    |  USB G2   |  ?          | no             | 00_0001
 *  B    |  USB G2   |  ?          | yes            | 00_0010
 *  C    |  DP       |  CONVERTED  | no             | 00_0100
 *  D    |  PD       |  CONVERTED  | yes            | 00_1000
 *  E    |  DP       |  DP         | no             | 01_0000
 *  F    |  PD       |  DP         | yes            | 10_0000
 *
 * if UFP has NOT asserted multi-function preferred code masks away B/D/F
 * leaving only A/C/E.  For single-output dongles that should leave only one
 * possible pin config depending on whether its a converter DP->(VGA|HDMI) or DP
 * output.  If UFP is a USB-C receptacle it may assert C/D/E/F.  The DFP USB-C
 * receptacle must always choose C/D in those cases.
 */
int pd_dfp_dp_get_pin_mode(int port, uint32_t status)
{
	struct svdm_amode_data *modep =
				pd_get_amode_data(port, USB_SID_DISPLAYPORT);
	uint32_t mode_caps;
	uint32_t pin_caps;

	if (!modep)
		return 0;

	mode_caps = modep->data->mode_vdo[modep->opos - 1];

	/* TODO(crosbug.com/p/39656) revisit with DFP that can be a sink */
	pin_caps = PD_DP_PIN_CAPS(mode_caps);

	/* if don't want multi-function then ignore those pin configs */
	if (!PD_VDO_DPSTS_MF_PREF(status))
		pin_caps &= ~MODE_DP_PIN_MF_MASK;

	/* TODO(crosbug.com/p/39656) revisit if DFP drives USB Gen 2 signals */
	pin_caps &= ~MODE_DP_PIN_BR2_MASK;

	/* if C/D present they have precedence over E/F for USB-C->USB-C */
	if (pin_caps & (MODE_DP_PIN_C | MODE_DP_PIN_D))
		pin_caps &= ~(MODE_DP_PIN_E | MODE_DP_PIN_F);

	/* get_next_bit returns undefined for zero */
	if (!pin_caps)
		return 0;

	return 1 << get_next_bit(&pin_caps);
}

struct svdm_amode_data *pd_get_amode_data(int port, uint16_t svid)
{
	int idx = pd_get_mode_idx(port, svid);
	struct pd_policy *pe = pd_get_am_policy(port);

	return (idx == -1) ? NULL : &pe->amodes[idx];
}

/*
 * Enter default mode ( payload[0] == 0 ) or attempt to enter mode via svid &
 * opos
 */
uint32_t pd_dfp_enter_mode(int port, uint16_t svid, int opos)
{
	int mode_idx = pd_allocate_mode(port, svid);
	struct pd_policy *pe = pd_get_am_policy(port);
	struct svdm_amode_data *modep;
	uint32_t mode_caps;

	if (mode_idx == -1)
		return 0;
	modep = &pe->amodes[mode_idx];

	if (!opos) {
		/* choose the lowest as default */
		modep->opos = 1;
	} else if (opos <= modep->data->mode_cnt) {
		modep->opos = opos;
	} else {
		CPRINTF("opos error\n");
		return 0;
	}

	mode_caps = modep->data->mode_vdo[modep->opos - 1];
	if (modep->fx->enter(port, mode_caps) == -1)
		return 0;

	pd_set_dfp_enter_mode_flag(port, true);

	/* SVDM to send to UFP for mode entry */
	return VDO(modep->fx->svid, 1, CMD_ENTER_MODE | VDO_OPOS(modep->opos));
}