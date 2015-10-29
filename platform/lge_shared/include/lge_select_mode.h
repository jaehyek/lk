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

#ifndef __LGE_SELECT_MODE_H
#define __LGE_SELECT_MODE_H

#define SM_CHECK_KEY_TIMER_STEP	        200
#define SM_LONG_KEY_TIME_IN_MS	        4000
#define SM_DOUBLE_KEY_TIME_IN_MS	3000

typedef enum {
	SM_NONE,
	SM_NATIVE_RECOVERY,
	SM_FACTORY_RESET,
	SM_FASTBOOT_DL,
	SM_LCDBREAK,
	SM_LAF_DOWNLOAD,
	SM_DEBUG,
} selectmode_mode_t;

typedef enum {
	SM_KEY_RELEASED		= 0x0,
	SM_KEY_PRESSED		= 0x1,
	SM_KEY_LONG_PRESSED	= (SM_KEY_PRESSED | 0x2),
	SM_KEY_DOUBLE_PRESSED = (SM_KEY_PRESSED | 0x4),
} selectmode_key_status_t;

typedef enum {
	SM_NOT_HAVE_USB_CABLE		= 1, // (001b) no usb cable
	SM_HAVE_USB_CABLE		= 2, // (010b) usb cable except factory cable
	SM_HAVE_FACTORY_USB_CABLE	= 4, // (100b) factory cable

	SM_HAVE_ANY_USB_CABLE		= 6, // (010b | 100b) usb cable + factory cable
	SM_DONOT_CARE_USB_CABLE		= 7, // (001b | 010b | 100b) // cover all case
} selectmode_usb_cable_status_t;

typedef struct {
	selectmode_key_status_t		vol_up;
	selectmode_key_status_t		vol_down;
	selectmode_key_status_t		power;
	selectmode_key_status_t		home;
} selectmode_key_combo_t;

typedef struct {
	selectmode_mode_t		mode;
	selectmode_usb_cable_status_t	usb_cable;
	selectmode_key_combo_t		key_combo;
} selectmode_menu_t;


extern int selectmode_set_new_menu_combo(selectmode_menu_t *menu, int menu_cnt);
extern int selectmode_update_combo(selectmode_mode_t mode, selectmode_menu_t *menu);

extern selectmode_mode_t selectmode_get_mode(void);


#endif
