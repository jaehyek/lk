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

// @sj 150706: Note, This Test Case is buildup upon TestCase DPIN_0004.


/* 	DAXX_0008


	W	= write (default)
	J	= jump
	R	= random

	PATTERN_1, zone interleaving.
		zone 1, 	4K(W)	4K(W)	4K(W)	4K(W)	4K(W)	4K(W)	...	4K(W)
		zone 2, 	4K(W)	4K(W)	4K(W)	4K(W)	4K(W)	4K(W)	...	4K(W)
			.
			.
			.
		zone N-1, 	4K(W)	4K(W)	4K(W)	4K(W)	4K(W)	4K(W)	...	4K(W)


	PATTERN_2, sequencial
		zone 1,	4K(W) 	4K(W)	4K(W)	4K(W)	4K(W)	4K(W)	....	4K(W)
		zone 2, 	4K(W) 	4K(W)	4K(W)	4K(W)	4K(W)	4K(W)	....	4K(W)
			.
			.
			.
		zone N-1,	4K(W) 	4K(W)	4K(W)	4K(W)	4K(W)	4K(W)	....	4K(W)

 */

#include "../plr_deviceio.h"
#include "../plr_common.h"
#include "../plr_err_handling.h"
#include "../plr_protocol.h"
#include "../plr_case_info.h"
#include "../plr_pattern.h"
#include "../plr_verify.h"
#include "../plr_verify_log.h"
#include "../plr_hooking.h"

#define COMPARE_64BIT	(TRUE)

#if 1	/* COMPARE_64BIT */

#define CHUNK_SIZE	NUM_OF_SECTORS_PER_PAGE


//@sj 150716, I recommend you to introduce following macros to common.h

#ifndef BYTES_PER_BUFFER

#define BUFFER_SIZE				(0x800000)			//@sj 150714, Note: No buffer size is explicitely defined anywhere.
													//If anyone defined the buffer size in global configuration header,
													//this value may changed to the value.

#define BYTES_PER_BUFFER		(BUFFER_SIZE)							// clearer and chainable !
#define WORDS_PER_BUFFER		(BYTES_PER_BUFFER/sizeof(u64))			//
#define BYTES_PER_TEST_AREA		(BUFFER_SIZE)							// Test area is 8 MB
#define BYTES_PER_SECTOR		(SECTOR_SIZE)							// 0x200 	(512)
#define WORDS_PER_SECTOR		(BYTES_PER_SECTOR/sizeof(u64))			// 0x80 	(128)
#define SECTORS_PER_BUFFER		(BYTES_PER_BUFFER / BYTES_PER_SECTOR)	// 0x4000	(16384)
#define SECTORS_PER_PAGE		(NUM_OF_SECTORS_PER_PAGE)				// 0x8		(8)
#define BYTES_PER_PAGE			(BYTES_PER_SECTOR * SECTORS_PER_PAGE)	// 0x1000 	(4096)
#define WORDS_PER_PAGE			(BYTES_PER_PAGE/sizeof(u64))			// 0x400	(1024)
#define PAGES_PER_ZONE			(ZONE_SIZE_X_PAGE)						// 0x4000 	(16384)
#define SECTORS_PER_ZONE		(SECTORS_PER_PAGE * PAGES_PER_ZONE)		// 0x20000 	(131072)
#define BUFFERS_PER_ZONE		(SECTORS_PER_ZONE / SECTORS_PER_BUFFER)
#define PAGES_PER_TEST_AREA		(BYTES_PER_TEST_AREA/BYTES_PER_PAGE)
#define SECTORS_PER_TEST_AREA	(BYTES_PER_TEST_AREA/BYTES_PER_SECTOR)
#define BUFFERS_PER_TEST_AREA	(BYTES_PER_TEST_AREA/BYTES_PER_BUFFER)
#define PAGES_PER_BUFFER		(BYTES_PER_BUFFER / BYTES_PER_PAGE)




#endif



//
// MACROS for read disturbance
//

//#define SJD_DEVELOPMENT_MODE
//#define OLD_METHOD						// joys,2015.10.16

#ifdef SJD_DEVELOPMENT_MODE

#define RD_PAGES 							(PAGES_PER_TEST_AREA)
#define RD_REPEAT_COUNT					(3)
#define SJD_CAUSE_INTENDED_CRASH

#else

#define RD_PAGES 							(PAGES_PER_TEST_AREA)
#define RD_REPEAT_COUNT					(1000)

#endif

#define	COMPARE_FIRST_4BYTE_ONLY			(FALSE)	// for fast comparison
#define COMPARE_CRC_CHECKSUM				(FALSE)	// joys,2015.10.16

#define RD_ERR_READ_VERIFY_FAILED			(-1)
#define RD_ERR_WRITE_FAILED				(-2)
#define RD_ERR_INVALID_ZONE_NUMBER		(-3)

#define RD_MAGIC_NUMBER 					(0xEFCAFE01EFCAFE01ULL)

#ifdef 	SJD_CAUSE_INTENDED_CRASH

#define RD_MAGIC_NUMBER_INTENSIVE_CRASH		(0xDEAD1234)
#define INTENDED_CRASH_PAGE					(2)
#define INTENDED_CRASH_WORD					(0)	// crash word in the page, must b2 0 for COMPARE_FIRST_4BYTE_ONLY


#endif

static int rd_loop_count = 0;

#ifndef OLD_METHOD
static uint wbuffer_checksum = 0;
#endif


//
// Helper FUNCTIONS for read disturbance
//


static int compare_buffer(const u64* buf_ref, u64* buf, int word_len, int* word_pos)
{
	u64* 	p = buf;
	u64* 	pe = p + word_len;										// pe : sentinel for more speed

#if COMPARE_FIRST_4BYTE_ONLY == TRUE
	while(p != pe) {
		if(*p != *buf_ref) {
			*word_pos = ((size_t)p - (size_t)buf) / sizeof (u64);	// postion in buffer in words
			plr_info("word_pos = %d, data = 0x%08x, p = 0x%08X, buf = 0x%08X\n", *word_pos, *p, p, buf);
			return RD_ERR_READ_VERIFY_FAILED;
		}
		p += WORDS_PER_SECTOR;
	}
#elif COMPARE_CRC_CHECKSUM == TRUE
	uint crc_checksum = 0;
	crc_checksum = crc32(0, (const uchar *)buf, BYTES_PER_BUFFER);

	if (0 == wbuffer_checksum)
		wbuffer_checksum = 0x6b9c3483;

	if (crc_checksum != wbuffer_checksum) {
		*word_pos = 0;
		plr_info("crc checksum error! write crc : 0x%8X, read crc : 0x%8X\n", wbuffer_checksum, crc_checksum);

		while(p != pe) {
			if(*p++ != *buf_ref) {
				*word_pos = ((size_t)--p - (size_t)buf) / sizeof (u64);	// postion in buffer in words
				plr_info("word_pos = %d, data = 0x%08x, p = 0x%08X, buf = 0x%08X\n", *word_pos, *p, p, buf);
				return RD_ERR_READ_VERIFY_FAILED;
			}
		}
	}

#else
	while(p != pe) {
		if(*p++ != *buf_ref) {
			*word_pos = ((size_t)--p - (size_t)buf) / sizeof (u64);	// postion in buffer in words
			plr_info("word_pos = %d, data = 0x%08x, p = 0x%08X, buf = 0x%08X\n", *word_pos, *p, p, buf);
			return RD_ERR_READ_VERIFY_FAILED;
		}
	}
#endif
	return 0;	// Ok

}
static void memory_dump(u64* p0, int words)
{
	u64* p = (u64*)p0;
	u64* pe = p + words;

	int i = 0, j = 0;


	plr_info("Memory dump of a buffer: 0x%p...\n", p0);


	plr_info("\n    : ");

	for(;i < 8;i++) {

		plr_info("  %8x ", i);
	}

	plr_info("\n");


	i = 1; j = 0;

	plr_info("\n %03d: ", j);

	while(1) {

		u64 a = *p++;

		if (RD_MAGIC_NUMBER ==  a)
			plr_info("0x%16llx ", a);
		else
			plr_err("0x%16llx ", a);

		if(p == pe) break;

		if(!((i++ ) % 8)) {
			j++;
			plr_info("\n %03d: ", j);
		}
	}
	plr_info("\n");
}

static u64* prepare_ref_buffer(void)
{
	u64 *xbuf 	= (u64*)plr_get_extra_buffer();
	u64 *xbufe 	= (u64*)xbuf + WORDS_PER_BUFFER;
	u64 *p		= xbuf;

	while ( p != xbufe ) {
		*p++ = RD_MAGIC_NUMBER;
	}
#ifndef OLD_METHOD
	wbuffer_checksum = crc32(0, (const uchar *)xbuf, BYTES_PER_BUFFER);
#endif


	return xbuf;
}

#ifdef SJD_CAUSE_INTENDED_CRASH
static u32* prepare_crash_buffer(void)
{
	int i, j;
	u32 *xbuf 	= (u32*)plr_get_extra_buffer();
	u32 *p		= xbuf;

	for (i = 0; i < PAGES_PER_TEST_AREA; i++) {
		for (j = 0; j < WORDS_PER_PAGE; j++) {
			if( j == INTENDED_CRASH_WORD) {
				*(p + i * WORDS_PER_PAGE + j) = RD_MAGIC_NUMBER_INTENSIVE_CRASH;
			}
			else {
				*(p + i * WORDS_PER_PAGE + j) = RD_MAGIC_NUMBER;
			}
		}
	}

	return xbuf;
}
#endif

#ifdef OLD_METHOD
static int read_disturb_verify(u64* ref_buf, int *sec_no_in_zone_p )
{
	int i;
	int ret = 0;
	int word_pos_in_buf;
	int secno_in_buf;
	int byte_no_in_sec;

	u64 *xbuf 	= ref_buf;
	u64 *rbuf 	= (u64*)plr_get_read_buffer();

	for (i = 0; i < BUFFERS_PER_TEST_AREA; i++) {

		dd_read((uchar*)rbuf, (SECTORS_PER_BUFFER * i), SECTORS_PER_BUFFER);

		if (compare_buffer(xbuf, rbuf, WORDS_PER_BUFFER, &word_pos_in_buf)) {


			secno_in_buf 		= word_pos_in_buf / WORDS_PER_SECTOR;	// sec_no in the buffer
			byte_no_in_sec 		= word_pos_in_buf % WORDS_PER_SECTOR;
			plr_info("word_pos_in_buf:%d, secno_in_buf:  %d, byte_no_in_sec: %d\n", word_pos_in_buf, secno_in_buf, byte_no_in_sec);
			*sec_no_in_zone_p	= (i * SECTORS_PER_BUFFER) + secno_in_buf;

			plr_info("i: %d, secno_in_buf: %d\n", i, secno_in_buf);

			plr_info("Invalid data in zone 0 at sec_no_in_zone: %u, byte_no_in_sec: %u\n", *sec_no_in_zone_p,  byte_no_in_sec);
			plr_info("Crached data: 0x%08X\n", *(xbuf + word_pos_in_buf));
			plr_info("Memory dump just after crash position:\n");

			memory_dump(rbuf + word_pos_in_buf, 256);

			ret = RD_ERR_READ_VERIFY_FAILED;
			break;
		}
	}

	return ret;
}

#else

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

static int read_disturb_verify(u64* ref_buf, int *sec_no_in_zone_p, int b_verify)
{
	int i = 0, j = 0;
	int ret = 0;
	int word_pos_in_buf;
	int secno_in_buf;
	int word_no_in_sec;

	u64 *xbuf 	= ref_buf;
	u64 *rbuf 	= (u64*)plr_get_read_buffer();

	u32 wLen = SECTORS_PER_BUFFER;
	u32 words_per_sectors = wLen * WORDS_PER_SECTOR;
	u32 start_addr = g_plr_state.test_start_sector;
	u32 end_addr = 0; //get_dd_total_sector_count();
	u64 start_ticks = 0, end_ticks = 0;

	/* Elapsed time : 80 sec per 1GB */
	if ((start_addr + g_plr_state.test_sector_length) <= get_dd_total_sector_count())
		end_addr = start_addr + g_plr_state.test_sector_length;
	else
		end_addr = get_dd_total_sector_count();

	if (b_verify)
		plr_info(" Start read verify!!\n");

	start_ticks = get_tick_count64();

	for (i = start_addr; i < end_addr; i += wLen) {
		if ( (i + wLen) > end_addr ) {
			wLen = end_addr - i;
			words_per_sectors = wLen* WORDS_PER_SECTOR;
		}

		if (dd_read((uchar*)rbuf, i, wLen)) {
			return RD_ERR_READ_VERIFY_FAILED;
		}
		if (b_verify) {
			if (compare_buffer(xbuf, rbuf, words_per_sectors, &word_pos_in_buf)) {

				secno_in_buf 		= word_pos_in_buf / WORDS_PER_SECTOR;	// sec_no in the buffer
				word_no_in_sec 		= word_pos_in_buf % WORDS_PER_SECTOR;
				plr_info("word_pos_in_buf:%d, secno_in_buf:  %d, word_no_in_sec: %d\n", word_pos_in_buf, secno_in_buf, word_no_in_sec);
				*sec_no_in_zone_p	= i + secno_in_buf;

				plr_info("i: %d, secno_in_buf: %d\n", i, secno_in_buf);

				plr_info("Invalid data in zone 0 at sec_no_in_zone: %u, word_no_in_sec: %u\n", *sec_no_in_zone_p,  word_no_in_sec);
				plr_info("Crached data: 0x%08X\n", *(xbuf + word_pos_in_buf));
				plr_info("Memory dump just after crash position:\n");

				memory_dump(rbuf + word_pos_in_buf, 256);

				ret = RD_ERR_READ_VERIFY_FAILED;
				break;
			}
		}
		if ((j % 10 == 0) || ((i + wLen) >= end_addr))
			plr_info("\r %u / %u", i + wLen - 1, end_addr - 1);
		j++;
	}
	printf("\n");

	end_ticks = get_tick_count64();

	plr_info(" Read disturb speed : %u KB/Sec\n",
		caculate_kb_per_sec(end_addr - start_addr, end_ticks - start_ticks));

	return ret;
}

#endif

#ifdef OLD_METHOD
static int fill_read_disturb_area( void )
{

	int i;
	int ret = 0;

	u64 *wbuf	= prepare_ref_buffer();


	plr_info(" Fill 8 MB in zone 0 with the MAGIC NUMBER ...\n");

	for (i = 0; i < BUFFERS_PER_TEST_AREA ; i++) {
		if (dd_write((u8*)wbuf, SECTORS_PER_BUFFER * i, SECTORS_PER_BUFFER)) {
			ret = RD_ERR_WRITE_FAILED;
			break;
		}
	}


	plr_info(" Checking filled data of zone 0 ...\n");

	if (!ret) {
		int sec_no_in_zone;
		ret = read_disturb_verify(wbuf, &sec_no_in_zone);
	}


#ifdef SJD_CAUSE_INTENDED_CRASH

	plr_info(" Cause intentional crash for debugging ...\n");

	wbuf = prepare_crash_buffer();

	if (dd_write((u8*)wbuf, INTENDED_CRASH_PAGE * SECTORS_PER_PAGE, 1)) { // forced crash of page INTENDED_CRASH_PAGE
		ret = RD_ERR_WRITE_FAILED;
		return ret;
	}

#endif

	return ret;
}

#else

static int fill_read_disturb_area( void )
{

	int i = 0, j = 0;
	int ret = 0;

	u64 *wbuf	= prepare_ref_buffer();
	u32 wLen = SECTORS_PER_BUFFER;

	u32 start_addr = 0;
	u32 end_addr = get_dd_total_sector_count();
	u32 repeat = g_plr_state.finish.value;

	plr_info(" Set precondition : Erase & Write %u cycles\n", repeat);

	if (repeat < 1)
		return 0;

	do {
		plr_info (" [Cycles : %u / %u]\n", (g_plr_state.finish.value - repeat + 1), (g_plr_state.finish.value));
		// Erase all
		dd_erase_for_poff(0, end_addr, TYPE_SANITIZE);
		for (i = start_addr; i < end_addr; i += wLen) {
			if ( (i + wLen) > end_addr )
				wLen = end_addr - i;

			if (dd_write((uchar*)wbuf, i, wLen)) {
				return RD_ERR_WRITE_FAILED;
			}

			if ((j % 20 == 0) || ((i + wLen) >= end_addr))
				plr_info("\r %u / %u", i + wLen - 1, end_addr - 1);
			j++;
		}
		printf("\n");
	} while (--repeat);

#ifdef SJD_CAUSE_INTENDED_CRASH

	plr_info(" Cause intentional crash for debugging ...\n");

	wbuf = prepare_crash_buffer();

	if (dd_write((u8*)wbuf, INTENDED_CRASH_PAGE * SECTORS_PER_PAGE, 1)) { // forced crash of page INTENDED_CRASH_PAGE
		ret = RD_ERR_WRITE_FAILED;
		return ret;
	}

#endif

	return ret;
}
#endif

//
// FUNCTIONS for test case
//

static int pattern1_interleaving(uchar *buf, uint start_lsn, uint request_sectors, op_pattern operate)
{
	int ret = 0;

	uint request_count  = 1;
	uint lsn			= start_lsn;

	uint last_lsn_in_first_zone = get_last_sector_in_zone(0);

	uint last_lsn_in_test_area  = get_last_sector_in_test_area();


	while (TRUE) {
		for (; lsn < last_lsn_in_test_area; lsn += get_sectors_in_zone()) {
			if(request_count % 30 == 0)
				plr_info("\rlsn : 0x%08X, request length : %d, request num : %d", lsn, CHUNK_SIZE, request_count);

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
		if(request_count % 30 == 0)
			plr_info("\rlsn: 0x%08X, request length: %d, request num: %d", lsn, CHUNK_SIZE, request_count);

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

typedef struct read_disturb_crash_info
{
	int					err_code;		// error code, reserved
	int 				sec_no_in_zone;	// sector number in a zone
	int 				repeat_no;		// repeat number in a boot
	int					boot_cnt;		// boot count

}  read_disturb_crash_info_s;

static read_disturb_crash_info_s _rd_crash_info[1];	// read_disturb_crash_info;


static void print_rd_crash_info(uchar * buf, struct plr_state * state, struct Unconfirmed_Info * info)
{
	plr_info("===========================\n");
	plr_info("Read disturbance crash info\n");
	plr_info("===========================\n");
	plr_info("1. boot cnt: %8d\n", _rd_crash_info->boot_cnt);
	plr_info("2.      lsn: %8d\n", _rd_crash_info->sec_no_in_zone);
	plr_info("3.   repeat: %8d\n", _rd_crash_info->repeat_no);
	plr_info("===========================\n");

}

#ifdef OLD_METHOD
static int extra_verification(uchar *buf, struct plr_state *state)	// 8 MB read buffer
{
	int 	i;
	int 	ret	= 0;
	u64* 	ref_buf;

	ref_buf = prepare_ref_buffer();


	plr_info("\n Loop count: %d\n", ++rd_loop_count);

	plr_info("\n Read disturb verify...%d times\n", RD_REPEAT_COUNT);

	for (i = 0; i < RD_REPEAT_COUNT; i++) {

		int crash_sec_no_in_zone;

		plr_info("\r #%d / #%d ", i, RD_REPEAT_COUNT);

		ret = read_disturb_verify(ref_buf, &crash_sec_no_in_zone);

		if (ret) {

			_rd_crash_info->sec_no_in_zone 	= crash_sec_no_in_zone;
			_rd_crash_info->repeat_no  		= i + 1;
			_rd_crash_info->boot_cnt		= g_plr_state.boot_cnt;

			verify_set_result(VERI_EREQUEST, crash_sec_no_in_zone);
			return VERI_EREQUEST;
		}

	}

	return VERI_NOERR;
}

#else

static int extra_verification(uchar *buf, struct plr_state *state)	// 8 MB read buffer
{
	int 	i = 0;
	int 	ret	= 0;
	int 	b_verify = 0;
	int 	crash_sec_no_in_zone = 0;
	u64* 	ref_buf = NULL;

	ref_buf = prepare_ref_buffer();


	plr_info("\n Loop count: %d\n", ++rd_loop_count);

	plr_info("\n Read disturb verify...%d times\n", RD_REPEAT_COUNT);

	for (i = 0; i < RD_REPEAT_COUNT; i++) {

		plr_info(" [Loop count : %d, Times : #%d / #%d]\n", rd_loop_count, i + 1, RD_REPEAT_COUNT);

		// Read disturb verify first and last RD_REPEAT_COUNT or
		// more than read cycle 9K
		if (0 == i || (RD_REPEAT_COUNT - 1) == i ||
			rd_loop_count >= 9) {
			b_verify = TRUE;
		}

		ret = read_disturb_verify(ref_buf, &crash_sec_no_in_zone, b_verify);

		if (ret) {

			_rd_crash_info->sec_no_in_zone 	= crash_sec_no_in_zone;
			_rd_crash_info->repeat_no  		= i + 1;
			_rd_crash_info->boot_cnt		= g_plr_state.boot_cnt;

			verify_set_result(VERI_EREQUEST, crash_sec_no_in_zone);
			return VERI_EREQUEST;
		}
		b_verify = FALSE;

	}

	return VERI_NOERR;
}
#endif

/* ---------------------------------------------------------------------------------------
 *
 *----------------------------------------------------------------------------------------
 */

static void tune_test_area(uint* test_area_start, uint* test_area_length)
{
	*test_area_start 	+= SECTORS_PER_ZONE;	// shift the start area by +1 zone
	*test_area_length 	-= SECTORS_PER_ZONE;	// but end is not changed
}

int initialize_daxx_0008( uchar * buf, uint test_area_start, uint test_area_length )
{
	int ret = 0;

	struct Pattern_Function pattern_linker;
	struct Print_Crash_Func print_crash_info;


	//
	// Change test area for Read disturbance
	//

	tune_test_area(&test_area_start, &test_area_length);


	init_case_info(test_area_start, test_area_length);
	set_test_sectors_in_zone(get_sectors_in_zone());

	/*
	* NOTE *************************************************************************************
	* 포팅을 위한 임시 코드.
	* ******************************************************************************************
	*/
	init_pattern(&g_plr_state, FALSE);

	print_crash_info.default_step 			= NULL;
	print_crash_info.checking_lsn_crc_step 	= NULL;
	print_crash_info.extra_step 			= print_rd_crash_info;
	verify_init_print_func(print_crash_info);


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

	//
	// Fill read disturbance area (zone 0) with RD_MAGIC_NUMBER
	//
	if(g_plr_state.boot_cnt == 1 || g_plr_state.boot_cnt == 0) //ieroa, 20150903 : initialization first once
	{
		if((ret = fill_read_disturb_area())) {

			plr_err("Fill of read disturb area failed !!!");
			return ret;
		}
	}
	return ret;
}

int read_daxx_0008(uchar * buf, uint test_area_start, uint test_area_length)
{
	return extra_verification(buf, NULL);	// read verify func
}

int write_daxx_0008(uchar * buf, uint test_area_start, uint test_area_length)
{
	set_loop_count(++g_plr_state.loop_cnt);
	return 0;
}

// @sj 150805
// Dummy test case related functions, for stuffing test cases between daxx_0003 ~ daxx_0008
// Remove some or all of these when you develop real test cases.
//

int initialize_dummy( uchar * buf, uint test_start_sector, uint test_sector_length )
{
	SJD_DEBUG_PRINTF("This is a function call for dummy test case!");
	return 0;
}

int read_dummy( uchar * buf, uint test_start_sector, uint test_sector_length )
{
	SJD_DEBUG_PRINTF("This is a function call for dummy test case!");
	return 0;
}

int write_dummy( uchar * buf, uint test_start_sector, uint test_sector_length )
{
	SJD_DEBUG_PRINTF("This is a function call for dummy test case!");
	return 0;
}
#else	// ======================================================================================================================

#define CHUNK_SIZE	NUM_OF_SECTORS_PER_PAGE


//@sj 150716, I recommend you to introduce following macros to common.h

#ifndef BYTES_PER_BUFFER

#define BUFFER_SIZE				(0x800000)			//@sj 150714, Note: No buffer size is explicitely defined anywhere.
													//If anyone defined the buffer size in global configuration header,
													//this value may changed to the value.

#define BYTES_PER_BUFFER		(BUFFER_SIZE)							// clearer and chainable !
#define WORDS_PER_BUFFER		(BYTES_PER_BUFFER/sizeof(u32))			//
#define BYTES_PER_TEST_AREA		(BUFFER_SIZE)							// Test area is 8 MB
#define BYTES_PER_SECTOR		(SECTOR_SIZE)							// 0x200 	(512)
#define WORDS_PER_SECTOR		(BYTES_PER_SECTOR/sizeof(u32))			// 0x80 	(128)
#define SECTORS_PER_BUFFER		(BYTES_PER_BUFFER / BYTES_PER_SECTOR)	// 0x4000	(16384)
#define SECTORS_PER_PAGE		(NUM_OF_SECTORS_PER_PAGE)				// 0x8		(8)
#define BYTES_PER_PAGE			(BYTES_PER_SECTOR * SECTORS_PER_PAGE)	// 0x1000 	(4096)
#define WORDS_PER_PAGE			(BYTES_PER_PAGE/sizeof(u32))			// 0x400	(1024)
#define PAGES_PER_ZONE			(ZONE_SIZE_X_PAGE)						// 0x4000 	(16384)
#define SECTORS_PER_ZONE		(SECTORS_PER_PAGE * PAGES_PER_ZONE)		// 0x20000 	(131072)
#define BUFFERS_PER_ZONE		(SECTORS_PER_ZONE / SECTORS_PER_BUFFER)
#define PAGES_PER_TEST_AREA		(BYTES_PER_TEST_AREA/BYTES_PER_PAGE)
#define SECTORS_PER_TEST_AREA	(BYTES_PER_TEST_AREA/BYTES_PER_SECTOR)
#define BUFFERS_PER_TEST_AREA	(BYTES_PER_TEST_AREA/BYTES_PER_BUFFER)
#define PAGES_PER_BUFFER		(BYTES_PER_BUFFER / BYTES_PER_PAGE)




#endif



//
// MACROS for read disturbance
//

//#define SJD_DEVELOPMENT_MODE
//#define OLD_METHOD						// joys,2015.10.16

#ifdef SJD_DEVELOPMENT_MODE

#define RD_PAGES 							(PAGES_PER_TEST_AREA)
#define RD_REPEAT_COUNT					(3)
#define SJD_CAUSE_INTENDED_CRASH

#else

#define RD_PAGES 							(PAGES_PER_TEST_AREA)
#define RD_REPEAT_COUNT					(1000)

#endif

#define	COMPARE_FIRST_4BYTE_ONLY			(FALSE)	// for fast comparison
#define COMPARE_CRC_CHECKSUM				(FALSE)	// joys,2015.10.16

#define RD_ERR_READ_VERIFY_FAILED			(-1)
#define RD_ERR_WRITE_FAILED				(-2)
#define RD_ERR_INVALID_ZONE_NUMBER		(-3)

#define RD_MAGIC_NUMBER 					(0xEFCAFE01UL)

#ifdef 	SJD_CAUSE_INTENDED_CRASH

#define RD_MAGIC_NUMBER_INTENSIVE_CRASH		(0xDEAD1234)
#define INTENDED_CRASH_PAGE					(2)
#define INTENDED_CRASH_WORD					(0)	// crash word in the page, must b2 0 for COMPARE_FIRST_4BYTE_ONLY


#endif

static int rd_loop_count = 0;

#ifndef OLD_METHOD
static uint wbuffer_checksum = 0;
#endif


//
// Helper FUNCTIONS for read disturbance
//


static int compare_buffer(const u32* buf_ref, u32* buf, int word_len, int* word_pos)
{
	u32* 	p = buf;
	u32* 	pe = p + word_len;										// pe : sentinel for more speed

#if COMPARE_FIRST_4BYTE_ONLY == TRUE
	while(p != pe) {
		if(*p != *buf_ref) {
			*word_pos = ((size_t)p - (size_t)buf) / sizeof (u32);	// postion in buffer in words
			plr_info("word_pos = %d, data = 0x%08x, p = 0x%08X, buf = 0x%08X\n", *word_pos, *p, p, buf);
			return RD_ERR_READ_VERIFY_FAILED;
		}
		p += WORDS_PER_SECTOR;
	}
#elif COMPARE_CRC_CHECKSUM == TRUE
	uint crc_checksum = 0;
	crc_checksum = crc32(0, (const uchar *)buf, BYTES_PER_BUFFER);

	if (0 == wbuffer_checksum)
		wbuffer_checksum = 0x6b9c3483;

	if (crc_checksum != wbuffer_checksum) {
		*word_pos = 0;
		plr_info("crc checksum error! write crc : 0x%8X, read crc : 0x%8X\n", wbuffer_checksum, crc_checksum);

		while(p != pe) {
			if(*p++ != *buf_ref) {
				*word_pos = ((size_t)--p - (size_t)buf) / sizeof (u32);	// postion in buffer in words
				plr_info("word_pos = %d, data = 0x%08x, p = 0x%08X, buf = 0x%08X\n", *word_pos, *p, p, buf);
				return RD_ERR_READ_VERIFY_FAILED;
			}
		}
	}

#else
	while(p != pe) {
		if(*p++ != *buf_ref) {
			*word_pos = ((size_t)--p - (size_t)buf) / sizeof (u32);	// postion in buffer in words
			plr_info("word_pos = %d, data = 0x%08x, p = 0x%08X, buf = 0x%08X\n", *word_pos, *p, p, buf);
			return RD_ERR_READ_VERIFY_FAILED;
		}
	}
#endif
	return 0;	// Ok

}
static void memory_dump(u32* p0, int words)
{
	u32* p = (u32*)p0;
	u32* pe = p + words;

	int i = 0, j = 0;


	plr_info("Memory dump of a buffer: 0x%p...\n", p0);


	plr_info("\n    : ");

	for(;i < 8;i++) {

		plr_info("  %8x ", i);
	}

	plr_info("\n");


	i = 1; j = 0;

	plr_info("\n %03d: ", j);

	while(1) {

		u32 a = *p++;

		if (RD_MAGIC_NUMBER ==  a)
			plr_info("0x%08x ", a);
		else
			plr_err("0x%08x ", a);

		if(p == pe) break;

		if(!((i++ ) % 8)) {
			j++;
			plr_info("\n %03d: ", j);
		}
	}
	plr_info("\n");
}

static u32* prepare_ref_buffer(void)
{
	u32 *xbuf 	= (u32*)plr_get_extra_buffer();
	u32 *xbufe 	= (u32*)xbuf + WORDS_PER_BUFFER;
	u32 *p		= xbuf;

	while ( p != xbufe ) {
		*p++ = RD_MAGIC_NUMBER;
	}
#ifndef OLD_METHOD
	wbuffer_checksum = crc32(0, (const uchar *)xbuf, BYTES_PER_BUFFER);
#endif


	return xbuf;
}

#ifdef SJD_CAUSE_INTENDED_CRASH
static u32* prepare_crash_buffer(void)
{
	int i, j;
	u32 *xbuf 	= (u32*)plr_get_extra_buffer();
	u32 *p		= xbuf;

	for (i = 0; i < PAGES_PER_TEST_AREA; i++) {
		for (j = 0; j < WORDS_PER_PAGE; j++) {
			if( j == INTENDED_CRASH_WORD) {
				*(p + i * WORDS_PER_PAGE + j) = RD_MAGIC_NUMBER_INTENSIVE_CRASH;
			}
			else {
				*(p + i * WORDS_PER_PAGE + j) = RD_MAGIC_NUMBER;
			}
		}
	}

	return xbuf;
}
#endif

#ifdef OLD_METHOD
static int read_disturb_verify(u32* ref_buf, int *sec_no_in_zone_p )
{
	int i;
	int ret = 0;
	int word_pos_in_buf;
	int secno_in_buf;
	int byte_no_in_sec;

	u32 *xbuf 	= ref_buf;
	u32 *rbuf 	= (u32*)plr_get_read_buffer();

	for (i = 0; i < BUFFERS_PER_TEST_AREA; i++) {

		dd_read((uchar*)rbuf, (SECTORS_PER_BUFFER * i), SECTORS_PER_BUFFER);

		if (compare_buffer(xbuf, rbuf, WORDS_PER_BUFFER, &word_pos_in_buf)) {


			secno_in_buf 		= word_pos_in_buf / WORDS_PER_SECTOR;	// sec_no in the buffer
			byte_no_in_sec 		= word_pos_in_buf % WORDS_PER_SECTOR;
			plr_info("word_pos_in_buf:%d, secno_in_buf:  %d, byte_no_in_sec: %d\n", word_pos_in_buf, secno_in_buf, byte_no_in_sec);
			*sec_no_in_zone_p	= (i * SECTORS_PER_BUFFER) + secno_in_buf;

			plr_info("i: %d, secno_in_buf: %d\n", i, secno_in_buf);

			plr_info("Invalid data in zone 0 at sec_no_in_zone: %u, byte_no_in_sec: %u\n", *sec_no_in_zone_p,  byte_no_in_sec);
			plr_info("Crached data: 0x%08X\n", *(xbuf + word_pos_in_buf));
			plr_info("Memory dump just after crash position:\n");

			memory_dump(rbuf + word_pos_in_buf, 256);

			ret = RD_ERR_READ_VERIFY_FAILED;
			break;
		}
	}

	return ret;
}

#else

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

static int read_disturb_verify(u32* ref_buf, int *sec_no_in_zone_p, int b_verify)
{
	int i = 0, j = 0;
	int ret = 0;
	int word_pos_in_buf;
	int secno_in_buf;
	int word_no_in_sec;

	u32 *xbuf 	= ref_buf;
	u32 *rbuf 	= (u32*)plr_get_read_buffer();

	u32 wLen = SECTORS_PER_BUFFER;
	u32 words_per_sectors = wLen * WORDS_PER_SECTOR;
	u32 start_addr = g_plr_state.test_start_sector;
	u32 end_addr = 0; //get_dd_total_sector_count();
	u32 start_ticks = 0, end_ticks = 0;

	/* Elapsed time : 80 sec per 1GB */
	if ((start_addr + g_plr_state.test_sector_length) <= get_dd_total_sector_count())
		end_addr = start_addr + g_plr_state.test_sector_length;
	else
		end_addr = get_dd_total_sector_count();

	if (b_verify)
		plr_info(" Start read verify!!\n");

	start_ticks = get_tick_count64();

	for (i = start_addr; i < end_addr; i += wLen) {
		if ( (i + wLen) > end_addr ) {
			wLen = end_addr - i;
			words_per_sectors = wLen* WORDS_PER_SECTOR;
		}

		if (dd_read((uchar*)rbuf, i, wLen)) {
			return RD_ERR_READ_VERIFY_FAILED;
		}
		if (b_verify) {
			if (compare_buffer(xbuf, rbuf, words_per_sectors, &word_pos_in_buf)) {

				secno_in_buf 		= word_pos_in_buf / WORDS_PER_SECTOR;	// sec_no in the buffer
				word_no_in_sec 		= word_pos_in_buf % WORDS_PER_SECTOR;
				plr_info("word_pos_in_buf:%d, secno_in_buf:  %d, word_no_in_sec: %d\n", word_pos_in_buf, secno_in_buf, word_no_in_sec);
				*sec_no_in_zone_p	= i + secno_in_buf;

				plr_info("i: %d, secno_in_buf: %d\n", i, secno_in_buf);

				plr_info("Invalid data in zone 0 at sec_no_in_zone: %u, word_no_in_sec: %u\n", *sec_no_in_zone_p,  word_no_in_sec);
				plr_info("Crached data: 0x%08X\n", *(xbuf + word_pos_in_buf));
				plr_info("Memory dump just after crash position:\n");

				memory_dump(rbuf + word_pos_in_buf, 256);

				ret = RD_ERR_READ_VERIFY_FAILED;
				break;
			}
		}
		if ((j % 10 == 0) || ((i + wLen) >= end_addr))
			plr_info("\r %u / %u", i + wLen - 1, end_addr - 1);
		j++;
	}
	printf("\n");

	end_ticks = get_tick_count64();

	plr_info(" Read disturb speed : %u KB/Sec\n",
		caculate_kb_per_sec(end_addr - start_addr, end_ticks - start_ticks));

	return ret;
}

#endif

#ifdef OLD_METHOD
static int fill_read_disturb_area( void )
{

	int i;
	int ret = 0;

	u32 *wbuf	= prepare_ref_buffer();


	plr_info(" Fill 8 MB in zone 0 with the MAGIC NUMBER ...\n");

	for (i = 0; i < BUFFERS_PER_TEST_AREA ; i++) {
		if (dd_write((u8*)wbuf, SECTORS_PER_BUFFER * i, SECTORS_PER_BUFFER)) {
			ret = RD_ERR_WRITE_FAILED;
			break;
		}
	}


	plr_info(" Checking filled data of zone 0 ...\n");

	if (!ret) {
		int sec_no_in_zone;
		ret = read_disturb_verify(wbuf, &sec_no_in_zone);
	}


#ifdef SJD_CAUSE_INTENDED_CRASH

	plr_info(" Cause intentional crash for debugging ...\n");

	wbuf = prepare_crash_buffer();

	if (dd_write((u8*)wbuf, INTENDED_CRASH_PAGE * SECTORS_PER_PAGE, 1)) { // forced crash of page INTENDED_CRASH_PAGE
		ret = RD_ERR_WRITE_FAILED;
		return ret;
	}

#endif

	return ret;
}

#else

static int fill_read_disturb_area( void )
{

	int i = 0, j = 0;
	int ret = 0;

	u32 *wbuf	= prepare_ref_buffer();
	u32 wLen = SECTORS_PER_BUFFER;

	u32 start_addr = 0;
	u32 end_addr = get_dd_total_sector_count();
	u32 repeat = g_plr_state.finish.value;

	plr_info(" Set precondition : Erase & Write %u cycles\n", repeat);

	if (repeat < 1)
		return 0;

	do {
		plr_info (" [Cycles : %u / %u]\n", (g_plr_state.finish.value - repeat + 1), (g_plr_state.finish.value));
		// Erase all
		dd_erase_for_poff(0, end_addr, TYPE_SANITIZE);
		for (i = start_addr; i < end_addr; i += wLen) {
			if ( (i + wLen) > end_addr )
				wLen = end_addr - i;

			if (dd_write((uchar*)wbuf, i, wLen)) {
				return RD_ERR_WRITE_FAILED;
			}

			if ((j % 20 == 0) || ((i + wLen) >= end_addr))
				plr_info("\r %u / %u", i + wLen - 1, end_addr - 1);
			j++;
		}
		printf("\n");
	} while (--repeat);

#ifdef SJD_CAUSE_INTENDED_CRASH

	plr_info(" Cause intentional crash for debugging ...\n");

	wbuf = prepare_crash_buffer();

	if (dd_write((u8*)wbuf, INTENDED_CRASH_PAGE * SECTORS_PER_PAGE, 1)) { // forced crash of page INTENDED_CRASH_PAGE
		ret = RD_ERR_WRITE_FAILED;
		return ret;
	}

#endif

	return ret;
}
#endif

//
// FUNCTIONS for test case
//

static int pattern1_interleaving(uchar *buf, uint start_lsn, uint request_sectors, op_pattern operate)
{
	int ret = 0;

	uint request_count  = 1;
	uint lsn			= start_lsn;

	uint last_lsn_in_first_zone = get_last_sector_in_zone(0);

	uint last_lsn_in_test_area  = get_last_sector_in_test_area();


	while (TRUE) {
		for (; lsn < last_lsn_in_test_area; lsn += get_sectors_in_zone()) {
			if(request_count % 30 == 0)
				plr_info("\rlsn : 0x%08X, request length : %d, request num : %d", lsn, CHUNK_SIZE, request_count);

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
		if(request_count % 30 == 0)
			plr_info("\rlsn: 0x%08X, request length: %d, request num: %d", lsn, CHUNK_SIZE, request_count);

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

typedef struct read_disturb_crash_info
{
	int					err_code;		// error code, reserved
	int 				sec_no_in_zone;	// sector number in a zone
	int 				repeat_no;		// repeat number in a boot
	int					boot_cnt;		// boot count

}  read_disturb_crash_info_s;

static read_disturb_crash_info_s _rd_crash_info[1];	// read_disturb_crash_info;


static void print_rd_crash_info(uchar * buf, struct plr_state * state, struct Unconfirmed_Info * info)
{
	plr_info("===========================\n");
	plr_info("Read disturbance crash info\n");
	plr_info("===========================\n");
	plr_info("1. boot cnt: %8d\n", _rd_crash_info->boot_cnt);
	plr_info("2.      lsn: %8d\n", _rd_crash_info->sec_no_in_zone);
	plr_info("3.   repeat: %8d\n", _rd_crash_info->repeat_no);
	plr_info("===========================\n");

}

#ifdef OLD_METHOD
static int extra_verification(uchar *buf, struct plr_state *state)	// 8 MB read buffer
{
	int 	i;
	int 	ret	= 0;
	u32* 	ref_buf;

	ref_buf = prepare_ref_buffer();


	plr_info("\n Loop count: %d\n", ++rd_loop_count);

	plr_info("\n Read disturb verify...%d times\n", RD_REPEAT_COUNT);

	for (i = 0; i < RD_REPEAT_COUNT; i++) {

		int crash_sec_no_in_zone;

		plr_info("\r #%d / #%d ", i, RD_REPEAT_COUNT);

		ret = read_disturb_verify(ref_buf, &crash_sec_no_in_zone);

		if (ret) {

			_rd_crash_info->sec_no_in_zone 	= crash_sec_no_in_zone;
			_rd_crash_info->repeat_no  		= i + 1;
			_rd_crash_info->boot_cnt		= g_plr_state.boot_cnt;

			verify_set_result(VERI_EREQUEST, crash_sec_no_in_zone);
			return VERI_EREQUEST;
		}

	}

	return VERI_NOERR;
}

#else

static int extra_verification(uchar *buf, struct plr_state *state)	// 8 MB read buffer
{
	int 	i = 0;
	int 	ret	= 0;
	int 	b_verify = 0;
	int 	crash_sec_no_in_zone = 0;
	u32* 	ref_buf = NULL;

	ref_buf = prepare_ref_buffer();


	plr_info("\n Loop count: %d\n", ++rd_loop_count);

	plr_info("\n Read disturb verify...%d times\n", RD_REPEAT_COUNT);

	for (i = 0; i < RD_REPEAT_COUNT; i++) {

		plr_info(" [Loop count : %d, Times : #%d / #%d]\n", rd_loop_count, i + 1, RD_REPEAT_COUNT);

		// Read disturb verify first and last RD_REPEAT_COUNT or
		// more than read cycle 9K
		if (0 == i || (RD_REPEAT_COUNT - 1) == i ||
			rd_loop_count >= 9) {
			b_verify = TRUE;
		}

		ret = read_disturb_verify(ref_buf, &crash_sec_no_in_zone, b_verify);

		if (ret) {

			_rd_crash_info->sec_no_in_zone 	= crash_sec_no_in_zone;
			_rd_crash_info->repeat_no  		= i + 1;
			_rd_crash_info->boot_cnt		= g_plr_state.boot_cnt;

			verify_set_result(VERI_EREQUEST, crash_sec_no_in_zone);
			return VERI_EREQUEST;
		}
		b_verify = FALSE;

	}

	return VERI_NOERR;
}
#endif

/* ---------------------------------------------------------------------------------------
 *
 *----------------------------------------------------------------------------------------
 */

static void tune_test_area(uint* test_area_start, uint* test_area_length)
{
	*test_area_start 	+= SECTORS_PER_ZONE;	// shift the start area by +1 zone
	*test_area_length 	-= SECTORS_PER_ZONE;	// but end is not changed
}

int initialize_daxx_0008( uchar * buf, uint test_area_start, uint test_area_length )
{
	int ret = 0;

	struct Pattern_Function pattern_linker;
	struct Print_Crash_Func print_crash_info;


	//
	// Change test area for Read disturbance
	//

	tune_test_area(&test_area_start, &test_area_length);


	init_case_info(test_area_start, test_area_length);
	set_test_sectors_in_zone(get_sectors_in_zone());

	/*
	* NOTE *************************************************************************************
	* 포팅을 위한 임시 코드.
	* ******************************************************************************************
	*/
	init_pattern(&g_plr_state, FALSE);

	print_crash_info.default_step 			= NULL;
	print_crash_info.checking_lsn_crc_step 	= NULL;
	print_crash_info.extra_step 			= print_rd_crash_info;
	verify_init_print_func(print_crash_info);


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

	//
	// Fill read disturbance area (zone 0) with RD_MAGIC_NUMBER
	//
	if(g_plr_state.boot_cnt == 1 || g_plr_state.boot_cnt == 0) //ieroa, 20150903 : initialization first once
	{
		if((ret = fill_read_disturb_area())) {

			plr_err("Fill of read disturb area failed !!!");
			return ret;
		}
	}
	return ret;
}

int read_daxx_0008(uchar * buf, uint test_area_start, uint test_area_length)
{
	return extra_verification(buf, NULL);	// read verify func
}

int write_daxx_0008(uchar * buf, uint test_area_start, uint test_area_length)
{
	set_loop_count(++g_plr_state.loop_cnt);
	return 0;
}

// @sj 150805
// Dummy test case related functions, for stuffing test cases between daxx_0003 ~ daxx_0008
// Remove some or all of these when you develop real test cases.
//

int initialize_dummy( uchar * buf, uint test_start_sector, uint test_sector_length )
{
	SJD_DEBUG_PRINTF("This is a function call for dummy test case!");
	return 0;
}

int read_dummy( uchar * buf, uint test_start_sector, uint test_sector_length )
{
	SJD_DEBUG_PRINTF("This is a function call for dummy test case!");
	return 0;
}

int write_dummy( uchar * buf, uint test_start_sector, uint test_sector_length )
{
	SJD_DEBUG_PRINTF("This is a function call for dummy test case!");
	return 0;
}

#endif
