/* Copyright (c) 2013-2014, The Linux Foundation. All rights reserved.
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
 *     * Neither the name of The Linux Foundation nor the names of its
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
 */

#ifndef __MMC_WRAPPER_H__
#define __MMC_WRAPPER_H__

#include <mmc_sdhci.h>

#define BOARD_KERNEL_PAGESIZE                2048
/* Wrapper APIs */

#ifdef LGE_WITH_FASTBOOT_MENU
struct mmc_card *get_mmc_card();
#endif

struct mmc_device *get_mmc_device();
uint32_t mmc_get_psn(void);

uint32_t mmc_read(uint64_t data_addr, uint32_t *out, uint32_t data_len);
uint32_t mmc_write(uint64_t data_addr, uint32_t data_len, void *in);
uint32_t mmc_erase_card(uint64_t, uint64_t);
uint64_t mmc_get_device_capacity(void);
uint32_t mmc_erase_card(uint64_t addr, uint64_t len);
uint32_t mmc_get_device_blocksize();
uint32_t mmc_page_size();
void mmc_device_sleep();
void mmc_set_lun(uint8_t lun);
uint8_t mmc_get_lun(void);
void  mmc_read_partition_table(uint8_t arg);
uint32_t mmc_write_protect(const char *name, int set_clr);

#if PLRTEST_ENABLE
uint32_t plr_mmc_write(int32_t dev_num, uint32_t start_sector, uint32_t len_sector, uint8_t *in);
uint32_t plr_mmc_read(int32_t dev_num, uint32_t start_sector, uint8_t *out, uint32_t len_sector);
uint32_t plr_mmc_erase_card(int32_t dev_num, uint32_t start_sector, uint32_t len_sector);
uint32_t plr_packed_add_list(int32_t dev_num, uint32_t start, lbaint_t blkcnt, void *buff, uint8_t rw);
uint32_t plr_packed_send_list(int32_t dev_num);
void* plr_packed_create_buf(int32_t dev_num, void *buff);
uint32_t plr_packed_delete_buf(int32_t dev_num);
struct mmc_packed* plr_get_packed_info(int32_t dev_num);
uint32_t plr_get_packed_count(int32_t dev_num);
uint32_t plr_get_packed_max_sectors(int32_t dev_num, uint8_t rw);
int32_t plr_cache_flush(int32_t dev_num);
int32_t plr_cache_ctrl(int32_t dev_num, uint8_t enable);
int32_t emmc_set_internal_info(int32_t dev_num, void* internal_info);
int32_t emmc_erase_for_poff(int32_t dev_num, uint32_t start_sector, uint32_t len, int type);
#endif
#endif
