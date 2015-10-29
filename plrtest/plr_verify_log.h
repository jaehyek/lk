#ifndef __VERIFY_LOG__
#define __VERIFY_LOG__

#include "plr_verify.h"

struct Verify_Request_Info
{
	struct Verify_Request_Info *next;

	uint request_start_lsn;
	uint request_sectors;	
	enum VERI_ERROR_NUM request_status;
	uchar *crash_pages_in_request;
};

struct Verify_Request_List
{
	struct Verify_Request_Info *head;
	struct Verify_Request_Info *tail;
	uint request_count;	
	
	struct Verify_Request_Info **crash_pointer;
	uint crash_request_count;	
	
	struct Verify_Request_List *next;
};

struct Verify_Log_Manager
{
	struct Verify_Request_List *head;
	struct Verify_Request_List *tail;
};


void verify_init_log(void);
void verify_destory_log(void);
void verify_print_log(unsigned char *buf, struct plr_state *state);
void verify_print_step1(unsigned char *buf, struct plr_state *state);
void verify_print_step2(unsigned char *buf, struct plr_state *state);
void verify_print_step3(unsigned char *buf, struct plr_state *state);
void verify_insert_request_info(uint lsn, uint sectors, uchar *crash_pages_in_request, enum VERI_ERROR_NUM status);

void verify_save_log(void);
void verify_load_log(void);



#endif
