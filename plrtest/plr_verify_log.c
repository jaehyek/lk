#include "plr_common.h"
#include "plr_verify.h"
#include "plr_verify_log.h"
#include "plr_deviceio.h"

//#define LOG_MSG_SHOW 
#ifdef LOG_MSG_SHOW
	#define LOG_DEBUG_MSG(fmt, args...) printf("\n[DEBUG][%d][%s] " fmt, __LINE__, __func__, ##args)
#else
	#define LOG_DEBUG_MSG(fmt, args...)
#endif

//#define LOG_SAVE_MSG_SHOW
#ifdef LOG_SAVE_MSG_SHOW
	#define LOG_SAVE_MSG(fmt, args...) printf("\n[DEBUG][%d][%s] " fmt, __LINE__, __func__, ##args)
#else
	#define LOG_SAVE_MSG(fmt, args...)
#endif


#define DEFAULT_CRASH_ALLOC_COUNT 	3
#define MAX_NORMAL_REQUEST			20
#define SEPARATOR_REQUEST			0xFFFFFF22

static struct Verify_Log_Manager _step1_log_manager_list = {.head = NULL, .tail = NULL};
static struct Verify_Log_Manager _step2_log_manager_list = {.head = NULL, .tail = NULL};
static struct Verify_Log_Manager _step3_log_manager_list = {.head = NULL, .tail = NULL};

static struct Verify_Request_List	*_log_request_list 	= NULL;

static uint _normal_request_count_after_crash = 0;
static uint _normal_request_count_before_crash = 0;
static enum VERIFICATION_STEP _current_step;

static void print_request_list(uchar *buf, struct plr_state *state, struct Verify_Request_Info *info);
static void print_manager_list(uchar *buf, struct plr_state *state, struct Verify_Log_Manager *step);
static void destory_request_list(struct Verify_Request_List *list);
static void destroy_manager_list(struct Verify_Log_Manager *step);
static void realloc_request_list(struct Verify_Request_List **history_request_list);
static void insert_manager_list(struct Verify_Request_List **request_list, enum VERIFICATION_STEP current_step);
static void insert_request_list(struct Verify_Request_Info *info, enum VERI_ERROR_NUM status);

void verify_init_log(void)
{
	verify_destory_log();
	_step1_log_manager_list.head = NULL;
	_step1_log_manager_list.tail = NULL;

	_step2_log_manager_list.head = NULL;
	_step2_log_manager_list.tail = NULL;

	_step3_log_manager_list.head = NULL;
	_step3_log_manager_list.tail = NULL;
	
	_current_step = VERI_DEFAULT_STEP;	
	realloc_request_list(&_log_request_list);
	insert_manager_list(&_log_request_list, _current_step);
}

void verify_destory_log(void)
{
	_log_request_list = NULL;
	
	destroy_manager_list(&_step1_log_manager_list);
	destroy_manager_list(&_step2_log_manager_list);
	destroy_manager_list(&_step3_log_manager_list);	
}

void verify_print_log(uchar *buf, struct plr_state *state)
{
	verify_print_step1(buf, state);
	verify_print_step2(buf, state);
	verify_print_step3(buf, state);
}

void verify_print_step1(uchar *buf, struct plr_state *state)
{
	if(_step1_log_manager_list.head->crash_request_count == 0)
		return ;

	plr_info("\n");
	if(state->print_crash_only == TRUE)
		plr_info("Show the verification history about crash request only.\n");
	else 
		plr_info("Show the verification history.\n");
	print_manager_list(buf, state, &_step1_log_manager_list);
}

void verify_print_step2(uchar *buf, struct plr_state *state)
{
	if(_step2_log_manager_list.head->crash_request_count == 0)
		return ;
	
	plr_info("\n");
	plr_info("Occur the crash on the Default Verification Step.(checking lsn and crc).\n");
	print_manager_list(buf, state, &_step2_log_manager_list);
}

void verify_print_step3(uchar *buf, struct plr_state *state)
{
	if(_step3_log_manager_list.head->crash_request_count == 0)
		return ;

	plr_info("\n");
	plr_info("Occur the crash on the Extra Verification Step.\n");
	print_manager_list(buf, state, &_step3_log_manager_list);
}

static void print_request_list(uchar *buf, struct plr_state *state, struct Verify_Request_Info *info)
{
	struct Verify_Request_Info *current_info = NULL;
	struct Verify_Request_Info *next_info 	= NULL;
	struct plr_header *header_info 			= NULL;	
	uint page_offset = 0;
	
	current_info = info;
	while(current_info != NULL)
	{		
		if(current_info->next != NULL)
			next_info = current_info->next;		
		else 
			next_info = NULL;
		
		if(current_info->request_status >= VERI_EREQUEST)
		{
			plr_err("\n");
			plr_err("\t");
			if(state->internal_poweroff_type == TRUE)
			{
				if(state->poweroff_pos == current_info->request_start_lsn)
					plr_err(" * ");
				if(state->b_cache_enable == TRUE && state->last_flush_pos == current_info->request_start_lsn)
					plr_err(" > ");
			}
			
			plr_err("Start Lsn in Request : 0x%08X, Pages in Request : %04d, Size of Request : %04dKB \n",
					current_info->request_start_lsn, 			
					SECTORS_TO_PAGES(current_info->request_sectors),
					SECTORS_TO_SIZE_KB(current_info->request_sectors));
			
			plr_err("\t--------------------------------------------------------------------------------------------------------------------------------------\n");

			dd_read(buf, current_info->request_start_lsn, current_info->request_sectors);
			for(page_offset = 0; page_offset < SECTORS_TO_PAGES(current_info->request_sectors); page_offset++)
			{
				header_info = (struct plr_header *)(buf + (page_offset * PAGE_SIZE));	
				if(current_info->crash_pages_in_request != NULL && current_info->crash_pages_in_request[page_offset] == EXPECTED)
				{
					plr_info( "\t\tSN 0x%08X MN 0x%08X  Boot %06d Loop %04d Req# %06d ReqSt 0x%08X PN %03d Pg# %03d NextSt 0x%08X CRC 0x%08X\n", 
						header_info->lsn, header_info->magic_number, header_info->boot_cnt, header_info->loop_cnt, header_info->req_info.req_seq_num, header_info->req_info.start_sector,
						header_info->req_info.page_num, header_info->req_info.page_index, header_info->next_start_sector, header_info->crc_checksum);
				}
				else 
				{
					plr_err( "\t\tSN 0x%08X MN 0x%08X  Boot %06d Loop %04d Req# %06d ReqSt 0x%08X PN %03d Pg# %03d NextSt 0x%08X CRC 0x%08X\n", 
						header_info->lsn, header_info->magic_number, header_info->boot_cnt, header_info->loop_cnt, header_info->req_info.req_seq_num, header_info->req_info.start_sector,
						header_info->req_info.page_num, header_info->req_info.page_index, header_info->next_start_sector, header_info->crc_checksum);
				}
			}					
			plr_err("\t--------------------------------------------------------------------------------------------------------------------------------------\n");				
			plr_err("\n");
		}
		else 
		{				
			if(state->print_crash_only == FALSE)
			{
				dd_read(buf, current_info->request_start_lsn, current_info->request_sectors);
				header_info = (struct plr_header *)buf;	
				if(header_info->boot_cnt == 0 && header_info->loop_cnt == 0 && header_info->magic_number == 0)
				{
					printf("\t");			
					if(state->internal_poweroff_type == TRUE)
					{			
						if(state->poweroff_pos == current_info->request_start_lsn)
							printf(" * ");
						if(state->b_cache_enable == TRUE && state->last_flush_pos == current_info->request_start_lsn)
							printf(" > ");
					}
					
					printf("Start Lsn in Request : 0x%08X, Pages in Request : %04d, Size of Request : %04dKB, Boot : %04d, Loop : %04d \n",
						current_info->request_start_lsn, 
						SECTORS_TO_PAGES(current_info->request_sectors),
						SECTORS_TO_SIZE_KB(current_info->request_sectors),
						header_info->boot_cnt,
						header_info->loop_cnt);
				}
				else
				{
					plr_info("\t");
					if(state->internal_poweroff_type == TRUE)
					{					
						if(state->poweroff_pos == current_info->request_start_lsn)
							plr_info(" * ");
						if(state->b_cache_enable == TRUE && state->last_flush_pos == current_info->request_start_lsn)
							plr_info(" > ");
					}
					
					plr_info("Start Lsn in Request : 0x%08X, Pages in Request : %04d, Size of Request : %04dKB, Boot : %04d, Loop : %04d \n",
						current_info->request_start_lsn, 
						SECTORS_TO_PAGES(current_info->request_sectors),
						SECTORS_TO_SIZE_KB(current_info->request_sectors),
						header_info->boot_cnt,
						header_info->loop_cnt);
				}
			}
		}	

		current_info = next_info;
	}
}

void verify_insert_request_info(uint lsn, uint sectors, uchar *crash_pages_in_request, enum VERI_ERROR_NUM status)
{
	struct Verify_Request_Info *info;
	info = (struct Verify_Request_Info *)malloc(sizeof(struct Verify_Request_Info));
	if(info == NULL)
	{
		LOG_DEBUG_MSG("Failed memory allocation. \n");
		return;
	}

	info->request_start_lsn	= lsn;
	info->request_sectors 	= sectors;
	info->request_status 	= status;
	info->next 				= NULL;
	if(crash_pages_in_request != NULL && status >= VERI_EREQUEST)
	{			
		info->crash_pages_in_request = (uchar *)malloc(SECTORS_TO_PAGES(sectors));
		if(info->crash_pages_in_request == NULL)
		{		
			LOG_DEBUG_MSG("Failed memory allocation. \n");
			return;
		}	

		memcpy(info->crash_pages_in_request, crash_pages_in_request, SECTORS_TO_PAGES(sectors));
	}
	else 
		info->crash_pages_in_request = NULL;	

	insert_request_list(info, status);
}

static void print_manager_list(uchar *buf, struct plr_state *state, struct Verify_Log_Manager *step)
{
	struct Verify_Request_List	*current_request_list 	= NULL;
	struct Verify_Request_List *next_request_list 		= NULL;

	if(step->head == NULL)
		return ;

	current_request_list = step->head;
	while(current_request_list != NULL)
	{
		if(current_request_list->next != NULL)
			next_request_list = current_request_list->next;
		else 
			next_request_list  = NULL;		
		
		if(current_request_list->head != NULL)
		{
			if(current_request_list->crash_request_count > 0)
				print_request_list(buf, state, current_request_list->head);
		}
		
		current_request_list = next_request_list;
		if(current_request_list != NULL && current_request_list->crash_request_count > 0)
			plr_info("\n\t\t\t\t\t\t ... \n\n");
	}	
}

static void destory_request_list(struct Verify_Request_List *list)
{
	struct Verify_Request_Info *del_info 	= NULL;
	struct Verify_Request_Info *next_info 	= NULL;

	LOG_DEBUG_MSG("Destroy History Request List START, Verify_Request_List : 0x%08X \n", list);

	if(list == NULL)
		return ;

	if(list->crash_pointer != NULL)
	{
		free(list->crash_pointer);
		list->crash_pointer = NULL;
	}

	if(list->head != NULL)
		del_info = list->head;

	while(del_info != NULL)
	{	
		if(del_info->next != NULL)
			next_info = del_info->next; 	
		else 
			next_info = NULL;

		if(del_info->crash_pages_in_request != NULL)
		{
			free(del_info->crash_pages_in_request); 
			del_info->crash_pages_in_request = NULL;
		}

		del_info->request_start_lsn = 0;
		del_info->request_sectors = 0;
		del_info->request_status = 0;
		del_info->next = NULL;

		free(del_info);
		del_info = next_info;
	}

	list->head = NULL;
	list->tail = NULL;
	list->next = NULL;
	list->crash_pointer = NULL;
	list->request_count = 0;
	list->crash_request_count = 0;
	
	free(list);	
}

static void destroy_manager_list(struct Verify_Log_Manager *step)
{
	struct Verify_Request_List	*del_request_list 	= NULL;
	struct Verify_Request_List *next_request_list 	= NULL;

	LOG_DEBUG_MSG("Destroy History Manager \n");
	
	if(step->head == NULL)
		return ;

	del_request_list = step->head;
	while(del_request_list != NULL)
	{
		if(del_request_list->next != NULL)
			next_request_list = del_request_list->next;
		else 
			next_request_list  = NULL;
		
		destory_request_list(del_request_list);
		del_request_list = next_request_list;
	}	

	step->head = NULL;
	step->tail = NULL;
}

static void realloc_request_list(struct Verify_Request_List **history_request_list)
{
	*history_request_list = (struct Verify_Request_List *)malloc(sizeof(struct Verify_Request_List));	
	if(*history_request_list == NULL)
	{
		LOG_DEBUG_MSG("Failed memory allocation. \n");
		return;
	}

	LOG_DEBUG_MSG("_log_request_list : 0x%08X \n", _log_request_list);

	(*history_request_list)->crash_pointer 		= NULL;
	(*history_request_list)->crash_request_count= 0;
	(*history_request_list)->request_count 		= 0;
	(*history_request_list)->head = NULL;
	(*history_request_list)->tail = NULL;
	(*history_request_list)->next = NULL;

	_normal_request_count_after_crash = 0;
	_normal_request_count_before_crash = 0;	
}

static void insert_manager_list(struct Verify_Request_List **request_list, enum VERIFICATION_STEP current_step)
{
	struct Verify_Log_Manager *history_manager_list = NULL;

	if(request_list == NULL && *request_list == NULL)
		return ;
	
	switch(current_step)
	{
		case VERI_DEFAULT_STEP:
			history_manager_list = &_step1_log_manager_list;
			break;
		case VERI_CHECKING_LSN_CRC_STEP:
			history_manager_list = &_step2_log_manager_list;
			break;
		case VERI_EXTRA_STEP:
			history_manager_list = &_step3_log_manager_list;
			break;
	}

	if(history_manager_list->head == NULL)
	{
		history_manager_list->head = *request_list;
		history_manager_list->tail = *request_list;	
	}
	else 
	{
		history_manager_list->tail->next = *request_list;
		history_manager_list->tail = *request_list;
	}

	LOG_DEBUG_MSG("Insert New Reqeust List, Current Step : %d  \n", current_step);
}

static void insert_request_list(struct Verify_Request_Info *info, enum VERI_ERROR_NUM status)
{
	if(_log_request_list == NULL)
	{
		LOG_DEBUG_MSG("History Request List is not allocation.\n");
		return;
	}

	if(_current_step != verify_get_current_step())
	{
		realloc_request_list(&_log_request_list);	
		_current_step = verify_get_current_step();
		insert_manager_list(&_log_request_list, _current_step);
	}
	
	if(_log_request_list->request_count == 0)
	{
		_log_request_list->head = info;
		_log_request_list->tail = info;
	}
	else
	{
		_log_request_list->tail->next = info;
		_log_request_list->tail = info;
	}

	_log_request_list->request_count++;	

	if(_log_request_list->crash_request_count == 0)
	{
		_normal_request_count_before_crash++;			
		
		if(_normal_request_count_before_crash > MAX_NORMAL_REQUEST + 1)
		{
			struct Verify_Request_Info *temp =	_log_request_list->head;
			_log_request_list->head = _log_request_list->head->next;
			free(temp);
			_log_request_list->request_count--;	
			_normal_request_count_before_crash--;
		}
	}
	else 
	{
		_normal_request_count_after_crash++;		
		
		if(_normal_request_count_after_crash >= MAX_NORMAL_REQUEST && status < VERI_EREQUEST)
		{
			realloc_request_list(&_log_request_list);	
			insert_manager_list(&_log_request_list, _current_step);
			return;
		}
	}

	if(status >= VERI_EREQUEST)
	{
		_normal_request_count_after_crash = 0;

		if(_log_request_list->crash_request_count == 0)
		{
			LOG_DEBUG_MSG("Create, Insert Crash Info (lsn : 0x%08X, sectors : %d, count before crash : %d, count after crash : %d, crash index : %d) \n", info->request_start_lsn, info->request_sectors, _normal_request_count_before_crash, _normal_request_count_after_crash, _log_request_list->crash_request_count);

			_log_request_list->crash_pointer = (struct Verify_Request_Info **)malloc(sizeof(struct Verify_Request_Info *) * DEFAULT_CRASH_ALLOC_COUNT);					
			if(_log_request_list->crash_pointer == NULL)
			{
				LOG_DEBUG_MSG("Failed memory allocation. \n");
				return;
			}
			memset(&*(_log_request_list->crash_pointer), 0, sizeof(struct Verify_Request_Info *) * DEFAULT_CRASH_ALLOC_COUNT);			
			_log_request_list->crash_pointer[_log_request_list->crash_request_count] = info;	
		}
		else if(_log_request_list->crash_request_count != 0 &&
				_log_request_list->crash_request_count % DEFAULT_CRASH_ALLOC_COUNT == 0)
		{
			struct Verify_Request_Info **temp = NULL;
			uint alloc_count = ((_log_request_list->crash_request_count / DEFAULT_CRASH_ALLOC_COUNT) + 1) * DEFAULT_CRASH_ALLOC_COUNT;
			LOG_DEBUG_MSG("Reset, Insert Crash Info (lsn : 0x%08X, sectors : %d, count before crash : %d, count after crash : %d,  crash index : %d) \n", info->request_start_lsn, info->request_sectors, _normal_request_count_before_crash, _normal_request_count_after_crash, _log_request_list->crash_request_count);
			
			temp = (struct Verify_Request_Info **)malloc(sizeof(struct Verify_Request_Info *) * alloc_count);					
			if(temp == NULL)
			{
				LOG_DEBUG_MSG("Failed memory allocation. \n");
				return;
			}		
			
			memset(&*temp, 0, sizeof(struct Verify_Request_Info *) * alloc_count);			
			memcpy(&*temp, &*(_log_request_list->crash_pointer), sizeof(struct Verify_Request_Info *) * _log_request_list->crash_request_count);

			free(_log_request_list->crash_pointer);		
			
			_log_request_list->crash_pointer = temp;			
			_log_request_list->crash_pointer[_log_request_list->crash_request_count] = info;		
		}
		else 
		{
			LOG_DEBUG_MSG("Insert Crash Info (lsn : 0x%08X, sectors : %d, count before crash : %d, count after crash : %d, crash index : %d) \n", info->request_start_lsn, info->request_sectors, _normal_request_count_before_crash, _normal_request_count_after_crash, _log_request_list->crash_request_count);
			_log_request_list->crash_pointer[_log_request_list->crash_request_count] = info;		
		}

		_log_request_list->crash_request_count++;
	}	
}


static int load_request_list(uchar *buf, uint start_sector, uint length)
{
	int ret = 0;
	uint lsn = 0;
	uint sectors = 0;
	uint status = 0;
	uchar *crash_pages_in_request = NULL;
	int i = 0;

	if(length <= 0)
		return 0;
	
	ret = load_crash_dump(buf, length, start_sector);
	if(ret < 0)
		return ret;

	while(*((uint*)buf + (i++)) == (uint)SEPARATOR_REQUEST)
	{
		lsn 	= *((uint*)buf + (i++));
		sectors = *((uint*)buf + (i++));
		status 	= *((uint*)buf + (i++));
		if(status >= VERI_EREQUEST)
		{
			if(*((uint*)buf + i) != (uint)SEPARATOR_REQUEST)
			{
				uint index = 0;
				crash_pages_in_request = (uchar*)malloc(sizeof(uchar) * SECTORS_TO_PAGES(sectors));

				for(index = 0; index < SECTORS_TO_PAGES(sectors); index++)
					crash_pages_in_request[index] = (uchar)(*((uint*)buf + (i++)));
			}
		}

		verify_insert_request_info(lsn, sectors, crash_pages_in_request, status);
		if(crash_pages_in_request != NULL)
		{
			free(crash_pages_in_request);
			crash_pages_in_request = NULL;
		}		
	}

	return 0;
}

static int save_request_list(uchar *buf, struct Verify_Request_Info *info)
{
	struct Verify_Request_Info *current_info = NULL;
	struct Verify_Request_Info *next_info 	= NULL;
	uint i = 0;
	int length_byte = 0;
	
	current_info = info;
	while(current_info != NULL)
	{		
		if(current_info->next != NULL)
			next_info = current_info->next;		
		else 
			next_info = NULL;

		*((uint*)buf + (i++)) = (uint)SEPARATOR_REQUEST;
		*((uint*)buf + (i++)) = (uint)current_info->request_start_lsn;
		*((uint*)buf + (i++)) = (uint)current_info->request_sectors;
		*((uint*)buf + (i++)) = (uint)current_info->request_status;

		if(current_info->request_status >= VERI_EREQUEST)
		{
			if(current_info->crash_pages_in_request != NULL)
			{		
				uint index = 0;
				for(index = 0; index < SECTORS_TO_PAGES(current_info->request_sectors); index++)
					*((uint*)buf + (i++)) = (uint)(current_info->crash_pages_in_request[index]);

			}
		}
		
		current_info = next_info;
	}

	length_byte = sizeof(uint) * i;	
	return length_byte;
}

static int save_manager_list(uchar *buf, struct Verify_Log_Manager *step, uint *sector_num)
{
	struct Verify_Request_List	*current_request_list 	= NULL;
	struct Verify_Request_List *next_request_list 		= NULL;

	uchar *save_buf = buf;
	int length_byte = 0;
	int total_length_byte = 0;
	int used_sector_num = 0; 
		
	if(step->head == NULL)
	{
		LOG_SAVE_MSG("step->head == NULL \n");
		return 0;
	}

	current_request_list = step->head;
	while(current_request_list != NULL)
	{
		if(current_request_list->next != NULL)
			next_request_list = current_request_list->next;
		else 
			next_request_list  = NULL;		
		
		if(current_request_list->head != NULL)
		{
			if(current_request_list->crash_request_count > 0)
			{
				length_byte = save_request_list(save_buf, current_request_list->head);
				if(length_byte <= 0)
				{
					LOG_SAVE_MSG("You can not reach here. \n");
					return -1;
				}				
				save_buf += length_byte;	
				total_length_byte += length_byte;
			}
		}
		
		current_request_list = next_request_list;
	}	

	if(total_length_byte > 0)
	{
		used_sector_num = save_crash_dump(buf, (uint)total_length_byte, *sector_num);							
		*sector_num += (used_sector_num - 1);
	}

	return total_length_byte;
}


void verify_save_log(void)
{
	int ret = 0;
	uchar *buf = plr_get_write_buffer();	
	uint save_start_sector_num[6] = {0,};
	uint current_sector_num =  SAVE_HISTORY_REQUEST_INFO_SECTOR_NUM;

	clear_sdcard_sectors(SAVE_HISTORY_INFO_SECTOR_NUM, 50);

	LOG_SAVE_MSG("------------------------- SAVE ------------------------- \n");

	/*****************************************************************************************************/	
	save_start_sector_num[0] = current_sector_num;
	ret = save_manager_list(buf, &_step1_log_manager_list, &current_sector_num);

	if(ret < 0)
	{
		plr_err("Can not save the crash history information. \n");
		return ;
	}
	save_start_sector_num[1] = ret;
	
	/*****************************************************************************************************/	
	current_sector_num++;
	save_start_sector_num[2] = current_sector_num;	
	ret = save_manager_list(buf, &_step2_log_manager_list, &current_sector_num);
	if(ret < 0)
	{
		plr_err("Can not save the crash history information. \n");
		return ;
	}
	save_start_sector_num[3] = ret;
	
	/*****************************************************************************************************/	
	current_sector_num++;
	save_start_sector_num[4] = current_sector_num;	
	ret = save_manager_list(buf, &_step3_log_manager_list, &current_sector_num);
	if(ret < 0)
	{
		plr_err("Can not save the crash history information. \n");
		return ;
	}	
	save_start_sector_num[5] = ret;
	
	save_crash_dump(save_start_sector_num, sizeof(uint) * 6, SAVE_HISTORY_INFO_SECTOR_NUM);		
	
	LOG_SAVE_MSG("------------------------- SAVE END ------------------------- \n");	
}

void verify_load_log(void)
{
	uchar *buf = plr_get_read_buffer();	
	uint save_start_sector_num[6] = {0, };

	LOG_SAVE_MSG("------------------------- LOAD ------------------------- \n");

	load_crash_dump(buf, sizeof(uint) * 6, SAVE_HISTORY_INFO_SECTOR_NUM);
	memcpy(save_start_sector_num, buf, sizeof(uint) * 6);

	if(save_start_sector_num[0] != SAVE_HISTORY_REQUEST_INFO_SECTOR_NUM)
	{
		plr_err("Can not load the crash history information. \n");
		return;
	}
	
	verify_init_log();
	
	verify_set_current_step(VERI_DEFAULT_STEP);
	load_request_list(buf, save_start_sector_num[0], save_start_sector_num[1]);

	verify_set_current_step(VERI_CHECKING_LSN_CRC_STEP);	
	load_request_list(buf, save_start_sector_num[2], save_start_sector_num[3]);

	verify_set_current_step(VERI_EXTRA_STEP);	
	load_request_list(buf, save_start_sector_num[4], save_start_sector_num[5]);

	LOG_SAVE_MSG("------------------------- LOAD END ------------------------- \n");
}
