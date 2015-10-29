#ifndef __PLR_PATTERN__
#define __PLR_PATTERN__

#include "plr_common.h"

typedef int (*op_pattern)(uchar *, uint ,uint ,uint ,uint);
typedef int (*process_pattern)(uchar *, uint, uint, op_pattern);
typedef int (*verify_extra)(uchar*, struct plr_state*);
typedef void (*pattern_init)(struct plr_write_info*);

struct Pattern_Function
{
	process_pattern do_pattern_1;
	process_pattern do_pattern_2;
	pattern_init 	init_pattern_1;
	pattern_init 	init_pattern_2;
	verify_extra 	do_extra_verification;
};

void init_pattern(struct plr_state* state, bool is_erase_test);
void regist_pattern(struct Pattern_Function pattern_link);
int verify_pattern(uchar* buf);
int write_pattern(uchar* buf);
int check_pattern_func(void);
#endif