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
#include "../plr_verify_report.h"


#define MAX_REQUEST_SECTORS 1024
static bool _writing_sequence  = FALSE;

static int wrapper_erase_request(uchar *buf, uint lsn, uint request_sectors, uint request_count, uint next_lsn)
{
	return	erase_request(lsn, request_sectors, request_count);
}

static void print_erase_customizing(uchar *buf, struct plr_state *state, struct Unconfirmed_Info *crash_info)
{
	plr_info("The page is not clean. \n");
	plr_info("The test type you selcted is Erase Test \n");

	print_chunk_simplly(state, crash_info);
	verify_print_step1(buf, state);
}


static void print_discard_customizing(uchar *buf, struct plr_state *state, struct Unconfirmed_Info *crash_info)
{
	plr_info("The page is not clean. \n");
	plr_info("The test type you selcted is Discard Test \n");

	print_chunk_simplly(state, crash_info);
}

static void print_trim_customizing(uchar *buf, struct plr_state *state, struct Unconfirmed_Info *crash_info)
{
	plr_info("The page is not clean. \n");
	plr_info("The test type you selcted is Trim Test \n");

	print_chunk_simplly(state, crash_info);
}

static void print_sanitize_customizing(uchar *buf, struct plr_state *state, struct Unconfirmed_Info *crash_info)
{
	plr_info("The page is not clean. \n");
	plr_info("The test type you selcted is Sanitize Test \n");

	print_chunk_simplly(state, crash_info);
}

static bool check_page(const void * cs, uint value, size_t count)
{
	const unsigned int *su1;
	const unsigned int check_unit_size = sizeof(uint);

	for( su1 = cs; 0 < count; ++su1, count-=check_unit_size)
	{
		if ((*su1 != value) ? TRUE:FALSE)
			return FALSE;
	}

	return TRUE;
}

static int verify_normal_customizing(uchar *buf, uint start_lsn, uint request_sectors, uint request_count, uint next_lsn)
{
	int ret = VERI_NOERR;
	uint page_offset			= 0;
	uint total_page_number 	= SECTORS_TO_PAGES(request_sectors);
	uchar *crash_pages_in_request = NULL;

	if(verify_get_loop_count() % 2 == 1)
		return verify_normal_status(buf, start_lsn, request_sectors, request_count, next_lsn);

	ret = dd_read(buf, start_lsn, request_sectors);
	if(ret != PLR_NOERROR)
	{
		verify_set_result(VERI_EIO, start_lsn);
		return VERI_EIO;
	}

	while(total_page_number--)
	{
		if(check_page(buf, 0, PAGE_SIZE) == FALSE)
		{
			if(crash_pages_in_request == NULL)
			{
				crash_pages_in_request = (uchar*)malloc(sizeof(uchar) * total_page_number);
				memset(crash_pages_in_request, EXPECTED, sizeof(uchar) * total_page_number);
			}

			crash_pages_in_request[page_offset] = UNEXPECTED;

			ret = VERI_EREQUEST;
		}
		page_offset++;
		buf += PAGE_SIZE;
	}

	if(ret == VERI_EREQUEST)
	{
		VERIFY_DEBUG_MSG("Find suspected crash, Start LSN in Chunk : 0x%08X \n", start_lsn);
		verify_set_unexpected_info(start_lsn, request_sectors, NULL, NULL, crash_pages_in_request);
		verify_set_status(VERI_STATUS_UNCONFIRMED);
		if(crash_pages_in_request == NULL)
			free(crash_pages_in_request);

		verify_insert_request_info(start_lsn, request_sectors, crash_pages_in_request, VERI_EREQUEST);
		return ret;
	}

	verify_insert_request_info(start_lsn, request_sectors, NULL, VERI_NOERR);
	return ret;
}


int verify_unconfirmed_customizing(uchar *buf, uint start_lsn, uint request_sectors, uint request_count, uint next_addr)
{
	int ret = VERI_NOERR;
	uint current_lsn 		= start_lsn;
	uint total_page_number 	= SECTORS_TO_PAGES(request_sectors);
	uint page_offset  		= 0;
	struct plr_header *header_info = NULL;

	if(verify_get_loop_count() % 2 == 1)
		return verify_unconfirmed_status(buf, start_lsn, request_sectors, request_count, next_addr);

	ret = dd_read(buf, start_lsn, request_sectors);
	if(ret != PLR_NOERROR)
	{
		verify_set_result(VERI_EIO, start_lsn);
		return VERI_EIO;
	}

	while(total_page_number--)
	{
		header_info = (struct plr_header *)(buf + (page_offset * PAGE_SIZE));
		ret = verify_check_page(header_info, current_lsn);
		if(ret != VERI_NOERR)
		{
			VERIFY_DEBUG_MSG("Confirm a Crash Status, Start LSN in Chunk : 0x%08X, Current LSN in Chunk : 0x%08X \n", start_lsn, current_lsn);
			ret = VERI_EREQUEST;
			break;
		}

		page_offset++;
		current_lsn += NUM_OF_SECTORS_PER_PAGE;
	}

	if(ret == VERI_EREQUEST)
	{
		verify_set_result(VERI_EREQUEST, start_lsn);
		verify_set_status(VERI_STATUS_CRASH);
		verify_insert_request_info(start_lsn, request_sectors, NULL, VERI_EREQUEST);
	}
	else
		verify_insert_request_info(start_lsn, request_sectors, NULL, VERI_NOERR);

	return ret;
}


static int pattern_write(uchar *buf, uint start_lsn, uint request_sectors, op_pattern operate)
{
	int ret 	= 0;
	uint lsn 	= start_lsn;
	uint request_count = 1;
	uint last_lsn_in_test_area = get_last_sector_in_test_area();

	while(TRUE)
	{
		if(request_count % 29 == 0)
			plr_info("\rlsn : 0x%08X, request size : %dKB, request num : %d", lsn, SECTORS_TO_SIZE_KB(MAX_REQUEST_SECTORS), request_count);

		ret = operate(buf, lsn, MAX_REQUEST_SECTORS, request_count, 0);
		if(ret != 0)
			return ret;

		lsn += MAX_REQUEST_SECTORS;
		if(lsn >= last_lsn_in_test_area && lsn + MAX_REQUEST_SECTORS >= last_lsn_in_test_area)
		{
			break;
		}

		request_count++;
	}

	return ret;
}

static int pattern_erase(uchar *buf, uint start_lsn, uint request_sectors, op_pattern operate)
{
	int ret = 0;
	if(_writing_sequence == TRUE)
	{
		ret = pattern_write(buf, start_lsn, request_sectors, wrapper_erase_request);
	}
	else
	{
		ret = pattern_write(buf, start_lsn, request_sectors, operate);
	}

	return ret;
}

static void init_pattern_1_2(struct plr_write_info* t_info)
{
	t_info->lsn			 	= get_first_sector_in_test_area();
	t_info->request_sectors	= MAX_REQUEST_SECTORS;
}

/* ---------------------------------------------------------------------------------------
 *
 *----------------------------------------------------------------------------------------
 */
int initialize_dpin_0009_12( uchar * buf, uint test_area_start, uint test_area_length )
{
	int ret = 0;
	struct Pattern_Function pattern_linker;
	struct Verification_Func verify_func;
	struct Print_Crash_Func print_crash_info;

	init_case_info(test_area_start, test_area_length);
	set_test_sectors_in_zone(get_sectors_in_zone());

	/*
	* NOTE *************************************************************************************
	* 포팅을 위한 임시 코드.
	* ******************************************************************************************
	*/
	init_pattern(&g_plr_state, TRUE);

	verify_func.normal_verification 		= verify_normal_customizing;
	verify_func.unconfirmed_verification	= verify_unconfirmed_customizing;
	verify_init_func(verify_func);

	/*
	* NOTE *************************************************************************************
	* 사용자는 initialize level 에서 register pattern을 호출해야 한다.
	* ******************************************************************************************
	*/
	memset(&pattern_linker, 0, sizeof(struct Pattern_Function));
	pattern_linker.do_pattern_1 = pattern_write;
	pattern_linker.do_pattern_2 = pattern_erase;
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

	switch(g_plr_state.test_num)
	{
		case 9:
			set_erase_test_type(TYPE_ERASE);
			print_crash_info.default_step 	= print_erase_customizing;
			break;
		case 10:
			set_erase_test_type(TYPE_TRIM);
			print_crash_info.default_step 	= print_trim_customizing;
			break;
		case 11:
			set_erase_test_type(TYPE_DISCARD);
			print_crash_info.default_step 	= print_discard_customizing;
			break;
		case 12:
			set_erase_test_type(TYPE_SANITIZE);
			print_crash_info.default_step 	= print_sanitize_customizing;
			break;
		default:
			plr_err("\n");
			break;
	}

	print_crash_info.checking_lsn_crc_step 	= NULL;
	print_crash_info.extra_step 			= NULL;
	verify_init_print_func(print_crash_info);

	return ret;
}

int read_dpin_0009_12(uchar * buf, uint test_area_start, uint test_area_length)
{
	return verify_pattern(buf);
}

int write_dpin_0009_12(uchar * buf, uint test_area_start, uint test_area_length)
{
	int ret = PLR_NOERROR;

	_writing_sequence = TRUE;
	ret = write_pattern(buf);
	_writing_sequence = FALSE;

	return ret;
}
