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
/* 	DPIN_0004
 
	W	= write (default)
	J	= jump
	R	= random	
	
	PATTERN_1, zone interleaving.
		zone 0, 	4K(W)	4K(W)	4K(W)	4K(W)	4K(W)	4K(W)	...	4K(W)
		zone 1, 	4K(W)	4K(W)	4K(W)	4K(W)	4K(W)	4K(W)	...	4K(W)
		zone 2, 	4K(W)	4K(W)	4K(W)	4K(W)	4K(W)	4K(W)	...	4K(W)
			.
			.
			.
		zone N, 	4K(W)	4K(W)	4K(W)	4K(W)	4K(W)	4K(W)	...	4K(W)
	
		
	PATTERN_2, sequencial
		zone 0,	4K(W) 	4K(W)	4K(W)	4K(W)	4K(W)	4K(W)	....	4K(W)
		zone 1,	4K(W) 	4K(W)	4K(W)	4K(W)	4K(W)	4K(W)	....	4K(W)
		zone 2, 	4K(W) 	4K(W)	4K(W)	4K(W)	4K(W)	4K(W)	....	4K(W)
			.
			.
			.
		zone N,	4K(W) 	4K(W)	4K(W)	4K(W)	4K(W)	4K(W)	....	4K(W)
	
 */
 
#include "../plr_deviceio.h"
#include "../plr_common.h"
#include "../plr_err_handling.h"
#include "../plr_protocol.h"
#include "../plr_case_info.h"
#include "../plr_pattern.h"
#include "../plr_verify.h"
#include "../plr_verify_log.h"



#define CHUNK_SIZE	NUM_OF_SECTORS_PER_PAGE
static bool _writing_sequence = FALSE;
	 
static int pattern1_interleaving(uchar *buf, uint start_lsn, uint request_sectors, op_pattern operate)
{
	int ret = 0;

	uint request_count  = 1;
	uint lsn			= start_lsn;

	uint last_lsn_in_first_zone = get_last_sector_in_zone(0);
	uint last_lsn_in_test_area  = get_last_sector_in_test_area();

	while (TRUE) 
	{
		for (; lsn < last_lsn_in_test_area; lsn += get_sectors_in_zone()) 
		{	 
			if(request_count % 29 == 0)
				plr_info("\rlsn : 0x%08X, request size : %dKB, request num : %d", lsn, SECTORS_TO_SIZE_KB(CHUNK_SIZE), request_count);		
			
			ret = operate(buf, lsn, CHUNK_SIZE, request_count, 0);
			if (ret != 0)
				return ret;

			request_count++;
		}

		lsn = get_first_sector_in_test_area() + (lsn & ZONE_SIZE_X_SECTOR_MASK) + CHUNK_SIZE;
		if(lsn >= last_lsn_in_first_zone)
			break;
	}

	return ret;
}

static int pattern2_sequential(uchar *buf, uint start_lsn, uint request_sectors, op_pattern operate)
{
	int ret = 0;

	uint request_count  = 1;
	uint lsn			= start_lsn;

	uint last_lsn_in_test_area  = get_last_sector_in_test_area();

	while (lsn  < last_lsn_in_test_area) 
	{
		if(request_count % 29 == 0)
			plr_info("\rlsn : 0x%08X, request size : %dKB, request num : %d", lsn, SECTORS_TO_SIZE_KB(CHUNK_SIZE), request_count);		

		ret = operate(buf, lsn, CHUNK_SIZE, request_count, 0);
		if(ret != 0)
			return ret;

		request_count++;			 
		lsn += CHUNK_SIZE;
	}

	return ret;
}

 
static void init_pattern_1_2(struct plr_write_info* t_info)
{
	t_info->lsn			 	= get_first_sector_in_test_area();
	t_info->request_sectors	= CHUNK_SIZE;
}

static int extra_verification(uchar *buf, struct plr_state *state)
{
	int ret = 0;
	uint selected_zone= 0;
	uint start_lsn	= 0;
	uint end_lsn 	= 0;
	uint current_zone_num = get_zone_num(state->write_info.lsn);

	selected_zone = well512_rand() % get_total_zone();

	if(state->write_info.loop_count % 2 == 1 || selected_zone == current_zone_num)
	{
		start_lsn = get_first_sector_in_zone(selected_zone);
		end_lsn = start_lsn + (state->write_info.lsn & ZONE_SIZE_X_SECTOR_MASK);
		if(selected_zone < current_zone_num)
			end_lsn += state->write_info.request_sectors;

		VERIFY_DEBUG_MSG("LSN and CRC, LOOP Check : Start LSN : 0x%08X, End LSN : 0x%08X, expected loop : %d \n", start_lsn, end_lsn, state->loop_cnt);
		ret = verify_check_pages_lsn_crc_loop(buf, start_lsn, end_lsn, TRUE, state->loop_cnt);
		if(ret != VERI_NOERR)
			return ret;

		if(state->b_packed_enable == TRUE || state->b_cache_enable == TRUE)
		{
			if(state->write_info.loop_count % 1 == 0)
			{
				if((verify_get_predicted_last_lsn_previous_boot() & ZONE_SIZE_X_SECTOR_MASK) < (end_lsn & ZONE_SIZE_X_SECTOR_MASK))
					return VERI_NOERR;
			}
			
			if(state->write_info.loop_count % 2 == 0)
			{
				if(selected_zone != get_zone_num(verify_get_predicted_last_lsn_previous_boot()))
					return VERI_NOERR;
			}
			
			start_lsn += (verify_get_predicted_last_lsn_previous_boot() & ZONE_SIZE_X_SECTOR_MASK) + NUM_OF_SECTORS_PER_PAGE;
		}
		else
		{
			start_lsn = end_lsn;
			if(selected_zone == current_zone_num)
				start_lsn = end_lsn + state->write_info.request_sectors;
		}		
		end_lsn = get_last_sector_in_zone(selected_zone) - get_reserved_sectors_in_zone();
		
		VERIFY_DEBUG_MSG("LSN and CRC, LOOP Check : Start LSN : 0x%08X, End LSN : 0x%08X, expected loop : %d \n", start_lsn, end_lsn, state->loop_cnt - 1);
		ret = verify_check_pages_lsn_crc_loop(buf, start_lsn, end_lsn, TRUE, state->loop_cnt - 1);
		if(ret != VERI_NOERR)
			return ret;
	}
	else 
	{
		start_lsn = get_first_sector_in_zone(selected_zone);
		end_lsn = get_last_sector_in_zone(selected_zone) - get_reserved_sectors_in_zone();
	
		if(selected_zone < current_zone_num)
		{
			VERIFY_DEBUG_MSG("LSN and CRC, LOOP Check : Start LSN : 0x%08X, End LSN : 0x%08X, expected loop : %d \n", start_lsn, end_lsn, state->loop_cnt);
			ret = verify_check_pages_lsn_crc_loop(buf, start_lsn, end_lsn, TRUE, state->loop_cnt);
			if(ret != VERI_NOERR)
				return ret;			
		}
		
		if(selected_zone > current_zone_num)
		{
			if((state->b_packed_enable == TRUE || state->b_cache_enable == TRUE)&& selected_zone == get_zone_num(verify_get_predicted_last_lsn_previous_boot()))
				start_lsn = verify_get_predicted_last_lsn_previous_boot() + NUM_OF_SECTORS_PER_PAGE;
		
			VERIFY_DEBUG_MSG("LSN and CRC, LOOP Check : Start LSN : 0x%08X, End LSN : 0x%08X, expected loop : %d \n", start_lsn, end_lsn, state->loop_cnt - 1);
			ret = verify_check_pages_lsn_crc_loop(buf, start_lsn, end_lsn, TRUE, state->loop_cnt - 1);
			if(ret != VERI_NOERR)
				return ret;
		}			
	}
	
	return VERI_NOERR;
}


/* ---------------------------------------------------------------------------------------
 * 
 *----------------------------------------------------------------------------------------
 */

int initialize_dpin_0004( uchar * buf, uint test_area_start, uint test_area_length )
{
	int ret = 0;	
	struct Pattern_Function pattern_linker;

	init_case_info(test_area_start, test_area_length);
	set_test_sectors_in_zone(get_sectors_in_zone());	

	/*
	* NOTE *************************************************************************************
	* 포팅을 위한 임시 코드. 
	* ******************************************************************************************
	*/	
	init_pattern(&g_plr_state, FALSE);	

	/*
	* NOTE *************************************************************************************
	* 사용자는 initialize level 에서 register pattern을 호출해야 한다.
	* ******************************************************************************************
	*/	
	memset(&pattern_linker, 0, sizeof(struct Pattern_Function));
	pattern_linker.do_pattern_1 = pattern1_interleaving;
	pattern_linker.do_pattern_2 = pattern2_sequential;
	pattern_linker.init_pattern_1 = init_pattern_1_2;
	pattern_linker.init_pattern_2 = init_pattern_1_2;
	pattern_linker.do_extra_verification = extra_verification;
	
	regist_pattern(pattern_linker);

	/*
	* NOTE *************************************************************************************
	* 포팅을 위한 임시 코드. 
	* ******************************************************************************************
	*/	
	ret = check_pattern_func();
	if(ret)
		return ret;
		
	return ret;
}

int read_dpin_0004(uchar * buf, uint test_area_start, uint test_area_length) 
{
	return verify_pattern(buf);	
}

int write_dpin_0004(uchar * buf, uint test_area_start, uint test_area_length) 
{
	int ret = 0;
	_writing_sequence = TRUE;
	ret = write_pattern(buf);
	_writing_sequence = FALSE;
	return ret;
}

