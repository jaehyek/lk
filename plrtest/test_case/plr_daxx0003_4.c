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
 // by joys,2015.07.17

#include "../plr_deviceio.h"
#include "../plr_common.h"
#include "../plr_err_handling.h"
#include "../plr_protocol.h"
#include "../plr_hooking.h"
#include "../plr_case_info.h"
#include "../plr_pattern.h"
#include "../plr_verify.h"
#include "../plr_io_statistics.h"
#include "../plr_verify_log.h"
#include "test_case/plr_case.h"

#define MIN_REQUST_SECTORS	8UL			// 4KB
#define MAX_REQUST_SECTORS	16384UL		// 8MB
#define RANDOM_ZONE_COUNT	4
#define MAX_TEST_CYCLE		1000UL

typedef enum {
	PERF_READ = 0,
	PERF_WRITE,
} perf_io_e;

typedef enum {
	PRE_CLEAN = 0,
	PRE_DIRTY,
} pre_condition_e;

typedef struct {
	u32 selected_zone_cnt;
	u32 start_addr[RANDOM_ZONE_COUNT];
} perf_random_zone_s;

typedef struct {
	u64 sq_write_ticks;
	u64 sq_read_ticks;
	u64 ran_write_ticks;
	u64 ran_read_ticks;
} perf_ticks_s;

typedef struct {
	u32 *req_table;			// request start address table
	u32 req_size;
	u32 req_count;			// number of sector
	u32 start_lsn;			// sector unit
	u32 test_area_length;	// sector unit
	u8	b_random_test;
	u8	b_using_random_zone;
	u8 *data_buff;
	perf_random_zone_s perf_rand_zone;
	perf_ticks_s perf_ticks;
} perf_static_test_s;

static unsigned long long _writing_page_per_loop;
static bool _writing_sequence = TRUE;
static u32 _perf_cycle;
//static u32 _start_time;

static u64 _total_writing_time = 0;

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

static int select_random_zone(perf_random_zone_s *ran_zone_info, u32 total_zone)
{
	int i = 0, j = 0;
	int zone_num = 0;
	u32 random_zone_cnt = ran_zone_info->selected_zone_cnt;
	u32 *arr_zone = NULL;

	arr_zone = (u32*)malloc(sizeof(u32) * random_zone_cnt);

	if (arr_zone == NULL) {
		printf ("[%s] arr_zone malloc failed\n", __func__);
		return -1;
	}

	memset(arr_zone, 0xff, sizeof(u32) * random_zone_cnt);

	for (i = 0; i < random_zone_cnt; i++)
	{
		zone_num = well512_rand() % total_zone;

		arr_zone[i] = zone_num;

		for (j = 0; j < i; j++) {
			if (arr_zone[j] == zone_num) {
				printf ("[%s][%d], arr_zone_num : %u\n", __func__, __LINE__, arr_zone[j]);
				i--;
				break;
			}
		}
	}

	for (i = 0; i < random_zone_cnt; i++) {

		ran_zone_info->selected_zone_cnt = random_zone_cnt;
		ran_zone_info->start_addr[i] = arr_zone[i] * get_sectors_in_zone();
	}

	for (i = 0; i < random_zone_cnt; i++) {
		printf ("arr[%d] zone : %d\n", i, arr_zone[i]);
		printf ("start addr[%d] : %d\n", i, ran_zone_info->start_addr[i]);
	}

	free(arr_zone);

	return 0;
}

static int make_request_table(perf_static_test_s *stat_test, int req_size, int file_size, int brandom)
{
	u32 big_rand = 0;
	u32 i = 0, temp = 0, numreqs = 0; // number of records
	u32 total_zone_cnt = 0;
	u32 *req_table = NULL;

	numreqs = file_size / req_size;

	// 4KB request 기준으로 1G를 Write 하면 req_table은 1MB의 memory가 필요하게 된다.
	stat_test->req_count = numreqs;

	if (stat_test->req_table == NULL)
		stat_test->req_table = (u32*)malloc(sizeof(*stat_test->req_table)*numreqs);

	if (stat_test->req_table == NULL) {
		printf ("[%s] req_table malloc failed\n", __func__);
		return -1;
	}

	req_table = stat_test->req_table;

	/* pre-compute random sequence based on
		Fischer-Yates (Knuth) card shuffle */
	for (i = 0; i < numreqs; i++) {
		req_table[i] = i;
	}

	if (!brandom) {
		stat_test->b_random_test = FALSE;
		return 0;
	}

	for (i = 0; i < numreqs; i++)
	{
		big_rand = well512_write_rand();
		big_rand = big_rand % numreqs;
		temp = req_table[i];
		req_table[i] = req_table[big_rand];
		req_table[big_rand] = temp;
	}

	if (stat_test->b_using_random_zone == FALSE)
		return 0;

	/* for random zone write */
	total_zone_cnt = file_size / get_sectors_in_zone();

	stat_test->perf_rand_zone.selected_zone_cnt = total_zone_cnt;

	if (select_random_zone(&stat_test->perf_rand_zone, total_zone_cnt))
		return -1;

	stat_test->b_random_test = TRUE;

	return 0;
}

static int release_random_table(perf_static_test_s *stat_test)
{
	if (stat_test->req_table)
		free(stat_test->req_table);

	memset(&stat_test, 0, sizeof(perf_static_test_s));

	return 0;
}

static u32 get_start_sector(perf_static_test_s *stat_test, int index)
{
	u32 lsn = 0, offset = 0;
	u32 zone_num = 0, zone_seperate = 0;
	u32 req_cnt = stat_test->req_count;
	u32 req_size = stat_test->req_size;
	u32 start_lsn = stat_test->start_lsn;
	u32 *req_table = stat_test->req_table;
	perf_random_zone_s *perf_rand_zone = &stat_test->perf_rand_zone;


	if (stat_test->b_random_test == TRUE && stat_test->b_using_random_zone == TRUE) {
		zone_seperate = req_cnt / perf_rand_zone->selected_zone_cnt;

		zone_num = req_table[index] / zone_seperate;
		offset = req_table[index] % (zone_seperate);

		lsn = start_lsn + perf_rand_zone->start_addr[zone_num] + (offset * req_size);
		printf ("zone_num : %u, offset : %u, lsn : %u\n", zone_num, offset, lsn);
	}
	else {
		lsn = start_lsn + (req_table[index] * req_size);
	}

	return lsn;
}

static ulong do_perf_test_io(perf_static_test_s *stat_test, perf_io_e RW, u64 *elapsed_ticks)
{
	int i = 0, ret = 0;
	u32 lsn = 0;
	u32 req_size = stat_test->req_size;
	u8 *buff = stat_test->data_buff;

	u64 start_ticks = 0, end_ticks = 0;

	//reset_tick_count64();				//temp code
	start_ticks = get_tick_count64();

	for (i = 0; i < stat_test->req_count; i++)
	{
		//lsn = start_lsn + (req_table[i] * req_size);
		lsn = get_start_sector(stat_test, i);

		if (PERF_READ == RW) {
			ret = dd_read(buff, lsn, req_size);
		}
		else {
			ret = write_request(buff, lsn, req_size, i + 1, 0);

			if(g_plr_state.b_cache_enable &&
				DO_CACHE_FLUSH(i + 1, g_plr_state.cache_flush_cycle)) {
				ret = dd_cache_flush();
				if (ret)
					goto out;
			}
		}

		if (ret)
			goto out;
	}

	if (PERF_WRITE == RW && g_plr_state.b_cache_enable)
	{
		ret = dd_cache_flush();

		if (ret)
			goto out;
	}
	end_ticks = get_tick_count64();

	if (start_ticks >= end_ticks)
		*elapsed_ticks = 1;
	else
		*elapsed_ticks = end_ticks - start_ticks;

out:
	return ret;
}

static int perf_test_request(perf_static_test_s *stat_test)
{
	int ret = 0;
	u32 request_len = stat_test->req_size;
	u32 test_area_len = stat_test->test_area_length;

	perf_ticks_s *perf_ticks = &stat_test->perf_ticks;

	/* Sequential test ---------------------------------------------------------------------------*/
	ret = make_request_table(stat_test, request_len, test_area_len, FALSE);
	if (ret)
		goto out;

	// write test
	ret = do_perf_test_io(stat_test, PERF_WRITE, &perf_ticks->sq_write_ticks);
	if (ret)
		goto out;

	// read test
	ret = do_perf_test_io(stat_test, PERF_READ, &perf_ticks->sq_read_ticks);
	if (ret)
		goto out;
	/*--------------------------------------------------------------------------------------------*/

	/* Random test -------------------------------------------------------------------------------*/
	ret = make_request_table(stat_test, request_len, test_area_len, TRUE);
	if (ret)
		goto out;

	// write test
	ret = do_perf_test_io(stat_test, PERF_WRITE, &perf_ticks->ran_write_ticks);
	if (ret)
		goto out;

	// read test
	ret = do_perf_test_io(stat_test, PERF_READ, &perf_ticks->ran_read_ticks);
	if (ret)
		goto out;
	/* --------------------------------------------------------------------------------------------*/

out:
	return ret;
}

static u32 caculate_kb_per_sec(u32 total_size, u64 elapsed_ticks)
{
	u32 msec = 0;
	u32 byte_per_ms = 0;

	msec = get_tick2msec(elapsed_ticks);

	if (msec < 1)
		msec = 1;

	byte_per_ms = (u64)(total_size * 512) / msec;

	return (u32)(byte_per_ms * 1000UL / 1024UL);
}

static int do_perf_test(uchar *buf, uint start_lsn, u32 test_area_len)
{
	int ret = 0;
	u32 request_len		= MIN_REQUST_SECTORS;
	u32 max_request_len = MAX_REQUST_SECTORS;
	perf_static_test_s stat_test;
	perf_ticks_s  *perf_ticks = NULL;

	memset(&stat_test, 0, sizeof(perf_static_test_s));

	perf_ticks = &stat_test.perf_ticks;

	stat_test.start_lsn = start_lsn;
	stat_test.req_size = request_len;
	stat_test.test_area_length = test_area_len;
	stat_test.data_buff = buf;
	// TBD .....
	stat_test.b_using_random_zone = FALSE;

	plr_info_highlight ("  -----------------------------------------------------------------------------------------------------------\n");
	plr_info_highlight (" |             |             |            Sequential (KB/S)          |             Random (KB/S)             |\n");
	plr_info_highlight (" |    Total    |   Request   |---------------------------------------|---------------------------------------|\n");
	plr_info_highlight (" |  size (KB)  |  size (KB)  |       Write       |       Read        |       Write       |       Read        |\n");
	plr_info_highlight (" |-------------|-------------|-------------------|-------------------|-------------------|-------------------|\n");

	do {

		ret = perf_test_request(&stat_test);

		if (ret)
			goto out;

		plr_info_highlight (" |     %7u |     %7u |           %7u |           %7u |           %7u |           %7u |\n",
				test_area_len / 2,
				request_len / 2,
				caculate_kb_per_sec(test_area_len, perf_ticks->sq_write_ticks),
				caculate_kb_per_sec(test_area_len, perf_ticks->sq_read_ticks),
				caculate_kb_per_sec(test_area_len, perf_ticks->ran_write_ticks),
				caculate_kb_per_sec(test_area_len, perf_ticks->ran_read_ticks)
		);

		// increase reqeust_size
		request_len <<= 1;
		stat_test.req_size = request_len;

	} while(request_len <= max_request_len);

	plr_info_highlight ("  -----------------------------------------------------------------------------------------------------------\n");

out:
	release_random_table(&stat_test);
	return ret;

}

static int dirty_pattern(uchar *buf, uint start_lsn, uint request_sectors, op_pattern operate)
{
	int ret = 0;
	uint request_count 	= 0;
	uint lsn 			= 0;
	uint request_length	= 0;
	uint total_write 	= 0;
	uint test_area_start_lsn= get_first_sector_in_test_area();
	uint test_area_length 	= get_last_sector_in_test_area() - test_area_start_lsn;
	uint written_page_per_loop = 0;

	if(_writing_sequence == TRUE)	//Writing Sequence
	{
		plr_info("\n --------------------------------------------------\n"
				 " Make a dirty condition\n"
				 " Area size	: %u KB\n"
				 " --------------------------------------------------\n", test_area_length / 2);
		while (written_page_per_loop < test_area_length / NUM_OF_SECTORS_PER_PAGE / 2) {
			lsn = (((well512_write_rand() % (test_area_length - 2048))>>3)<<3) + test_area_start_lsn;
			request_length = get_request_length();

			total_write += request_length;

			if (request_count % 200 == 0)
					printf("\rRequest Num : %u", request_count);

			ret = write_request(buf, lsn, request_length, request_count, 0);
			if (ret != 0)
				return ret;

			written_page_per_loop 	+= (unsigned long long)(request_length / NUM_OF_SECTORS_PER_PAGE);

			request_count++;
		}
	}

	return 0;

}

static int start_perf_test(uchar *buf, uint start_lsn, pre_condition_e condition)
{
	int ret = 0;
 	u32 test_area_len = 0;

	if (g_plr_state.test_num == 3)
		test_area_len = 32768;			// sectors : 16MB
	else
		test_area_len = 2097152;		// sectors : 1GB

	statistic_clear();

	plr_info("\n ----------------------------------------------------\n"
			 " * Start a performance test for static request size *\n"
			 "   Sequential Read/Write, Random Read/Write test\n"
			 "   Elapsed Time	: %llu days %llu hours %llu minutes %llu seconds \n",
			 _total_writing_time / SECONDS_PER_DAY, ( _total_writing_time % SECONDS_PER_DAY ) / SECONDS_PER_HOUR,
							(_total_writing_time % SECONDS_PER_HOUR) / SECONDS_PER_MINUTE, _total_writing_time % SECONDS_PER_MINUTE);

	plr_info("   Condition  	: %s\n"
			 "   Cache mode 	: %s, Flush cycle : %d\n"
			 "   Packed mode 	: %s, Packed cycle : %d\n"
			 "   Request size 	: 4KB ~ 16MB\n"
			 "   Area size		: %u KB\n"
			 "   Test Cycle		: %u / %u\n"
			 " ----------------------------------------------------\n",
			 (PRE_CLEAN == condition) ? "Clean" : "Dirty",
			 g_plr_state.b_cache_enable ? "On" : "Off",
			 g_plr_state.cache_flush_cycle,
			 g_plr_state.b_packed_enable ? "On" : "Off",
			 g_plr_state.packed_flush_cycle,
			 (test_area_len / 2),
			 _perf_cycle,
			 MAX_TEST_CYCLE
			 );

	ret = do_perf_test(buf, start_lsn, test_area_len);

	if (ret)
		goto out;

	statistic_result();

out:
	return ret;
}

static int pattern1(uchar *buf, uint start_lsn, uint request_sectors, op_pattern operate)
{
	int ret = 0;
	u32 start_time = 0, end_time = 0;
	static pre_condition_e pre_con = PRE_CLEAN;

	if (_writing_sequence == FALSE)
		goto out;

	// Start a performance test
	start_time = get_rtc_time();
	ret = start_perf_test(buf, start_lsn, pre_con);
	end_time = get_rtc_time();
	_total_writing_time += (u64)(end_time - start_time);

	if (ret)
		goto out;

	pre_con = PRE_DIRTY;
out:
	return ret;
}

static int pattern2(uchar *buf, uint start_lsn, uint request_sectors, op_pattern operate)
{
	int ret = 0;
	u32 start_time = 0, end_time = 0;

	if (_writing_sequence == FALSE)
		goto out;

	// Make a dirty condition
	start_time = (u32)get_rtc_time();
	ret = dirty_pattern(buf, start_lsn, request_sectors, operate);
	end_time = (u32)get_rtc_time();
	_total_writing_time += (u64)(end_time - start_time);

	if (ret)
		goto out;

	_perf_cycle++;

	if (_perf_cycle >= MAX_TEST_CYCLE) {
		g_plr_state.b_finish_condition = TRUE;
		statistic_enable(FALSE);
	}

out:
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
int initialize_daxx_0003_4( uchar * buf, uint test_start_sector, uint test_sector_length )
{
	int ret = 0;
	struct Pattern_Function pattern_linker;

	init_case_info(test_start_sector, test_sector_length);
	set_test_sectors_in_zone(get_sectors_in_zone());

	ret = statistic_enable(TRUE);
	if (ret)
		return ret;

	g_plr_state.boot_cnt = 1;

	_perf_cycle = 0;

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
	pattern_linker.do_pattern_2 = pattern2;
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

int read_daxx_0003_4( uchar * buf, uint test_start_sector, uint test_sector_length )
{
	return verify_pattern(buf);
}

int write_daxx_0003_4( uchar * buf, uint test_start_sector, uint test_sector_length )
{
	int ret = 0;

	_writing_page_per_loop = 0;
	_writing_sequence = TRUE;

	ret = write_pattern(buf);
	if(ret != 0) {
		statistic_enable(FALSE);
		return ret;
	}

	_writing_sequence = FALSE;

	return ret;
}
