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
 *     * Neither the name of The Linux Fundation, Inc. nor the names of its
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

#ifndef __LGE_BOOTMODE_H
#define __LGE_BOOTMODE_H

/*
 * it shoud be same number as boot_qpnp_vadc.h @ boot_images
 * Each maching name strings are in lge_bootmode.c
 */
typedef enum {
	HW_REV_HDKA = 0,
	HW_REV_HDKB,
	HW_REV_RDK,
	HW_REV_A,
	HW_REV_B,
	HW_REV_C,
	HW_REV_1_0,
	HW_REV_1_1,
	HW_REV_MAX
} rev_type;

enum mode_type {
	DISABLE = 0,
	ENABLE = 1,
	DETECTIVE = 2,
};

typedef enum mode_type uart_mode_type;
typedef enum mode_type bootchart_mode_type;
typedef enum mode_type hreset_mode_type;

typedef enum {
	LT_CABLE_56K = 6,
	LT_CABLE_130K,
	USB_CABLE_400MA,
	USB_CABLE_DTC_500MA,
	ABNORMAL_USB_CABLE_400MA,
	LT_CABLE_910K,
	NO_INIT_CABLE,
} usb_cable_type;

typedef enum {
    CHARGER_PORT_SDP,
    CHARGER_PORT_CDP,
    CHARGER_PORT_DCP,
    CHARGER_PORT_INVALID,
    CHARGER_PORT_UNKNOWN,
} charger_port_type;

#ifdef LGE_PM_BATT_ID
enum {
	BATT_UNKNOWN = 0,
	BATT_DS2704_N = 17,
	BATT_DS2704_L = 32,
	BATT_DS2704_C = 48,
	BATT_ISL6296_N = 73,
	BATT_ISL6296_L = 94,
	BATT_ISL6296_C = 105,
	BATT_RA4301_VC0 = 130,
	BATT_RA4301_VC1 = 147,
	BATT_RA4301_VC2 = 162,
	BATT_SW3800_VC0 = 187,
	BATT_MISSED = 200,
	BATT_SW3800_VC1 = 204,
	BATT_SW3800_VC2 = 219,
};
extern void bootmode_set_androidboot_mode(void);
#endif

typedef enum {
	FACTORY_MODE_NORMAL = 0,
	FACTORY_MODE_QEM_56K = 1,
	FACTORY_MODE_QEM_130K = 2,
	FACTORY_MODE_QEM_910K = 3,
	FACTORY_MODE_PIF_56K = 4,
	FACTORY_MODE_PIF_130K = 5,
	FACTORY_MODE_PIF_910K = 6,
} factory_mode_type;

#define onoffs(option)	((option ? "on" : "off"))
#define toggles(option)	((option ? "enable" : "disable"))

extern void bootmode_set_uart(uart_mode_type mode);
extern uart_mode_type bootmode_get_uart(void);
extern char *bootmode_get_uart_str(void);

extern void bootmode_set_bootchart(bootchart_mode_type mode);
extern bootchart_mode_type bootmode_get_bootchart(void);
extern char *bootmode_get_bootchart_str(void);

extern void bootmode_set_hreset(hreset_mode_type mode);
extern hreset_mode_type bootmode_get_hreset(void);

extern void bootmode_set_reboot_reason(int reason);
extern int bootmode_get_reboot_reason(void);

extern void bootmode_set_board_revision(void);
extern rev_type bootmode_get_board_revision(void);
extern char *bootmode_get_board_revision_string(void);

extern void bootmode_set_cable(void);
extern void bootmode_set_cable_port(void);
extern usb_cable_type bootmode_get_cable(void);
extern char *bootmode_get_cable_string(void);

extern unsigned int bootmode_get_port(void);

extern void bootmode_set_factory(void);
extern factory_mode_type bootmode_get_factory(void);
extern char *bootmode_get_factory_string(void);

extern void bootmode_set_androidboot_mode(void);
extern void bootmode_set_batt_info(void);
#ifdef LGE_PM_BATT_ID
extern int bootmode_get_batt_id(void);
extern void bootmode_set_batt_info(void);
extern char *bootmode_get_battery_id_string(void);
#endif
#ifdef TARGET_USES_CHARGERLOGO
unsigned int bootmode_check_pif_detect(void);
#endif
#endif
