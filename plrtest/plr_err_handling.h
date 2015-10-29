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

#ifndef _PLR_ERR_HANDLING_H_
#define _PLR_ERR_HANDLING_H_

#include "plr_common.h"

int get_crash_state_sdcard(int *err_num);
int set_crash_state_sdcard(int err_num);

int load_crash_dump(void* buf, uint length_byte, uint start_location);
int save_crash_dump(void* buf, uint length_byte, uint start_location);

int clear_state_sdcard(void);
int clear_sdcard_sectors(uint s_sector, uint sector_num);

#endif

