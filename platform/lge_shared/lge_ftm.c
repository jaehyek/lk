/*
 * Copyright (c) 2011-2014, LG Electronics. All rights reserved.
 *
 * LGE Factory Mode Items from pre-determined partition
 *
 * 2011-10-28, jinkyu.choi@lge.com
 * 2013-06-21, sungyoung.lee@lge.com - refactoring all codes
 */

#include <err.h>
#include <debug.h>
#include <mmc.h>
#include <partition_parser.h>

#include <string.h>
#include <lge_ftm.h>
#include <smem.h>
#include <list.h>
#include <malloc.h>
#include <target.h>

enum ftm_valid_type {
	FTM_INVALID = -1,
	FTM_NOT_MATCHED_MAGIC = 0,
	FTM_VALID,
};

static enum ftm_valid_type ftm_valid = FTM_INVALID;

#define FTM_ITEM_NAME_MAX_LEN	32
#ifdef WITH_LGE_DOWNLOAD
#define MAX_BUF_WEBDLOAD 256
#endif

struct ftm_item_info {
	char name[FTM_ITEM_NAME_MAX_LEN];
	unsigned int index;
	unsigned int size;
};

struct ftm_items {
	struct ftm_item_info *info;
	struct list_node list;
};

static struct list_node ftm_item_list;

struct ftm_item_info ftm_init_item[] = {
	{"ftm_magic", LGFTM_MAGIC_ITEM, LGFTM_MAGIC_ITEM_SIZE},
	{"ftm_frst", LGFTM_FRST_STATUS, LGFTM_FRST_STATUS_SIZE},
	{"ftm_qem", LGFTM_QEM, LGFTM_QEM_SIZE},
	{"ftm_serial_num", LGFTM_SERIAL_NUMBER, LGFTM_SERIAL_NUMBER_SIZE},
	{"ftm_charger_skip", LGFTM_CHARGER_SKIP, LGFTM_CHARGER_SKIP_SIZE},
	{"ftm_display_kcal", LGFTM_DISPLAY_KCAL, LGFTM_DISPLAY_KCAL_SIZE},
	{"ftm_dload_factory_reset_flag", LGFTM_DLOAD_FACTORY_RESET_FLAG, LGFTM_DLOAD_FACTORY_RESET_FLAG_SIZE},
	{"ftm_imei", LGFTM_IMEI, LGFTM_IMEI_SIZE},
	{"ftm_msn", LGFTM_MSN, LGFTM_MSN_SIZE},
	{"ftm_manufacture_date", LGFTM_MANUFACTURE_DATE, LGFTM_MANUFACTURE_DATE_SIZE},
	{"ftm_frst_running", LGFTM_FRST_RUNNING, LGFTM_FRST_RUNNING_SIZE},
	{"ftm_aat_testorder", LGFTM_AAT_TESTORDER, LGFTM_AAT_TEST_ORDER_SIZE},
	{"ftm_max_cpu", LGFTM_MAX_CPU, LGFTM_MAX_CPU_SIZE},
	{"ftm_etm_log_enable", LGFTM_ETM_LOG_ENABLE, LGFTM_ETM_LOG_ENABLE_SIZE},
	{"ftm_sku", LGFTM_SKU, LGFTM_SKU_SIZE},
	{"ftm_work_frst_mode", LGFTM_WORK_FRST_MODE, LGFTM_WORK_FRST_MODE_SIZE},
	{"ftm_lcdbreak", LGFTM_LCDBREAK, LGFTM_LCDBREAK_SIZE},
	{"ftm_download_nv_area", LGFTM_DOWNLOAD_NV_AREA, LGFTM_DOWNLOAD_NV_AREA_SIZE},
	{"ftm_firstboot_finished", LGFTM_FIRSTBOOT_FINISHED, LGFTM_FIRSTBOOT_FINISHED_SIZE},
	{"ftm_fakebattery_status", LGFTM_FAKE_BATTERY, LGFTM_FAKE_BATTERY_SIZE},
	{"ftm_uart_status", LGFTM_UART, LGFTM_UART_SIZE},
	{"ftm_bootchart_status", LGFTM_BOOTCHART, LGFTM_BOOTCHART_SIZE},
#ifdef WITH_LGE_KSWITCH
	{"lgftm_kill_switch", LGFTM_KILL_SWITCH, LGFTM_KILL_SWITCH_SIZE},
#endif
#ifdef LGE_BOOTLOADER_UNLOCK
	{"lgftm_blunlock", LGFTM_BLUNLOCK_KEY, LGFTM_BLUNLOCK_KEY_SIZE},
#endif // LGE_BOOTLOADER_UNLOCK
#ifdef LGE_KILLSWITCH_UNLOCK
	{"lgftm_kswitch_unlock", LGFTM_KILL_SWITCH_UNLOCK_KEY, LGFTM_KILL_SWITCH_UNLOCK_KEY_SIZE},
	{"lgftm_kswitch_nonce", LGFTM_KILL_SWITCH_NONCE, LGFTM_KILL_SWITCH_NONCE_SIZE},
	{"lgftm_kswitch_unlock_info", LGFTM_KILL_SWITCH_UNLOCK_INFO, LGFTM_KILL_SWITCH_UNLOCK_INFO_SIZE},
#endif // LGE_KILLSWITCH_UNLOCK
};

static int ftm_add_item(const char *name, unsigned index, unsigned size)
{
	unsigned name_len;
	struct ftm_items *item;

	if (ftm_valid < FTM_NOT_MATCHED_MAGIC) {
		dprintf(CRITICAL, "%s: FTM partition can't use\n", __func__);
		return -1;
	}

	if (!name) {
		dprintf(CRITICAL, "%s: name is null\n", __func__);
		return -1;
	}

	name_len = strlen(name);
	if (name_len > FTM_ITEM_NAME_MAX_LEN) {
		dprintf(CRITICAL, "%s: name len(%d) is so long\n", __func__, name_len);
		return -1;
	}

	item = malloc(sizeof(struct ftm_items));
	if (!item) {
		dprintf(CRITICAL, "%s: memory allocation failed\n", __func__);
		return -1;
	}
	memset(item, 0, sizeof(struct ftm_items));

	item->info = malloc(sizeof(struct ftm_item_info));
	if (!(item->info)) {
		dprintf(CRITICAL, "%s: memory allocation failed\n", __func__);
		free(item);
		return -1;
	}

	memset(item->info, 0, sizeof(struct ftm_item_info));

	strncpy(item->info->name, name, name_len);
	item->info->index = index;
	item->info->size = size;

	list_add_tail(&ftm_item_list, &item->list);

	return 0;
}

/**
 * ftm_get_partition_offset - get FTM partition's offset
 * from partition table
 */
static unsigned long ftm_get_partition_offset(void)
{
	unsigned long ret = -1L;
	int index = -1;

	index = partition_get_index((const char *)FTM_PARTITION_NAME);
	if (index == INVALID_PTN) {
		dprintf(CRITICAL, "%s: FTM partition doesn't exist\n", __func__);
		return ret;
	}

	ret = partition_get_offset(index);

	return ret;
}

/**
 * ftm_check_validation - validate a usage of FTM partition and magic number
 */
static int ftm_check_validation(void)
{
	unsigned long long ptn = 0;
	unsigned int size = FTM_ROUND_TO_PAGE(strlen(FTM_MAGIC_STRING) + 1, FTM_PAGE_MASK);
	unsigned char data[size];

	if (ftm_valid > FTM_NOT_MATCHED_MAGIC)
		return NO_ERROR;

	if (!target_is_emmc_boot()) {
		dprintf(CRITICAL, "%s: target is not emmc boot\n", __func__);
		goto invalidated;
	}

	ptn = ftm_get_partition_offset();

	if (ptn == 0) {
		dprintf(CRITICAL, "%s: failed get offset\n", __func__);
		goto invalidated;
	}

	/* read first page from ftm partition */
	if (mmc_read(ptn + (LGFTM_MAGIC_ITEM * FTM_PAGE_SIZE), (unsigned int *)data, size)) {
		dprintf(CRITICAL, "%s: mmc read failure\n", __func__);
		goto invalidated;
	}

	if (!strcmp((void *)data, FTM_MAGIC_STRING)) {
		ftm_valid = FTM_VALID;
	} else {
		ftm_valid = FTM_NOT_MATCHED_MAGIC;
		dprintf(CRITICAL, "%s: FTM magic string is not equal. %s\n", __func__, data);
	}

	return NO_ERROR;

invalidated:
	ftm_valid = FTM_INVALID;
	return ERROR;
}

/**
 * ftm_set_item - set value for given item at the FTM partition
 * @id: FTM item number index in the ftm_item_index
 * @val: value for setting
 */
int ftm_set_item(unsigned int id, void *val)
{
	struct ftm_items *item;
	unsigned long long ptn = 0;
	unsigned int size = 0;
	void *data;

	if (val == NULL) {
		dprintf(CRITICAL, "%s: id=%d, val buf is NULL\n", __func__, id);
		goto ftm_set_failed;
	}

	if ((id >= LGFTM_ITEM_MAX) || (id == LGFTM_ITEM_MIN)) {
		dprintf(CRITICAL, "%s: out of id range(%d)\n", __func__, id);
		goto ftm_set_failed;
	}

	if (ftm_valid < FTM_NOT_MATCHED_MAGIC)
		ftm_check_validation();

	if ((ftm_valid != FTM_VALID) && (id != LGFTM_MAGIC_ITEM)) {
		dprintf(CRITICAL, "%s: invalid id(%d)\n", __func__, id);
		goto ftm_set_failed;
	}

	ptn = ftm_get_partition_offset();

	if (ptn == 0) {
		dprintf(CRITICAL, "%s: failed get offset\n", __func__);
		goto ftm_set_failed;
	}

	list_for_every_entry(&ftm_item_list, item, struct ftm_items, list) {
		if ((item->info->index) == id) {
#ifdef WITH_LGE_DOWNLOAD
			if(id == LGFTM_DOWNLOAD_NV_AREA)
				size = FTM_ROUND_TO_PAGE(MAX_BUF_WEBDLOAD, FTM_PAGE_MASK);
			else
#endif
			size = FTM_ROUND_TO_PAGE(item->info->size, FTM_PAGE_MASK);
			data = malloc(size);
#ifdef WITH_LGE_DOWNLOAD
			if(id == LGFTM_DOWNLOAD_NV_AREA)
				memcpy(data, val, MAX_BUF_WEBDLOAD);
			else
#endif
			memcpy(data, val, item->info->size);

			if (mmc_write(ptn + (id * FTM_PAGE_SIZE), size, (unsigned int *)data)) {
				dprintf(CRITICAL, "%s: mmc write failure - %s\n", __func__,
						item->info->name);
				free(data);
				goto ftm_set_failed;
			}
			free(data);
			return 0;
		}
	}

ftm_set_failed:
	return -1;
}

/**
 * ftm_get_item - get value for given item at the FTM partition
 * @id: FTM item number index in the ftm_item_index
 * @val: buffer for saving value from founded location
 */
int ftm_get_item(unsigned int id, void *val)
{
	struct ftm_items *item;
	unsigned long long ptn = 0;
	unsigned int size = 0;
	void *data;

	if (val == NULL) {
		dprintf(CRITICAL, "%s: id=%d, val buf is NULL\n", __func__, id);
		goto ftm_get_failed;
	}

	if ((id >= LGFTM_ITEM_MAX) || (id == LGFTM_ITEM_MIN)) {
		dprintf(CRITICAL, "%s: out of id range(%d)\n", __func__, id);
		goto ftm_get_failed;
	}

	if (ftm_valid < FTM_NOT_MATCHED_MAGIC)
		ftm_check_validation();

	if ((ftm_valid != FTM_VALID) && (id != LGFTM_MAGIC_ITEM)) {
		dprintf(CRITICAL, "%s: invalid id(%d)\n", __func__, id);
		goto ftm_get_failed;
	}

	ptn = ftm_get_partition_offset();

	if (ptn == 0) {
		dprintf(CRITICAL, "%s: failed get offset\n", __func__);
		goto ftm_get_failed;
	}

	list_for_every_entry(&ftm_item_list, item, struct ftm_items, list) {
		if ((item->info->index) == id) {
#ifdef WITH_LGE_DOWNLOAD
			if(id == LGFTM_DOWNLOAD_NV_AREA)
				size = FTM_ROUND_TO_PAGE(MAX_BUF_WEBDLOAD, FTM_PAGE_MASK);
			else
#endif
			size = FTM_ROUND_TO_PAGE(item->info->size, FTM_PAGE_MASK);
			data = malloc(size);

			if (mmc_read(ptn + (id * FTM_PAGE_SIZE), (unsigned int *)data, size)) {
				dprintf(CRITICAL, "%s: mmc read failure - %s\n", __func__,
						item->info->name);
				free(data);
				goto ftm_get_failed;
			}
#ifdef WITH_LGE_DOWNLOAD
			if(id == LGFTM_DOWNLOAD_NV_AREA)
				memcpy(val, data, MAX_BUF_WEBDLOAD);
			else
#endif
			memcpy(val, data, item->info->size);
			free(data);
			return 0;
		}
	}

ftm_get_failed:
	return -1;
}

/*
 * set_ftm_frst - set value for LGFTM_FRST_STATUS item.
 * @status: status of LGFTM_FRST_RUNNING item.
 */
int set_ftm_frst(int status)
{
	char s = (char)status;
	if (ftm_set_item(LGFTM_FRST_STATUS, &s) < 0)
		return -1;
	else
		return 0;
}

/*
 * get_ftm_frst - get value from LGFTM_FRST_STATUS item.
 */
int get_ftm_frst(void)
{
	char status;
	if (ftm_get_item(LGFTM_FRST_STATUS, &status) < 0)
		return -1;

	return (int)status;
}

/*
 * set_ftm_frst_running - set value for LGFTM_FRST_RUNNING item.
 * @status: status of LGFTM_FRST_RUNNING item.
 */
int set_ftm_frst_running(int status)
{
	char s = (char)status;
	if (ftm_set_item(LGFTM_FRST_RUNNING, &s) < 0)
		return -1;
	else
		return 0;
}

/*
 * get_ftm_frst_running - get value from LGFTM_FRST_RUNNING item.
 */
int get_ftm_frst_running(void)
{
	char status;
	if (ftm_get_item(LGFTM_FRST_RUNNING, &status) < 0)
		return -1;

	return (int)status;
}

/*
 * get_qem - get value from LGFTM_QEM item.
 */
int get_qem(void)
{
	char qem;
	if (ftm_get_item(LGFTM_QEM, &qem) < 0)
		return -1;

	dprintf(INFO, "get_qem = %d\n", qem);
	return (int)qem;
}

#ifdef TARGET_USES_CHARGERLOGO
int get_charger_skip(void)
{
	char charger_skip;
	if (ftm_get_item(LGFTM_CHARGER_SKIP, &charger_skip) < 0) {
		return -1;
	}
	dprintf(INFO, "get_charger_skip = %d\n", charger_skip);
	return (int)charger_skip;
}
#endif

#ifdef LGE_MINIOS3_FRESET
/*
 * get_ftm_dload_frst_flag - get value from LGFTM_DLOAD_FACTORY_RESET_FLAG item.
 */
int get_ftm_dload_frst_flag(void)
{
	char status;
	if (ftm_get_item(LGFTM_DLOAD_FACTORY_RESET_FLAG, &status) < 0) {
		return -1 ;
	}

	if (status == 0xEE) {
		return 1 ;
	} else {
		return 0 ;
	}
}

/*
 * get_ftm_work_frst_mode - get value from LGFTM_WORK_FRST_MODE item.
 */
int get_ftm_work_frst_mode(void)
{
	char status;
	if (ftm_get_item(LGFTM_WORK_FRST_MODE, &status) < 0) {
		return -1;
	}
	return (int)status;
}

/*
 * set_ftm_work_frst_mode - set value for LGFTM_WORK_FRST_MODE item.
 * @status: status of LGFTM_WORK_FRST_MODE item.
 */
int set_ftm_work_frst_mode(int status)
{
	char s = (char)status;
	if (ftm_set_item(LGFTM_WORK_FRST_MODE, &s) < 0) {
		return -1;
	} else {
		return 0;
	}
}
#endif

#ifdef WITH_LGE_DOWNLOAD
int get_ftm_webflag(void)
{
	int  upgrade_status;
	char buf[MAX_BUF_WEBDLOAD];

	if (ftm_get_item(LGFTM_DOWNLOAD_NV_AREA, (void*)buf) < 0)
		return -1;

	/* webdownload flag is located 13th byte */
	memcpy(&upgrade_status, buf+13, sizeof(int) );

	dprintf(INFO,"webdload status:%d\n",upgrade_status);

	return upgrade_status;
}

void set_ftm_webflag(int status)
{
	char buf[MAX_BUF_WEBDLOAD];
	if (ftm_get_item(LGFTM_DOWNLOAD_NV_AREA, (void*)buf) < 0)
		return ;

	memcpy(buf+13, &status , sizeof(int) );
	if (ftm_set_item(LGFTM_DOWNLOAD_NV_AREA, (void*)buf) < 0)
		return ;
}
#endif // WITH_LGE_DOWNLOAD
void ftm_init(void)
{
	unsigned i;
	unsigned num = sizeof(ftm_init_item) / sizeof(struct ftm_item_info);
	int ret = -1;

	ftm_check_validation();

	if (ftm_valid < FTM_NOT_MATCHED_MAGIC) {
		dprintf(CRITICAL, "%s: FTM partition can't use\n", __func__);
		return;
	}

	list_initialize(&ftm_item_list);

	for (i = 0; i < num; i++) {
		ret = ftm_add_item(ftm_init_item[i].name,
						ftm_init_item[i].index,
						ftm_init_item[i].size);
		if (ret < 0) {
			dprintf(CRITICAL, "%s: FTM item init failed in %d\n", __func__, i);
			return;
		}
	}
}
