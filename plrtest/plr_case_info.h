#ifndef __PLR_CASE_INFO__
#define __PLR_CASE_INFO__

#include "plr_type.h"

//#define PRINT_CASE_INFO 

#define ZONE_SIZE_X_BYTE		0x4000000		//(1024 * 1024 * 64) = 64MB
#define ZONE_SIZE_X_SECTOR		0x20000			//(1024 * 1024 * 64) / 512
#define ZONE_SIZE_X_SECTOR_MASK	0x1FFFF	
#define ZONE_SIZE_X_PAGE		0x4000			//(1024 * 1024 * 64) / 4096

#define NUM_OF_SECTORS_PER_8MB 	16384

struct CASE_INFO
{
	uint first_sector;
	uint last_sector;

	/*for writing chunk*/
	uint total_chunk_type;
	uint total_sectors_chunk_arr;
	uint *p_sectors_each_chunk;		

	uint sectors_per_zone;
	uint total_zone_num;	
	uint test_sectors_in_zone;
	uint reserved_sectors_in_zone;
};

void 	init_case_info(uint test_first_sector, uint test_sector_length);
int		set_chunk_info(uint *array_chunk, uint num_of_chunk_type);
void 	set_test_sectors_in_zone(uint test_sectors_in_zone);

uint 	get_first_sector_in_test_area(void);
uint 	get_last_sector_in_test_area(void);

uint 	get_total_zone(void);
uint 	get_zone_num(uint sector_addr);
uint 	get_sectors_in_zone(void);
uint 	get_pages_in_zone(void);
uint 	get_first_sector_in_zone(uint zone_num);
uint 	get_last_sector_in_zone(uint zone_num);
uint 	get_test_sectors_in_zone(void);
uint 	get_test_pgs_in_zone(void);
uint 	get_reserved_sectors_in_zone(void);

uint	get_total_chunk_type(void);
uint 	get_total_sectors_in_chunk_arr(void);
uint 	get_total_pages_in_chunk_arr(void);
int 	get_index_in_chunk_arr(uint sectors);
uint 	get_sectors_in_chunk(uint index);
uint 	get_pages_in_chunk(uint index);

void 	print_case_info(void);


#endif
