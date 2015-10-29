/*
 * Copyright (c) 2011-2013, LGE Inc.
 * Copyright (c) 2009, Google Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <debug.h>
#include <string.h>
#include <stdlib.h>
#include <platform.h>
#include <kernel/thread.h>
#include <kernel/event.h>
#include <dev/udc.h>
#include <dev/fbcon.h>
#include <dev/keys.h>
#include <reg.h>
#include <target.h>
#include <platform/timer.h>

#include "recovery.h"
#include "fastboot.h"
#include "aboot.h"

#include <lge_target_init.h>
#include <lge_splash_screen.h>

#define FASTBOOT_MODE              0x77665500
#define REBOOT_NORMAL              0x77665501

#define MAX_BUF_SIZE               64

static int select;
static int old_select;
static int fastboot_cmd_processing;
static int start_line;

enum lk_menu {
	START,
	RESTART_BOOTLOADER,
	RECOVERY,
	POWEROFF,
};

struct menu {
	const char *name;
	const char *name_text;
};

#if LGE_WITH_FASTBOOT_MENU_TEXT
static char *lk_menu_usage[] = {
	"FASTBOOT MENU\n",
	"\n",
	"  Select  : Voume UP/DOWN Key\n",
	"  Continue: Power Key\n",
	"\n"
};

static char *lk_unlock_message[] = {
	"Unlock bootloader?\n",
	"\n",
	"If you unlock the bootloader, you will be\n",
	"able to install custome operting system\n",
	"software on the phone.\n",
	"\n",
	"A custom OS is not subject to the same\n",
	"testing as the original OS, and can cause\n",
	"your phone and installed applications to\n",
	"stop working perperly.\n",
	"\n",
	"To prevent unauthorized access to your\n",
	"personal data, unlocking the bootloader will\n",
	"also delete all personal data from your\n",
	"phone (a \"factory data reset\")\n",
	"\n",
	"Press the Volume Up/Down buttons to selct\n",
	"Yes or No. Then press the Power button to\n",
	"contiue.",
	"\n"
};

#define NUMBER_OF_LK_MENU_USAGE (sizeof(lk_menu_usage)/sizeof(char *))
#define NUMBER_OF_LK_UNLOCK_MESSAGE (sizeof(lk_unlock_message)/sizeof(char *))
#endif

static struct menu lk_menu_list[] = {
	[START] = {"start", "START\n"},
	[RESTART_BOOTLOADER] = {"bootloader", "RESTART BOOTLOADER\n"},
	[RECOVERY] = {"recovery", "RECOVERY MODE\n"},
	[POWEROFF] = {"poweroff", "POWER OFF\n"},
};

enum unlock_answer {
	UNLOCK_YES,
	UNLOCK_NO,
};

static struct menu unlock_list[] = {
	[UNLOCK_YES] = {"unlock_yes",
		"Yes - Unlock bootloader (may void warrenty)\n"},
	[UNLOCK_NO] = {"unlock_no",
		"No - Do not unlock bootloader and restart phone\n"},
};

struct information {
	int color;
	const char *str;
	const char *name;
};

static struct information lk_info_list[] = {
	{0x00FF0000, " FASTBOOT MODE", NULL},
	{0x00FFFFFF, " PRODUCT_NAME", "product"},
	{0x00FFFFFF, " VARIANT", "variant"},
	{0x00FFFFFF, " HW VERSION", "version-hardware"},
	{0x00FFFFFF, " BOOTLOADER VERSION", "version-bootloader"},
	{0x00FFFFFF, " BASEBAND VERSION", "version-baseband"},
	{0x00FFFFFF, " CARRIER INFO", "carrier"},
	{0x00FFFFFF, " SERIAL NUMBER", "serialno"},
	{0x00FFFFFF, " SIGNING", "signing"},
	{0x00FFFFFF, " SECURE BOOT", "secure-boot"},
	{0x00FFFFFF, " LOCK STATE", "unlocked"},
};

#define NUMBER_OF_LK_MENU_LIST (sizeof(lk_menu_list)/sizeof(struct menu))
#define NUMBER_OF_UNLOCK_LIST (sizeof(unlock_list)/sizeof(struct menu))
#define NUMBER_OF_LK_INFO_LIST (sizeof(lk_info_list)/sizeof(struct information))
#define KEY_DEBOUNCE_DELAY 100

static void fastboot_keys_pre_event(void)
{
	if (target_volume_up())
		keys_post_event(KEY_VOLUMEUP, 1);
	if (target_volume_down())
		keys_post_event(KEY_VOLUMEDOWN, 1);
}

static void fastboot_keys_post_event(void)
{
	int debounce_time = 400; //target_get_key_debounce_time();

	if (debounce_time)
		thread_sleep(debounce_time);
	else
		thread_sleep(KEY_DEBOUNCE_DELAY);

	keys_post_event(KEY_VOLUMEUP, 0);
	keys_post_event(KEY_VOLUMEDOWN, 0);
}

static void fastboot_menu_select(int new_select)
{
#if LGE_WITH_FASTBOOT_MENU_TEXT
	fbcon_clear_line(1 + NUMBER_OF_LK_MENU_USAGE);
	fbcon_set_colors(0x00000000, 0x00FF0000);
	fbcon_puts(lk_menu_list[new_select].name_text);
#elif LGE_WITH_SPLASH_SCREEN_IMG
	display_lge_splash_on_screen(lk_menu_list[new_select].name);
#endif

	fastboot_keys_post_event();
}

void fastboot_info_display(void)
{
	unsigned i, n;
	int max_line, start_line;

	max_line = fbcon_get_max_lines();
#if LGE_WITH_FASTBOOT_MENU_TEXT
	start_line = max_line - NUMBER_OF_LK_INFO_LIST - 2;
#else
	start_line = 0;
#endif
	if (start_line < 0) {
		dprintf(CRITICAL, "screen resolution is too low ");
		dprintf(CRITICAL, "(screen lines: %d, required lines: %d)\n",
				max_line, (NUMBER_OF_LK_INFO_LIST + 2));
		start_line = 0;
	}

	fbcon_flush_disable();
	fbcon_set_colors(0x00000000, 0x00000000);
	fbcon_clear_line(start_line);
	for (i = 0; i < NUMBER_OF_LK_INFO_LIST; i++) {
		char str[MAX_BUF_SIZE] = {0};
		struct information *info = &lk_info_list[i];
		fbcon_set_colors(0x00000000, info->color);
		fbcon_clear_line(start_line+i);
		n = snprintf(str, MAX_BUF_SIZE, "%s", info->str);
		if (lk_info_list[i].name != NULL) {
			if (!strcmp("unlocked", info->name)) {
				if (!strcmp("yes", fastboot_get_getvar(info->name))) {
					fbcon_set_colors(0x00000000,
							 0x00FF0000);
					n += snprintf(str+n, MAX_BUF_SIZE-n,
							" - unlocked");
				} else {
					n += snprintf(str+n, MAX_BUF_SIZE-n,
							" - locked");
				}
			} else if (!strcmp("secure-boot", info->name)) {
				if (!strcmp("yes", fastboot_get_getvar(info->name))) {
					fbcon_set_colors(0x00000000,
							 0x00008000);
					n += snprintf(str+n, MAX_BUF_SIZE-n,
							" - enabled");
				} else {
					n += snprintf(str+n, MAX_BUF_SIZE-n,
							" - disabled");
				}
			} else if (!strcmp("signing", info->name)) {
				n += snprintf(str+n, MAX_BUF_SIZE-n, " - %s",
					target_is_production() ?
					"production" : "development");
			} else {
				n += snprintf(str+n, MAX_BUF_SIZE-n, " - %s",
					fastboot_get_getvar(info->name));
			}
		}
		snprintf(str+n, MAX_BUF_SIZE-n, "\n");
		fbcon_puts(str);
	}
	fbcon_flush_enable();
}

void fastboot_menu_display(void)
{
#if LGE_WITH_FASTBOOT_MENU_TEXT
	unsigned i;

	fbcon_clear_color(0x00000000, 0x00FFFFFF);
	for (i = 0; i < NUMBER_OF_LK_MENU_USAGE; i++)
		fbcon_puts(lk_menu_usage[i]);

	fbcon_clear_line(1 + NUMBER_OF_LK_MENU_USAGE);
	fbcon_puts(lk_menu_list[0].name_text);
#elif LGE_WITH_SPLASH_SCREEN_IMG
	fbcon_clear_color(0x00000000, 0x00FFFFFF);
	display_lge_splash_on_screen("fastboot_op");
#endif
	fastboot_info_display();
	old_select = select = 0;
	fastboot_menu_select(select);
	mdelay(500);
}

int oem_unlock_processing;

static int fastboot_menu_handler(void *arg)
{
	int is_pwr_key_pressed = 0;

	while (!is_pwr_key_pressed || fastboot_cmd_processing) {

		if (oem_unlock_processing) {
			return 0;
		}

		fastboot_keys_pre_event();
		if (0 != keys_get_state(KEY_VOLUMEUP)) {
			select = (select + NUMBER_OF_LK_MENU_LIST - 1)
				% NUMBER_OF_LK_MENU_LIST;
		} else if (0 != keys_get_state(KEY_VOLUMEDOWN)) {
			select = (select + 1) % NUMBER_OF_LK_MENU_LIST;
		}

		if (select != old_select) {
			fastboot_menu_select(select);
			old_select = select;
		}
		is_pwr_key_pressed = target_power_key();
		if (keys_get_state(KEY_CENTER)) {
			is_pwr_key_pressed |= 1;
			keys_post_event(KEY_CENTER, 0);
		}

	}

	fastboot_okay("");
	fbcon_clear_color(0x00000000, 0x00FFFFFF);

	switch (select) {
	case RECOVERY:
		boot_into_recovery = 1;
	case START:
		udc_stop();
#ifdef LGE_WITH_SPLASH_SCREEN_IMG
		display_lge_splash_screen("lglogo_image");
		display_lge_splash_screen("powered_android_image");
#endif
		if (target_is_emmc_boot()) {
			if (emmc_recovery_init())
				dprintf(ALWAYS, "error in emmc_recovery_init\n");
			boot_linux_from_mmc();
		} else {
			if (recovery_init())
				dprintf(ALWAYS, "error in recovery_init\n");
			boot_linux_from_flash();
		}
		break;
	case POWEROFF:
		shutdown_device();
		break;
	case RESTART_BOOTLOADER:
	default:
		reboot_device(FASTBOOT_MODE);
		break;
	}
	return 0;
}

static thread_t *fastboot_menu_thread;

static void fastboot_menu_suspend(void)
{
	if (fastboot_menu_thread)
		thread_suspend(fastboot_menu_thread);
}

static void fastboot_menu_resume(void)
{
	if (fastboot_menu_thread)
		thread_resume(fastboot_menu_thread);
}

static void fastboot_unlock_select(int new_select)
{
#if LGE_WITH_FASTBOOT_MENU_TEXT
	fbcon_clear_line(1 + NUMBER_OF_LK_UNLOCK_MESSAGE);
	fbcon_set_colors(0x00000000, 0x00FF0000);
	fbcon_puts(unlock_list[new_select].name_text);
#elif LGE_WITH_SPLASH_SCREEN_IMG
	display_lge_splash_on_screen(unlock_list[new_select].name);
#endif
	fastboot_keys_post_event();
}

static void fastboot_unlock_display(void)
{
#if LGE_WITH_FASTBOOT_MENU_TEXT
	unsigned i;

	fbcon_clear_color(0x00000000, 0x00FFFFFF);
	for (i = 0; i < NUMBER_OF_LK_UNLOCK_MESSAGE; i++)
		fbcon_puts(lk_unlock_message[i]);
#elif LGE_WITH_SPLASH_SCREEN_IMG
	fbcon_clear_color(0x00000000, 0x00FFFFFF);
	display_lge_splash_on_screen("oem_unlock");
	display_lge_splash_on_screen("unlocked");
#endif
	old_select = select = UNLOCK_NO;
	fastboot_unlock_select(select);
	mdelay(500);
}

int fastboot_do_unlock(void)
{
	int is_pwr_key_pressed = 0;

	oem_unlock_processing = 1;
	fastboot_unlock_display();

	while (!is_pwr_key_pressed) {
		fastboot_keys_pre_event();
		if (0 != keys_get_state(KEY_VOLUMEUP)) {
			select = (select + NUMBER_OF_UNLOCK_LIST - 1)
				% NUMBER_OF_UNLOCK_LIST;
		} else if (0 != keys_get_state(KEY_VOLUMEDOWN)) {
			select = (select + 1) % NUMBER_OF_UNLOCK_LIST;
		}

		if (select != old_select) {
			fastboot_unlock_select(select);
			old_select = select;
		}
		is_pwr_key_pressed = target_power_key();
		if (keys_get_state(KEY_CENTER)) {
			is_pwr_key_pressed |= 1;
			keys_post_event(KEY_CENTER, 0);
		}
	}

	fbcon_clear_color(0x00000000, 0x00FFFFFF);

	oem_unlock_processing = 0;

	if (select == UNLOCK_NO)
		return 0;

	return 1;
}

void fastboot_progress_display(const char *s)
{
	int max_line;

	max_line = fbcon_get_max_lines();
	start_line = max_line - 2;

	if (start_line < 0) {
		dprintf(CRITICAL,
			"screen resolution is too low screen lines: %d, required lines: %d)\n",
			max_line, 1);
		start_line = 0;
	}

	fbcon_set_colors(0x00000000, 0x00000000);
	fbcon_clear_line(start_line);

	if (strlen(s)) {
		fbcon_set_colors(0x00000000, 0x00008000);
		fbcon_puts("    ");
		fbcon_puts(s);
		fbcon_puts("\n");
	}
}

void fastboot_menu_init(void)
{
	fastboot_menu_display();
#if LGE_WITH_FASTBOOT_MENU_TEXT
	fastboot_menu_thread = thread_create("fastboot_menu",
			fastboot_menu_handler, 0, DEFAULT_PRIORITY, 4096);

	fastboot_menu_resume();
#endif
}

