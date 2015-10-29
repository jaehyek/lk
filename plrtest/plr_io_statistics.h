/*
 *  linux/include/linux/mmc/discard.h
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *  Card driver specific definitions.
 */

#ifndef _PLR_STATISTICS_H
#define _PLR_STATISTICS_H

#include "plr_common.h"

#define REQ_SIZE_COUNT 9
#define MIN_REQ_SIZE 4096

typedef enum {
	STAT_MMC_READ	= 1,
	STAT_MMC_WRITE,
	STAT_MMC_CACHE_FLUSH
} mmc_cmd_e;

typedef struct {
	u32 chuck_size;
	u64 rCnt;
	u64 wCnt;
	u64 r_latency;
	u64 r_min_latency;
	u64 r_max_latency;
	u64 w_latency;
	u64 w_min_latency;
	u64 w_max_latency;
}request_stats_s;

typedef struct {
	int init;
	int enable;
	u64 start_time;
	u64 total_rCnt;
	u64 total_rBlocks;
	u64 total_rlatency;
	u64 total_wCnt;
	u64 total_wBlocks;
	u64 total_wlatency;
	u64 total_cf_Cnt;
	u64 total_cf_latency;
	u64 high_latency;
	u32 high_latency_chunk;
	request_stats_s *mmc_requests;
}mmc_statistic_t;

int statistic_clear(void);
int statistic_enable(int enable);
int statistic_result(void);
void statistic_mmc_request_start(void);
void statistic_mmc_request_done(mmc_cmd_e cmd, uint blocks);

/* debug utility functions */
#ifdef CONFIG_MMC_STATISTIC_DEBUG
#define _sdbg_msg(fmt, args...) printf("[MEMLOG]//%s(%d): " fmt, __func__, __LINE__, ##args)
#else
#define _sdbg_msg(fmt, args...)
#endif /* CONFIG_MMC_MEM_LOG_BUFF_DEBUG */

#ifndef _err_msg
#define _err_msg(fmt, args...) printf("%s(%d): " fmt, __func__, __LINE__, ##args)
#endif

#endif /* _PLR_STATISTICS_H */


