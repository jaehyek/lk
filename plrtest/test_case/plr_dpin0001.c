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

#include "../plr_deviceio.h"
#include "../plr_common.h"
#include "../plr_err_handling.h"
#include "../plr_protocol.h"
#include "../plr_case_info.h"
#include "../plr_pattern.h"
#include "../plr_verify.h"
#include "../plr_verify_log.h"


#define MIN_REQUEST_SECTORS	64
#define MAX_REQUEST_SECTORS	1024

enum CURRENT_REQUEST
{
	SKIP_16K = 0,
	WRITE_RANDOM,
	WRITE_4K,
};

static int pattern_sequential(uchar *buf, uint start_lsn, uint request_sectors, op_pattern operate)
{
	int ret = 0;
	uint request_count	= 1;
	uint lsn			= start_lsn;
	uint request_length	= request_sectors;
	uint skip_sectors 	= NUM_OF_SECTORS_PER_PAGE << 2;
	uint last_lsn_in_test_area  = get_last_sector_in_test_area();
	enum CURRENT_REQUEST current_req = WRITE_RANDOM;

	if(request_length == NUM_OF_SECTORS_PER_PAGE && lsn != get_first_sector_in_test_area())
		current_req = WRITE_4K;
	
	while(TRUE)
	{
		while(TRUE)
		{
			if(request_count % 29 == 0)
				plr_info("\rlsn : 0x%08X, request size : %dKB, request num : %d", lsn, SECTORS_TO_SIZE_KB(request_length), request_count);		
			ret = operate(buf, lsn, request_length, request_count, 0);
			if(ret != 0)
				return ret;

			lsn += request_length;
			request_count++;
			if(current_req == WRITE_4K)
				break;
			
			current_req = WRITE_4K;			
			request_length = NUM_OF_SECTORS_PER_PAGE;
		}
		
		current_req = WRITE_RANDOM;

		if(g_plr_state.b_cache_enable == TRUE && request_count <= g_plr_state.cache_flush_cycle)
			request_length = MAX_REQUEST_SECTORS;							
		else 
			request_length = random_sector_count(4);		

		lsn += skip_sectors;

		if(lsn + request_length + NUM_OF_SECTORS_PER_PAGE >= last_lsn_in_test_area)
		{
			if(lsn >= last_lsn_in_test_area)
				break;
			
			if(	last_lsn_in_test_area - lsn <= 	MAX_REQUEST_SECTORS + NUM_OF_SECTORS_PER_PAGE 	&&
				last_lsn_in_test_area - lsn >= 	MIN_REQUEST_SECTORS + NUM_OF_SECTORS_PER_PAGE)				
			{
				request_length = last_lsn_in_test_area - lsn - NUM_OF_SECTORS_PER_PAGE;
				continue;
			}

			break;
		}
	}

	return ret;	
}
 
static void init_pattern_1_2(struct plr_write_info* t_info)
{
	t_info->lsn			 	= get_first_sector_in_test_area();
	t_info->request_sectors	= NUM_OF_SECTORS_PER_PAGE << 4;
}

static int verify_customizing_01(uchar *buf, uint start_lsn, uint end_lsn, uint expected_loop)
{
	int ret 		= 0;
	uint length 	= NUM_OF_SECTORS_PER_8MB;	
	uint page_offset= 0;
	uint s_lsn 		= 0;
	uint e_lsn 		= 0;
	uint page_index = 0;
	uint s_lsn_in_chunk = 0;
	uint pages_in_chunk = 0;
	enum CURRENT_REQUEST current_req = SKIP_16K;
	uint skip_pages	= SECTORS_TO_PAGES(NUM_OF_SECTORS_PER_PAGE << 2);
	bool flag_find 	= FALSE;
	bool occurred_crash = FALSE;
	struct plr_header *header_info = NULL;
	
	if(start_lsn >= end_lsn)
		return VERI_NOERR;
	
	if(expected_loop <= 0)
		return VERI_NOERR;

	ret = verify_find_chunk(buf, start_lsn, end_lsn, expected_loop, &s_lsn, &pages_in_chunk, &flag_find);
	if(ret != 0)
		return ret;

	if(flag_find == FALSE)
		return VERI_NOERR;
	
	page_index = 0;
	if(pages_in_chunk > 1)
		current_req = WRITE_RANDOM;
	else 
		current_req = WRITE_4K;
	
	while(s_lsn < end_lsn)
	{
		if(s_lsn + NUM_OF_SECTORS_PER_8MB >= end_lsn)
			length = end_lsn - s_lsn;

		ret = dd_read(buf, s_lsn, length);
		if(ret != PLR_NOERROR)
		{
			verify_set_result(VERI_EIO, s_lsn);
			return VERI_EIO;
		}
		
		e_lsn = s_lsn + length;
		while(s_lsn < e_lsn)
		{		
			if(s_lsn % 2048 == 0)
				plr_info("\rCheck LSN, CRC, Loop(0x%08X)", s_lsn);	

			header_info = (struct plr_header *)(buf + (page_offset * PAGE_SIZE));	
			ret = verify_check_page(header_info, s_lsn);						
			if(ret != 0)
				occurred_crash = TRUE;

			if(occurred_crash == FALSE)
			{
				switch(current_req)
				{
					case SKIP_16K:
						#if 0	
						if(header_info->loop_cnt == expected_loop)						
							occurred_crash = TRUE;						
						#endif
						
						page_index++;									
						break;
						
					case WRITE_RANDOM:
						if(page_index == 0)
						{
							s_lsn_in_chunk = s_lsn;
							if(header_info->req_info.page_index == 0 && header_info->loop_cnt == expected_loop)
							{
								pages_in_chunk = header_info->req_info.page_num;
							}
							else 
							{
								if(e_lsn - s_lsn <=  MIN_REQUEST_SECTORS + NUM_OF_SECTORS_PER_PAGE)
									return VERI_NOERR;
								
								occurred_crash = TRUE;
								pages_in_chunk = SECTORS_TO_PAGES(MAX_REQUEST_SECTORS);
							}
						}
						
						if(header_info->req_info.page_index != page_index)
							occurred_crash = TRUE;
						if(header_info->loop_cnt != expected_loop)
							occurred_crash = TRUE;

						page_index++;	
						break;
						
					case WRITE_4K:
						if(header_info->loop_cnt != expected_loop)
						{
							VERIFY_DEBUG_MSG("Crash LSN : 0x%08X \n", s_lsn);
							verify_set_result(VERI_EREQUEST, s_lsn);
							verify_insert_request_info(s_lsn, NUM_OF_SECTORS_PER_PAGE, NULL, VERI_EREQUEST);
							return VERI_EREQUEST;
						}
					
						break;			
				}
			}

			if(occurred_crash == TRUE)
			{
				VERIFY_DEBUG_MSG("Crash LSN : 0x%08X \n", s_lsn);
				verify_set_result(VERI_EREQUEST, s_lsn_in_chunk);
				verify_insert_request_info(s_lsn_in_chunk, PAGES_TO_SECTORS(pages_in_chunk), NULL, VERI_EREQUEST);
				return VERI_EREQUEST;
			}
			else
			{
				switch(current_req)
				{
					case SKIP_16K:
						if(page_index == skip_pages)
						{
							page_index = 0;
							current_req = WRITE_RANDOM;					
						}
						
						break;
						
					case WRITE_RANDOM:
						if(page_index == pages_in_chunk)
						{
							page_index = 0;
							current_req = WRITE_4K;
							verify_insert_request_info(s_lsn_in_chunk, PAGES_TO_SECTORS(pages_in_chunk), NULL, VERI_NOERR);			
						}
						break;
						
					case WRITE_4K:
						page_index = 0;
						current_req = SKIP_16K;
						verify_insert_request_info(s_lsn, NUM_OF_SECTORS_PER_PAGE, NULL, VERI_NOERR);
						break;
				}
			}
			
			s_lsn += NUM_OF_SECTORS_PER_PAGE;
			page_offset++;
		}		

		page_offset = 0;
	}

	return VERI_NOERR;	
}

static int extra_verification(uchar *buf, struct plr_state *state)
{
	int ret = 0;
	uint selected_zone	= 0;
	uint start_lsn 		= 0;
	uint end_lsn 		= 0;
	uint current_zone_num = get_zone_num(state->write_info.lsn);

	selected_zone = well512_rand() % get_total_zone();
	
	start_lsn = get_first_sector_in_zone(selected_zone);	
	end_lsn = get_last_sector_in_zone(selected_zone) - get_reserved_sectors_in_zone();
	
	if(selected_zone == current_zone_num)
	{
		end_lsn = state->write_info.lsn;

		VERIFY_DEBUG_MSG("LSN and CRC, LOOP Check : Start LSN : 0x%08X, End LSN : 0x%08X, expected loop : %d \n", start_lsn, end_lsn, state->loop_cnt);
		ret = verify_customizing_01(buf, start_lsn, end_lsn, state->loop_cnt);
		if(ret != VERI_NOERR)
			return ret;

		if(state->b_packed_enable == TRUE || state->b_cache_enable == TRUE)
		{
			if(selected_zone == get_zone_num(verify_get_predicted_last_lsn_previous_boot()))
				start_lsn = verify_get_predicted_last_lsn_previous_boot() + NUM_OF_SECTORS_PER_PAGE;
			else
				return VERI_NOERR;
		}
		else
			start_lsn = end_lsn + state->write_info.request_sectors;		
		end_lsn = get_last_sector_in_zone(selected_zone) - get_reserved_sectors_in_zone();
		
		VERIFY_DEBUG_MSG("LSN and CRC, LOOP Check : Start LSN : 0x%08X, End LSN : 0x%08X, expected loop : %d \n", start_lsn, end_lsn, state->loop_cnt - 1);
		ret = verify_customizing_01(buf, start_lsn, end_lsn, state->loop_cnt - 1);
		if(ret != VERI_NOERR)
			return ret; 	
	}
	
	if(selected_zone < current_zone_num)
	{
		VERIFY_DEBUG_MSG("LSN and CRC, LOOP Check : Start LSN : 0x%08X, End LSN : 0x%08X, expected loop : %d \n", start_lsn, end_lsn, state->loop_cnt);
		ret = verify_customizing_01(buf, start_lsn, end_lsn, state->loop_cnt);
		if(ret != VERI_NOERR)
			return ret;			
	}

	if(selected_zone > current_zone_num)
	{
		if((state->b_packed_enable == TRUE || state->b_cache_enable == TRUE) && selected_zone == get_zone_num(verify_get_predicted_last_lsn_previous_boot()))
			start_lsn = verify_get_predicted_last_lsn_previous_boot() + NUM_OF_SECTORS_PER_PAGE;

		VERIFY_DEBUG_MSG("LSN and CRC, LOOP Check : Start LSN : 0x%08X, End LSN : 0x%08X, expected loop : %d \n", start_lsn, end_lsn, state->loop_cnt - 1);
		ret = verify_customizing_01(buf, start_lsn, end_lsn, state->loop_cnt - 1);
		if(ret != VERI_NOERR)
			return ret;
	}		

	return VERI_NOERR;
}	


/* ---------------------------------------------------------------------------------------
 * 
 *----------------------------------------------------------------------------------------
 */

int initialize_dpin_0001( uchar * buf, uint test_area_start, uint test_area_length )
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
	pattern_linker.do_pattern_1 	= pattern_sequential;
	pattern_linker.init_pattern_1 	= init_pattern_1_2;	
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

int read_dpin_0001( uchar * buf, uint test_start_sector, uint test_sector_length )
{
	return verify_pattern(buf);
}

int write_dpin_0001( uchar * buf, uint test_start_sector, uint test_sector_length )
{
	return write_pattern(buf);
}

