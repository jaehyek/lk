#include "plr_common.h"
#include "plr_verify.h"
#include "plr_verify_report.h"
#include "plr_verify_log.h"
#include "plr_deviceio.h"
#include "plr_case_info.h"
#include "plr_errno.h"

void print_default_step(uchar *buf, struct plr_state *state, struct Unconfirmed_Info *crash_info)
{
	if(crash_info == NULL)
		return ;

	if(crash_info->crash_pages_in_chunk == NULL)
		return ;
	
	print_chunk_simplly(state, crash_info);
	print_crash_status_header_info(crash_info);
	
	verify_print_step1(buf, state);
}

void print_checking_lsn_crc_step(uchar *buf, struct plr_state *state, struct Unconfirmed_Info *crash_info)
{
	verify_print_step2(buf, state);
}

void print_extra_step(uchar *buf, struct plr_state *state, struct Unconfirmed_Info *crash_info)
{
	verify_print_step3(buf, state);
}

void print_chunk_simplly(struct plr_state *state, struct Unconfirmed_Info *crash_info)
{
	uint chunk_pages	= SECTORS_TO_PAGES(crash_info->request_sectors_cnt);
	uint page_index 	= 0;
	
	plr_info("Brief the crashed chunk. ([index:O or X]\tNormal Page = Yellow, O\tAbnormal Page = Red, X)\n");		

	if(state->internal_poweroff_type == TRUE)
		plr_info("\tLast Power Loss Pointer : 0x%08X\n", state->poweroff_pos);
	if(state->b_cache_enable == TRUE)
		plr_info("\tLast Cache Flush Pointer : 0x%08X\n", state->last_flush_pos);
	
	plr_info("\tStart LSN in crashed chunk : 0x%08X, Chunk Size : %dkB, Current Setting about Partial Type : %d \n", crash_info->request_start_lsn, chunk_pages*4, state->test_minor);
	plr_info("\tCurrent Boot Count : %d, Current Loop Count : %d\n", crash_info->current_boot_count, crash_info->current_loop_count);
	plr_info("\t");

	if(crash_info->crash_pages_in_chunk != NULL)
	{
		for(page_index = 0; page_index < chunk_pages; page_index++)
		{		
			if(page_index  != 0 && page_index % 16 == 0)
				plr_info("\n\t");

			if(crash_info->crash_pages_in_chunk[page_index] == EXPECTED)
				plr_info("[%03d:O]", page_index);
			else
				plr_err("[%03d:X]", page_index);
		}	
	}
	plr_info("\n");
}

void print_crash_status_header_info(struct Unconfirmed_Info *crash_info)
{	
	struct plr_header *expected_data 	= &crash_info->expected_data;
	struct plr_header *crashed_data 	= &crash_info->crashed_data;

	if(expected_data == NULL || crashed_data == NULL)
		return ;

	plr_info("\nPrint the last crashed page\n");
	plr_info ("\tExpected Header Information\n");
	plr_info ("\t--------------------------------------\n");
	plr_info ("\tmagic number   : 0x%08X\n", 	expected_data->magic_number);	
	plr_info ("\tsector number  : 0x%07X\n", 	expected_data->lsn);
	plr_info ("\tboot cnt       : %d\n", 		expected_data->boot_cnt);		
	plr_info ("\tloop cnt       : %d\n", 		expected_data->loop_cnt);		
	plr_info ("\trequest cnt    : %d\n", 		expected_data->req_info.req_seq_num);
	plr_info ("\tstart sector   : 0x%07X\n", 	expected_data->req_info.start_sector);
	plr_info ("\tpage index     : %d\n", 		expected_data->req_info.page_index);
	plr_info ("\tpage num       : %d\n", 		expected_data->req_info.page_num);
	plr_info ("\tnext req addr  : 0x%07X\n", 	expected_data->next_start_sector);
	plr_info ("\treserved1      : %d\n", 		expected_data->reserved1);
	plr_info ("\tCRC            : 0x%08X\n", 	expected_data->crc_checksum);	
	plr_info ("\t--------------------------------------\n");
	
	plr_info ("\n");
	plr_info ("\tCrashed Header Information\n");
	plr_info ("\t--------------------------------------\n");
	if(expected_data->magic_number == crashed_data->magic_number)
		plr_info ("\tmagic number   : 0x%08X\n", crashed_data->magic_number);	
	else 
		plr_err ("\tmagic number   : 0x%08X\n", crashed_data->magic_number);	
	
	if(expected_data->lsn == crashed_data->lsn)
		plr_info ("\tsector number  : 0x%07X\n", crashed_data->lsn);
	else 
		plr_err ("\tsector number  : 0x%07X\n", crashed_data->lsn);
		
	if(expected_data->boot_cnt == crashed_data->boot_cnt)
		plr_info ("\tboot cnt       : %d\n", crashed_data->boot_cnt);		
	else 
		plr_err ("\tboot cnt       : %d\n", crashed_data->boot_cnt);		
	
	if(expected_data->loop_cnt == crashed_data->loop_cnt)
		plr_info ("\tloop cnt       : %d\n", crashed_data->loop_cnt);		
	else 
		plr_err ("\tloop cnt       : %d\n", crashed_data->loop_cnt);		
	
	if(expected_data->req_info.req_seq_num == crashed_data->req_info.req_seq_num)
		plr_info ("\trequest cnt    : %d\n",crashed_data->req_info.req_seq_num);
	else 
		plr_err ("\trequest cnt    : %d\n", crashed_data->req_info.req_seq_num);
	
	if(expected_data->req_info.start_sector == crashed_data->req_info.start_sector)
		plr_info ("\tstart sector   : 0x%07X\n", crashed_data->req_info.start_sector);
	else 
		plr_err ("\tstart sector   : 0x%07X\n", crashed_data->req_info.start_sector);
	
	if(expected_data->req_info.page_index == crashed_data->req_info.page_index)
		plr_info ("\tpage index     : %d\n", crashed_data->req_info.page_index);
	else 
		plr_err ("\tpage index     : %d\n", crashed_data->req_info.page_index);
	
	if(expected_data->req_info.page_num == crashed_data->req_info.page_num)
		plr_info ("\tpage num       : %d\n", crashed_data->req_info.page_num);
	else
		plr_err ("\tpage num       : %d\n", crashed_data->req_info.page_num);
	
	if(expected_data->next_start_sector == crashed_data->next_start_sector)
		plr_info ("\tnext req addr  : 0x%07X\n", crashed_data->next_start_sector);
	else
		plr_err ("\tnext req addr  : 0x%07X\n", crashed_data->next_start_sector);
	
	if(expected_data->reserved1 == crashed_data->reserved1)
		plr_info ("\treserved1      : %d\n", crashed_data->reserved1);
	else
		plr_err ("\treserved1      : %d\n", crashed_data->reserved1);
	
	if(expected_data->crc_checksum == crashed_data->crc_checksum)
		plr_info ("\tCRC            : 0x%08X\n", crashed_data->crc_checksum);	
	else
		plr_err ("\tCRC            : 0x%08X\n", crashed_data->crc_checksum);		
	plr_info ("\t--------------------------------------\n");
}
