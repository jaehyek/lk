#include "plr_common.h"
#include "plr_case_info.h"

struct CASE_INFO g_test_info;

void init_case_info(uint test_first_sector, uint test_sector_length)
{
	g_test_info.first_sector 		= test_first_sector;
	g_test_info.last_sector			= test_first_sector + test_sector_length;

	g_test_info.total_chunk_type 		= 0;
	g_test_info.total_sectors_chunk_arr	= 0;
	g_test_info.p_sectors_each_chunk	= NULL;

	g_test_info.sectors_per_zone 		= (ZONE_SIZE_X_BYTE/ SECTOR_SIZE);
	g_test_info.total_zone_num			= test_sector_length / g_test_info.sectors_per_zone;
	g_test_info.test_sectors_in_zone	= 0;
	g_test_info.reserved_sectors_in_zone= 0;
}

int set_chunk_info(uint *array_chunk, uint num_of_chunk_type){
	int i = 0;

	if(array_chunk == NULL || num_of_chunk_type <= 0)
		return -1;

	if(g_test_info.p_sectors_each_chunk != NULL)
		free(g_test_info.p_sectors_each_chunk);

	g_test_info.total_chunk_type = num_of_chunk_type;

	/*ieroa, need to free(p_sectors_chunk_arr), free(p_pgs_chunk_arr)*/
	g_test_info.p_sectors_each_chunk	= (uint*)malloc(sizeof(uint) * num_of_chunk_type);

	for(i = 0; i < g_test_info.total_chunk_type; i++){
		g_test_info.p_sectors_each_chunk[i]	= array_chunk[i];
		g_test_info.total_sectors_chunk_arr += array_chunk[i];
	}
	
	return 0;
}

void set_test_sectors_in_zone(uint test_sectors_in_zone)
{
	g_test_info.test_sectors_in_zone 		= test_sectors_in_zone;
	g_test_info.reserved_sectors_in_zone 	= g_test_info.sectors_per_zone - test_sectors_in_zone;
}

uint get_first_sector_in_test_area(void)
{
	return g_test_info.first_sector;
}

uint get_last_sector_in_test_area(void)
{
	return g_test_info.last_sector - get_reserved_sectors_in_zone();
}

uint get_total_zone(void)
{
	return g_test_info.total_zone_num;
}

uint get_zone_num(uint sector_addr)
{
	return (sector_addr - g_test_info.first_sector) / g_test_info.sectors_per_zone;
}

uint get_sectors_in_zone(void)
{
	return g_test_info.sectors_per_zone;
}

uint get_pages_in_zone(void)
{
	return g_test_info.sectors_per_zone >> 3;
}

uint get_first_sector_in_zone(uint zone_num)
{
	return g_test_info.first_sector + (zone_num * g_test_info.sectors_per_zone);
}

uint get_last_sector_in_zone(uint zone_num)
{
	return get_first_sector_in_zone(zone_num) + g_test_info.sectors_per_zone;
}

uint get_test_sectors_in_zone(void)
{
	return g_test_info.test_sectors_in_zone;
}

uint get_test_pages_in_zone(void)
{
	return g_test_info.test_sectors_in_zone >> 3;
}

uint get_reserved_sectors_in_zone(void)
{
	return g_test_info.reserved_sectors_in_zone;
}

uint get_total_chunk_type(void)
{
	return g_test_info.total_chunk_type;
}

uint get_total_sectors_in_chunk_arr(void)
{
	return g_test_info.total_sectors_chunk_arr;
}

uint get_total_pgs_in_chunk_arr(void)
{
	return g_test_info.total_sectors_chunk_arr >> 3;
}

int get_index_in_chunk_arr(uint sectors)
{
	int i = 0;

	for(i = 0; i < g_test_info.total_chunk_type; i++) 
	{
		if(g_test_info.p_sectors_each_chunk[i] == sectors)
			return i;
	}

	return -1;
}

uint get_sectors_in_chunk(uint index)
{
	return	g_test_info.p_sectors_each_chunk[index];
}

uint get_pages_in_chunk(uint index)
{
	return g_test_info.p_sectors_each_chunk[index] >> 3;
}

void print_case_info(void)
{
#ifdef PRINT_CASE_INFO
	uint i = 0;

	plr_info("\n");
	plr_info("Print case infomation \n");
	plr_info("--------------------------------------------------------------------- \n");
	plr_info("First sector in test area : 0x%08x \n", g_test_info.first_sector);
	plr_info("Last sector in test area : 0x%08x \n", g_test_info.last_sector);
	plr_info("Total sectors in zone : %d \n", g_test_info.sectors_per_zone);
	plr_info("Total pages in zone : %d \n", g_test_info.sectors_per_zone >> 3);
	plr_info("Total zone count : %d \n", g_test_info.total_zone_num);

	if(g_test_info.total_chunk_type != 0){
		plr_info("Total chunk type count : %d \n", g_test_info.total_chunk_type);
		plr_info("Total pages in chunk array : %d \n", g_test_info.total_sectors_chunk_arr >> 3);

		plr_info("request length :");
		for(i = 0; i < g_test_info.total_chunk_type; i++)
			plr_info(" [%d] ", g_test_info.p_sectors_each_chunk[i]);
		plr_info("\n");

		plr_info("Pages in chunk :");
		for(i = 0; i < g_test_info.total_chunk_type; i++)
			plr_info(" [%d] ", g_test_info.p_sectors_each_chunk[i] >> 3);
		plr_info("\n");
	}

	plr_info("Number of sectors per zone for the testing : %d \n", g_test_info.test_sectors_in_zone);
	plr_info("Number of reserved sectors per zone for the testing : %d \n", g_test_info.reserved_sectors_in_zone);

	plr_info("--------------------------------------------------------------------- \n");
#endif
}

