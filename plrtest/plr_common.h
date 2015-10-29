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

#ifndef _PLR_COMMON_H_
#define _PLR_COMMON_H_

#include "plr_type.h"
#include "plr_errno.h"
#include "plr_tick_conversion.h"
#include <stdio.h>
#include <string.h>
#include "test_case/plr_case.h"
#include "plr_lk_wrapper.h"

//#define PLR_DEBUG

#define PLR_TFTP_FW_UP

#define PLR_VERSION  "2.0.0.0"

#define SECTOR_SIZE 						512			/* byte */
#define PAGE_SIZE							4096		/* byte */

#define SECTORS_TO_SIZE_KB(X) 	((X) >> 1)
#define SECTORS_TO_SIZE_GB(X)	((X) >> 21)
#define PAGES_TO_SECTORS(X)		((X) << 3)
#define SECTORS_TO_PAGES(X)		((X) >> 3)

#define READ_CHUNK_SIZE						8388608     /* byte */ // 8MB

#define NUM_OF_SECTORS_PER_PAGE				8			/* sectors */
#define NUM_OF_SECTORS_PER_READ_CHUNK 		0x4000		/* sectors */ // 16384 sector = 8MB

#define MAX_PAGE_OF_READ_CHUNK				(NUM_OF_SECTORS_PER_READ_CHUNK/NUM_OF_SECTORS_PER_PAGE)		// 2048

#define MAGIC_NUMBER 						0xFFEF0815
#define MAGIC_NUMBER_FLUSH					0xFFEF1128
#define FLUSH_FREQUENCY						(g_plr_state.cache_flush_cycle)

#define INTERNAL_POWEROFF_FLAG 				0x7FFF0924
#define U8_INTERNAL_POWEROFF_FLAG 			0x9C

#define SAVE_VERIFY_RESULT_SECTOR_NUM			5
#define SAVE_EXTRA_INFO_SECTOR_NUM				6
#define SAVE_HISTORY_INFO_SECTOR_NUM 			10
#define SAVE_HISTORY_REQUEST_INFO_SECTOR_NUM	11

//#define FEATURE_INTERNAL_POWERLOSS_ONE_LOOP

#define stdin		0
#define stdout		1
#define stderr		2
#define MAX_FILES	3

#define ESC_COLOR_NORMAL    "\033[m"
#define ESC_COLOR_RESET     "\033[0m"
#define ESC_COLOR_BLACK		"\033[30m"
#define ESC_COLOR_RED		"\033[31m"
#define ESC_COLOR_GREEN		"\033[32m"
#define ESC_COLOR_YELLOW	"\033[33m"
#define ESC_COLOR_BLUE		"\033[34m"
#define ESC_COLOR_MAGENTA	"\033[35m"
#define ESC_COLOR_CYAN		"\033[36m"
#define ESC_COLOR_WHITE		"\033[37m"

#define plr_info(fmt, args...) 				printf(ESC_COLOR_YELLOW fmt ESC_COLOR_NORMAL, ##args)
#define plr_info_highlight(fmt, args...)	printf(ESC_COLOR_MAGENTA fmt ESC_COLOR_NORMAL, ##args)
#define plr_err(fmt, args...) 				printf(ESC_COLOR_RED fmt ESC_COLOR_NORMAL, ##args)

#define IS_DRIVER_TEST 			(g_plr_state.test_type1 == 'D')
#define IS_SYSTEM_TEST 			(g_plr_state.test_type1 == 'F')

#define IS_POWERLOSS_TEST 		(g_plr_state.test_type2 == 'P')
#define IS_AGING_TEST 			(g_plr_state.test_type2 == 'A')
#define IS_DEVELOPMENT_TEST 	(g_plr_state.test_type2 == 'D')

#define IS_EXTERNAL_TEST		(g_plr_state.test_type3 == 'E')
#define IS_INTERNAL_TEST		(g_plr_state.test_type3 == 'I')
#define IS_RETENTION_TEST		(g_plr_state.test_type3 == 'V')
#define IS_NORMAL_AGING_TEST	(g_plr_state.test_type3 == 'x')

#define IS_CACHEOFF_TEST 		(g_plr_state.test_type4 == 'N')
#define IS_CACHEON_TEST 		(g_plr_state.test_type4 == 'C')
#define IS_USER_DEFINED_TEST	(g_plr_state.test_type3 == 'x')

#ifndef MIN
#define MIN( a, b ) ( ( a ) < ( b ) ) ? ( a ) : ( b )
#endif
#ifndef MAX
#define MAX( a, b ) ( ( a ) > ( b ) ) ? ( a ) : ( b )
#endif


#ifdef PLR_DEBUG
#define plr_debug(fmt, args...) printf(ESC_COLOR_CYAN "[%s][L%d] " fmt ESC_COLOR_NORMAL, __func__, __LINE__, ##args)
#else
#define plr_debug(fmt, args...)
#endif

#define DO_CACHE_FLUSH(request_count, flush_cycle)	(((request_count) % (flush_cycle)) == 0)
#define DO_PACKED_FLUSH(request_count, flush_cycle)	(((request_count) % (flush_cycle)) == 0)

struct plr_req_info {
	union {
		uint req_seq_num;		// request sequential number after booting process
		uint commit_seq_num;
	};
	uint start_sector;			// request start sector address
	uint page_num;				// number of pages in request
	uint page_index;			// page index 
};

/* head size 48 byte */
struct plr_header {

	uint magic_number;				// magic number "0xFFEF0815"
	uint lsn;						// logical sector number
	uint boot_cnt;					// booting count 
	uint loop_cnt;					// loop count 
	struct plr_req_info req_info;	// request information
	uint next_start_sector;			// start sector address of next request
	uint reserved1;	
	uint reserved2;
	uint crc_checksum;				// crc_checksum of 36 bytes (from magic_number to next_start_sector)
};

struct plr_internal_pl {
	uint pl_time_min;
	uint pl_time_max;
	uint pl_time_writing;
};

struct plr_finish_condition {
	uint type;				// 0 : Power Off Count 1 : Written Data Size
	uint value; 			// Power Off Count or Written Data Size (GB)
};

struct plr_filled_data {
	uint type;				// 0 : CKBD 1 : ICKBD 2 : RANDOM 3 : USER
	uint value; 			
};

struct plr_chunk_size {
	uint type;				// 0 : Fixed 1 : Random
	union {
		uint req_len;
		uint ratio_of_4kb; 			
	};
	union {
		uint skip_size;
		uint ratio_of_8kb_64kb; 			
	};			
	uint ratio_of_64kb_over; 				
};

struct plr_write_info
{
	uint lsn;
	uint request_sectors;
	uint loop_count;
	uint boot_count;
	uint random_seed;
};

struct plr_state {
	uchar test_name[16];			// test name
	uchar test_type1;				// D : Driver Test F : File System Test
	uchar test_type2;				// P : Powerloss Test A : Aging Test
	uchar test_type3;				// E : External I : Internal V : Aging Retention
	uchar test_type4;				// C : Cache On 0 : Cache Off
	uint test_num;					// test number
	uint test_minor; 				// minor_version
	uint boot_cnt;					// booting count
	uint loop_cnt;					// loop count that applicatoin sent
	uint checked_addr;				// address that applicatoin sent
	uint poweroff_pos;			
	uint last_flush_pos;
	uint total_device_sector; 
	uint test_start_sector;			// start sector address of test area
	uint test_sector_length;		// length of test area (sectors)
	uint event1_type;               // event1 type 
	uint event1_cnt;				// event1 count 
	uint event2_type;				// event2 type 
	uint event2_cnt;				// event2 count
	bool b_cache_enable;
	bool b_packed_enable;
	bool b_first_commit;			// power off after writing first commit pages. //ieroa

	uint cache_flush_cycle;
	uint packed_flush_cycle;
	bool internal_poweroff_type;		// power off by gpio control
	uchar date_info[14];				// date information
	bool b_calibration_test;
	bool b_current_monitoring;		
	bool b_statistics_monitoring;		// edward
	bool b_current_accumulating;		// edward

	struct plr_finish_condition finish;
	struct plr_internal_pl pl_info;
	struct plr_filled_data filled_data;
	struct plr_chunk_size chunk_size;
	uint ratio_of_filled;
	bool b_finish_condition; 

	//ieroa
	uint commit_err_cnt;		 	
	bool print_crash_only;
	bool b_commit_err_cnt_enable;	

	struct plr_write_info write_info;
	
	uint reserved;	
};

extern struct plr_state g_plr_state;

struct plr_monitor {
	short tag;
	short value;
	uint ticcnt;
};

typedef enum {
	TYPE_ERASE,
	TYPE_TRIM,
	TYPE_DISCARD,
	TYPE_SANITIZE
} erase_type_e;

/* System depency ********************************************************/ 
extern int printf (const char *fmt, ...);
extern int sprintf(char * buf, const char *fmt, ...);

extern void *malloc (size_t len);
extern void free(void*);

#if 0
extern void* memcpy( void *dst, const void* src, uint len );
extern void * memset(void * s, int c, uint count);

extern uint strlen(const char * s);
extern char * strcpy(char * dest, const char *src);
extern int strncmp(const char * cs,const char * ct, size_t count);
extern int strcmp(const char * cs,const char * ct);
extern char * strcat(char * dest, const char * src);
extern char * strtok(char * s,const char * ct);
extern char * strdup(const char *s);
extern int getc(void);
#ifndef lplrlib_c
extern void puts(const char *s);
#endif
#ifndef mdelay
#define mdelay(x)	udelay(1000*x)
#endif
extern void udelay(unsigned long usec);
#endif
extern int snprintf(char *buf, size_t size, const char *fmt, ...);

extern int run_command (const char *cmd, int flag);
//extern int console_init_r(void);
extern int tstc(void);

extern uint crc32 (uint, const unsigned char *, uint);

/***********************************************************************/

extern ulong get_rtc_time(void);
extern ulong get_cpu_timer (ulong base);
extern void well512_seed(unsigned int nSeed);
extern unsigned int well512_rand(void);
extern void well512_write_seed(unsigned int nSeed);
extern unsigned int well512_write_rand(void);

uint _strspn(const char *s, const char *accept);
uint strtoud(const char *cp);
bool is_numeric(const char * cp);
void display_date(void);

uint random_sector_count(int num);

void* plr_get_read_buffer(void);
void* plr_get_write_buffer(void);
void* plr_get_extra_buffer(void);

uint get_power_loss_time(void);
void set_poweroff_info(void);

int  get_erase_test_type(void);
void set_erase_test_type(erase_type_e type);

int write_request(uchar * buf, uint req_start, uint req_len, uint req_seq_num, uint next_start_sector);
int write_flush_page(uchar * buf, uint req_start, uint req_len, uint req_seq_num, uint next_start_sector);
int erase_request(uint req_start, uint req_len, uint req_seq_num);

#ifdef PLR_DEBUG
void plr_print_state_info(void);
#endif

int packed_flush(void);

void set_loop_count(uint n_loop_cnt);
void set_checked_addr(uint n_check_addr);
void print_head_info(struct plr_header *header);

/***************************************** 
* For Source code analysis and debugging
* 
* by 2015.06.19 woosj@elixirflash.com
*
*****************************************/
char* util_file_basename(char* path); 				// basename(): returns pure filename from file path

// DEBUG CONTROLS
//#define SJD_DEBUG 									// turn on SJD_DEBUG mode
#ifdef SJD_DEBUG



#define SJD_DEBUG_PRINTF(fmt, args...)\
	printf(ESC_COLOR_CYAN "@SJD: %s, ", util_file_basename(__FILE__));\
	printf(ESC_COLOR_MAGENTA "%s(), %d -- ", __func__, __LINE__);\
	printf(ESC_COLOR_CYAN fmt ESC_COLOR_NORMAL, ##args)

#define SJD_WARNING_PRINTF(fmt, args...)\
	printf(ESC_COLOR_RED "@SJD: WARNING!!! %s, %s(), %d -- ", util_file_basename(__FILE__), __func__, __LINE__);\
	printf(fmt ESC_COLOR_NORMAL, ##args)

#define SJD_CALIBRATION_MODE(ON_OFF) \
	SJD_DEBUG_PRINTF("SJD_CALIBRATION_MODE [ %s ]", ON_OFF ? "ON" : "OFF");\
	g_plr_state.b_calibration_test = ON_OFF

#define SJD_INTERNAL_POWER_CONTROL_MODE(ON_OFF) \
	SJD_DEBUG_PRINTF("SJD_INTERNAL_POWER_CONTROL_MODE [ %s ]", ON_OFF ? "ON" : "OFF");\
	g_plr_state.internal_poweroff_type = ON_OFF


#define SJD_FUNCTION_ENTER() \
	SJD_DEBUG_PRINTF("FUNCTION ENTER >>> %s() \n", __func__)
 
#define SJD_FUNCTION_EXIT() \
	SJD_DEBUG_PRINTF("FUNCTION EXIT  <<< %s() \n", __func__)

#define SJD_TRACE() \
	SJD_DEBUG_PRINTF(">>> EXECUTED <<< \n")


#define SJD_ASSERT(expr, fmt, args...)\
	if(!(expr)){\
		printf(ESC_COLOR_RED "@SJD: ASSERT FAILED %s, %s(), %d -- ", util_file_basename(__FILE__), __func__, __LINE__);\
		printf(fmt ESC_COLOR_NORMAL, ##args);\
		printf("... Now system intentionally halted ...");\
		for(;;);\
	}


#else

#define SJD_DEBUG_PRINTF(fmt, args...)
#define SJD_WARNING_PRINTF(fmt, args...)
#define SJD_CALIBRATION_MODE(ON_OFF)
#define SJD_INTERNAL_POWER_CONTROL_MODE(ON_OFF)
#define SJD_FUNCTION_ENTER()
#define SJD_FUNCTION_EXIT()
#define SJD_ASSERT(expr, fmt, args...)

#endif

#define MAXPATH	1024


#endif
