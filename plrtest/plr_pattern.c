#include "plr_common.h"
#include "plr_protocol.h"
#include "plr_deviceio.h"
#include "plr_pattern.h"
#include "plr_verify.h"
#include "plr_verify_log.h"
#include "plr_case_info.h"

//#define PATTERN_DEBUG_MSG_SHOW
#ifdef PATTERN_DEBUG_MSG_SHOW
	#define PATTERN_DEBUG_MSG(fmt, args...) printf("\n[DEBUG][%d][%s] " fmt, __LINE__, __func__, ##args)
#else
	#define PATTERN_DEBUG_MSG(fmt, args...)
#endif


#define AGING_VERIFICATION_INTERVAL	5
#define IS_PATTERN_ODD(X) 			((X) & 0x01)
#define IS_PATTERN_EVEN(X)		 	!((X) & 0x01)
#define IS_AGING(X) 				(((X) == 'A') ? TRUE : FALSE)
#define IS_ERASE_TEST(X)			(((X) >= 9) ? (((X) <= 12) ? TRUE: FALSE) : FALSE)

static struct Pattern_Function _pattern_linker;
static struct plr_state *_state = NULL;
static uint _num_of_pattern = 0;

static char *OPTYPE_WRITE	= "Write";
static char *OPTYPE_VERIFY 	= "Verify";

static int _write(uchar* buf, uint lsn, uint request_sectors, uint request_num, uint next_lsn)
{
	if(_state->b_cache_enable == TRUE)
	{
		if(DO_CACHE_FLUSH(request_num ,_state->cache_flush_cycle))
		{
			return write_flush_page(buf, lsn, request_sectors, request_num, next_lsn);
		}
	}

	return write_request(buf, lsn, request_sectors, request_num, next_lsn);
}

static void send_token_writing(uint lsn, uint request_sectors, uint loop, uint boot)
{
	char buf[64] = {0,};
	sprintf(buf, "%d/%d/%d/%d/", lsn, request_sectors, loop, boot);
	send_token(PLRTOKEN_SEND_WRITE_INFO, buf);
}

static void send_token_writing_seed(uint seed)
{
	char buf[64] = {0,};
	sprintf(buf, "%d/", seed);
	send_token(PLRTOKEN_SEND_RANDOM_SEED, buf);
}

static void print_start_pattern(char *op_type, uint current_loop)
{
	if(IS_AGING(_state->test_type2))
	{
		if(_num_of_pattern == 1)
		{
			plr_info("%s, Test Area(0x%08X ~ 0x%08X)\n",
					op_type,
					_state->test_start_sector,
					_state->test_start_sector + _state->test_sector_length);
		}
		else
		{
			plr_info("%s PATTERN %d, Test Area(0x%08X ~ 0x%08X)\n",
					op_type,
					IS_PATTERN_ODD(current_loop) ? 1:2,
					_state->test_start_sector,
					_state->test_start_sector + _state->test_sector_length);
		}
	}
	else if(IS_ERASE_TEST(_state->test_num))
	{
		if(current_loop % 2 == 1)
		{
			plr_info("%s, Start LSN : 0x%08X\n",
					op_type,
					_state->write_info.lsn);
		}
		else
		{
			char *op = op_type;
			if(op_type != OPTYPE_VERIFY)
			{
				switch(_state->test_num)
				{
					case 9:
						op = "Erase";
						break;
					case 10:
						op = "Trim";
						break;
					case 11:
						op = "Discard";
						break;
					case 12:
						op = "Sanitize";
						break;
				}
			}

			plr_info("%s, Start LSN : 0x%08X\n",
					op,
					_state->write_info.lsn);
		}
	}
	else
	{
		if(_num_of_pattern == 1)
		{
			plr_info("%s, Start LSN : 0x%08X\n",
						op_type,
						_state->write_info.lsn);
		}
		else
		{
			plr_info("%s PATTERN %d, Start LSN : 0x%08X\n",
						op_type,
						IS_PATTERN_ODD(current_loop) ? 1:2,
						_state->write_info.lsn);
		}
	}
}

void init_pattern(struct plr_state* state, bool is_erase_test)
{
	verify_init(state, is_erase_test);
	memset(&_pattern_linker, 0, sizeof(struct Pattern_Function));
	state->write_info.loop_count = state->loop_cnt;
	state->write_info.boot_count = state->boot_cnt;
	_state 		= state;
}

void regist_pattern(struct Pattern_Function pattern_link)
{
	_pattern_linker.do_pattern_1 	= pattern_link.do_pattern_1;
	_pattern_linker.do_pattern_2 	= pattern_link.do_pattern_2;
	_pattern_linker.init_pattern_1	= pattern_link.init_pattern_1;
	_pattern_linker.init_pattern_2 	= pattern_link.init_pattern_2;
	_pattern_linker.do_extra_verification = pattern_link.do_extra_verification;
}

int check_pattern_func(void)
{
	if(_pattern_linker.do_pattern_1 == NULL && _pattern_linker.do_pattern_2 == NULL)
	{
		_num_of_pattern = 0;
		return -1;
	}

	if(_pattern_linker.do_pattern_1 != NULL && _pattern_linker.do_pattern_2 == NULL)
	{
		_pattern_linker.do_pattern_2 	= _pattern_linker.do_pattern_1;
		if(_pattern_linker.init_pattern_1 == NULL)
			return -1;

		_pattern_linker.init_pattern_2 	= _pattern_linker.init_pattern_1;
		_num_of_pattern = 1;
		return 0;
	}

	if(_pattern_linker.do_pattern_1 == NULL && _pattern_linker.do_pattern_2 != NULL)
	{
		_pattern_linker.do_pattern_1 	= _pattern_linker.do_pattern_2;
		if(_pattern_linker.init_pattern_2 == NULL)
			return -1;

		_pattern_linker.init_pattern_1 	= _pattern_linker.init_pattern_2;
		_num_of_pattern = 1;
		return 0;
	}

	_num_of_pattern = 2;
	return 0;
}

int verify_pattern(uchar* buf)
{
	int ret = 0;
	process_pattern do_pattern	= NULL;

	if(_state == NULL)
	{
		PATTERN_DEBUG_MSG("\nTest Information Link Error \n");
		return -1;
	}

	send_token(PLRTOKEN_RECV_WRITE_INFO, NULL);
	send_token(PLRTOKEN_RECV_RANDOM_SEED, NULL);

	verify_reset(_state->write_info.loop_count, _state->write_info.boot_count);
	well512_write_seed(_state->write_info.random_seed);
	verify_init_log();

	PATTERN_DEBUG_MSG("recv loop count : %d, recv boot count : %d \n", _state->write_info.loop_count, _state->write_info.boot_count);

	if (IS_AGING(_state->test_type2) && _state->write_info.loop_count % AGING_VERIFICATION_INTERVAL != 0)
	{
		PATTERN_DEBUG_MSG("Do not have to verify the test area. \n");
		return VERI_NOERR;
	}

	verify_set_current_step(VERI_DEFAULT_STEP);
	while(TRUE)
	{
		print_start_pattern(OPTYPE_VERIFY, _state->write_info.loop_count);

		if(IS_PATTERN_ODD(_state->write_info.loop_count))
			do_pattern = _pattern_linker.do_pattern_1;
		else
			do_pattern = _pattern_linker.do_pattern_2;

		ret = do_pattern(buf, _state->write_info.lsn, _state->write_info.request_sectors, do_verify);
		if(ret != VERI_NOERR)
			break;

		_state->write_info.loop_count++;
		verify_set_loop_count(_state->write_info.loop_count);
		if(IS_PATTERN_ODD(_state->write_info.loop_count))
			_pattern_linker.init_pattern_1(&_state->write_info);
		else
			_pattern_linker.init_pattern_2(&_state->write_info);

		plr_info("\n");

		if(IS_AGING(_state->test_type2))
			break;
	}

	do{
		if(verify_get_error_num(_state) != 0)
			break;

		if(IS_AGING(_state->test_type2) == TRUE)
			break;

		verify_apply_result(_state);
		verify_set_current_step(VERI_CHECKING_LSN_CRC_STEP);
		ret = verify_check_random_zone_only_lsn_crc(buf);
		if(ret == VERI_EIO)
			break;

		if(_pattern_linker.do_extra_verification != NULL)
		{
			verify_set_current_step(VERI_EXTRA_STEP);
			ret = _pattern_linker.do_extra_verification(buf, _state);
			if(ret == VERI_EIO)
				break;
		}
	}while(FALSE);

	ret = verify_get_result(_state);
	if(ret != 0 && ret != VERI_EIO)
	{
		PATTERN_DEBUG_MSG("SAVE CRASH INFO \n");

		verify_save_crash_info();
		verify_save_log();
	}

	return ret;
}

int write_pattern(uchar* buf)
{
	int ret = 0;
	process_pattern do_pattern	= NULL;
	int writing_loop_count = _state->loop_cnt;

	if(_state == NULL)
		_state = _state;

	if(_state->boot_cnt == 1 && _state->loop_cnt == 1)
		_pattern_linker.init_pattern_1(&_state->write_info);

	send_token_writing(	_state->write_info.lsn,
						_state->write_info.request_sectors,
						_state->loop_cnt,
						_state->boot_cnt);



	_state->write_info.random_seed = get_cpu_timer(0);
	well512_write_seed(_state->write_info.random_seed);
	send_token_writing_seed(_state->write_info.random_seed);

	print_start_pattern(OPTYPE_WRITE, writing_loop_count);

	if(IS_PATTERN_ODD(writing_loop_count))
		do_pattern = _pattern_linker.do_pattern_1;
	else
		do_pattern = _pattern_linker.do_pattern_2;

	ret = do_pattern(buf, _state->write_info.lsn, _state->write_info.request_sectors, _write);
	if(ret != 0)
		return ret;

	writing_loop_count++;

	/*
	* HACK ******************************************************************************************************
	* make_header 함수는 state->loop_cnt 값을 이용해서 header를 생성한다.
	* 그러므로 loop count의 증가를 state->loop_cnt에 적용해줘야 한다.
	* state는 g_plr_state를 가리키는 포인터이다. g_plr_state는 테스트에 관한 정보를 갖고 있는 전역 구조체 변수다.
	* ***********************************************************************************************************
	*/
	set_loop_count(writing_loop_count);
	if(IS_PATTERN_ODD(writing_loop_count))
		_pattern_linker.init_pattern_1(&_state->write_info);
	else
		_pattern_linker.init_pattern_2(&_state->write_info);

	/*
	* TEMP ******************************************************************************************************
	* 2015.05.29
	* Temptation.
	* Write -> (Flush) -> Verify -> Write
	* ***********************************************************************************************************
	*/
	packed_flush();
	if (ret)
		return ret;

	if (_state->b_cache_enable) {
		ret = dd_cache_flush();
		if (ret)
			return ret;
	}

	_state->poweroff_pos 	= _state->write_info.lsn;
	_state->last_flush_pos 	= _state->write_info.lsn;

	plr_info("\n");

	return ret;
}
