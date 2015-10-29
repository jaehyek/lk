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
 
#include "plr_common.h"
#include "plr_precondition.h"
#include "plr_protocol.h"
#include "plr_err_handling.h"
#include "plr_hooking.h"
#include "plr_deviceio.h"

#define ERASE_ALL
#define FILL_BLOCK_CNT 0x2000	// 4MB
//#define FILL_BLOCK_CNT 0x400

/* ------------------------------------------------
* Static Function 
* ------------------------------------------------*/

int check_erase_zero_condition(uint erase_area_start, uint erase_area_length)
{
	int i = 0, j =0, index = 0, ret = 0;
	int read_count = 0;
	int check_count = 0;
	int check_unit = PAGE_SIZE;
	uint erase_area_end = 0;
	ulong *data = 0;
	// joys,2014.11.19
	ulong start_tick = 0, end_tick = 0, latency = 0;
	uchar *read_buff = (uchar*)plr_get_write_buffer();
	
	erase_area_end = erase_area_start + erase_area_length;
	
	// Make dirty to front area
	plr_info("Checking erase area...\n");
	read_count = FILL_BLOCK_CNT;
	// joys,2014.11.19
	start_tick = get_rtc_time();
	
	// Read data(4MB) from device (eMMC)
	for (i = erase_area_start; i < erase_area_end; i += read_count) {
		if ( (i + read_count) > erase_area_end )
			read_count = erase_area_end - i;
		
		ret = dd_read((uchar*)read_buff, i, read_count);
		
		if (ret)
			return ret;

		if ( (i / read_count) % 4 == 0)
			plr_info("\r %u / %u", i + read_count - 1, erase_area_end - 1);
		else if ( (i + read_count) == erase_area_end)
			plr_info("\r %u / %u", i + read_count - 1, erase_area_end - 1);
		
		/* Checking data */
		/* only check 4byte per sector */
		check_count = read_count / (check_unit / SECTOR_SIZE);
		for (j = 0; j < check_count; j++) {
			index = j * check_unit;
			data = (ulong*)(read_buff + index);

			if (*data && *data != -1) {
				plr_err("\nError!! Device is not clean. sector : %u\n", 
							i + (j * (check_unit / SECTOR_SIZE)) );
				return PLR_ECLEAN;
			}
		}
	}
	printf("\n");

	// joys,2014.11.19
	end_tick = get_rtc_time();
	latency = ((end_tick - start_tick)); //sec
	plr_info ("check read speed : %u KB\n", 
		(ulong)((u64)((u64)erase_area_length * (u64)SECTOR_SIZE) / (u64)latency / (u64)1000));
	
	return ret;
}

/* 
 *  mmc erase operation 
 */
static int 
make_erase_zero_condition(uint erase_area_start, uint erase_area_length, int total_sectors)
{	
	int ret = 0;
	#ifdef PLR_TLI_EMMC
	int i;
	char *temp = plr_get_write_buffer();

	uint temp_start = erase_area_start;
	uint temp_length = erase_area_length;
	#endif
#ifdef ERASE_ALL
	/* erase all */
	erase_area_start = 0;
	erase_area_length = total_sectors;
#endif
	if (erase_area_start + erase_area_length > total_sectors) {
		plr_err ("Error!! Erase area is not correct.\n"
				"erase start : %u, erase length : %u, physical end sector : %d\n",
					erase_area_start, erase_area_length, total_sectors);
		return -1;
	}

	plr_debug ("erase start : %u, erase length : %u\n", 
				erase_area_start, erase_area_length);
	
	/* Test blocks format */
	ret = dd_erase(erase_area_start, erase_area_length);

	#ifdef PLR_TLI_EMMC
	memset(temp, 0, 4096);
	
	for (i = 0; i < temp_length; i+=8) {
		if (i % 1024 == 0)
			plr_info("\r %u / %u", i, temp_length);
		dd_write(temp, temp_start+i, 8);
	}
	printf("\n");
	#endif
	return ret;
}

/* 
 *  mmc dirty operation 
 */
static int make_dirty_condition(uint test_start_sector, uint test_sector_length, int total_sectors)
{
	int i = 0, j = 0, ret = 0;
	uint dirty_start = 0;
	uint dirty_len = 0;
	uint dirty_end = 0;
	uint *dirty_buff = (uint*)plr_get_write_buffer();
	uint dirty_data = g_plr_state.filled_data.value;
	int dirty_count = FILL_BLOCK_CNT * 128;		// * ulong (4byte) = 4MB

	// joys,2014.11.05 modify uint overflow
	uint sectors_cnt_to_fill = total_sectors - test_sector_length;
	uint sector_cnt = 0;

	// joys,2014.11.05 ratio calculation
	sectors_cnt_to_fill = (uint)( ((u64)sectors_cnt_to_fill * (u64)g_plr_state.ratio_of_filled) / 100 );

	plr_debug("sectors_cnt_to_fill =  [%d] total_sectors = [%d] test_sector_length [%d]\n", sectors_cnt_to_fill, total_sectors, test_sector_length);

	// Fill dirty memory 1MB
	while (dirty_count-- > 0) {
		*((ulong  *)dirty_buff) = (ulong )dirty_data;
		dirty_buff += 1;
	}
	
	dirty_buff = (uint*) plr_get_write_buffer();
	
	// Make dirty to front area
	if (test_start_sector > 0) {
		plr_info("Making dirty to front area...\n");
		dirty_start = 0;
		dirty_len = test_start_sector;
		dirty_end = dirty_start + dirty_len;
		dirty_count = FILL_BLOCK_CNT;
		
		// Write dirty data to device (eMMC)
		for (i = dirty_start; i < dirty_end && sector_cnt<= sectors_cnt_to_fill; i += dirty_count, sector_cnt += dirty_count) {
			if ( (i + dirty_count) > dirty_end )
				dirty_count = dirty_end - i;
								
			if (dirty_count > 0)
				ret = dd_write((uchar*)dirty_buff, i, dirty_count);
			
			if (ret)
				return ret;
			
			if ((j % 20 == 0) || ((i + dirty_count) >= dirty_end))
				plr_info("\r %u / %u", i + dirty_count - 1, dirty_end - 1);
			j++;
		}
		printf("\n");
	}

	// Make dirty to back area
	if ( (test_start_sector + test_sector_length) < total_sectors) {
		plr_info("Making dirty to back area...\n");
		dirty_start = test_start_sector + test_sector_length;
		dirty_len = total_sectors - dirty_start;
		dirty_end = dirty_start + dirty_len;
		dirty_count = FILL_BLOCK_CNT;
		j = 0;
		
		// Write dirty data to device (eMMC)
		for (i = dirty_start; i < dirty_end && sector_cnt<= sectors_cnt_to_fill; i += dirty_count, sector_cnt += dirty_count) {
			if ( (i + dirty_count) > dirty_end )
				dirty_count = dirty_end - i;
			
			if (dirty_count > 0)
				ret = dd_write((uchar*)dirty_buff, i, dirty_count);
			
			if (ret)	
				return ret;
			
			if ((j % 20 == 0) || ((i + dirty_count) >= dirty_end))
				plr_info("\r %u / %u", i + dirty_count - 1, dirty_end - 1);
			j++;
		}
		printf("\n");
	}
	return ret;
}

/* ------------------------------------------------
* External Function 
* ------------------------------------------------*/

void precondition_by_protocol()
{
	int ret = 0;
	
//	memset (&g_plr_state, 0, sizeof(struct plr_state));
	
	send_token(PLRTOKEN_INIT_INFO, NULL);
	send_token(PLRTOKEN_INIT_START, NULL);

	/* start prepare */
	ret = plr_prepare(	g_plr_state.test_start_sector, 
						g_plr_state.test_sector_length, 
						g_plr_state.ratio_of_filled);
	
	send_token_param(PLRTOKEN_INIT_DONE, ret ? 1 : 0); 

	if(!ret)
	{
		/* board reset */
		reset_board(0);
	}

	return;
}

int plr_prepare(	uint test_start_sector, 
				 	uint test_sector_length, 
					bool bdirty_fill)
{	
	int ret = 0;
	
	plr_debug(" test_start_sector[%d] test_sector_length[%d]\n", test_start_sector, test_sector_length);
	
	/* clear error state sector (SD card) */
	ret = clear_state_sdcard();

	if (IS_AGING_TEST && g_plr_state.test_num == 8)
		return 0;

	/* Checking for test area*/
	if (test_sector_length < 1) {
		plr_err ("Error!! Test area is not correct. test area length : %u\n", 
					test_sector_length);
		return -1;
	}

	/* Checking for start test address */
	if (test_start_sector >= g_plr_state.total_device_sector) {
		
		plr_err ("Error!! Test area is not correct. test start sector : %u, physical end sector : %d\n",
					test_start_sector, g_plr_state.total_device_sector - 1);
		return -1;
	}
	
	/* Checking test_sector_length */
	if (test_start_sector + test_sector_length > g_plr_state.total_device_sector) {
	
		plr_err ("Error!! Test area is not correct. test end sector : %u, physical end sector : %d\n",
					test_start_sector + test_sector_length - 1, g_plr_state.total_device_sector - 1 );
		return -1;
	}

	ret = make_erase_zero_condition(test_start_sector, test_sector_length, g_plr_state.total_device_sector);
	
	if (ret) {
		plr_err ("Error!! make_erase_zero_condition [%d]\n", ret);
		return ret;
	}

	ret = check_erase_zero_condition(test_start_sector, test_sector_length);
	if (ret) {
		plr_err ("Error!! check_erase_zero_condition [%d]\n", ret);
		return ret;
	}

	if (bdirty_fill) {
		ret = make_dirty_condition(test_start_sector, test_sector_length, g_plr_state.total_device_sector);
		if (ret) {
			plr_err ("Error!! make_dirty_condition [%d]\n", ret);
			return ret;
		}
	}

	return ret;
}
