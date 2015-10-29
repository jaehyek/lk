/*
 * Copyright (c) 2013  ElixirFlash Technology Co., Ltd.
 *              http://www.elixirflash.com/
 *
 * Powerloss Test for U-boot
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */


#include <config.h>
#include "plr_common.h"
#include "plr_hooking.h"
#include "plr_errno.h"
#include <mmc_wrapper.h>
#include "target.h"

extern uint64_t qtimer_get_phy_timer_cnt();

#define TICK_CNT_FOR_30USEC			576
#define CRASH_LOG_BUF_START_SECTOR	1000000	/* DATA part: */
#define CRASH_LOG_BUF_SECTOR_SIZE	1024	/* 512KB */
#ifndef BLOCK_SIZE
#define BLOCK_SIZE 512
#endif
int get_mmc_product_name(uint dev_num, char *name)
{
	struct mmc_device *dev;

	if(dev_num != EMMC_DEV_NUM){
		plr_info("[%s] You can choose only eMMC (device_num = 1)\n",__func__);
		return -PLR_ENODEV;
	}

	dev = (struct mmc_device*)find_mmc_device(dev_num);

	strcpy(name, (char*)dev->card.cid.pnm);

	return 0;
}

#if 1
int get_mmc_part_info(char *dev_name, int part_num, int *start, int *count, unsigned char *pid)
{
	if(strtoud(dev_name) != SDCARD_DEV_NUM)
		return 0;

	*start = CRASH_LOG_BUF_START_SECTOR;
	*count = CRASH_LOG_BUF_SECTOR_SIZE;
	*pid = 0;
	return 0;
}

int get_mmc_block_count(char *device_name)
{
	struct mmc_device *dev;
	struct mmc_card *card;
	int block_count = 0;
	int dev_num;

	if(*device_name!='1') {
		return -1;
	}

	dev_num = EMMC_DEV_NUM;

	dev = (struct mmc_device*)find_mmc_device(dev_num);
	if (!dev)
	{
		printf("mmc/sd device is NOT founded.\n");
		return -1;
	}
	card = &dev->card;
	block_count = card->capacity / card->csd.read_blk_len;

	//	printf("capacity = %llu, read_blk_len = %lu, BLOCK_SIZE = %u\n",
	//			card->capacity, card->csd.read_blk_len, BLOCK_SIZE);
	return block_count;
}
#endif

int get_mmc_cache_enable(uint dev_num)
{
	struct mmc_device *dev;

	if(dev_num != EMMC_DEV_NUM){
		plr_info("[%s] You can choose only eMMC (device_num = 1)\n",__func__);
		return -PLR_ENODEV;
	}

	dev = (struct mmc_device*)find_mmc_device(dev_num);

	plr_info("%s: %d \n", __func__, dev->config.cache_ctrl);

	return dev->config.cache_ctrl;
}

int get_mmc_packed_enable(uint dev_num)
{
	struct mmc_device *dev;

	if(dev_num != EMMC_DEV_NUM){
		plr_info("[%s] You can choose only eMMC (device_num = 1)\n",__func__);
		return -PLR_ENODEV;
	}

	dev = (struct mmc_device*)find_mmc_device(dev_num);

	//	plr_debug("%s: %d \n", __func__, dev->config.packed_event_en);

	return dev->config.packed_event_en;
}

int do_cache_flush(uint dev_num)
{
	if(dev_num != EMMC_DEV_NUM)
		return -PLR_ENODEV;

	return plr_cache_flush(dev_num);
}

int do_cache_ctrl(uint dev_num, uint enable)
{
	if(dev_num != EMMC_DEV_NUM)
		return -PLR_ENODEV;

	return plr_cache_ctrl(dev_num, enable);
}

int do_packed_add_list(int dev_num, ulong start, lbaint_t blkcnt, void*src, int rw)
{
	if(!get_mmc_packed_enable(dev_num))
		return 0;

	return plr_packed_add_list(dev_num, start, blkcnt, src, rw);
}

int do_packed_send_list(int dev_num)
{
	if(!get_mmc_packed_enable(dev_num))
		return 0;

	return plr_packed_send_list(dev_num);
}

void* do_packed_create_buff(int dev_num, void *buff)
{
	if(!get_mmc_packed_enable(dev_num))
		return 0;

	return plr_packed_create_buf(dev_num, buff);
}

int do_packed_delete_buff(int dev_num)
{
	if(!get_mmc_packed_enable(dev_num))
		return 0;

	return plr_packed_delete_buf(dev_num);
}

struct mmc_packed* do_get_packed_info(int dev_num)
{
	if(!get_mmc_packed_enable(dev_num))
		return 0;

	return plr_get_packed_info(dev_num);
}

uint get_packed_count(int dev_num)
{
	if(!get_mmc_packed_enable(dev_num))
		return 0;

	return plr_get_packed_count(dev_num);
}

uint get_packed_max_sectors(int dev_num, int rw)
{
	if(!get_mmc_packed_enable(dev_num))
		return 0;

	return plr_get_packed_max_sectors(dev_num, rw);
}

int do_read(uint dev_num, uchar *data, uint start_sector, uint len)
{
	uint ret = 0;
	ret = plr_mmc_read(dev_num, start_sector, data, len);

	if (ret) {
		if (ret == -(INTERNAL_POWEROFF_FLAG))
			return ret;
		else {
			plr_info("Read Fail LSN : 0x%08X \n", start_sector);
			return -PLR_EIO;
		}
	}
	return ret;
}

int do_write(uint dev_num, uchar *data, uint start_sector, uint len)
{
	uint ret = 0;

	ret = plr_mmc_write(dev_num, start_sector, len, data);

	if (ret) {
		if (ret == -(INTERNAL_POWEROFF_FLAG))
			return ret;
		else {
			plr_info("Write Fail LSN : 0x%08X \n", start_sector);
			return -PLR_EIO;
		}
	}
	return ret;
}

int do_erase(uint dev_num, uint start_sector, uint len) 
{
	int ret = 0;

	ret = plr_mmc_erase_card(dev_num, start_sector, len);
	if (ret){
		plr_info("Erase Failure!!!\n");
		return -PLR_EIO;
	}

	return ret;
}

// joys,2015.06.19 for internal poff
int do_send_internal_info(int dev_num, void * internal_info)
{
	emmc_set_internal_info(dev_num, internal_info);
	return 0;
}

// joys,2015.07.13
int do_erase_for_test(uint dev_num, uint start_sector, uint len, int type)
{
	int ret = 0;

	ret = emmc_erase_for_poff(dev_num, start_sector, len, type);

	if (ret)
	{
		if (ret == -(INTERNAL_POWEROFF_FLAG))
			return ret;
		else {
			plr_info("Erase Fail LSN : 0x%08X \n", start_sector);
			return -PLR_EIO;
		}
	}

	return 0;
}

extern void reset_cpu(ulong addr);
extern unsigned long get_timer(unsigned long base);
extern unsigned long rtc_get_time(void);
extern unsigned long rtc_get_tick_count(void);
extern void rtc_reset_tick_count(int enable);
uint64_t base_tick;

/* RTC module (second) */
ulong get_rtc_time(void)
{
	return rtc_get_time();
}

/* cpu timer register */
ulong get_cpu_timer (ulong base)
{
	return get_timer(base);
}

/* RTC module tick (under ms) */
ulong get_tick_count(void)
{
	return (unsigned long)((qtimer_get_phy_timer_cnt()-base_tick)/TICK_CNT_FOR_30USEC);
}

/* 64bit tick count */
u64 get_tick_count64(void)
{
	return (qtimer_get_phy_timer_cnt()-base_tick)/TICK_CNT_FOR_30USEC;
}

/* 64bit tick counter reset */
void reset_tick_count64(void)
{
	reset_tick_count(1);
}

/* RTC tick count initialize */
void reset_tick_count(int enable)
{
	base_tick = (enable)?qtimer_get_phy_timer_cnt():0ULL;
	return;
}

void reset_board(ulong ignored)
{
	run_command("reset", 0);
}

// Internal poweroff -------------------------------------------------------------------------------//

int do_internal_power_control(int dev_num, bool b_power)
{
	if(dev_num != EMMC_DEV_NUM){
		plr_info("[%s] You can choose only eMMC (device_num = 1)\n",__func__);
		return -PLR_ENODEV;
	}

	if(!b_power) {
		turn_off_mmc();
	} else {
		turn_on_and_reinit_mmc();
	}
	return 0;
}
