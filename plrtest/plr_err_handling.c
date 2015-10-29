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


#include "plr_common.h"
#include "plr_err_handling.h"
#include "plr_deviceio.h"
#include "plr_protocol.h"
#include "plr_verify.h"
#include "plr_verify_log.h"

extern struct plr_state g_plr_state;

#define plr_info_loopboot(fmt, args...)	printf(ESC_COLOR_CYAN fmt ESC_COLOR_NORMAL, ##args)
#define plr_info_loop(fmt, args...)		printf(ESC_COLOR_GREEN fmt ESC_COLOR_NORMAL, ##args)
#define plr_info_boot(fmt, args...)		printf(ESC_COLOR_YELLOW fmt ESC_COLOR_NORMAL, ##args)


/* ------------------------------------------------
* SD Card I/O Function for Crash
* ------------------------------------------------*/

#define DEBUG_ERR_HANDLING(fmt, args...) printf("\n[DEBUG][%d][%s] " fmt, __LINE__, __func__, ##args)


enum PRINT_COMMAND_OPTION
{
	PRINT_COMMAND = 0,
	PRINT_OPTION_LSN,
	PRINT_OPTION_CRASH,
	PRINT_OPTION_TESTINFO,
	PRINT_OPTION_HEADER,
	PRINT_OPTION_COUNT,
	PRINT_OPTION_LOOP,
	PRINT_OPTION_BOOT,
	PRINT_OPTION_MAX,
};

static char *print_option_name[PRINT_OPTION_MAX] = 
{
	"tr",
	"lsn",
	"-crash",
	"-testinfo",	
	"-header",
	"-count=",
	"-loop=",
	"-boot=",
};

int get_crash_state_sdcard(int *err_num)
{
	uint start_sector = 0;
	uint sector_count = 0;
	uchar pid;
	uchar buf[SECTOR_SIZE] = {0, };
	int ret;

	if (get_sdmmc_part_info(&start_sector, &sector_count, &pid)) 
		return -PLR_ENODEV;

	ret = sdmmc_read(buf, start_sector, 1);	
		
	if (ret == 0)
		memcpy(&err_num, buf, sizeof(int));

	return ret;
}	

int set_crash_state_sdcard(int err_num)
{
	int ret = 0;
	uint start_sector = 0;
	uint sector_count = 0;
	uchar pid;
	uchar *buf = NULL;
	if (get_sdmmc_part_info(&start_sector, &sector_count, &pid)) 
		return -PLR_ENODEV;
	
	buf = (uchar*)malloc(sizeof(SECTOR_SIZE));
	if(buf == NULL)
		return -1;

	*((uint*)buf) = (uint)err_num;		
	ret = sdmmc_write(buf, start_sector, 1);
	if(buf != NULL)
		free(buf);

	return ret;
}

int clear_sdcard_sectors(uint s_sector, uint sector_num)
{
	uint start_sector = 0, sector_count = 0;
	uchar pid;
	char temp[SECTOR_SIZE];
	int ret = 0;
	
	if (get_sdmmc_part_info(&start_sector, &sector_count, &pid)) {
		return -PLR_ENODEV;
	}

	start_sector += s_sector;
	memset(temp, 0, SECTOR_SIZE);
	
	while(sector_num--)
	{
		ret = sdmmc_write((uchar*)temp, start_sector++, 1);
		if(ret < 0)
			return ret;
	}
	
	return ret;
}

int clear_state_sdcard(void)
{
	return clear_sdcard_sectors(0, 1);
}

int load_crash_dump(void* buf, uint length_byte, uint start_location)
{
	int ret = 0;
	uint buf_offset 	= 0;
	uint used_sector_num= 0;
	uint start_sector 	= 0;
	uint sector_count 	= 0;	
	uchar pid;
	int length 			= (int)length_byte;
	
	if (get_sdmmc_part_info(&start_sector, &sector_count, &pid)) {
		return -PLR_ENODEV;
	}

	start_sector += start_location;
	while(length > 0)
	{
		ret = sdmmc_read((uchar*)(buf + buf_offset), start_sector + used_sector_num, 1);
		if(ret < 0)
			return ret;

		length		-= SECTOR_SIZE;
		buf_offset 	+= SECTOR_SIZE;
		used_sector_num++;
	}	

	return used_sector_num;
}

int save_crash_dump(void* buf, uint length_byte, uint start_location)
{
	int ret = 0;
	uint buf_offset 	= 0;
	uint used_sector_num= 0;
	uint start_sector 	= 0;
	uint sector_count 	= 0;	
	uchar pid;
	int length 			= (int)length_byte;
	uchar temp[SECTOR_SIZE];

	if (get_sdmmc_part_info(&start_sector, &sector_count, &pid))
		return -PLR_ENODEV;
		
	start_sector += start_location;
	while(length > 0)
	{		
		memset(temp, 0, SECTOR_SIZE);
		memcpy(temp, buf + buf_offset, (length > SECTOR_SIZE) ? SECTOR_SIZE:length );
		
		ret = sdmmc_write(temp, start_sector + used_sector_num, 1);
		if(ret < 0)
			return ret;

		length		-= SECTOR_SIZE;
		buf_offset 	+= SECTOR_SIZE;
		used_sector_num++;
	}	
	return used_sector_num;
}

int  plr_print_page_dump(uint lsn, uint page_count, uint loop, uint boot)
{
	int ret = 0;	
	uchar *extra_buf = plr_get_extra_buffer();
	uint *p_print = NULL;
	uint i, j = 0;	
	uint print_num_per_page = PAGE_SIZE / sizeof(uint);

	ret = dd_read(extra_buf, lsn, PAGES_TO_SECTORS(page_count));
	if(ret != 0)
		return ret;
			
	if(page_count <= 0)
		return ret;

	p_print = (uint*)extra_buf;	
			
	while(page_count--)
	{	
		plr_info("\nLSN : 0x%08X \n", lsn);

		plr_info("\n      ");	
		for(i = 0; i < 8; i++)
			plr_info("  %8x ", i);		
		plr_info("\n");
		
		for(i = 0, j = 0; i < print_num_per_page; i++)
		{
			if(i % 8 == 0)
				plr_info("\n %03d: ", j++);
			
			plr_info("0x%08X ", *p_print++);
		}
		lsn += NUM_OF_SECTORS_PER_PAGE;
		
		plr_info("\n\n");
	}

	return ret;
}

int plr_print_page_header(uint lsn, uint page_count, uint loop, uint boot)
{
	int ret = 0;		
	uchar *extra_buf = plr_get_extra_buffer();
	uint print_page_count = 0;
	struct plr_header *header_info = NULL;

	ret = dd_read(extra_buf, lsn, PAGES_TO_SECTORS(page_count));
	if(ret != 0)
		return ret;
			
	if(page_count <= 0)
		return ret;
	
	while(print_page_count < page_count)
	{	
		header_info = (struct plr_header*)(extra_buf + (print_page_count * PAGE_SIZE));

		if(loop != 0 || boot != 0)
		{
			if(header_info->loop_cnt == loop && header_info->boot_cnt == boot)
			{
				plr_info_loopboot("%04d: SN 0x%08X MN 0x%08X Boot %06d Loop %04d Req# %06d ReqSt 0x%08X PN %03d Pg# %03d NextSt 0x%08X CRC 0x%08X\n", print_page_count,
									header_info->lsn, header_info->magic_number, header_info->boot_cnt, header_info->loop_cnt, header_info->req_info.req_seq_num, header_info->req_info.start_sector,
									header_info->req_info.page_num, header_info->req_info.page_index, header_info->next_start_sector, header_info->crc_checksum);		
			}
			else if(header_info->loop_cnt == loop && header_info->loop_cnt != 0)
			{
				plr_info_loop("%04d: SN 0x%08X MN 0x%08X Boot %06d Loop %04d Req# %06d ReqSt 0x%08X PN %03d Pg# %03d NextSt 0x%08X CRC 0x%08X\n",  print_page_count,
									header_info->lsn, header_info->magic_number, header_info->boot_cnt, header_info->loop_cnt, header_info->req_info.req_seq_num, header_info->req_info.start_sector,
									header_info->req_info.page_num, header_info->req_info.page_index, header_info->next_start_sector, header_info->crc_checksum);		
			}
			else if(header_info->boot_cnt == boot && header_info->boot_cnt != 0)
			{
				plr_info_boot("%04d: SN 0x%08X MN 0x%08X Boot %06d Loop %04d Req# %06d ReqSt 0x%08X PN %03d Pg# %03d NextSt 0x%08X CRC 0x%08X\n",  print_page_count,
									header_info->lsn, header_info->magic_number, header_info->boot_cnt, header_info->loop_cnt, header_info->req_info.req_seq_num, header_info->req_info.start_sector,
									header_info->req_info.page_num, header_info->req_info.page_index, header_info->next_start_sector, header_info->crc_checksum);		
			}
			else 
			{
				printf("%04d: SN 0x%08X MN 0x%08X Boot %06d Loop %04d Req# %06d ReqSt 0x%08X PN %03d Pg# %03d NextSt 0x%08X CRC 0x%08X\n",  print_page_count,
								header_info->lsn, header_info->magic_number, header_info->boot_cnt, header_info->loop_cnt, header_info->req_info.req_seq_num, header_info->req_info.start_sector,
								header_info->req_info.page_num, header_info->req_info.page_index, header_info->next_start_sector, header_info->crc_checksum);		
			}
				
		}
		else 
		{	
			if(header_info->magic_number == MAGIC_NUMBER) 
			{
				plr_info("%04d: SN 0x%08X MN 0x%08X  Boot %06d Loop %04d Req# %06d ReqSt 0x%08X PN %03d Pg# %03d NextSt 0x%08X CRC 0x%08X\n",  print_page_count,
							header_info->lsn, header_info->magic_number, header_info->boot_cnt, header_info->loop_cnt, header_info->req_info.req_seq_num, header_info->req_info.start_sector,
							header_info->req_info.page_num, header_info->req_info.page_index, header_info->next_start_sector, header_info->crc_checksum);
			}		
			else if(header_info->magic_number == MAGIC_NUMBER_FLUSH) 
			{
				plr_info_highlight("%04d: SN 0x%08X MN 0x%08X	Boot %06d Loop %04d Req# %06d ReqSt 0x%08X PN %03d Pg# %03d NextSt 0x%08X CRC 0x%08X\n",  print_page_count,
								header_info->lsn, header_info->magic_number, header_info->boot_cnt, header_info->loop_cnt, header_info->req_info.req_seq_num, header_info->req_info.start_sector,
								header_info->req_info.page_num, header_info->req_info.page_index, header_info->next_start_sector, header_info->crc_checksum);
			}			
			else 
			{		
				printf("%04d: SN 0x%08X MN 0x%08X	Boot %06d Loop %04d Req# %06d ReqSt 0x%08X PN %03d Pg# %03d NextSt 0x%08X CRC 0x%08X\n",  print_page_count,
						header_info->lsn, header_info->magic_number, header_info->boot_cnt, header_info->loop_cnt, header_info->req_info.req_seq_num, header_info->req_info.start_sector,
						header_info->req_info.page_num, header_info->req_info.page_index, header_info->next_start_sector, header_info->crc_checksum);		
			}		
		}

		print_page_count++;
	}

	printf("\n");
	
	return ret;
}

int plr_print_log(bool show_crash_only)
{
	int ret = 0;
	uchar* extra_buf = plr_get_extra_buffer();

	extern int register_func_table(void);		//from plr_main.c
	extern int plr_start_initialization(void);	//from plr_main.c

	verify_load_crash_info(&g_plr_state);
	ret = register_func_table();
	if(ret)
	{
		plr_err("Don't find the crash log. \n");
		return ret;
	}
	plr_start_initialization();

	verify_load_crash_info(NULL);
	verify_load_log();	

	if(show_crash_only == TRUE)
		g_plr_state.print_crash_only = TRUE;
	else 
		g_plr_state.print_crash_only = FALSE;
	
	verify_print_crash_info(extra_buf, &g_plr_state);
	
	verify_destory_log();
	return ret;
}

int plr_print_log_sector(uint lsn, uint page_count, uint loop, uint boot, int (*funcp)(uint, uint,uint,uint))
{
	int ret 	= 0;
	int print_page_count	= 0;
	uint read_sector_count 	= 0;
	uint sector_count = 0;
	int total_sectors = get_dd_total_sector_count();
	
	sector_count = PAGES_TO_SECTORS(page_count);


	/* 4KB aligned. 8sector(1000b)*/
	lsn = (lsn >> 3) << 3;

	/* Checking for start log address */
	if (lsn >= total_sectors) {
		
		plr_err ("Error!! start sector is not correct. log start sector : %u, physical end sector : %d\n",
					lsn, total_sectors - 1);
		return -1;
	}

	/* Resize read length for total sector */
	if (lsn + sector_count > total_sectors) {
		read_sector_count = sector_count - (lsn + sector_count - total_sectors);
		plr_info ("Access to the last page. total sectors : %d\n", total_sectors);
	}
	else {
		read_sector_count = sector_count;
	}

	print_page_count = read_sector_count / NUM_OF_SECTORS_PER_PAGE;

	ret = funcp(lsn, print_page_count, loop, boot);		
	return ret;
}

static void plr_print_test_info(void)
{
	int ret = 0;
	extern int register_func_table(void);		//from plr_main.c
	extern int plr_start_initialization(void);	//from plr_main.c

	verify_load_crash_info(&g_plr_state);
	ret = register_func_table();
	if(ret)
	{
		plr_err("Don't find the test info. \n");
		return ;
	}	

	printf( " Test Information \n");
	printf( " --------------------------------------- \n"
			" test name : %s\n"
			" start sector : 0x%08X\n"
			" test size : %dGB\n"
			" boot count : %u\n"
			" loop count : %u\n"
			, g_plr_state.test_name
			, g_plr_state.test_start_sector 
			, SECTORS_TO_SIZE_GB(g_plr_state.test_sector_length)
            , g_plr_state.boot_cnt 
            , g_plr_state.loop_cnt
			);

	if(g_plr_state.b_cache_enable == TRUE)
	{
		printf( " cache : %s, flush cycle : %d \n"
				, "enable"
				, g_plr_state.cache_flush_cycle);
	}
	else 
		printf( " cache : %s\n", "disable");

	if(g_plr_state.b_packed_enable == TRUE)
	{
		printf( " packed : %s, flush cycle : %d \n"
				, "enable"
				, g_plr_state.packed_flush_cycle);
	}
	else 
		printf( " packed : %s\n", "disable");


	if(g_plr_state.test_type2 == 'A')
		printf(	" SPO Type : %s\n", "Aging");

	else 
	{
		printf( " SPO Type : %s\n",
			(g_plr_state.internal_poweroff_type == TRUE) ? "Internal":"External");  
	}
	
	printf(
		" finish type [%s], value [%d%s\n",
		(g_plr_state.finish.type == 0) ? "SPO Count":"Data Size",
		 g_plr_state.finish.value, 
		(g_plr_state.finish.type == 0) ? "]":"GB]");
	
	printf (" --------------------------------------- \n");
}

static void plr_print_help_message(void)
{
	printf("\n");
	printf("The test report command display the verification logs and the data in page.\n\n");
	plr_info("- tr \n");
	plr_info("- tr [-crash]\n");
	plr_info("- tr [lsn]\n");
	printf("\t+ [-count=N]\n");
	plr_info("- tr [lsn] [-header]\n");
	printf("\t+ [-count=N]\n");
	printf("\t+ [-loop=N] \n");
	printf("\t+ [-boot=N] \n");
	plr_info("- tr [-testinfo] \n");

	printf("\n");
	printf("Example. \n");
	plr_info("1. tr \n");
	printf("\t: Display the verification logs. \n\n");
	
	plr_info("2. tr -crash \n");
	printf("\t: Display the only crash requests in verification logs\n\n");
	
	plr_info("3. tr 0x200000 \n"); 
	printf("\t: Display the 1 page by the 4 byte.\n\n");
	
	plr_info("4. tr 0x200000 -count=2 \n"); 
	printf("\t: Display the 2 pages data by the 4 byte.\n\n");		
	
	plr_info("5. tr 0x200000 -header \n");
	printf("\t: Display the header data in the page.\n\n");
	
	plr_info("6. tr 0x200000 -loop=20 -header \n");
	printf("\t: Display the header data in the 128 pages. And the pages that the loop count is 20 is highlighted.\n\n");
	
	plr_info("7. tr 0x200000 -header -count=1000 -boot=10 \n");	
	printf("\t: Display the header data in the 1000 pages. And the pages that the boot count is 10 is highlighted.\n\n");
	printf("\n");
}

static bool match_arg(char *args[], uint argc, char *argument, uint *ret_setup_value)
{
	uint i = 0;
	uint pos = 0;
	enum PRINT_COMMAND_OPTION print_arg = PRINT_OPTION_MAX;

	for(i = 1; i < PRINT_OPTION_MAX; i++)
	{
		if(strcmp(print_option_name[i], argument) == 0)
			print_arg = i;
	}

	if(print_arg == PRINT_OPTION_MAX)
		return FALSE;	
	
	for(i = 1; i < argc; i++)
	{
		switch(print_arg)
		{
			case PRINT_OPTION_LSN:
				if(is_numeric(args[i]) == TRUE)
				{
					*ret_setup_value = strtoud(args[i]);
					return TRUE;
				}
				break;

			case PRINT_OPTION_CRASH:
			case PRINT_OPTION_TESTINFO:
			case PRINT_OPTION_HEADER:	
				if(strcmp(args[i], print_option_name[print_arg]) == 0)
					return TRUE;				
				break;
				
			case PRINT_OPTION_COUNT:					
			case PRINT_OPTION_LOOP:
			case PRINT_OPTION_BOOT:
				if(_strspn(args[i], print_option_name[print_arg]) != strlen(print_option_name[print_arg]))
					break;
					
				pos = strlen(print_option_name[print_arg]);
				if(is_numeric(args[i] + pos) == TRUE)
				{
					*ret_setup_value = strtoud(args[i] + pos );
					return TRUE;
				}		
				
				break;
			default:
				break;
		}
	}

	return FALSE;
}
	
int test_report(char *args[], uint argc)
{
	int ret = 0;
	uint setup_value = 0;
	uint lsn = 0;
	uint page_count = 0;
	uint loop_count = 0;
	uint boot_count = 0;
	int (*funcp)(uint, uint,uint,uint) = NULL;

	if(argc == 1)
	{
		ret = plr_print_log(FALSE);
		return ret; 
	}

	if(match_arg(args, argc, print_option_name[PRINT_OPTION_CRASH], &setup_value) == TRUE)
	{
		ret = plr_print_log(TRUE);
		return ret;		
	}

	if(match_arg(args, argc, print_option_name[PRINT_OPTION_TESTINFO], &setup_value) == TRUE)
	{
		plr_print_test_info();
		return ret;
	}

	if(match_arg(args, argc, print_option_name[PRINT_OPTION_LSN], &setup_value) == TRUE)
	{
		lsn = setup_value;
		
		if(match_arg(args, argc, print_option_name[PRINT_OPTION_HEADER], &setup_value) == TRUE)
		{
			page_count = 128; 	//default is 128.
			funcp = plr_print_page_header;
		}
		else 
		{
			page_count = 1; 	//default is 1.
			funcp = plr_print_page_dump;
		}

		if(match_arg(args, argc, print_option_name[PRINT_OPTION_COUNT], &setup_value) == TRUE)	
			page_count = setup_value;

		if(match_arg(args,argc, print_option_name[PRINT_OPTION_LOOP], &setup_value) == TRUE)
			loop_count = setup_value;

		if(match_arg(args,argc, print_option_name[PRINT_OPTION_BOOT], &setup_value) == TRUE)
			boot_count = setup_value;

		ret = plr_print_log_sector(lsn, page_count, loop_count, boot_count, funcp);

		return ret;		
	}	
	
	plr_print_help_message();

	return ret;	
}
