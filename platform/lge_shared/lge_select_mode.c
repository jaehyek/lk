/* Copyright (c) 2013-2014, LGE Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of The Linux Foundation, Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <debug.h>
#include <string.h>
#include <platform/timer.h>
#include <kernel/thread.h>

#include <lge_bootcmd.h>
#include <lge_bootmode.h>
#include <lge_select_mode.h>
#include <lge_target_init.h>


static int selectmode_menu_cnt;
static selectmode_menu_t *selectmode_menu;


static inline void selectmode_clear_key_combo(selectmode_key_combo_t *key_combo)
{
	if (key_combo != NULL) {
		key_combo->vol_up	= SM_KEY_RELEASED;
		key_combo->vol_down	= SM_KEY_RELEASED;
		key_combo->power	= SM_KEY_RELEASED;
		key_combo->home		= SM_KEY_RELEASED;
	}
}

static inline int selectmode_has_pressed_key(selectmode_key_combo_t *key_combo)
{
	if (key_combo != NULL)
		return key_combo->vol_up | key_combo->vol_down | key_combo->home;

	return 0;
}

static void selectmode_get_current_key_info(selectmode_key_combo_t *key_combo)
{
	if (key_combo != NULL) {
		selectmode_clear_key_combo(key_combo);

		key_combo->vol_up	= target_volume_up();
		key_combo->vol_down	= target_volume_down();
		key_combo->power	= target_power_key();
		key_combo->home		= target_home_key();

		dprintf(INFO, "%s: vol_up=%d, vol_down=%d, power=%d, home=%d\n", __func__,
					key_combo->vol_up,
					key_combo->vol_down,
					key_combo->power,
					key_combo->home);
	}
}

static inline void selectmode_update_long_double_key(
					selectmode_key_status_t *key_begin,
					selectmode_key_status_t key_mid,
					selectmode_key_status_t key_end1,
					selectmode_key_status_t key_end2)
{
	if (*key_begin == SM_KEY_PRESSED) {
		if (key_mid == SM_KEY_PRESSED
				&& key_end1 == SM_KEY_PRESSED
				&& key_end2 == SM_KEY_PRESSED) {
			*key_begin = SM_KEY_LONG_PRESSED;
		} else if (key_mid == SM_KEY_RELEASED && key_end1 == SM_KEY_PRESSED) {
			*key_begin = SM_KEY_DOUBLE_PRESSED;
		}
	}
}

int selectmode_has_changed_key(selectmode_key_combo_t *combo, selectmode_key_combo_t *new_combo)
{
	if (combo->vol_up == new_combo->vol_up
			&& combo->vol_down == new_combo->vol_down
			&& combo->power == new_combo->power
			&& combo->home == new_combo->home) {

		return 0;
	} else {
		return 1;
	}
}

static void selectmode_check_long_double_key(selectmode_key_combo_t *key_combo,
						int long_key_time, int double_key_time)
{
	selectmode_key_combo_t	extra_key_combo[3];

	int elapsed_long_key_time = 0;
	int elapsed_double_key_time = 0;

	if (key_combo != NULL) {
		elapsed_long_key_time = 0;

		// first, check long key
		do {
			mdelay(SM_CHECK_KEY_TIMER_STEP);
			elapsed_long_key_time += SM_CHECK_KEY_TIMER_STEP;
			selectmode_get_current_key_info(&extra_key_combo[0]);
			if (selectmode_has_changed_key(key_combo, &extra_key_combo[0])) {
				break;  // key released
			}
		} while (elapsed_long_key_time < long_key_time);


		if (elapsed_long_key_time >= long_key_time) {
			extra_key_combo[2] = extra_key_combo[1] = extra_key_combo[0];
		} else {
			elapsed_double_key_time = 0;

			// second, check double key
			do {
				mdelay(SM_CHECK_KEY_TIMER_STEP);
				elapsed_long_key_time += SM_CHECK_KEY_TIMER_STEP;
				elapsed_double_key_time += SM_CHECK_KEY_TIMER_STEP;
				selectmode_get_current_key_info(&extra_key_combo[1]);

				if (selectmode_has_changed_key(&extra_key_combo[0],
								&extra_key_combo[1])) {
					break;  // key pressed
				}
			} while (elapsed_double_key_time < double_key_time);

			extra_key_combo[2] = extra_key_combo[1];

			// spend rest of time for checking long key,
			// if checking time for double key is too short
			while (elapsed_long_key_time < long_key_time) {
				mdelay(SM_CHECK_KEY_TIMER_STEP);
				elapsed_long_key_time += SM_CHECK_KEY_TIMER_STEP;
				selectmode_get_current_key_info(&extra_key_combo[2]);
			}
		}

		// determine the key status
		selectmode_update_long_double_key(&key_combo->vol_up,
						extra_key_combo[0].vol_up,
						extra_key_combo[1].vol_up ,
						extra_key_combo[2].vol_up);
		selectmode_update_long_double_key(&key_combo->vol_down,
						extra_key_combo[0].vol_down,
						extra_key_combo[1].vol_down,
						extra_key_combo[2].vol_down);
		selectmode_update_long_double_key(&key_combo->power,
						extra_key_combo[0].power,
						extra_key_combo[1].power,
						extra_key_combo[2].power);
		selectmode_update_long_double_key(&key_combo->home,
						extra_key_combo[0].home,
						extra_key_combo[1].home,
						extra_key_combo[2].home);

		dprintf(INFO, "final key combination: vol_up=%d, vol_down=%d, power=%d, home=%d\n",
					key_combo->vol_up,
					key_combo->vol_down,
					key_combo->power,
					key_combo->home);
	}
}



static int selectmode_check_factory_usb_cable(void)
{
	usb_cable_type cable_type = bootmode_get_cable();
	if (cable_type == LT_CABLE_56K || cable_type == LT_CABLE_130K)
		return 1;
	else
		return 0;
}

static selectmode_usb_cable_status_t selectmode_check_usb_cable(void)
{
	if (selectmode_check_factory_usb_cable()) {
		dprintf(INFO, "%s: with factory usb cable\n", __func__);
		return SM_HAVE_FACTORY_USB_CABLE;
	}

	if (bootmode_get_cable() == NO_INIT_CABLE || bootmode_get_port() == CHARGER_PORT_DCP) {
		dprintf(INFO, "%s: without usb cable\n", __func__);
		return SM_NOT_HAVE_USB_CABLE;
	} else {
		dprintf(INFO, "%s: with usb cable. not factory cable\n", __func__);
		return SM_HAVE_USB_CABLE;
	}
}

selectmode_mode_t selectmode_get_mode(void)
{
	int i;
	int check_key_press_time = 0;
	selectmode_key_combo_t		key_combo;
	selectmode_usb_cable_status_t	usb_cable = SM_DONOT_CARE_USB_CABLE;
	selectmode_menu_t		*candidate = NULL;

	// get key press and usb cable status
	selectmode_get_current_key_info(&key_combo);
	usb_cable = selectmode_check_usb_cable();

	// no key combination
	if (!selectmode_has_pressed_key(&key_combo))
		return SM_NONE;

	// first, check short time key combination
	for (i = 0; i < selectmode_menu_cnt; i++) {
		// 1. check key combination
		if (!memcmp(&key_combo, &(selectmode_menu[i].key_combo), sizeof(key_combo))) {
			// 2. check usb cable
			if (selectmode_menu[i].usb_cable == SM_DONOT_CARE_USB_CABLE)
				candidate = &selectmode_menu[i];
			else if (selectmode_menu[i].usb_cable & usb_cable)
				return selectmode_menu[i].mode;
		} else if ((selectmode_menu[i].key_combo.vol_up & SM_KEY_PRESSED)
						== key_combo.vol_up
					&& (selectmode_menu[i].key_combo.vol_down & SM_KEY_PRESSED)
						== key_combo.vol_down
					&& (selectmode_menu[i].key_combo.power & SM_KEY_PRESSED)
						== key_combo.power
					&& (selectmode_menu[i].key_combo.home & SM_KEY_PRESSED)
						== key_combo.home) {
			// different key press time case
			if (selectmode_menu[i].usb_cable & usb_cable)
				check_key_press_time = 1;
		}
	}

	// second, check long key or double key combination if need to.
	if (check_key_press_time) {
		selectmode_check_long_double_key(&key_combo,
					SM_LONG_KEY_TIME_IN_MS, SM_DOUBLE_KEY_TIME_IN_MS);

		for (i = 0; i < selectmode_menu_cnt; i++) {
			// 1. check key combination
			if (!memcmp(&key_combo,
					&(selectmode_menu[i].key_combo), sizeof(key_combo))) {
				// 2. check usb cable
				if (selectmode_menu[i].usb_cable == SM_DONOT_CARE_USB_CABLE)
					candidate = &selectmode_menu[i];
				else if (selectmode_menu[i].usb_cable & usb_cable)
					return selectmode_menu[i].mode;
			}
		}
	}

	// third, check candidate
	if (candidate)
		return candidate->mode;

	return SM_NONE;

}

int selectmode_set_new_menu_combo(selectmode_menu_t *menu, int menu_cnt)
{
	selectmode_menu		= menu;
	selectmode_menu_cnt	= menu_cnt;

	return selectmode_menu_cnt;
}

int selectmode_update_combo(selectmode_mode_t mode, selectmode_menu_t *menu)
{
	int i;

	if (menu) {
		for (i = 0; i < selectmode_menu_cnt; i++) {
			if (mode == selectmode_menu[i].mode) {
				selectmode_menu[i] = *menu;
				return 0;
			}
		}
	}

	// fail to update
	return -1;
}

