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
#include <dev/keys.h>
#include <dev/fbcon.h>
#include <recovery.h>

#include <malloc.h>
#include <string.h>
#include <platform.h>
#include <platform/timer.h>

#include <lge_select_mode.h>
#include <lge_bootmode.h>
#include <lge_target_init.h>
#include <lge_target_select_mode.h>
#include <lge_splash_screen.h>
#include <lge_ftm.h>
#include <lge_bootcmd.h>

#ifdef WITH_LGE_KSWITCH
#include <lge_kswitch.h>
#endif

/*
 * It should be defined at other file (eg. init.c)
 *
 *  target_power_key()
 *  target_volume_down()
 *  target_volume_up()
 *  target_home_key()
 *  target_get_pon_usb_chg()
 */

__WEAK uint32_t target_power_key(void)
{
	dprintf(CRITICAL, "%s: should be implemented\n", __func__);
	return 0;
}
__WEAK uint32_t target_volume_down(void)
{
	dprintf(CRITICAL, "%s: should be implemented\n", __func__);
	return 0;
}
__WEAK uint32_t target_volume_up(void)
{
	dprintf(CRITICAL, "%s: should be implemented\n", __func__);
	return 0;
}
__WEAK uint32_t target_home_key(void)
{
	dprintf(CRITICAL, "%s: should be implemented\n", __func__);
	return 0;
}
__WEAK int target_get_pon_usb_chg(void)
{
	dprintf(CRITICAL, "%s: should be implemented\n", __func__);
	return 0;
}

static selectmode_menu_t target_selectmode_menu[] = {
#if LGE_WITH_FACTORY_RSET_METHOD == 1
		// Method 1
		//	Used : W/X Series, G2 MINI, G, G Pro, G2, G4
		//
		//	 vol down & power press
		//	 -> power release in 150ms * 50
		//	 -> power press in 150ms * 20
		{
			.mode		= SM_FACTORY_RESET,
			.usb_cable	= SM_DONOT_CARE_USB_CABLE,
			.key_combo	= {
				.vol_up 	= SM_KEY_RELEASED,
				.vol_down	= SM_KEY_LONG_PRESSED,
				.power		= SM_KEY_DOUBLE_PRESSED,
				.home		= SM_KEY_RELEASED
			},
		},
#endif

#if LGE_WITH_FACTORY_RSET_METHOD == 2
		// Method 2
		//	-- NOT SUPPORTED --
		//	Used : G Flex
		//
		//	 vol down & power press
		//	 -> vol down & power release in 150ms * 50
		//	 -> vol down & vodl up press long(150ms * 4) in 150ms * 20
		{
			.mode		= SM_FACTORY_RESET,
			.usb_cable	= SM_DONOT_CARE_USB_CABLE,
			.key_combo	= {
				.vol_up 	= SM_KEY_AFTER_PRESSED,
				.vol_down	= SM_KEY_DOUBLE_PRESSED,
				.power		= SM_KEY_PRESSED,
				.home		= SM_KEY_RELEASED
			},
		},
#endif

#if LGE_WITH_FACTORY_RSET_METHOD == 3
		// Method 3
		//	-- It could be support by update 'selectmode_update_long_double_key()'
		//	-- by adding a case 'begin=pressed, middle=release, end1=release'
		//	-- named 'SM_KEY_CLICK_PRESSED'
		//	Used : L2 Series
		//
		//	 vol down & power & home press
		//	 -> vol down & power & home release in 150ms * 50
		{
			.mode		= SM_FACTORY_RESET,
			.usb_cable	= SM_DONOT_CARE_USB_CABLE,
			.key_combo	= {
				.vol_up 	= SM_KEY_RELEASED,
				.vol_down	= SM_KEY_CLICK_PRESSED,
				.power		= SM_KEY_CLICK_PRESSED,
				.home		= SM_KEY_CLICK_PRESSED,
			},
		},
#endif

	/* Sprint only */
#if LGE_WITH_FACTORY_RSET_METHOD == 4
		// for sprint spec
		// power+up -> LG log -> power release -> power re-pressed
		{
			.mode		= SM_FACTORY_RESET,
			.usb_cable	= SM_DONOT_CARE_USB_CABLE,
			.key_combo	= {
				.vol_up 	= SM_KEY_LONG_PRESSED,
				.vol_down	= SM_KEY_RELEASED,
				.power		= SM_KEY_DOUBLE_PRESSED,
				.home		= SM_KEY_RELEASED
			},
		},
#endif

	{
		.mode		= SM_FASTBOOT_DL,
		.usb_cable	= SM_HAVE_ANY_USB_CABLE,
		.key_combo	= {
			.vol_up		= SM_KEY_RELEASED,
			.vol_down	= SM_KEY_PRESSED,
			.power		= SM_KEY_RELEASED,
			.home		= SM_KEY_RELEASED
		},
	},
	{
		.mode		= SM_DEBUG,
		.usb_cable	= SM_DONOT_CARE_USB_CABLE,
		.key_combo	= {
			.vol_up		= SM_KEY_RELEASED,
			.vol_down	= SM_KEY_PRESSED,
			.power		= SM_KEY_PRESSED,
			.home		= SM_KEY_RELEASED
		},
	},

	/* Emergency mode */
	{
		.mode		= SM_LAF_DOWNLOAD,
		.usb_cable	= SM_HAVE_ANY_USB_CABLE,
		.key_combo	= {
			.vol_up		= SM_KEY_PRESSED,
			.vol_down	= SM_KEY_RELEASED,
			.power		= SM_KEY_RELEASED,
			.home		= SM_KEY_RELEASED
		},
	},

	/* HW key control mode: power+up+down (6secs) */
	{
		.mode		= SM_LCDBREAK,
		.usb_cable	= SM_DONOT_CARE_USB_CABLE,
		.key_combo	= {
			.vol_up		= SM_KEY_LONG_PRESSED,
			.vol_down	= SM_KEY_LONG_PRESSED,
			.power		= SM_KEY_LONG_PRESSED,
			.home		= SM_KEY_RELEASED
		},
	},
};

#define KEY_SENSITIVITY_TICK 400

extern void target_keystatus();

static int wait_vol_keys_release(int max_cnt)
{
    int loop_cnt = 0;

    do
    {
        loop_cnt++;
		if (loop_cnt > max_cnt)       /* too long pressed */
            return 0;
        mdelay(50);
        target_keystatus();
    } while (keys_get_state(KEY_VOLUMEUP) || keys_get_state(KEY_VOLUMEDOWN));

    if (loop_cnt)
        return 1;

    return 0;
}

static int check_power_key_release_detect(int max_cnt)
{
    int loop_cnt = 0;

    while (target_power_key())
    {
        loop_cnt++;
		if (loop_cnt > max_cnt)			/* too long pressed */
			return 0;
        mdelay(150);
    }

    if (loop_cnt)
        return 1;

    return 0;
}

void wait_all_key_release()
{
    target_keystatus();
    while( 0 != keys_get_state(KEY_VOLUMEUP) || 0 != keys_get_state(KEY_VOLUMEDOWN) \
           || 0 != target_power_key() )
    {
        mdelay(100);
        target_keystatus();
    }
}

int wait_pwr_or_any_key()
{
    int any_key_toggled = 0;

    // 1st step : wait to release all key
    wait_all_key_release();

    // 2nd step : wait to press any key
    while (!any_key_toggled)
    {
        target_keystatus();
        if (0 != keys_get_state(KEY_VOLUMEUP) || 0 != keys_get_state(KEY_VOLUMEDOWN))
        {
            if (wait_vol_keys_release(50))
                any_key_toggled = 1;
        }
        if (check_power_key_release_detect(50))
            return 1;
    }
    return 0;
}

enum cancel_on_off_menu {
    MENU_CANCEL,
    MENU_ON,
    MENU_OFF,
};

enum no_yes_menu
{
    MENU_NO,
    MENU_YES,
};

enum ok_dontshow_menu
{
    MENU_OK,
    MENU_DONT_SHOW,
};

struct menu {
    const char *image_name;
};
#define NUMBER_OF_MENU_LIST(l) (sizeof(l)/sizeof(struct menu))
struct menu lcdbreak_menu_list[] = {
	[MENU_CANCEL] = {"lcd_break_cancel_image"},
	[MENU_ON] = {"lcd_break_on_image"},
	[MENU_OFF] = {"lcd_break_off_image"},
};

struct menu lcdbreak_menu_2_list[] = {
	[MENU_OK] = {"lcd_break_ok_image"},
	[MENU_DONT_SHOW] = {"lcd_break_dontshow_image"},
};

struct menu factory_reset_menu_1st_list[] = {
	[MENU_NO] = {"factory_reset_no_1st_image"},
	[MENU_YES] = {"factory_reset_yes_1st_image"},
};

struct menu factory_reset_menu_2nd_list[] = {
	[MENU_NO] = {"factory_reset_no_2nd_image"},
	[MENU_YES] = {"factory_reset_yes_2nd_image"},
};

#ifdef VZW_HARD_KEY_FACTORY_RESET_SCENARIO
enum system_recovery_menu
{
	SYS_RECOVERY_CONTINUE_POWERUP,
	SYS_RECOVERY_SAFE_MODE,
	SYS_RECOVERY_USB_DEBUG_MODE,
	SYS_RECOVERY_FACTORY_DATA_RESET,
	SYS_RECOVERY_WIPE_CACHE,
	SYS_RECOVERY_POWER_DOWN,
};

struct menu system_recovery_menu_list[] = {
	[SYS_RECOVERY_CONTINUE_POWERUP] = {"system_recovery_continue_image"},
	[SYS_RECOVERY_SAFE_MODE] = {"system_recovery_safemode_image"},
	[SYS_RECOVERY_USB_DEBUG_MODE] = {"system_recovery_usbmode_image"},
	[SYS_RECOVERY_FACTORY_DATA_RESET] = {"system_recovery_factoryreset_image"},
	[SYS_RECOVERY_WIPE_CACHE] = {"system_recovery_wipecache_image"},
	[SYS_RECOVERY_POWER_DOWN] = {"system_recovery_powerdown_image"},
};
#endif

static void select_menu(struct menu *menu_list, int new_select)
{
        time_t current;

        display_lge_splash_screen(menu_list[new_select].image_name);

        current = current_time();
        while (current + KEY_SENSITIVITY_TICK > current_time())
                ;
        keys_post_event(KEY_VOLUMEUP, 0);
        keys_post_event(KEY_VOLUMEDOWN, 0);
}
#ifdef WITH_LGE_LCD_BREAK_MODE
#define BAR_CODE_HEIGHT 120
#define MARGIN 20
#else
#define BAR_CODE_HEIGHT 350
#define MARGIN 12
#endif


#if defined(LGE_MIPI_DSI_P1_INCELL_QHD_CMD_PANEL)
#define FTM_START_Y 328
#else
#define FTM_START_Y 351
#endif
#define FONT_WIDTH 30
#define FONT_HEIGHT 40
#define SLOT_HEIGHT (BAR_CODE_HEIGHT + FONT_HEIGHT + MARGIN*2)
#define BAR_CODE_128_LEN 33
#define BAR_CODE_BACKGROUND_START_Y 1250

static void display_ftm_item(int ftm_item, int ftm_item_size, const char *item_name, int index)
{
    struct fbcon_config *config = fbcon_display();
    int x, y;
    int code_pos;
    int buf_size = strlen(item_name)+2+ftm_item_size+1;
    char *buf = (char*)malloc(buf_size);
    memset(buf, '\0', buf_size);
    sprintf(buf, "%s: ", item_name);
    code_pos = strlen(buf);
    if (0 == ftm_get_item(ftm_item, buf+code_pos))
    {
#ifdef WITH_LGE_LCD_BREAK_MODE
        y = config->height - SLOT_HEIGHT*index;
#else
	   y = FTM_START_Y + SLOT_HEIGHT*index;
#endif
//      x = (config->width - FONT_WIDTH*strlen(buf)) / 2;
        x = 70;
		display_string(buf, x, y, FONT_WIDTH, FONT_HEIGHT);
//      x = (config->width - (BAR_CODE_128_LEN*(strlen(buf)-code_pos+3)+6)) / 2; // total barcode length = start code(33) + value code(33*value length) + checksum code(33) + end code(39)
        x = 70;
		display_bar(buf+code_pos, strlen(buf)-code_pos, BAR_CODE_HEIGHT, x, y+FONT_HEIGHT+MARGIN);
    }
	else
	{
		y = config->height - SLOT_HEIGHT*index;
		x = (config->width - FONT_WIDTH*strlen(buf)) / 2;
		display_string(buf, x, y, FONT_WIDTH, FONT_HEIGHT);
	}
    free(buf);
}

int check_ftm_item(int ftm_item, int ftm_item_size)
{
	char *temp = (char *)malloc(ftm_item_size);
	if(!temp)
	{
		temp = NULL;
		return 0;
	}
	memset(temp, '\0', ftm_item_size);

	if(0 == ftm_get_item(ftm_item, temp))
	{
		if(strlen(temp) > 0)
		{
			free(temp);
			temp = NULL;
			return 1;
		}
	}
	free(temp);
	temp = NULL;
	return 0;

}

static void display_device_info(void)
{
    //struct fbcon_config *config = fbcon_display();
#ifdef WITH_LGE_LCD_BREAK_MODE
	int pos = 2;

	//modify barcode display position for ATT
#if defined(WITH_LGE_SHOW_SKU_IN_HW_KEY_CTL_MODE)
	pos = 3;
#endif

	display_ftm_item(LGFTM_IMEI, LGFTM_IMEI_SIZE, "IMEI", pos--);
#if defined(WITH_LGE_SHOW_SN_IN_HW_KEY_CTL_MODE)
	display_ftm_item(LGFTM_MSN, LGFTM_MSN_SIZE, "S/N", pos--);
#endif
#if defined(WITH_LGE_SHOW_SKU_IN_HW_KEY_CTL_MODE)
	display_ftm_item(LGFTM_SKU, LGFTM_SKU_SIZE, "SKU", pos--);
#endif

#else
	int pos = 0;
	display_ftm_item(LGFTM_IMEI, LGFTM_IMEI_SIZE, "IMEI", pos++);
#if defined(WITH_LGE_SHOW_SN_IN_HW_KEY_CTL_MODE)
	display_ftm_item(LGFTM_MSN, LGFTM_MSN_SIZE, "S/N", pos++);
#endif

#if defined(WITH_LGE_SHOW_SKU_IN_HW_KEY_CTL_MODE)
	display_ftm_item(LGFTM_SKU, LGFTM_SKU_SIZE, "SKU", pos++);
#endif

#if defined(WITH_LGE_SHOW_ATT_SN_IN_HW_KEY_CTL_MODE)
	//check MSN, SKU value for Z2/P1 ATT
	if(check_ftm_item(LGFTM_MSN, LGFTM_MSN_SIZE))
	{
		display_ftm_item(LGFTM_MSN, LGFTM_MSN_SIZE, "S/N", pos++);
	}
#endif
#if defined(WITH_LGE_SHOW_ATT_SKU_IN_HW_KEY_CTL_MODE)
	if(check_ftm_item(LGFTM_SKU, LGFTM_SKU_SIZE))
	{
		display_ftm_item(LGFTM_SKU, LGFTM_SKU_SIZE, "SKU", pos++);
	}
#endif
#endif
}

static int menu_handler(struct menu *menu_list, int number_of_menus)
{
    int is_pwr_key_toggled = 0;
	int select = 0;
	int old_select = 0;

    select_menu(menu_list, select);
    mdelay(500);
    while (!is_pwr_key_toggled)
    {
        target_keystatus();
        if (0 != keys_get_state(KEY_VOLUMEUP))
        {
            if(wait_vol_keys_release(50))
                select = (select + number_of_menus - 1) % number_of_menus;
        } else if(0 != keys_get_state(KEY_VOLUMEDOWN))
        {
            if(wait_vol_keys_release(50))
                select = (select + 1) % number_of_menus;
        }
        if (select != old_select)
        {
            select_menu(menu_list, select);
            old_select = select;
        }
        is_pwr_key_toggled = check_power_key_release_detect(50);
        mdelay(50);
    }

    //lge_fbcon_flush();
    return select;
}
void display_logo()
{
    fbcon_clear_color(0x000000, 0xFFFFFF);
    display_lge_splash_screen("lglogo_image");
    display_lge_splash_screen("powered_android_image");
}

int handle_factory_reset_mode()
{
#ifdef WITH_LGE_KSWITCH
	if( !kswitch_flag_enabled(KSWITCH_FLAG_RECOVERY) ) { // disabled is 0
		dprintf(INFO, "[KSwitch] factory reset disabled, Do not go to handle_factory_reset_mode()\n");
		return SM_FACTORY_RESET;
	}
#endif

#ifndef VZW_HARD_KEY_FACTORY_RESET_SCENARIO
	fbcon_clear_color(0x00FFFFFF, 0x00000000);
	display_lge_splash_screen("factory_reset_image");
	display_lge_splash_screen("factory_reset_1st_line_image");

	if (MENU_YES == menu_handler(factory_reset_menu_1st_list,
		NUMBER_OF_MENU_LIST(factory_reset_menu_1st_list))) {
#endif
		fbcon_clear_color(0x00FFFFFF, 0x00000000);
		display_lge_splash_screen("factory_reset_image");
		display_lge_splash_screen("factory_reset_2nd_line_image");
		if (MENU_YES == menu_handler(factory_reset_menu_2nd_list,
			NUMBER_OF_MENU_LIST(factory_reset_menu_2nd_list))) {
			emmc_set_recovery_cmd(WIPE_DATA);
			return SM_FACTORY_RESET;
		}
#ifndef VZW_HARD_KEY_FACTORY_RESET_SCENARIO
    }
#endif

    display_logo();
    return SM_NONE;
}
void handle_lcd_break_mode()
{
    int lcd_break_mode = -1;

    fbcon_clear_color(0x00FFFFFF, 0x00000000);
    display_lge_splash_screen("lcd_break_main_image");
    display_lge_splash_screen("lcd_break_cancel_image");
    display_lge_splash_screen("lcd_break_guide_image");

    display_device_info();

    switch(menu_handler(lcdbreak_menu_list, NUMBER_OF_MENU_LIST(lcdbreak_menu_list)))
    {
    case MENU_CANCEL:
        dprintf(INFO, "LCDBREAK_MODE Canceled\n");
        break;
    case MENU_ON:
        dprintf(INFO, "LCDBREAK_MODE ON\n");
        lcd_break_mode = LCD_BREAK_MODE_ON;
        break;
    case MENU_OFF:
        dprintf(INFO, "LCDBREAK_MODE OFF\n");
        lcd_break_mode = LCD_BREAK_MODE_OFF;
        break;
    }
    if (lcd_break_mode != -1)
    {
		ftm_set_item(LGFTM_LCDBREAK, &lcd_break_mode);
		bootcmd_set_value_by_key("androidboot.lge.lcdbreak_mode", lcd_break_mode==LCD_BREAK_MODE_ON?"true":"false");
    }
    display_logo();
}

void handle_imei_display_mode()
{
    fbcon_clear_color(0x00FFFFFF, 0x00000000);
    display_lge_splash_screen("imei_main_image");
    display_lge_splash_screen("imei_guide_image");
    display_device_info();
    mdelay(500);
    wait_all_key_release();
    while (!check_power_key_release_detect(50));
    display_logo();
}

int target_selectmode_init(void)
{
	int mode = SM_NONE;

	selectmode_set_new_menu_combo(target_selectmode_menu,
				sizeof(target_selectmode_menu) / sizeof(selectmode_menu_t));

	mode = selectmode_get_mode();
	dprintf(CRITICAL, "%s: selectmode_get_mode()=%d\n", __func__, mode);

	switch (mode) {
	case SM_NATIVE_RECOVERY:
		dprintf(INFO, "SM_NATIVE_RECOVERY start\n");
		break;

	case SM_FACTORY_RESET:
		dprintf(INFO, "SM_FACTORY_RESET start\n");
		mode = handle_factory_reset_mode();
		break;

	case SM_LCDBREAK:
		dprintf(INFO, "SM_LCDBREAK_MODE start\n");
#ifdef WITH_LGE_LCD_BREAK_MODE
        handle_lcd_break_mode();
#else
        handle_imei_display_mode();
#endif
        break;


	case SM_LAF_DOWNLOAD:
		dprintf(INFO, "SM_LAF_DOWNLOAD start\n");
		break;

	case SM_FASTBOOT_DL:
		dprintf(INFO, "SM_FASTBOOT_DL start\n");
		break;

	case SM_DEBUG:
#ifdef LGE_WITH_DISABLE_UART_BY_HWKEY
		return SM_NONE;
#else
		//dprintf(INFO, "SM_DEBUG start\n");
		//bootmode_set_uart(ENABLE);
		break;
#endif

	default:
		dprintf(INFO, "SM_NONE\n");
	}

	return mode;
}

