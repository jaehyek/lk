/*
 * Copyright (c) 2011-2014, LG Electronics. All rights reserved.
 *
 * LGE Factory Mode Items from pre-determined partition
 *
 * 2011-10-28, jinkyu.choi@lge.com
 */

#ifndef __LGE_FTM_H__
#define __LGE_FTM_H__

#define FTM_PARTITION_NAME      "misc"
#define FTM_MAGIC_STRING        "BSP Forever"
#define FTM_ROUND_TO_PAGE(x, y) (((x) + (y)) & (~(y)))
#define FTM_PAGE_SIZE           2048
#define FTM_PAGE_MASK           (FTM_PAGE_SIZE-1)

#define FTM_MAX_ITEM_STRING     10

/* LGE Factory Mode Items
 * These items should be synchronized with ATD FTM Item List
 * You can find ATD FTM Item List at android/vendor/factory/atd/lgftmitem/ftmitem.h
 */
enum ftm_item_index {
	LGFTM_ITEM_MIN = 0,

	/* 1~100 Use for Factory Item
	 */
	LGFTM_MAGIC_ITEM               = 1,
	LGFTM_FRST_STATUS              = 2,
	LGFTM_QEM                      = 3,
	LGFTM_PID                      = 4,
	LGFTM_DEVICETEST_RESULT        = 5,
	LGFTM_WLAN_MAC_ADDR            = 6,
	LGFTM_AAT_RESULT               = 7,
	LGFTM_BT_ADDR                  = 8,
	LGFTM_SERIAL_NUMBER            = 9,
	LGFTM_CHARGER_SKIP             = 10,
	LGFTM_DISPLAY_KCAL             = 11,
	LGFTM_DLOAD_FACTORY_RESET_FLAG = 12,
	LGFTM_IMEI                     = 13,
	LGFTM_SW_VERSION               = 14,
	LGFTM_SW_ORIGINAL_VERSION      = 15,
	LGFTM_QFUSE_BLOWNED            = 16,
	LGFTM_AATSET                   = 17,
	LGFTM_MSN                      = 18,
	LGFTM_MANUFACTURE_DATE         = 19,
	LGFTM_NTCODE                   = 20,
	LGFTM_DIAG_ENABLE              = 21,
	LGFTM_PROXIMITY_LIMIT          = 22,
	LGFTM_FRST_RUNNING             = 23,
	LGFTM_FIRSTMCCMNC              = 24,
	LGFTM_AAT_TESTORDER            = 25,
	LGFTM_MAX_CPU                  = 26,
	LGFTM_ETM_LOG_ENABLE           = 27,
	LGFTM_SPT                      = 28,
	LGFTM_AFT_RESULT               = 29,
	LGFTM_MTS_FRSTCHECK            = 30,
	LGFTM_SW_FIXED_AREA_VERSION    = 31,
	LGFTM_BUYER                    = 32,
	LGFTM_THEME_ITEM               = 33,
	LGFTM_FORCE_FRST_ON_PIF        = 35,
	LGFTM_WORK_FRST_MODE           = 40,

	/* 101~ Use for LK & Etc
	 */
	LGFTM_LCDBREAK                 = 101,
	LGFTM_FIRSTBOOT_FINISHED       = 102,
	LGFTM_ONE_BINARY_HWINFO        = 103,
#ifdef WITH_LGE_KSWITCH
	LGFTM_KILL_SWITCH              = 104,
#endif
#ifdef LGE_BOOTLOADER_UNLOCK
    LGFTM_BLUNLOCK_KEY             = 108,
#endif // LGE_BOOTLOADER_UNLOCK
#ifdef LGE_KILLSWITCH_UNLOCK
	LGFTM_KILL_SWITCH_UNLOCK_KEY   = 115,
	LGFTM_KILL_SWITCH_NONCE        = 117,
	LGFTM_KILL_SWITCH_UNLOCK_INFO  = 118,
#endif // LGE_KILLSWITCH_UNLOCK

	/* Functionality Control
	*/
	LGFTM_FAKE_BATTERY             = 200,
	LGFTM_UART                     = 203,
	LGFTM_BOOTCHART                = 204,

	LGFTM_DOWNLOAD_NV_AREA         = 3584,
	LGFTM_ITEM_MAX,
};

#define LGFTM_MAGIC_ITEM_SIZE                  12
#define LGFTM_FRST_STATUS_SIZE                 1
#define LGFTM_QEM_SIZE                         1
#define LGFTM_PID_SIZE                         30
#define LGFTM_DEVICETEST_RESULT_SIZE           31
#define LGFTM_WLAN_MAC_ADDR_SIZE               6
#define LGFTM_AAT_RESULT_SIZE                  31
#define LGFTM_BT_ADDR_SIZE                     6
#define LGFTM_SERIAL_NUMBER_SIZE               12
#define LGFTM_CHARGER_SKIP_SIZE                1
#define LGFTM_DISPLAY_KCAL_SIZE                4
#define LGFTM_DLOAD_FACTORY_RESET_FLAG_SIZE    1
#define LGFTM_IMEI_SIZE                        20
#define LGFTM_SW_VERSION_SIZE                  50
#define LGFTM_SW_ORIGINAL_VERSION_SIZE         50
#define LGFTM_QFUSE_BLOWNED_SIZE               20
#define LGFTM_AATSET_SIZE                      128
#define LGFTM_MSN_SIZE                         16
#define LGFTM_MANUFACTURE_DATE_SIZE            16
#define LGFTM_NTCODE_SIZE                      2000
#define LGFTM_DIAG_ENABLE_SIZE                 512
#define LGFTM_PROXIMITY_LIMIT_SIZE             4
#define LGFTM_FRST_RUNNING_SIZE                1
#define LGFTM_FIRSTMCCMNC_SIZE                 8
#define LGFTM_AAT_TEST_ORDER_SIZE              128
#define LGFTM_MAX_CPU_SIZE                     1
#define LGFTM_ETM_LOG_ENABLE_SIZE              1
#define LGFTM_SPT_SIZE                         23
#define LGFTM_AFT_RESULT_SIZE                  31
#define LGFTM_MTS_FRSTCHECK_SIZE               1
#define LGFTM_SW_FIXED_AREA_VERSION_SIZE       50
#define LGFTM_BUYER_SIZE                       8
#define LGFTM_THEME_ITEM_SIZE                  40

#define LGFTM_FORCE_FRST_ON_PIF_SIZE           1
#define LGFTM_WORK_FRST_MODE_SIZE              1
#define LGFTM_LCDBREAK_SIZE                    1
#define LGFTM_FIRSTBOOT_FINISHED_SIZE          1
#define LGFTM_ONE_BINARY_HWINFO_SIZE           128

#define LGFTM_FAKE_BATTERY_SIZE				   1
#define LGFTM_UART_SIZE                        1
#define LGFTM_BOOTCHART_SIZE                   1

#ifdef WITH_LGE_KSWITCH
#define LGFTM_KILL_SWITCH_SIZE                 11
#endif
#ifdef LGE_BOOTLOADER_UNLOCK
#define LGFTM_BLUNLOCK_KEY_SIZE                1024
#endif // LGE_BOOTLOADER_UNLOCK
#ifdef LGE_KILLSWITCH_UNLOCK
#define LGFTM_KILL_SWITCH_UNLOCK_KEY_SIZE      1024
#define LGFTM_KILL_SWITCH_NONCE_SIZE           4
#define LGFTM_KILL_SWITCH_UNLOCK_INFO_SIZE     64
#endif // LGE_KILLSWITCH_UNLOCK
/* non volatile area size(1MB). for web download flag,
 * other download flag, factory info and so on
 */
#define LGFTM_DOWNLOAD_NV_AREA_SIZE                  256

/* LGFTM block size(2KB) * 512 = 1MB
 */
#define LGFTM_DOWNLOAD_NV_AREA_BLOCK_SIZE            512

/* misc partition start + 7MB(3584 * 2KB)
 */
#define LGFTM_DOWNLOAD_NV_AREA_START_BLOCK_NUMBER    3584

#ifdef LGE_MINIOS3_FRESET
int get_ftm_dload_frst_flag(void);
int get_ftm_work_frst_mode(void);
int set_ftm_work_frst_mode(int status);
#endif

int get_ftm_frst(void);
int set_ftm_frst(int status);
int get_qem(void);
int set_ftm_frst_running(int status);
#ifdef TARGET_USES_CHARGERLOGO
int get_charger_skip(void);
#endif
extern void ftm_init(void);
extern int ftm_get_item(unsigned int id, void *val);
extern int ftm_set_item(unsigned int id, void *val);

#ifdef WITH_LGE_DOWNLOAD
int get_ftm_webflag(void);
void set_ftm_webflag(int status);
#endif

#endif
