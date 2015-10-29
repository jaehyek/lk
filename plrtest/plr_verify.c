/*
* HACK ******************************************************************************************************
* 인자로 받아오는 모든 buffer는 read buffer임을 보장받아야 한다.
* read buffer는 충분한 크기로 보장 받는다. (현재 시스템에서는 8MB로 고정) 
* ***********************************************************************************************************
*/

#include "plr_common.h"
#include "plr_verify.h"
#include "plr_verify_report.h"
#include "plr_verify_log.h"
#include "plr_deviceio.h"
#include "plr_case_info.h"
#include "plr_errno.h"

#define LAST_PO				 	 0
#define PREVIOUS_LAST_PO		-1

#define GET_CRASHED_IN_STEP(X,step)	(((X) & (0x1 << (step))) != 0) ? TRUE:FALSE
#define SET_CRASHED_IN_STEP(X,step)	((X) |= (0x1 << (step)))

enum CACHE_PACKED
{
	NONE = 0,
	CACHE,
	PACKED,
};

static struct plr_state* 		_state;
static struct Unconfirmed_Info 	_unconfirmed_crash_info;
static enum VERIFICATION_STATUS _verification_status;

static uint	_expected_loop_count 			= 0;
static uint	_expected_boot_count 			= 0;	
static uint	_veri_request_cnt_unconfirmed	= 0;
static int 	_request_cnt_after_last_PO		= PREVIOUS_LAST_PO;
static int 	_current_verification_step 		= VERI_DEFAULT_STEP;
static uchar _crashed_flag_in_each_step 	= 0;
static int 	_verification_result = VERI_NOERR;

static bool _finish_cal_last_lsn_in_packed_req 	= FALSE;
static uint _packed_request_count = 0;
static uint _predicted_packed_req_sectors	 	= 0;
static uint _predicted_last_lsn_in_packed_req 	= 0;

static bool _finish_cal_last_lsn_in_cached_req 	= FALSE;
static uint _cached_request_count = 0;
static uint _predicted_cached_req_sectors	 	= 0;
static uint _predicted_last_lsn_in_cached_req 	= 0;

static enum CACHE_PACKED _cache_or_packed_list	= NONE;
static bool _is_erase_test = FALSE;

VERIFY_FUNC _verify = NULL;


/*
* ***********************************************************************************************************
* Declare the static function.
* ***********************************************************************************************************
*/
static uint classify_partial_type(uchar *, uint);
static int 	check_partial_writing(uchar *, uint , uint);
static int 	memorycmp(const void *,const void *,size_t);
static void make_origin_chunk(uchar *, uint, uint, uint, uint);
static void make_expected_header(uchar *, uint, uint, uint, uint, uint, bool);
static void make_expected_header_data(uchar *, uint, uint, uint, uint, bool);
static void set_last_lsn_in_packed_list(uint, uint, int);
static void set_last_lsn_in_cached_list(uint, uint, int);

static struct Verification_Func _verify_function ={ .normal_verification = verify_normal_status,
													.unconfirmed_verification = verify_unconfirmed_status} ;

static struct Print_Crash_Func _print_crash_info ={	.default_step = print_default_step,
													.checking_lsn_crc_step = print_checking_lsn_crc_step,
													.extra_step = print_extra_step};

void verify_init(struct plr_state* state, bool is_erase_test)
{
	_state = state;
	_state->print_crash_only = FALSE;
	_is_erase_test = is_erase_test;
	verify_reset(0, 0);
}

void verify_init_func(struct Verification_Func verify_func)
{
	if(verify_func.normal_verification != NULL)
	{
		_verify_function.normal_verification = verify_func.normal_verification;
		_verify = _verify_function.normal_verification;	
	}
	if(verify_func.unconfirmed_verification != NULL)
		_verify_function.unconfirmed_verification = verify_func.unconfirmed_verification;	
}

void verify_init_print_func(struct Print_Crash_Func print_crash_info)
{
	if(print_crash_info.default_step != NULL)
		_print_crash_info.default_step = print_crash_info.default_step; 

	if(print_crash_info.checking_lsn_crc_step != NULL)
		_print_crash_info.checking_lsn_crc_step = print_crash_info.checking_lsn_crc_step;

	if(print_crash_info.extra_step != NULL)
		_print_crash_info.extra_step = print_crash_info.extra_step;
}


void verify_reset(uint	expected_loop, uint expected_boot)
{
	_veri_request_cnt_unconfirmed 	= 0;			
	_expected_loop_count			= expected_loop;
	_expected_boot_count 			= expected_boot;	
	_verification_status 			= VERI_STATUS_NORMAL;	
	_request_cnt_after_last_PO		= PREVIOUS_LAST_PO;
	_current_verification_step 		= VERI_DEFAULT_STEP;

	_crashed_flag_in_each_step		= 0;

	_finish_cal_last_lsn_in_packed_req	= FALSE;
	_packed_request_count 				= 0;
	_predicted_packed_req_sectors		= 0;
	_predicted_last_lsn_in_packed_req	= 0;
	
	_finish_cal_last_lsn_in_cached_req	= FALSE;
	_cached_request_count 				= 0;
	_predicted_cached_req_sectors		= 0;
	_predicted_last_lsn_in_cached_req	= 0;

	_verification_result				= VERI_NOERR;
	_verify = _verify_function.normal_verification;	
		
	if(_unconfirmed_crash_info.crash_pages_in_chunk != NULL)
		free(_unconfirmed_crash_info.crash_pages_in_chunk);

	memset(&_unconfirmed_crash_info, 0, sizeof(struct Unconfirmed_Info));		
	_unconfirmed_crash_info.crash_pages_in_chunk = NULL;
}

void verify_set_loop_count(uint expected_loop)
{
	_expected_loop_count = expected_loop;
}

void verify_set_boot_count(uint expected_boot)
{
	_expected_boot_count = expected_boot;
}

uint verify_get_loop_count(void)
{
	return _expected_loop_count;
}

uint verify_get_boot_count(void)
{
	return _expected_boot_count;
}

/*
* NOTE ******************************************************************************************************
* _verify는 함수 포인트이다. _verify가 호출한 함수 내부에서는 _verification_status의 값(상태)과 _device_error의 값이 변경된다. 
*		1. _verification_status의 값은 현재 verification의 진행 상태를 나타낸다.
*		2. _device_error의 값은 device로 부터 발생한 error를 저장한다.
*
* _verify가 호출하는 함수의 리턴값은 
*		1. 0	: 문제 없음 
*		2. -1	: _device_error or _verification_status 의 상태가 변경됨 
*
* _verification_status 의 상태 변화 
*		1. VERI_STATUS_NORMAL --> VERI_STATUS_UNCONFIRMED --> VERI_STATUS_CRASH 	(Crash 발생)
*		2. VERI_STATUS_NORMAL --> VERI_STATUS_UNCONFIRMED --> VERI_STATUS_FINISH	(No Problem)
*		3. VERI_STATUS_NORMAL --> VERI_STATUS_CRASH 								(Partial Write가 존재)
* ***********************************************************************************************************
*/
int do_verify(uchar *buf, uint lsn, uint request_sectors, uint request_count, uint next_addr)
{
	int ret = 0;
	if(_veri_request_cnt_unconfirmed >= VERI_REQUEST_CNT_UNCONFIRMED_STATUS) 
		return VERI_STATUS_FINISH;

	if(verify_get_status() != VERI_STATUS_NORMAL)
		_veri_request_cnt_unconfirmed++;

	if(_request_cnt_after_last_PO >= LAST_PO)
		_request_cnt_after_last_PO++;	
	
	if(_state->internal_poweroff_type == TRUE && lsn == _state->poweroff_pos)
		_request_cnt_after_last_PO = LAST_PO;		

	ret = _verify(buf, lsn, request_sectors, request_count, next_addr);	
	set_last_lsn_in_packed_list(lsn, request_sectors, request_count);
	set_last_lsn_in_cached_list(lsn, request_sectors, request_count);
	switch(ret)
	{
		case VERI_NOERR:
			return VERI_NOERR;
		case VERI_EIO:
			return verify_get_result(_state);
		case VERI_EREQUEST:
			break;
		default : 
			VERIFY_DEBUG_MSG("Can't reach here!\n");
			break;
	}

	if(verify_get_status() == VERI_STATUS_UNCONFIRMED)	
	{			
		// for sleep and awake 
		// _state->poweroff_pos != 0x0
		// HACK : Really, If the power off occurs at the 0x0, ? 
		if(_state->poweroff_pos != 0x0 && (_state->internal_poweroff_type == TRUE && _state->b_cache_enable == FALSE && _state->b_packed_enable == FALSE))
		
		{			
			if(_is_erase_test == FALSE && (_request_cnt_after_last_PO == PREVIOUS_LAST_PO || _request_cnt_after_last_PO > LAST_PO + 1))
			{	
				verify_set_status(VERI_STATUS_CRASH);	
				verify_set_result(VERI_EREQUEST, lsn);
			}	
		}					
		
		_verify = _verify_function.unconfirmed_verification;							
	}

	return VERI_DONT_CARE;
}

void verify_print_crash_info(uchar *buf, struct plr_state* state)
{
	struct Unconfirmed_Info	crash_info = {0, };

	if(_crashed_flag_in_each_step == 0)
	{
		VERIFY_DEBUG_MSG("_crashed_flag_in_each_step is zero \n");
		return ;
	}

	plr_info("\n\n\nCrash ***************************************************************************************************************************************\n\n");		
	if(GET_CRASHED_IN_STEP(_crashed_flag_in_each_step, VERI_DEFAULT_STEP))
	{		
		_print_crash_info.default_step(buf, state, &_unconfirmed_crash_info);
		plr_info("\n*********************************************************************************************************************************************\n\n");
	}

	if(GET_CRASHED_IN_STEP(_crashed_flag_in_each_step, VERI_CHECKING_LSN_CRC_STEP))
	{
		_print_crash_info.checking_lsn_crc_step(buf, state, &crash_info);
		plr_info("\n*********************************************************************************************************************************************\n\n");
	}

	if(GET_CRASHED_IN_STEP(_crashed_flag_in_each_step, VERI_EXTRA_STEP))
	{
		_print_crash_info.extra_step(buf, state, &crash_info);
		plr_info("\n*********************************************************************************************************************************************\n\n");
	}
}

int verify_report_result(struct plr_state* state)
{
	return _verification_result;
}

void verify_apply_result(struct plr_state* state)
{
	state->write_info.lsn 			= _unconfirmed_crash_info.request_start_lsn;
	state->write_info.request_sectors= _unconfirmed_crash_info.request_sectors_cnt;
	
	state->write_info.loop_count 	= _unconfirmed_crash_info.current_loop_count;		
	state->loop_cnt 				= _unconfirmed_crash_info.current_loop_count;	
}

void verify_set_current_step(enum VERIFICATION_STEP step)
{
	_current_verification_step = step;
}

enum VERIFICATION_STEP verify_get_current_step(void)
{
	return _current_verification_step;
}

void verify_set_result(enum VERI_ERROR_NUM result, uint lsn)
{
	_verification_result = result;

	VERIFY_DEBUG_MSG("Result : %d, lsn : 0x%08X \n", result, lsn);
	
	if(result >= VERI_EREQUEST)
		SET_CRASHED_IN_STEP(_crashed_flag_in_each_step, _current_verification_step);

}

int verify_get_result(struct plr_state *state)
{	
	int ret = PLR_NOERROR;		
	ret = verify_get_error_num(state);
	if(ret != 0)
		return ret;

	ret = verify_get_crash_num(state);
	if(ret != 0)
		return ret;

	return ret;
}

int verify_get_error_num(struct plr_state *state)
{
	if(_verification_result == VERI_NOERR)
		return 0;
	
	if(_verification_result < VERI_EREQUEST)
		return PLR_EIO;

	return 0;
}

int verify_get_crash_num(struct plr_state *state)
{
	if(_verification_result >= VERI_EREQUEST)
		return PLR_ECRASH;

	return 0;
}

uint verify_get_predicted_last_lsn_previous_boot(void)
{
	if(_current_verification_step <= VERI_DEFAULT_STEP)
	{
		VERIFY_DEBUG_MSG("You need to call the this function when the default verification step is finish.\n");
		return 0;
	}	

	if(_cache_or_packed_list == CACHE)
	{
		VERIFY_DEBUG_MSG("_predicted_last_lsn_in_cached_req : 0x%08X \n", _predicted_last_lsn_in_cached_req);
		return _predicted_last_lsn_in_cached_req;
	}
	else 
	{
		VERIFY_DEBUG_MSG("_predicted_last_lsn_in_packed_req : 0x%08X \n", _predicted_last_lsn_in_packed_req);
		return _predicted_last_lsn_in_packed_req;
	}
}

int verify_find_chunk(uchar *buf, uint start_lsn, uint end_lsn, uint expected_loop, uint *chunk_start, uint *pages_in_chunk, bool *flag_find)
{
	int ret = -1;
	uint s_lsn 		= start_lsn;
	uint e_lsn 		= 0;
	uint length 	= NUM_OF_SECTORS_PER_8MB;	
	uint page_offset= 0;
	uint boot_count = 0;
	struct plr_header *header_info = NULL;
		
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
		page_offset = 0;
		while(s_lsn < e_lsn)
		{
			header_info = (struct plr_header *)(buf + (page_offset * PAGE_SIZE));
			if(boot_count == 0)
				boot_count = header_info->boot_cnt;
		
			if(boot_count >= header_info->boot_cnt)
			{
				if(header_info->magic_number == MAGIC_NUMBER || header_info->magic_number == MAGIC_NUMBER_FLUSH)				
				{		
					if(header_info->loop_cnt == expected_loop && header_info->req_info.page_index == 0)
					{
						*chunk_start 	= header_info->req_info.start_sector;
						*pages_in_chunk	= header_info->req_info.page_num;
						*flag_find = TRUE;
						return 0;
					}
				}

				boot_count = header_info->boot_cnt;			
			}

			s_lsn += NUM_OF_SECTORS_PER_PAGE;
			page_offset++;
		}
	}		

	return 0;
}

int verify_check_page(struct plr_header * header_info, uint lsn)
{	
	if(header_info->magic_number == 0)
	{
		if( header_info->lsn 				!= 0)	return VERI_EREQUEST;
		if( header_info->loop_cnt			!= 0)	return VERI_EREQUEST;
		if (header_info->crc_checksum 		!= 0) 	return VERI_EREQUEST;

		return VERI_NOERR;
	}

	if((header_info->magic_number == MAGIC_NUMBER 			||
		header_info->magic_number == MAGIC_NUMBER_FLUSH) == FALSE)
	{		
		return VERI_EREQUEST;
	}
	
	if(lsn != header_info->lsn)
		return VERI_EREQUEST;
		
	if(header_info->crc_checksum != crc32(0, (const uchar *)(&(header_info->lsn)), sizeof(struct plr_header) - sizeof(uint) * 4)) 
		return VERI_EREQUEST;
	
	return VERI_NOERR;
}

int verify_check_pages_lsn_crc_loop(uchar *buf, uint start_lsn, uint end_lsn, bool is_expected_loop, uint expected_loop)
{
	int ret 		= VERI_NOERR;
	int result 		= VERI_NOERR;
	uint length 	= NUM_OF_SECTORS_PER_8MB;	
	uint page_offset= 0;
	uint s_lsn 		= 0;	
	uint e_lsn 		= 0;
	struct plr_header *header_info = NULL;

	s_lsn = start_lsn;

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
			{
				if(is_expected_loop == FALSE)
					plr_info("\rCheck LSN, CRC(0x%08X)", s_lsn);	
				else 
					plr_info("\rCheck LSN, CRC, Loop(0x%08X)", s_lsn);	
			}
			
			header_info = (struct plr_header *)(buf + (page_offset * PAGE_SIZE));
			result = verify_check_page(header_info, s_lsn);		

			if(is_expected_loop == TRUE && header_info->loop_cnt != expected_loop)
				result = VERI_EREQUEST;

			if(result == VERI_EREQUEST)			
			{
				VERIFY_DEBUG_MSG("LoopCount Error. LSN(0x%08X), Expected Loop Count(%d) != Header Loop Count(%d) \n", s_lsn, expected_loop, header_info->loop_cnt);
				verify_set_result(VERI_EREQUEST, s_lsn);
				verify_set_status(VERI_STATUS_CRASH);
				verify_insert_request_info(s_lsn, NUM_OF_SECTORS_PER_PAGE, NULL, VERI_EREQUEST);
				ret = VERI_EREQUEST;
			}
			else 
				verify_insert_request_info(s_lsn, NUM_OF_SECTORS_PER_PAGE, NULL, VERI_NOERR);					

			result = VERI_NOERR;
			s_lsn += NUM_OF_SECTORS_PER_PAGE;
			page_offset++;
		}

		page_offset = 0;
	}

	return ret;
}

int verify_check_random_zone_only_lsn_crc(uchar *buf)
{
	uint random_zone;
	uint start_lsn;
	uint end_lsn;
	
	random_zone = well512_rand() % get_total_zone();
	start_lsn = get_first_sector_in_zone(random_zone);
	end_lsn = start_lsn + get_sectors_in_zone();

	VERIFY_DEBUG_MSG("LSN and CRC Check, Start LSN : 0x%08X, End LSN : 0x%08X\n", start_lsn, end_lsn);
	return verify_check_pages_lsn_crc_loop(buf, start_lsn, end_lsn, FALSE, 0);
}


void verify_set_status(enum VERIFICATION_STATUS status)
{
	_verification_status = status;
}

enum VERIFICATION_STATUS verify_get_status(void)
{
	return _verification_status;
}

void verify_set_unexpected_info(uint lsn, uint request_sectors, struct plr_header *expected_data, struct plr_header *crashed_data, uchar *crash_pages_in_request)
{
	_unconfirmed_crash_info.request_start_lsn 	= lsn;
	_unconfirmed_crash_info.request_sectors_cnt = request_sectors;
	_unconfirmed_crash_info.current_boot_count 	= _expected_boot_count;
	_unconfirmed_crash_info.current_loop_count 	= _expected_loop_count;

	if(expected_data != NULL)
		memcpy(&_unconfirmed_crash_info.expected_data, expected_data, sizeof(struct plr_header));	
	if(crashed_data != NULL)
		memcpy(&_unconfirmed_crash_info.crashed_data, crashed_data, sizeof(struct plr_header));	

	if(crash_pages_in_request != NULL)
	{
		if(_unconfirmed_crash_info.crash_pages_in_chunk != NULL)
			free(_unconfirmed_crash_info.crash_pages_in_chunk);

		_unconfirmed_crash_info.crash_pages_in_chunk = (uchar*)malloc(sizeof(uchar) * SECTORS_TO_PAGES(request_sectors));
		memcpy(_unconfirmed_crash_info.crash_pages_in_chunk, crash_pages_in_request, SECTORS_TO_PAGES(request_sectors));
	}
}

void save_test_info(struct plr_state *state)
{
	uchar *buf = plr_get_extra_buffer();

	memcpy(buf, state, sizeof(struct plr_state));}

void verify_save_crash_info(void)
{
	uchar *buf = plr_get_write_buffer();
	uint offset = 0;

	memcpy(buf + offset, _state, sizeof(struct plr_state));
	offset += sizeof(struct plr_state);

	memcpy(buf + offset, &_unconfirmed_crash_info, sizeof(struct Unconfirmed_Info));
	offset += sizeof(struct Unconfirmed_Info);

	if(_unconfirmed_crash_info.crash_pages_in_chunk != NULL)
	{
		memcpy(buf + offset, _unconfirmed_crash_info.crash_pages_in_chunk, SECTORS_TO_PAGES(_unconfirmed_crash_info.request_sectors_cnt));		
		offset += SECTORS_TO_PAGES(_unconfirmed_crash_info.request_sectors_cnt);
	}

	memcpy(buf + offset, &_crashed_flag_in_each_step, sizeof(uchar));
	offset += sizeof(uchar);


	clear_sdcard_sectors(SAVE_VERIFY_RESULT_SECTOR_NUM, 1);
	save_crash_dump(buf, offset, SAVE_VERIFY_RESULT_SECTOR_NUM);

	VERIFY_DEBUG_MSG("_crashed_flag_in_each_step : %d \n", _crashed_flag_in_each_step);	
}

void verify_load_crash_info(struct plr_state *state)
{
	uchar *buf = plr_get_read_buffer();
	uint offset = 0;
	
	load_crash_dump(buf, SECTOR_SIZE * 2, SAVE_VERIFY_RESULT_SECTOR_NUM);

	if(state != NULL)
	{
		memcpy(state, buf, sizeof(struct plr_state));
	}
	else
	{
		offset += sizeof(struct plr_state);
		verify_reset(0, 0);

		memcpy(&_unconfirmed_crash_info, buf + offset, sizeof(struct Unconfirmed_Info));	
		offset += sizeof(struct Unconfirmed_Info);

		if(_unconfirmed_crash_info.crash_pages_in_chunk != NULL)
		{
			_unconfirmed_crash_info.crash_pages_in_chunk = NULL;
			_unconfirmed_crash_info.crash_pages_in_chunk = (uchar*)malloc(sizeof(uchar) * SECTORS_TO_PAGES(_unconfirmed_crash_info.request_sectors_cnt));
			if(_unconfirmed_crash_info.crash_pages_in_chunk == NULL)
			{
				plr_info("Fail the memory allocation \n");
				return ;
			}

			memcpy(_unconfirmed_crash_info.crash_pages_in_chunk, buf + offset, SECTORS_TO_PAGES(_unconfirmed_crash_info.request_sectors_cnt));
			offset += SECTORS_TO_PAGES(_unconfirmed_crash_info.request_sectors_cnt);
		}

		memcpy(&_crashed_flag_in_each_step, buf + offset, sizeof(uchar));

		VERIFY_DEBUG_MSG("_crashed_flag_in_each_step : %d \n", _crashed_flag_in_each_step);
	}
}

int verify_normal_status(uchar *buf, uint start_lsn, uint request_sectors, uint request_count, uint next_addr)
{
	int ret = VERI_NOERR;
	uint page_offset 		= 0;
	uint total_page_number 	= SECTORS_TO_PAGES(request_sectors);
	uchar *expected_data 	= NULL;
	uchar *read_data 		= NULL;
	uchar *p_read 			= NULL;
	uchar *p_expected		= NULL;
	uchar *crash_pages_in_request = NULL;
	uint header_size 		= sizeof(struct plr_header);	

	ret = dd_read(buf, start_lsn, request_sectors);
	if(ret != PLR_NOERROR)
	{
		verify_set_result(VERI_EIO, start_lsn);
		return VERI_EIO;
	}

	/*
	* HACK ****************************************************************************************************** 
	* 인자로 받은 buf를 반으로 나눠서 사용한다. buf의 크기는 "아마도" 8MB로 픽스 일 것이다.  
	* 만약 하나의 Request 크기가 4MB를 넘는다면 해당 코드는 버그를 발생시킨다.
	* ***********************************************************************************************************
	*/	
	read_data 		= buf;
	expected_data	= buf + (total_page_number * PAGE_SIZE);	
	make_origin_chunk(expected_data, start_lsn, request_sectors, request_count, next_addr);

	while(page_offset < total_page_number)
	{
		/*
		* NOTE ****************************************************************************************************** 
		* Body 부분은 일단 검사하지 않는다. 
		* ***********************************************************************************************************
		*/		
		if( memorycmp(read_data, expected_data, header_size) != 0)
		{				
			ret = VERI_EREQUEST;						
			if(crash_pages_in_request == NULL)
			{
				crash_pages_in_request = (uchar*)malloc(sizeof(uchar) * total_page_number);
				memset(crash_pages_in_request, EXPECTED, sizeof(uchar) * total_page_number);			
			}
			
			crash_pages_in_request[page_offset] = UNEXPECTED;
			p_read 		= read_data;
			p_expected = expected_data;
		}
		
		read_data 		+= PAGE_SIZE;
		expected_data 	+= PAGE_SIZE;			
		page_offset++;
	}
	
	if(ret == VERI_EREQUEST)
	{		
		if(	_state->b_cache_enable == FALSE 
			&& check_partial_writing(crash_pages_in_request, start_lsn, request_sectors) == VERI_EPARTIAL)
		{

			verify_set_status(VERI_STATUS_CRASH);
			verify_set_result(VERI_EPARTIAL, start_lsn);
		}
		else 
			verify_set_status(VERI_STATUS_UNCONFIRMED);

		VERIFY_DEBUG_MSG("Find suspected crash, Start LSN in Chunk : 0x%08X \n", start_lsn);				
		verify_set_unexpected_info(start_lsn, 
									request_sectors, 
									(struct plr_header*)p_expected,
									(struct plr_header*)p_read,
									crash_pages_in_request);	

		verify_insert_request_info(start_lsn, request_sectors, crash_pages_in_request ,VERI_EREQUEST);
		if(crash_pages_in_request != NULL)
			free(crash_pages_in_request);
		return VERI_EREQUEST;
	}

	verify_insert_request_info(start_lsn, request_sectors, NULL ,VERI_NOERR);
	
	return VERI_NOERR;
}


int verify_unconfirmed_status(uchar *buf, uint start_lsn, uint request_sectors, uint request_count, uint next_addr)
{
	int ret = VERI_NOERR;		
	uint current_lsn 		= start_lsn;
	uint total_page_number 	= SECTORS_TO_PAGES(request_sectors);		
	uint page_offset  		= 0;
	bool check_member 		= TRUE;
	
	struct plr_header *header_info = NULL;
		
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

		if(_state->b_cache_enable == TRUE)
		{
			if((_state->internal_poweroff_type == TRUE && header_info->lsn == _state->last_flush_pos) || 
				header_info->magic_number == MAGIC_NUMBER_FLUSH)
			{
				check_member = TRUE;
			}
			else 
				check_member = FALSE;				
		}
	
		if(check_member == TRUE && _expected_loop_count == header_info->loop_cnt)
			ret = VERI_EREQUEST;
		
		if(ret == VERI_EREQUEST)
		{
			break;
		}

		page_offset++;
		current_lsn += NUM_OF_SECTORS_PER_PAGE;
	}	

	if(ret == VERI_EREQUEST)
	{
		VERIFY_DEBUG_MSG("Confirm a Crash Status, Start LSN in Chunk : 0x%08X, Current LSN in Chunk : 0x%08X \n", start_lsn, current_lsn);
		verify_set_result(VERI_EPARTIAL, start_lsn);
		verify_set_status(VERI_STATUS_CRASH);
		verify_insert_request_info(start_lsn, request_sectors, NULL ,VERI_EREQUEST);
	}
	else 
		verify_insert_request_info(start_lsn, request_sectors, NULL ,VERI_NOERR);	

	return ret;
}


/*---------------------------------------------------------------------------------------------------------------
*
* Static Function
*
---------------------------------------------------------------------------------------------------------------*/

static void set_last_lsn_in_packed_list(uint lsn, uint request_sectors, int request_count)
{	
	if(_finish_cal_last_lsn_in_packed_req == TRUE)
		return ;

	if(_state->b_packed_enable == TRUE)
	{
		if(DO_PACKED_FLUSH(request_count, _state->packed_flush_cycle))
		{
			if(verify_get_status() == VERI_STATUS_NORMAL)
			{
				_packed_request_count = 0;
				_predicted_packed_req_sectors = 0;			
			}
		}

		_packed_request_count++;
		_predicted_packed_req_sectors += request_sectors;
		if(verify_get_status() > VERI_STATUS_NORMAL)
		{
			if(_packed_request_count >= _state->packed_flush_cycle)			
			{	
				_predicted_last_lsn_in_packed_req 	= lsn + request_sectors - NUM_OF_SECTORS_PER_PAGE;
				_finish_cal_last_lsn_in_packed_req 	= TRUE;
				_cache_or_packed_list = PACKED;
				VERIFY_DEBUG_MSG("_predicted_last_lsn_in_packed_req : 0x%08X \n", _predicted_last_lsn_in_packed_req);
			}
		}
	}
}

static void set_last_lsn_in_cached_list(uint lsn, uint request_sectors, int request_count)
{
	if(_finish_cal_last_lsn_in_cached_req == TRUE)
		return ;

	if(_state->b_cache_enable == TRUE)
	{
		if(DO_CACHE_FLUSH(request_count, _state->cache_flush_cycle))
		{
			if(verify_get_status() == VERI_STATUS_NORMAL)
			{
				_cached_request_count = 0;
				_predicted_cached_req_sectors = 0;			
			}
		}

		_cached_request_count++;
		_predicted_cached_req_sectors += request_sectors;
		if(verify_get_status() > VERI_STATUS_NORMAL)
		{
			if(_cached_request_count >= _state->cache_flush_cycle)			
			{	
				_predicted_last_lsn_in_cached_req 	= lsn + request_sectors - NUM_OF_SECTORS_PER_PAGE;
				_finish_cal_last_lsn_in_cached_req 	= TRUE;
				_cache_or_packed_list = CACHE;
				VERIFY_DEBUG_MSG("_predicted_last_lsn_in_cached_req : 0x%08X \n", _predicted_last_lsn_in_cached_req);
			}
		}
	}
}


static int memorycmp(const void * cs,const void * ct,size_t count)
{
	const unsigned char *su1, *su2;
	int res = 0;

	for( su1 = cs, su2 = ct; 0 < count; ++su1, ++su2, count--)
	{
		if ((res = *su1 - *su2) != 0)
			break;
	}
	
	return res;
}

static void make_expected_header(uchar *buf, uint req_seq_num, uint req_start, uint req_len, uint page_index, uint next_start_sector, bool is_commit_pg)
{	
	struct plr_header* p_header = (struct plr_header*)buf;
	if(is_commit_pg)
		p_header->magic_number = MAGIC_NUMBER_FLUSH;
	else
		p_header->magic_number = MAGIC_NUMBER;

	p_header->lsn					= req_start + (page_index * NUM_OF_SECTORS_PER_PAGE);
	p_header->boot_cnt 				= _expected_boot_count;
	p_header->loop_cnt				= _expected_loop_count;
	p_header->req_info.req_seq_num 	= req_seq_num;
	p_header->req_info.start_sector	= req_start;
	p_header->req_info.page_num 	= req_len / NUM_OF_SECTORS_PER_PAGE;
	p_header->req_info.page_index 	= page_index;
	p_header->next_start_sector 	= next_start_sector;
	p_header->reserved1 			= 0;
	p_header->reserved2 			= 0;
	p_header->crc_checksum = crc32(0, (const uchar *)(&(p_header->lsn)), sizeof(struct plr_header) - sizeof(uint)* 4);
}

static void make_expected_header_data(uchar * buf, uint req_start, uint req_len, uint req_seq_num, uint next_start_sector, bool is_commit)
{
	uint index = 0;
	
	for (index = 0; index < req_len / NUM_OF_SECTORS_PER_PAGE; index++)	{
		make_expected_header(buf + (index * PAGE_SIZE), req_seq_num, req_start, req_len, index, next_start_sector, is_commit);
	}
}

static void make_origin_chunk(uchar* buf, uint lsn, uint request_sectors, uint request_num, uint next_lsn)
{
	if(_state->b_cache_enable == TRUE)
	{
		if(DO_CACHE_FLUSH(request_num, _state->cache_flush_cycle))
		{
			make_expected_header_data(buf, lsn, request_sectors, request_num, next_lsn, TRUE);
			return ;
		}
	}
	
	make_expected_header_data(buf, lsn, request_sectors, request_num, next_lsn, FALSE);	
}

static uint classify_partial_type(uchar *buf, uint pages)
{
	uint	page_index 		= 0;
	uint 	partial_type 	= 0;
	uint 	changed_boot_cnt= 0;	

	if(buf[page_index] == 0)
		changed_boot_cnt = EXPECTED;
	else 
		changed_boot_cnt = UNEXPECTED;

	for( ; page_index < pages; page_index++)
	{
		if(changed_boot_cnt == EXPECTED)
		{
			if(buf[page_index] != 0)
			{
				changed_boot_cnt = UNEXPECTED;
				partial_type++;
				continue;
			}
		}
		else 
		{
			if(buf[page_index] == 0)
			{
				changed_boot_cnt = EXPECTED;
				partial_type++;
				continue;
			}
		}
	}

	if(partial_type == 0 && changed_boot_cnt == EXPECTED)
		partial_type = 0;	//all new
	else if(partial_type == 1 && changed_boot_cnt == UNEXPECTED)
		partial_type = 1;	//new old
	else if(partial_type >= 1 || (partial_type == 1 && changed_boot_cnt == EXPECTED))
		partial_type = 2;	//new old new || old new old || .... etc
	else if(partial_type == 0 && changed_boot_cnt == UNEXPECTED)
		partial_type = 3;	//all old
		
	return partial_type;
}

static int check_partial_writing(uchar *buf, uint start_lsn, uint request_sectors)
{	
	uint chunk_pages	= SECTORS_TO_PAGES(request_sectors);
	uint partial_error  = VERI_NOERR;
	uint partial_type 	= 0;
			
	partial_type = classify_partial_type(buf, chunk_pages);		

	switch(partial_type)
	{
		case 0:	//all new
		case 3: //all old
			break;
			
		case 1: //new  old
			_state->event1_cnt++;
			if(_state->test_minor <= 0)		
				partial_error = VERI_EPARTIAL;
			break;			
			
		case 2: //new old new || old new old 
			_state->event2_cnt++;
			if(_state->test_minor <= 1)		
				partial_error = VERI_EPARTIAL;
			break;	
	}

	return partial_error;
}

