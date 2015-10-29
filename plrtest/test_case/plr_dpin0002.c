 /*
  * Copyright (c) 2013	ElixirFlash Technology Co., Ltd.
  * 			 http://www.elixirflash.com/
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


static bool _verification_sequence = FALSE;

static int pattern_interleaving(uchar *buf, uint start_lsn, uint request_sectors, op_pattern operate)
{
  	int ret = 0;

	uint request_count  = 1;
	uint lsn			= get_first_sector_in_test_area();
	uint request_length = NUM_OF_SECTORS_PER_PAGE << 1;

	uint last_lsn_in_test_area  = get_last_sector_in_test_area();

	if(_verification_sequence == TRUE)
		return VERI_STATUS_END;

	while (TRUE) 
	{
		for (; lsn < last_lsn_in_test_area; lsn += get_sectors_in_zone()) 
		{	 
			if(request_count % 29 == 0)
				plr_info("\rlsn : 0x%08X, request size : %dKB, request num : %d", lsn, SECTORS_TO_SIZE_KB(request_length), request_count);		
			
			ret = operate(buf, lsn, request_length, request_count, 0);
			if (ret != 0)
				return ret;

			request_count++;
		}

		lsn = get_first_sector_in_test_area();
	}

	return ret;
}

static int extra_verification(uchar *buf, struct plr_state *state)
{
  	int ret = VERI_NOERR;
	uint lsn			= get_first_sector_in_test_area();
	uint request_length = NUM_OF_SECTORS_PER_PAGE << 1;
	uint last_lsn_in_test_area  = get_last_sector_in_test_area();
	struct plr_header *header_info;

	for (; lsn < last_lsn_in_test_area; lsn += get_sectors_in_zone()) 
	{	 		
		plr_info("\rCheck LSN, CRC(0x%08X)", lsn);	
		ret = dd_read(buf, lsn, request_length);
		if (ret != VERI_NOERR)
			return ret;

		header_info = (struct plr_header *)(buf);
		ret = verify_check_page(header_info, lsn);
		if(ret != VERI_NOERR)
		{
			VERIFY_DEBUG_MSG("CRASH : 0x%08X ---------------- \n", lsn);
			verify_set_result(VERI_EREQUEST, lsn);
			verify_insert_request_info(lsn, request_length, NULL, VERI_EREQUEST);			
		}				
		
		verify_insert_request_info(lsn, request_length, NULL, VERI_NOERR);			
		ret = VERI_NOERR;
	}

	return ret;
}


static void init_pattern_1_2(struct plr_write_info* t_info)
{
	t_info->lsn			  = get_first_sector_in_test_area();
	t_info->request_sectors   = NUM_OF_SECTORS_PER_PAGE << 1;
}

/* ---------------------------------------------------------------------------------------
* 
*----------------------------------------------------------------------------------------
*/

int initialize_dpin_0002( uchar * buf, uint test_area_start, uint test_area_length )
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
	pattern_linker.do_pattern_1	  	= pattern_interleaving;
	pattern_linker.init_pattern_1   = init_pattern_1_2; 
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

int read_dpin_0002( uchar * buf, uint test_start_sector, uint test_sector_length )
{
	int ret = 0;
	
	_verification_sequence = TRUE;
	ret = verify_pattern(buf);
	_verification_sequence = FALSE;
	
	return ret;
}

int write_dpin_0002( uchar * buf, uint test_start_sector, uint test_sector_length )
{
	return write_pattern(buf);
}

