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

#include <err.h>
#include <malloc.h>
#include <string.h>
#include <printf.h>
#include <debug.h>
#include <lge_bootcmd.h>
#include <lge_smem.h>
#include <lge_bootmode.h>
#include <lge_ftm.h>
#include <pm8x41_hw.h>

/* strings for board revision : it must be same to the rev_type in SBL */
static const char * const revision_string[] = {
	"hdk_a",
	"hdk_b",
	"rdk",
	"rev_a",
	"rev_b",
	"rev_c",
	"rev_10",
	"rev_11",
	"reserved",
};

/* string for USB cable */
static const char * const usb_type_string[] = {
	" ",
	" ",
	" ",
	" ",
	" ",
	" ",
	"LT_56K",
	"LT_130K",
	"400MA",
	"DTC_500MA",
	"Abnormal_400MA",
	"LT_910K",
	"NO_INIT",
};

static const char * const factory_type_string[] = {
	"normal",
	"qem_56k",
	"qem_130k",
	"qem_910k",
	"pif_56k",
	"pif_130k",
	"pif_910k",
};

static const char * const port_type_string[] = {
	"CHARGER_PORT_SDP",
	"CHARGER_PORT_CDP",
	"CHARGER_PORT_DCP",
	"CHARGER_PORT_INVALID",
	"CHARGER_PORT_UNKNOWN",
};

#define BIT(x)					(1 << (x))
#define PM_MISC_IDEV_STS		0x21608
#define PM_CHGR_PORT_CDP		BIT(7)
#define PM_CHGR_PORT_DCP		BIT(6)
#define PM_CHGR_PORT_OTHER		BIT(5)
#define PM_CHGR_PORT_SDP		BIT(4)

static int              bootmode_uart = DISABLE;
static int              bootmode_bootchart = DISABLE;
static int              bootmode_hreset = DISABLE;
static int              bootmode_reboot_reason = 0xffffffff;
static rev_type         bootmode_rev = HW_REV_MAX;
static usb_cable_type   bootmode_cable = ABNORMAL_USB_CABLE_400MA;
static unsigned int     bootmode_port = 1;
static factory_mode_type    factory_mode = FACTORY_MODE_NORMAL;
extern int get_qem(void);
extern int get_ftm_frst_running(void);

#ifdef LGE_PM_BATT_ID
static int bootmode_battery_info = BATT_UNKNOWN;
#endif

void bootmode_set_uart(uart_mode_type mode)
{
	if ((mode != ENABLE) && (mode != DISABLE) && (mode != DETECTIVE)) {
		dprintf(ERROR, "%s: invalid arguement(%d)\n", __func__, mode);
		dprintf(INFO, "%s: disabled as default\n", __func__);
		bootmode_uart = DISABLE;
		return;
	}
	bootmode_uart = mode;
}

uart_mode_type bootmode_get_uart(void)
{
	return bootmode_uart;
}

char *bootmode_get_uart_str(void)
{
	uart_mode_type type = bootmode_get_uart();
	switch (type) {
	case ENABLE:
		return "enable";
	case DISABLE:
		return "disable";
	case DETECTIVE:
		return "detective";
	}
	return "";
}

void bootmode_set_bootchart(bootchart_mode_type mode)
{
	if ((mode != ENABLE) && (mode != DISABLE) && (mode != DETECTIVE))
	{
		dprintf(ERROR, "%s: invalid arguement(%d)\n", __func__, mode);
		dprintf(INFO, "%s: disabled as default\n", __func__);
		bootmode_bootchart = DISABLE;
		return;
	}

	bootmode_bootchart = mode;
}

bootchart_mode_type bootmode_get_bootchart(void)
{
	return bootmode_bootchart;
}

char *bootmode_get_bootchart_str(void)
{
	bootchart_mode_type type = bootmode_get_bootchart();

	switch (type)
	{
	case ENABLE:
		return "enable";
	case DISABLE:
		return "disable";
	case DETECTIVE:
		return "detective";
	}

	return "";
}

void bootmode_set_hreset(hreset_mode_type mode)
{
	if ((mode != ENABLE) && (mode != DISABLE)) {
		dprintf(ERROR, "%s: invalid arguement(%d)\n", __func__ , mode);
		dprintf(INFO, "%s: disabled as default\n", __func__);
		bootmode_hreset = DISABLE;
		return;
	}
	bootmode_hreset = mode;
}

hreset_mode_type bootmode_get_hreset(void)
{
	return bootmode_hreset;
}

void bootmode_set_reboot_reason(int reason)
{
	bootmode_reboot_reason = reason;
}

int bootmode_get_reboot_reason(void)
{
	return bootmode_reboot_reason;
}

void bootmode_set_board_revision(void)
{
	smem_vendor0_type vendor0_info;
	int ret = -1;

	ret = smem_get_vendor0_info(&vendor0_info);
	if (!ret) {
		bootmode_rev = (rev_type)vendor0_info.hw_rev;

		dprintf(INFO,
			"%s: board revision value is %d from smem\n",
			__func__, bootmode_rev);
	} else {
		dprintf(ERROR, "%s: smem read failed\n", __func__);
		return;
	}
}

rev_type bootmode_get_board_revision(void)
{
	return bootmode_rev;
}

char *bootmode_get_board_revision_string(void)
{
	return (char *)revision_string[bootmode_rev];
}

void bootmode_set_cable(void)
{
	smem_vendor1_type vendor1_info;
	int ret = -1;

	ret = smem_get_vendor1_info(&vendor1_info);
	if (!ret) {
		bootmode_cable = (usb_cable_type)vendor1_info.cable_type;

		dprintf(INFO,
			"%s: usb cable type is %d from smem\n",
			__func__, bootmode_cable);
	} else {
		dprintf(ERROR, "%s: smem read failed\n", __func__);
		return;
	}
}

void bootmode_set_cable_port(void)
{
	uint8_t port_type = 0;

	port_type = pm8x41_reg_read(PM_MISC_IDEV_STS);

	if (port_type & PM_CHGR_PORT_CDP)
		bootmode_port = CHARGER_PORT_CDP;
	else if (port_type & PM_CHGR_PORT_DCP)
		bootmode_port = CHARGER_PORT_DCP;
	else if (port_type & PM_CHGR_PORT_SDP)
		bootmode_port = CHARGER_PORT_SDP;
	else if (port_type & PM_CHGR_PORT_OTHER)
		bootmode_port = CHARGER_PORT_UNKNOWN;
	else
		bootmode_port = CHARGER_PORT_INVALID;

	dprintf(INFO,
		"%s: chg port type is 0x%x : %s\n",
		__func__, port_type, port_type_string[bootmode_port]);

	return;
}

usb_cable_type bootmode_get_cable(void)
{
	return bootmode_cable;
}

char *bootmode_get_cable_string(void)
{
	return (char *)usb_type_string[bootmode_cable];
}

unsigned int bootmode_get_port(void)
{
	return bootmode_port;
}

void bootmode_set_factory(void)
{
	if (get_qem() == 1) {
		if (bootmode_cable == LT_CABLE_56K) {
			factory_mode = FACTORY_MODE_QEM_56K;
		} else if (bootmode_cable == LT_CABLE_130K) {
			factory_mode = FACTORY_MODE_QEM_130K; //MINIOS2.0
		} else if (bootmode_cable == LT_CABLE_910K) {
			factory_mode = FACTORY_MODE_QEM_910K;
		} else {
			factory_mode = FACTORY_MODE_QEM_56K;
		}
	} else {
		if (get_ftm_frst_running() == 1) {
			factory_mode = FACTORY_MODE_PIF_56K;
		} else {
			if (bootmode_cable == LT_CABLE_56K) {
				factory_mode = FACTORY_MODE_PIF_56K;
			} else if (bootmode_cable == LT_CABLE_130K) {
				factory_mode = FACTORY_MODE_PIF_130K;
			} else if (bootmode_cable == LT_CABLE_910K) {
				factory_mode = FACTORY_MODE_PIF_910K;
			} else {
#ifdef LGE_MINIOS3_FRESET
				if ((get_ftm_dload_frst_flag() == 1) || (get_ftm_frst() == 2)) {
#else
				if (get_ftm_frst() == 2) {
#endif
					factory_mode = FACTORY_MODE_PIF_910K;
				}
			}
		}
	}
	smem_set_vendor1_info((unsigned int) bootmode_cable);
}

factory_mode_type bootmode_get_factory(void)
{
	return factory_mode;
}

char *bootmode_get_factory_string(void)
{
	return (char *)factory_type_string[factory_mode];
}

void bootmode_set_androidboot_mode(void)
{
	bootmode_set_factory();

	if (bootmode_get_factory() != FACTORY_MODE_NORMAL){
		bootcmd_add_pair("androidboot.mode", bootmode_get_factory_string());
		if (bootmode_get_factory() == FACTORY_MODE_QEM_130K) {
			//bootcmd_add_pair("maxcpus", "2");
			bootcmd_add_pair("boot_cpus", "0-1");
		} else {
			//bootcmd_add_pair("maxcpus", "4");
			bootcmd_add_pair("boot_cpus", "0-3");
		}
		if (bootmode_get_batt_id() == 0) {
			bootcmd_add_pair("maxcpus", "2");
			bootcmd_add_pair("boot_cpus", "4");
		}
	} else {
		/* normal booting */
		bootcmd_add_pair("boot_cpus", "0-5");
	}
}

#ifdef LGE_PM_BATT_ID
void bootmode_set_batt_info(void)
{
	smem_batt_info_type lge_batt_id;

#ifdef WITH_LGE_POWER_BATT_ID_DEFAULT
	/* This feature is for testing without typical battery */
	lge_batt_id.lge_battery_info = BATT_DS2704_L;
	smem_set_battery_info(&lge_batt_id);
#endif

	if (smem_get_battery_info(&lge_batt_id)) {
		dprintf(CRITICAL, "ERROR: unable to read battery info\n");
	}

	bootmode_battery_info = lge_batt_id.lge_battery_info;
	dprintf(INFO, "batt_info = %d\n", bootmode_battery_info);
}
char *bootmode_get_battery_id_string(void)
{
	char *batt_cmd;

	switch (bootmode_battery_info) {
	case BATT_DS2704_N:
		batt_cmd = "DS2704_N";
		break;
	case BATT_DS2704_L:
		batt_cmd = "DS2704_L";
		break;
	case BATT_DS2704_C:
		batt_cmd = "DS2704_C";
		break;
	case BATT_ISL6296_N:
		batt_cmd = "ISL6296_N";
		break;
	case BATT_ISL6296_L:
		batt_cmd = "ISL6296_L";
		break;
	case BATT_ISL6296_C:
		batt_cmd = "ISL6296_C";
		break;
	case BATT_RA4301_VC0:
		batt_cmd = "RA4301_VC0";
		break;
	case BATT_RA4301_VC1:
		batt_cmd = "RA4301_VC1";
		break;
	case BATT_RA4301_VC2:
		batt_cmd = "RA4301_VC2";
		break;
	case BATT_SW3800_VC0:
		batt_cmd = "SW3800_VC0";
		break;
	case BATT_SW3800_VC1:
		batt_cmd = "SW3800_VC1";
		break;
	case BATT_SW3800_VC2:
		batt_cmd = "SW3800_VC2";
		break;
	case BATT_MISSED:
		batt_cmd = "MISSED";
		break;
	default:
		batt_cmd = "UNKNOWN";
		break;
	}

	return batt_cmd;
}

int bootmode_get_batt_id(void)
{
	smem_batt_info_type lge_batt_id;
	int local_batteryid;

	if (smem_get_battery_info(&lge_batt_id)) {
		dprintf(CRITICAL, "ERROR: unable to read shared memory for battery info\n");
		return 0;
	}
	local_batteryid = lge_batt_id.lge_battery_info;
	dprintf(INFO, "[Factory] batt_id : %d\n", local_batteryid);

	if (local_batteryid != 200)
		return 1;

	return 0;
}
#endif

#ifdef TARGET_USES_CHARGERLOGO
unsigned int bootmode_check_pif_detect(void)
{
	unsigned int pif_detection;

	pif_detection = bootmode_cable;
	dprintf(INFO, "PIF_DETECT:%d\n", pif_detection);

	if (pif_detection) {
		if ((LT_CABLE_56K == pif_detection) ||
			(LT_CABLE_130K == pif_detection) ||
			(LT_CABLE_910K == pif_detection)) {
			return 1;
		} else {
#ifdef LGE_MINIOS3_FRESET
			if ((get_ftm_dload_frst_flag() == 1) || (get_ftm_frst() == 2)) {
#else
			if (get_ftm_frst() == 2) {
#endif
				return 1;
			} else {
				return 0;
			}
		}
	} else {
		return 0;
	}
}

#endif
