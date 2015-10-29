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


/* 	DPIN_0008  

 
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
									

#ifdef SJD_DEVELOPMENT_MODE

#define RD_PAGES 							(PAGES_PER_TEST_AREA)
#define RD_REPEAT_COUNT						(3)
#define SJD_CAUSE_INTENDED_CRASH

#else

#define RD_PAGES 							(PAGES_PER_TEST_AREA)
#define RD_REPEAT_COUNT						(1000)

#endif

#define	COMPARE_FIRST_4BYTE_ONLY			(TRUE)	// for fast comparison


#define RD_ERR_READ_VERIFY_FAILED			(-1)
#define RD_ERR_WRITE_FAILED					(-2)
#define RD_ERR_INVALID_ZONE_NUMBER			(-3)

 
#define RD_MAGIC_NUMBER 					(0xEFCAFE01)


#ifdef 	SJD_CAUSE_INTENDED_CRASH

#define RD_MAGIC_NUMBER_INTENSIVE_CRASH		(0xDEAD1234)
#define INTENDED_CRASH_PAGE					(2)
#define INTENDED_CRASH_WORD					(0)	// crash word in the page, must b2 0 for COMPARE_FIRST_4BYTE_ONLY


#endif





//
// Helper FUNCTIONS for read disturbance 
//
static int compare_buffer(const u32* buf_ref, u32* buf, int word_len, int* word_pos)
{
	u32* 	p = buf;
	u32* 	pe = p + word_len;													// pe : sentinel for more speed
		
#if COMPARE_FIRST_4BYTE_ONLY == TRUE
	while(p != pe) {
		if(*p != *buf_ref) {
			*word_pos = ((u32)p - (u32)buf) / sizeof (u32);						// postion in buffer in words
			plr_info("word_pos = %d, data = 0x%08x, p = 0x%08X, buf = 0x%08X\n", *word_pos, *p, p, buf);
			return RD_ERR_READ_VERIFY_FAILED;
		}
		p += WORDS_PER_SECTOR;		
	}
#else
	while(p != pe) {
		if(*p++ != *buf_ref) {
			*word_pos = ((u32)--p - (u32)buf) / sizeof (u32);					// postion in buffer in words
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

	plr_info("1. Prepare pattern of zone 0 ...\n");	
	
	while ( p != xbufe ) {
		*p++ = RD_MAGIC_NUMBER;
	}
	return xbuf;
}

#ifdef SJD_CAUSE_INTENDED_CRASH
static u32* prepare_crash_buffer(void)
{
	int i, j;
	u32 *xbuf 	= (u32*)plr_get_extra_buffer();
	u32 *p		= xbuf; 

	plr_info("1. Prepare pattern of zone 0 ...\n");	

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

		dd_read((uchar*)rbuf, SECTORS_PER_BUFFER * i, SECTORS_PER_BUFFER);

		if (compare_buffer(xbuf, rbuf, WORDS_PER_BUFFER, &word_pos_in_buf)) {


			secno_in_buf 		= word_pos_in_buf / WORDS_PER_SECTOR;	// sec_no in the buffer
			byte_no_in_sec 		= word_pos_in_buf % WORDS_PER_SECTOR;
			plr_info("word_pos_in_buf:%d, secno_in_buf:  %d, byte_no_in_sec: %d\n", word_pos_in_buf, secno_in_buf, byte_no_in_sec);			
			*sec_no_in_zone_p	= i * SECTORS_PER_BUFFER + secno_in_buf;

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

static int fill_read_disturb_area( void )
{

	int i;
	int ret = 0;

	u32 *wbuf	= (u32*)prepare_ref_buffer();


	plr_info("2. Fill 8 MB in zone 0 with the MAGIC NUMBER ...\n");			


	for (i = 0; i < BUFFERS_PER_TEST_AREA ; i++) {		
		if (dd_write((u8*)wbuf, SECTORS_PER_BUFFER * i, SECTORS_PER_BUFFER)) {
			ret = RD_ERR_WRITE_FAILED;
			break;
		}
	}


	plr_info("3. Checking filled data of zone 0 ...\n");

	if (!ret) {
		int sec_no_in_zone;
		ret = read_disturb_verify(wbuf, &sec_no_in_zone);
	}


#ifdef SJD_CAUSE_INTENDED_CRASH

	plr_info("4. Cause intentional crash for debugging ...\n");

	wbuf = prepare_crash_buffer();

	if (dd_write((u8*)wbuf, INTENDED_CRASH_PAGE * SECTORS_PER_PAGE, 1)) { // forced crash of page INTENDED_CRASH_PAGE
		ret = RD_ERR_WRITE_FAILED;
		return ret;
	}
		
#endif	

	return ret;
}


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
		if(request_count % 30 == 0)
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
	plr_info("1. boot cnt: %4d\n", _rd_crash_info->boot_cnt);		
	plr_info("2.      lsn: %4d\n", _rd_crash_info->sec_no_in_zone);	
	plr_info("3.   repeat: %4d\n", _rd_crash_info->repeat_no);		
	plr_info("===========================\n");
	
}

static int extra_verification(uchar *buf, struct plr_state *state)	// 8 MB read buffer 
{
	int 	i;
	int 	ret 			= 0;
	u32* 	ref_buf;

	ref_buf = prepare_ref_buffer();

	plr_info("\n2. Read disturb verify...%d times\n", RD_REPEAT_COUNT);

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

/* ---------------------------------------------------------------------------------------
 * 
 *----------------------------------------------------------------------------------------
 */

static void tune_test_area(uint* test_area_start, uint* test_area_length)
{
	*test_area_start 	+= SECTORS_PER_ZONE;	// shift the start area by +1 zone
	*test_area_length 	-= SECTORS_PER_ZONE;	// but end is not changed	
}

int initialize_dpin_0008( uchar * buf, uint test_area_start, uint test_area_length )
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
	pattern_linker.do_extra_verification = extra_verification;	// read verify func
	
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
	
	if((ret = fill_read_disturb_area())) {
		
		plr_err("Fill of read disturb area failed !!!");
		return ret;
	}	
	
	return ret;
}

int read_dpin_0008(uchar * buf, uint test_area_start, uint test_area_length) 
{
	return verify_pattern(buf);	
}

int write_dpin_0008(uchar * buf, uint test_area_start, uint test_area_length) 
{
	int ret;
	ret = write_pattern(buf);
	return ret;
}

