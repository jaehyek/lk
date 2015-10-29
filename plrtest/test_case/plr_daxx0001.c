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
#include "test_case/plr_case.h"



static unsigned long long _writing_page_per_loop;
static long long _total_writing_page;
static long long _total_writing_time;
static uint _start_time = 0;
static uint _end_time = 0;
static bool _writing_sequence = TRUE;

static uint get_request_length(void)
{
	if (g_plr_state.chunk_size.type) {
		uint rand = well512_write_rand() % 100;

		if (rand < g_plr_state.chunk_size.ratio_of_4kb) {
			return 8;
		}
		else if (rand < g_plr_state.chunk_size.ratio_of_4kb + g_plr_state.chunk_size.ratio_of_8kb_64kb) {
			rand = well512_write_rand();
			return ((rand % 15) << 3) + 16;
		}
		else {
			rand = well512_write_rand();
			return ((rand % 112) << 3) + 136;
		}
	}
	else {
		return (g_plr_state.chunk_size.req_len >> 2) << 3;
	}
}

static void print_daxx_0001(void)
{
	plr_info_highlight("\n\n [ Random Write Test ]\n");
	if (g_plr_state.chunk_size.type) {
		plr_info(" Chunk Size    : 4KB [%d%%] 8KB ~ 64KB [%d%%] 68KB ~ 512KB [%d%%]\n",
			g_plr_state.chunk_size.ratio_of_4kb, g_plr_state.chunk_size.ratio_of_8kb_64kb, g_plr_state.chunk_size.ratio_of_64kb_over);
	}
	else {
		plr_info(" Chunk Size    : %dKB\n", g_plr_state.chunk_size.req_len);
	}
}

static void print_statistics(void)
{
	char send_buf[128];

	plr_info(" Loop Count    : %u\n", g_plr_state.loop_cnt);
	plr_info(" Total Data    : %llu GB %llu MB \n",
							_total_writing_page/PAGE_PER_GB, (_total_writing_page%PAGE_PER_GB)/PAGE_PER_MB);
	plr_info(" Total Time    : %llu days %llu hours %llu minutes %llu seconds \n",
							_total_writing_time / SECONDS_PER_DAY, ( _total_writing_time % SECONDS_PER_DAY ) / SECONDS_PER_HOUR,
							(_total_writing_time % SECONDS_PER_HOUR) / SECONDS_PER_MINUTE, _total_writing_time % SECONDS_PER_MINUTE); 
	plr_info(" Writing Speed : %llu MB %llu KB / second \n\n", _total_writing_page / _total_writing_time / 256, ((_total_writing_page / _total_writing_time) % 256) * 4);	

	sprintf(send_buf, "%llu/%llu/%llu/", _total_writing_page, _total_writing_time, _total_writing_page * 4 / _total_writing_time); 
	send_token(PLRTOKEN_SET_STATISTICS, send_buf);		
}

static int pattern1(uchar *buf, uint start_lsn, uint request_sectors, op_pattern operate)
{
	int ret = 0;
	uint request_count 	= 0;	
	uint lsn 			= 0;
	uint request_length	= 0;
	uint total_write 	= 0;
	uint test_area_start_lsn= get_first_sector_in_test_area();
	uint test_area_length 	= get_last_sector_in_test_area() - test_area_start_lsn;

	if(_writing_sequence == TRUE)
	{	
		while (_writing_page_per_loop < test_area_length / NUM_OF_SECTORS_PER_PAGE / 2) { 
			lsn = (((well512_write_rand() % (test_area_length - 2048))>>3)<<3) + test_area_start_lsn;
			request_length = get_request_length();

			total_write += request_length;
			
			if (request_count % 99 == 0)
				plr_info("\r %u / %u", total_write, test_area_length);				
			
			ret = write_request(buf, lsn, request_length, request_count, 0);
			if (ret != 0)
				return ret;

			_writing_page_per_loop += (unsigned long long)(request_length / NUM_OF_SECTORS_PER_PAGE);
			_total_writing_page 	+= (unsigned long long)(request_length / NUM_OF_SECTORS_PER_PAGE);
			

			if (_total_writing_page/PAGE_PER_GB >= g_plr_state.finish.value) 
			{
				g_plr_state.b_finish_condition = TRUE;	
				break;
			}
		
			request_count++;
		}
	}
	else 
	{		
		ret = verify_check_pages_lsn_crc_loop(buf, test_area_start_lsn, get_last_sector_in_test_area(), FALSE, 0);
	}
	
	return ret;
}

static void init_pattern_1_2(struct plr_write_info* t_info)
{	
	t_info->lsn			 	= get_first_sector_in_test_area();	
	t_info->request_sectors	= 0;
}

/* ---------------------------------------------------------------------------------------
 * Test Main Function
 *----------------------------------------------------------------------------------------
 */

int initialize_daxx_0001( uchar * buf, uint test_start_sector, uint test_sector_length )
{
	int ret = 0;	
	struct Pattern_Function pattern_linker;

	init_case_info(test_start_sector, test_sector_length);
	set_test_sectors_in_zone(get_sectors_in_zone());	

	g_plr_state.boot_cnt = 1;

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
	pattern_linker.do_pattern_1 = pattern1;
	pattern_linker.do_pattern_2 = pattern1;
	pattern_linker.init_pattern_1 = init_pattern_1_2;
	pattern_linker.init_pattern_2 = init_pattern_1_2;
	pattern_linker.do_extra_verification = NULL;
	
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

int read_daxx_0001(uchar * buf, uint test_start_sector, uint test_sector_length)
{
	return verify_pattern(buf);	
}

int write_daxx_0001( uchar * buf, uint test_start_sector, uint test_sector_length )
{
	int ret = 0;

	if(_total_writing_page / PAGE_PER_GB >= g_plr_state.finish.value)
	{
		plr_info_highlight("\n\n========== Finish ===========\n");		
		print_statistics();
		g_plr_state.b_finish_condition = TRUE;
		plr_info_highlight("=============================\n");
		return 0;
	}

	_writing_page_per_loop = 0;
	_start_time = get_rtc_time();
	_writing_sequence = TRUE;

	ret = write_pattern(buf);
	if(ret < 0)
		return ret;

	_writing_sequence = FALSE;
	_end_time = get_rtc_time();
	_total_writing_time += (_end_time - _start_time);

	print_daxx_0001();
	print_statistics();

	return ret;
}
