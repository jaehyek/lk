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

#ifndef _PLR_HOOKING_H_
#define _PLR_HOOKING_H_

#ifdef SCRATCH_ADDR
#define READ_BUF_ADDR				(SCRATCH_ADDR)
#define EXTRA_BUF_ADDR				(SCRATCH_ADDR + 0x800000)
#define WRITE_BUF_ADDR				(SCRATCH_ADDR + 0x1000000)
#else
#define READ_BUF_ADDR				0x40000000
#define EXTRA_BUF_ADDR				0x40800000
#define WRITE_BUF_ADDR				0x41000000
#endif
#define EXTRA_BUF_SIZE              (WRITE_BUF_ADDR-EXTRA_BUF_ADDR-1) //tftp buff size kkr
#define SDCARD_DEV_NUM				0ULL		
#define EMMC_DEV_NUM				1ULL		
#define SDCARD_PART_NUM				4
#define SDCARD_FAT_PART_NUM			1

int get_mmc_product_name(uint dev_num, char *name);
int get_mmc_cache_enable(uint dev_num);

int do_read(uint dev_num, uchar *data, uint start_sector, uint len);
int do_write(uint dev_num, uchar *data, uint start_sector, uint len);
int do_erase(uint dev_num, uint start_sector, uint len);

int do_cache_flush(uint dev_num);
int do_cache_ctrl(uint dev_num, uint enable);

// joys,2015.07.13 Erase power off --------------------------------------------
int do_erase_for_test(uint dev_num, uint start_sector, uint len, int type);
// ----------------------------------------------------------------------------

int get_mmc_packed_enable(uint dev_num);
int do_packed_add_list(int dev_num, ulong start, lbaint_t blkcnt, void*src, int rw);
int do_packed_send_list(int dev_num);
void* do_packed_create_buff(int dev_num, void *buff);
int do_packed_delete_buff(int dev_num);
struct mmc_packed* do_get_packed_info(int dev_num);
uint get_packed_count(int dev_num);
uint get_packed_max_sectors(int dev_num, int rw);
// joys,2015.06.19 for internal poff
int do_send_internal_info(int dev_num, void * internal_info);
int do_internal_power_control(int dev_num, bool b_power);

ulong get_tick_count(void);
void reset_tick_count(int enable);
u64 get_tick_count64(void);
void reset_tick_count64(void);
ulong get_rtc_time(void);
ulong get_cpu_timer (ulong base);
void reset_board(ulong ignored);

#endif
