#ifndef __VERIFY__
#define __VERIFY__

#define EXPECTED			1
#define UNEXPECTED			0

#define VERI_REQUEST_CNT_UNCONFIRMED_STATUS	256		//MAX 512

//#define VERIFY_DEBUG_MSG_SHOW 
#ifdef VERIFY_DEBUG_MSG_SHOW
	#define VERIFY_DEBUG_MSG(fmt, args...) printf("\n[DEBUG][%d][%s] " fmt, __LINE__, __func__, ##args)
#else
	#define VERIFY_DEBUG_MSG(fmt, args...)
#endif

enum VERI_ERROR_NUM
{
	VERI_DONT_CARE	= 0,
	VERI_NOERR 		= 0,
	VERI_EIO		= 1,
	VERI_EREQUEST	= 5,
	VERI_EPARTIAL	= 6,
};

enum VERIFICATION_STEP
{
	VERI_DEFAULT_STEP = 1,
	VERI_CHECKING_LSN_CRC_STEP,
	VERI_EXTRA_STEP,
};

enum VERIFICATION_STATUS
{
	VERI_STATUS_NORMAL = 0,
	VERI_STATUS_UNCONFIRMED,
	VERI_STATUS_CRASH,
	VERI_STATUS_FINISH,
	VERI_STATUS_END = 20000,
};

struct Unconfirmed_Info
{
	uint request_start_lsn;
	uint request_sectors_cnt;
	uint current_boot_count;
	uint current_loop_count;
	struct plr_header expected_data;
	struct plr_header crashed_data;	

	uchar *crash_pages_in_chunk;
};

typedef int (*VERIFY_FUNC)(uchar *, uint ,uint ,uint ,uint);
struct Verification_Func
{
	VERIFY_FUNC normal_verification;
	VERIFY_FUNC unconfirmed_verification;
};

typedef void (*PRINT_CRASH_INFO)(uchar *, struct plr_state *, struct Unconfirmed_Info *);
struct Print_Crash_Func
{
	PRINT_CRASH_INFO default_step;
	PRINT_CRASH_INFO checking_lsn_crc_step;
	PRINT_CRASH_INFO extra_step;
};


int		do_verify(uchar *buf, uint lsn, uint request_sectors, uint request_count, uint next_addr);
void 	verify_init(struct plr_state* state, bool is_erase_test);
void 	verify_reset(uint expected_loop, uint expected_boot);
void 	verify_init_func(struct Verification_Func verify_func);
void	verify_init_print_func(struct Print_Crash_Func print_crash_info);

int 	verify_check_random_zone_only_lsn_crc(uchar *buf);
int 	verify_check_page(struct plr_header * header_info, uint lsn);
int 	verify_check_pages_lsn_crc_loop(uchar *buf, uint start_lsn, uint end_lsn, bool is_expected_loop, uint expected_loop);
int 	verify_find_chunk(uchar *buf, uint start_lsn, uint end_lsn, uint expected_loop, uint *chunk_start, uint *pages_in_chunk, bool *flag_find);

int 	verify_report_result(struct plr_state* state);
void 	verify_apply_result(struct plr_state* state);
void 	verify_print_crash_info(uchar *buf, struct plr_state *state);


void 	verify_set_status(enum VERIFICATION_STATUS status);
enum VERIFICATION_STATUS verify_get_status(void);

void 	verify_set_current_step(enum VERIFICATION_STEP step);
enum VERIFICATION_STEP verify_get_current_step(void);

void	verify_set_result(enum VERI_ERROR_NUM result, uint lsn);
int 	verify_get_result(struct plr_state *state);

int 	verify_get_error_num(struct plr_state *state);
int 	verify_get_crash_num(struct plr_state *state);

void 	verify_set_loop_count(uint expected_loop);
void 	verify_set_boot_count(uint expected_boot);
uint 	verify_get_loop_count(void);
uint 	verify_get_boot_count(void);

uint 	verify_get_predicted_last_lsn_previous_boot(void);
void 	verify_set_unexpected_info(uint lsn, uint request_sectors, struct plr_header *expected_data, struct plr_header *crashed_data, uchar *crash_pages_in_request);

int 	verify_normal_status(uchar *buf, uint start_lsn, uint request_sectors, uint request_count, uint next_addr);
int 	verify_unconfirmed_status(uchar *buf, uint start_lsn, uint request_sectors, uint request_count, uint next_addr);

void 	verify_save_crash_info(void);
void 	verify_load_crash_info(struct plr_state *state);

#endif
