#ifndef __VERIFY_REPORT__
#define __VERIFY_REPORT__

#include "plr_verify.h"

void print_default_step(uchar *buf, struct plr_state *state, struct Unconfirmed_Info *crash_info);
void print_checking_lsn_crc_step(uchar *buf, struct plr_state *state, struct Unconfirmed_Info *crash_info);
void print_extra_step(uchar *buf, struct plr_state *state, struct Unconfirmed_Info *crash_info);

void print_chunk_simplly(struct plr_state *state, struct Unconfirmed_Info *crash_info);
void print_crash_status_header_info(struct Unconfirmed_Info *crash_info);
void print_crash_status_chunk_info(uchar *buf, struct Unconfirmed_Info *crash_info);


#endif
