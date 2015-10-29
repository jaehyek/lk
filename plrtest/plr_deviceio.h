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

#ifndef _PLR_FLASHIO_H_
#define _PLR_FLASHIO_H_
 
#include "plr_err_handling.h"

typedef unsigned char uchar;

int sdmmc_read(uchar *data, uint start_sector, uint len);
int sdmmc_write(uchar *data, uint start_sector, uint len);

int get_sdmmc_part_info(uint *start, uint *count, uchar *pid);
int get_dd_part_info(uint *start, uint *count);
int get_dd_dirty_part_info(uint *start, uint *count);

int get_dd_total_sector_count(void);
int	get_dd_product_name(char *proc_name);

int dd_read(uchar *data, uint start_sector, uint len);
int dd_write(uchar *data, uint start_sector, uint len);
int dd_erase(uint start_sector, uint len);
int dd_erase_for_poff(uint start_sector, uint len, int type);

int get_dd_cache_enable(void);
int dd_cache_flush(void);
int dd_cache_ctrl(uint enable);

int get_dd_packed_enable(void);
int dd_packed_add_list(ulong start, lbaint_t blkcnt, void*src, int rw);
int dd_packed_send_list(void);
void* dd_packed_create_buff(void *buff);
int dd_packed_delete_buff(void);
int dd_mmc_get_packed_count(void);
int dd_get_packed_max_sectors(int rw);
int dd_send_internal_info(void * internal_info);

// joys,2015.08.27
int get_dd_bkops_enable(void);
int get_dd_hpi_enable(void);
int dd_bkops_test_enable(int b_enable);
int dd_hpi_test_enable(int b_enable);

#endif
