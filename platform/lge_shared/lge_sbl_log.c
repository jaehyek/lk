/* Copyright (c) 2013-2014, LGE Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of The Linux Foundation, Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <string.h>
#include <stdlib.h>
#include <reg.h>
#include <debug.h>
#include <smem.h>
#include <lge_smem.h>

#define SBL_LOG_SIZE 2500

static char *sbl_log;
static int log_size;
static uint32_t delta_time;

/* refer to 'boot_images/core/boot/secboot3/src/boot_logger.h' */
struct boot_log_meta_info {
	uint8_t  *log_buf_start;
	uint8_t  *log_buf_ptr;
	uint32_t log_buf_size;
	uint32_t log_buf_init;
	uint32_t ref_time;
	uint32_t start_time;
	uint32_t stopwatch_locked;
};

void sbl_log_init(void)
{
	smem_vendor2_type smem_vendor2_info;
	struct boot_log_meta_info *meta_info = NULL;
	char *sbl_log_end = NULL;

	int ret;

	ret = smem_get_vendor2_info(&smem_vendor2_info);
	if (ret) {
		dprintf(CRITICAL, "Failed to read SMEM_ID_VENDOR2\n");
		return;
	}

	meta_info = (struct boot_log_meta_info *)smem_vendor2_info.sbl_log_meta_info;

	sbl_log = (char *)meta_info->log_buf_start;
	sbl_log_end = (char *)meta_info->log_buf_ptr;

	log_size = (int)(sbl_log_end - sbl_log);
	if (log_size > SBL_LOG_SIZE) {
		dprintf(INFO, "%s : log_size: %d is not normal!!\n", __func__, log_size);
		dprintf(INFO, "skip sbl log!!\n");
		log_size = 0;
	} else if (log_size < 500) {
		/* to avoid cutting log if log_buf_ptr circulates as size of log buffer */
		log_size = SBL_LOG_SIZE;
	}

	delta_time = smem_vendor2_info.sbl_delta_time;

	dprintf(INFO, "%s : sbl_log=%p, log_size=%d, delta_time=%d\n",
		__func__, sbl_log, log_size, delta_time);
}

void sbl_log_print(void)
{
	int i = 0;

	set_display_ctime(0);
	for (i = 0; i < log_size; i++)
		dprintf(INFO, "%c", sbl_log[i]);

	set_display_ctime(1);
}

uint32_t sbl_log_get_delta_time(void)
{
	return delta_time;
}
