#include <debug.h>
#include <string.h>
#include <stdlib.h>

#ifdef LGE_WITH_BOOT_MODE
#include <lge_bootcmd.h>
#include <lge_smem.h>
#include <lge_bootmode.h>
#endif

#ifdef LGE_WITH_FTM
#include <lge_ftm.h>
#include <lge_target_ftm.h>
#endif

#ifdef LGE_WITH_SBL_LOG
#include <lge_sbl_log.h>
#endif

#ifdef LGE_WITH_FLEXIBLE_GPT
#include <partition_parser.h>
#include <lge_partition.h>
#endif

#include <lge_target_common.h>

#ifdef WITH_LGE_KSWITCH
#include <lge_kswitch.h>
#endif

static void target_common_update_static_bootcmd(void)
{
#ifdef LGE_WITH_BOOT_MODE
	bootcmd_add_new_key("gpt");
#endif
}

static void target_common_update_bootcmd_from_smem(void)
{
#ifdef LGE_WITH_BOOT_MODE
	smem_vendor0_type vendor0_info;

	if (!smem_get_vendor0_info(&vendor0_info))
		bootcmd_add_pair("model.name", vendor0_info.model_name);

	bootcmd_add_pair("lge.rev", bootmode_get_board_revision_string());

	bootcmd_add_pair("bootcable.type", bootmode_get_cable_string());
#ifdef LGE_PM_BATT_ID
	bootcmd_add_pair("lge.battid" , bootmode_get_battery_id_string());
#endif
#endif
}

static void target_common_update_bootcmd_from_ftm(void)
{
#ifdef LGE_WITH_FTM
	switch (target_ftm_get_fakebattery()) {
	case 0:
		bootcmd_add_pair("fakebattery", "disable");
		break;
	case 1:
		bootcmd_add_pair("fakebattery", "enable");
		break;
	}
#endif
}

static void target_common_update_dynamic_bootcmd(void)
{
#ifdef LGE_WITH_BOOT_MODE
	char tmp[12] = { '\0', };

	snprintf(tmp, sizeof(tmp), "0x%x", bootmode_get_reboot_reason());
	bootcmd_add_pair("lge.bootreason", tmp);

#ifdef LGE_WITH_FTM
#if WITH_DEBUG_UART
	bootmode_set_uart(target_ftm_get_uart());
#endif
#if WITH_DEBUG_BOOTCHART
	bootmode_set_bootchart(target_ftm_get_bootchart());
	bootcmd_add_pair("lge.bootchart", (char *)bootmode_get_bootchart_str());
	if (bootmode_get_bootchart() == ENABLE)
	{
		bootcmd_add_pair("init", "/sbin/bootchartd");
		bootcmd_add_pair("initcall_debug", "1");
		bootcmd_add_pair("printk.time", "1");
	}
#endif
#endif
	bootcmd_add_pair("lge.uart", (char *)bootmode_get_uart_str());

	bootmode_set_androidboot_mode();
#endif
}

void target_common_update_bootcmd(void)
{
	target_common_update_static_bootcmd();

	target_common_update_bootcmd_from_smem();

	target_common_update_bootcmd_from_ftm();

	target_common_update_dynamic_bootcmd();
}

void target_common_update_ftm(void)
{
}

void target_common_update_reboot_info(unsigned reboot_reason)
{
#ifdef LGE_WITH_BOOT_MODE
	/* save reboot reason */
	bootmode_set_reboot_reason(reboot_reason);
#endif
}

static void target_common_init_ftm(void)
{
#ifdef LGE_WITH_FTM
	ftm_init();
#endif
#ifdef WITH_LGE_KSWITCH
	kswitch_load_flag();
#endif
}

static void target_common_init_bootcmd(void)
{
#ifdef LGE_WITH_BOOT_MODE
	bootcmd_list_init();

	bootmode_set_board_revision();
	bootmode_set_cable();
#endif
#ifdef LGE_PM_BATT_ID
	bootmode_set_batt_info();
#endif
}

void target_common_init_chg_port(void)
{
#ifdef LGE_WITH_BOOT_MODE
	bootmode_set_cable_port();
#endif
}

static void target_common_init_sbl_log(void)
{
#ifdef LGE_WITH_SBL_LOG
	sbl_log_init();
	sbl_log_print();
#endif
}

#ifdef LGE_WITH_FLEXIBLE_GPT
static void target_make_flexible_gpt(void)
{
	partition_dump();
	do_extend_partition_20();
	partition_dump();
}
#endif

/* it is called by aboot_init() */
void target_common_aboot_init(void)
{
}

/* it is called by target_init() */
void target_common_init(void)
{
	target_common_init_ftm();

#ifdef LGE_WITH_FLEXIBLE_GPT
	target_make_flexible_gpt();
#endif

	target_common_init_bootcmd();

	target_common_init_sbl_log();
}

/* it is called by platform_init() */
void target_common_platform_init(void)
{
}
