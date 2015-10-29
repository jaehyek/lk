/* Copyright (c) 2013, LGE Inc. All rights reserved.
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
 *     * Neither the name of Code Aurora Forum, Inc. nor the names of its
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

#include <stdint.h>
#include <mmc.h>
#include <debug.h>
#include <target.h>
#include <recovery.h>
#include <lge_target_ftm.h>
#include <partition_parser.h>

#define GPT_SIZE        (BLOCK_SIZE*1024)
#define BACKUP_GPT_BLOCK_COUNT 33

#if UPDATE_GROW_PARTITION
#define GRW_BLOCK_COUNT 10
#endif

static void update_partition_info(void)
{
	int ret = 0;
	unsigned int partition_index = 0;
	unsigned long long lba_start = 0, lba_end = 0, last_lba = 0;
	uint64_t device_capacity = 0;
	unsigned char *image;

	image = target_get_scratch_address() + 0x10000000;
	read_gpt(GPT_SIZE, image);
	if (ret != 0)
		return;

	device_capacity = mmc_get_device_capacity();

	last_lba = (device_capacity / BLOCK_SIZE) - 1;
#if UPDATE_GROW_PARTITION
	/* Update grow partition */
	lba_end = last_lba - BACKUP_GPT_BLOCK_COUNT;
	lba_start = last_lba - BACKUP_GPT_BLOCK_COUNT - GRW_BLOCK_COUNT;

	replace_gpt_entry(GPT_SIZE, image, "grow", lba_start, lba_end);
#else
	lba_start = last_lba - BACKUP_GPT_BLOCK_COUNT + 1;
#endif

	/* Update userdata partition */
	partition_index = partition_get_index("userdata");
	lba_end = lba_start - 1;
	lba_start = partition_get_offset(partition_index) / BLOCK_SIZE;

	ret = replace_gpt_entry(GPT_SIZE, image, "userdata", lba_start, lba_end);
	if (ret)
		return;

	/* Update gpt */
	update_gpt(GPT_SIZE, image);

#ifndef LGE_WITH_RESIZE_DATA
	if (mmc_erase_card(lba_start * BLOCK_SIZE,
			(lba_end - lba_start * 1) * BLOCK_SIZE))
			drpintf(CRITICAL, "failed to erase mmc\n");
#endif
}

void do_extend_partition_20(void)
{
	int res = 0;
	int firstboot_num = 'F';
	unsigned long long last_lba = 0, backup_gpt_lba = 0;

	res = target_ftm_get_firstboot();
	if (res == -1) {
		dprintf(INFO, "check FTM magic string\n");
		return;
	} else if (res == firstboot_num) {
		dprintf(INFO, "firstboot already finished\n");
		return;
	} else {
		dprintf(INFO, "firstboot and try to update gpt\n");
		target_ftm_set_firstboot((char)firstboot_num);
	}

	/* Get the real last lba for device and the saved backup gpt's location */
	last_lba = (mmc_get_device_capacity() / BLOCK_SIZE) - 1;
	get_backup_gpt_position(&backup_gpt_lba);

	dprintf(INFO, "card info : last LBA %lld, backup gpt %lld\n",
			last_lba, backup_gpt_lba);

	/* If the last lba is not equal to the position of the backup gpt,
	   it updates backup gpt header and extends userdata partition. */
	if (last_lba != backup_gpt_lba) {
		update_partition_info();
#ifdef LGE_WITH_SELECT_MODE
#ifdef LGE_WITH_RESIZE_DATA
		emmc_set_recovery_cmd(RESIZE_DATA);
#else
		emmc_set_recovery_cmd(WIPE_DATA);
#endif
#endif
	}
}
