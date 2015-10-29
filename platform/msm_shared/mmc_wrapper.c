/* Copyright (c) 2013-2015, The Linux Foundation. All rights reserved.
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

#include <stdlib.h>
#include <stdint.h>
#include <mmc_wrapper.h>
#include <mmc_sdhci.h>
#include <sdhci.h>
#include <ufs.h>
#include <target.h>
#include <string.h>
#include <partition_parser.h>
#include <boot_device.h>
#include <dme.h>
#include <boot_device.h>
#if PLRTEST_ENABLE
#include "plr_common.h"
#include "../plrtest/plr_io_statistics.h"

static void plr_mmc_init_packed_info(struct mmc_device *dev);
#endif

/*
 * Weak function for UFS.
 * These are needed to avoid link errors for platforms which
 * do not support UFS. Its better to keep this inside the
 * mmc wrapper.
 */
__WEAK int ufs_write(struct ufs_dev *dev, uint64_t data_addr, addr_t in, uint32_t len)
{
	return 0;
}

__WEAK int ufs_read(struct ufs_dev *dev, uint64_t data_addr, addr_t in, uint32_t len)
{
	return 0;
}

__WEAK uint32_t ufs_get_page_size(struct ufs_dev *dev)
{
	return 0;
}

__WEAK uint32_t ufs_get_serial_num(struct ufs_dev *dev)
{
	return 0;
}

__WEAK uint64_t ufs_get_dev_capacity(struct ufs_dev *dev)
{
	return 0;
}

__WEAK uint32_t ufs_get_erase_blk_size(struct ufs_dev *dev)
{
	return 0;
}

__WEAK int ufs_erase(struct ufs_dev* dev, uint64_t start_lba, uint32_t num_blocks)
{
	return 0;
}

__WEAK uint8_t ufs_get_num_of_luns(struct ufs_dev* dev)
{
	return 0;
}

/*
 * Function: get mmc card
 * Arg     : None
 * Return  : Pointer to mmc card structure
 * Flow    : Get the card pointer from the device structure
 */
#ifdef LGE_WITH_FASTBOOT_MENU
struct mmc_card *get_mmc_card()
#else
static struct mmc_card *get_mmc_card()
#endif
{
	void *dev;
	struct mmc_card *card;

	dev = target_mmc_device();
	card = &((struct mmc_device*)dev)->card;

	return card;
}

/*
 * Function: mmc_write
 * Arg     : Data address on card, data length, i/p buffer
 * Return  : 0 on Success, non zero on failure
 * Flow    : Write the data from in to the card
 */
uint32_t mmc_write(uint64_t data_addr, uint32_t data_len, void *in)
{
	uint32_t val = 0;
	int ret = 0;
	uint32_t block_size = 0;
	uint32_t write_size = SDHCI_ADMA_MAX_TRANS_SZ;
	uint8_t *sptr = (uint8_t *)in;
	void *dev;

	dev = target_mmc_device();

	block_size = mmc_get_device_blocksize();

	ASSERT(!(data_addr % block_size));

	if (data_len % block_size)
		data_len = ROUNDUP(data_len, block_size);

	/*
	 * Flush the cache before handing over the data to
	 * storage driver
	 */
	arch_clean_invalidate_cache_range((addr_t)in, data_len);

	if (platform_boot_dev_isemmc())
	{
		/* TODO: This function is aware of max data that can be
		 * tranferred using sdhci adma mode, need to have a cleaner
		 * implementation to keep this function independent of sdhci
		 * limitations
		 */
		while (data_len > write_size) {
			val = mmc_sdhci_write((struct mmc_device *)dev, (void *)sptr, (data_addr / block_size), (write_size / block_size));
			if (val)
			{
				dprintf(CRITICAL, "Failed Writing block @ %x\n",(unsigned int)(data_addr / block_size));
				return val;
			}
			sptr += write_size;
			data_addr += write_size;
			data_len -= write_size;
		}

		if (data_len)
			val = mmc_sdhci_write((struct mmc_device *)dev, (void *)sptr, (data_addr / block_size), (data_len / block_size));

		if (val)
			dprintf(CRITICAL, "Failed Writing block @ %x\n",(unsigned int)(data_addr / block_size));
	}
	else
	{
		ret = ufs_write((struct ufs_dev *)dev, data_addr, (addr_t)in, (data_len / block_size));

		if (ret)
		{
			dprintf(CRITICAL, "Error: UFS write failed writing to block: %llu\n", data_addr);
			val = 1;
		}
	}

	return val;
}

/*
 * Function: mmc_read
 * Arg     : Data address on card, o/p buffer & data length
 * Return  : 0 on Success, non zero on failure
 * Flow    : Read data from the card to out
 */
uint32_t mmc_read(uint64_t data_addr, uint32_t *out, uint32_t data_len)
{
	uint32_t ret = 0;
	uint32_t block_size;
	uint32_t read_size = SDHCI_ADMA_MAX_TRANS_SZ;
	void *dev;
	uint8_t *sptr = (uint8_t *)out;

	dev = target_mmc_device();
	block_size = mmc_get_device_blocksize();

	ASSERT(!(data_addr % block_size));
	ASSERT(!(data_len % block_size));

	/*
	 * dma onto write back memory is unsafe/nonportable,
	 * but callers to this routine normally provide
	 * write back buffers. Invalidate cache
	 * before read data from mmc.
         */
	arch_clean_invalidate_cache_range((addr_t)(out), data_len);

	if (platform_boot_dev_isemmc())
	{
		/* TODO: This function is aware of max data that can be
		 * tranferred using sdhci adma mode, need to have a cleaner
		 * implementation to keep this function independent of sdhci
		 * limitations
		 */
		while (data_len > read_size) {
			ret = mmc_sdhci_read((struct mmc_device *)dev, (void *)sptr, (data_addr / block_size), (read_size / block_size));
			if (ret)
			{
				dprintf(CRITICAL, "Failed Reading block @ %x\n",(unsigned int) (data_addr / block_size));
				return ret;
			}
			sptr += read_size;
			data_addr += read_size;
			data_len -= read_size;
		}

		if (data_len)
			ret = mmc_sdhci_read((struct mmc_device *)dev, (void *)sptr, (data_addr / block_size), (data_len / block_size));

		if (ret)
			dprintf(CRITICAL, "Failed Reading block @ %x\n",(unsigned int) (data_addr / block_size));
	}
	else
	{
		ret = ufs_read((struct ufs_dev *) dev, data_addr, (addr_t)out, (data_len / block_size));
		if (ret)
		{
			dprintf(CRITICAL, "Error: UFS read failed writing to block: %llu\n", data_addr);
		}

		arch_invalidate_cache_range((addr_t)out, data_len);
	}

	return ret;
}


/*
 * Function: mmc get erase unit size
 * Arg     : None
 * Return  : Returns the erase unit size of the storage
 * Flow    : Get the erase unit size from the card
 */

uint32_t mmc_get_eraseunit_size()
{
	uint32_t erase_unit_sz = 0;

	if (platform_boot_dev_isemmc()) {
		struct mmc_device *dev;
		struct mmc_card *card;

		dev = target_mmc_device();
		card = &dev->card;
		/*
		 * Calculate the erase unit size,
		 * 1. Based on emmc 4.5 spec for emmc card
		 * 2. Use SD Card Status info for SD cards
		 */
		if (MMC_CARD_MMC(card))
		{
			/*
			 * Calculate the erase unit size as per the emmc specification v4.5
			 */
			if (dev->card.ext_csd[MMC_ERASE_GRP_DEF])
				erase_unit_sz = (MMC_HC_ERASE_MULT * dev->card.ext_csd[MMC_HC_ERASE_GRP_SIZE]) / MMC_BLK_SZ;
			else
				erase_unit_sz = (dev->card.csd.erase_grp_size + 1) * (dev->card.csd.erase_grp_mult + 1);
		}
		else
			erase_unit_sz = dev->card.ssr.au_size * dev->card.ssr.num_aus;
	}

	return erase_unit_sz;
}

/*
 * Function: Zero out blk_len blocks at the blk_addr by writing zeros. The
 *           function can be used when we want to erase the blocks not
 *           aligned with the mmc erase group.
 * Arg     : Block address & length
 * Return  : Returns 0
 * Flow    : Erase the card from specified addr
 */

static uint32_t mmc_zero_out(struct mmc_device* dev, uint32_t blk_addr, uint32_t num_blks)
{
	uint32_t *out;
	uint32_t block_size = mmc_get_device_blocksize();
	uint32_t erase_size = (block_size * num_blks);
	uint32_t scratch_size = target_get_max_flash_size();

	dprintf(INFO, "erasing 0x%x:0x%x\n", blk_addr, num_blks);

	if (erase_size <= scratch_size)
	{
		/* Use scratch address if the unaligned blocks */
		out = (uint32_t *) target_get_scratch_address();
	}
	else
	{
		dprintf(CRITICAL, "Erase Fail: Erase size: %u is bigger than scratch region\n", scratch_size);
		return 1;
	}

	memset((void *)out, 0, erase_size);

	/* Flush the data to memory before writing to storage */
	arch_clean_invalidate_cache_range((addr_t) out , erase_size);

	if (mmc_sdhci_write(dev, out, blk_addr, num_blks))
	{
		dprintf(CRITICAL, "failed to erase the partition: %x\n", blk_addr);
		return 1;
	}

	return 0;
}

/*
 * Function: mmc erase card
 * Arg     : Block address & length
 * Return  : Returns 0
 * Flow    : Erase the card from specified addr
 */
uint32_t mmc_erase_card(uint64_t addr, uint64_t len)
{
	struct mmc_device *dev;
	uint32_t block_size;
	uint32_t unaligned_blks;
	uint32_t head_unit;
	uint32_t tail_unit;
	uint32_t erase_unit_sz;
	uint32_t blk_addr;
	uint32_t blk_count;
	uint64_t blks_to_erase;

	block_size = mmc_get_device_blocksize();

	dev = target_mmc_device();

	ASSERT(!(addr % block_size));
	ASSERT(!(len % block_size));

	if (platform_boot_dev_isemmc())
	{
		erase_unit_sz = mmc_get_eraseunit_size();
		dprintf(SPEW, "erase_unit_sz:0x%x\n", erase_unit_sz);

		blk_addr = addr / block_size;
		blk_count = len / block_size;

		dprintf(INFO, "Erasing card: 0x%x:0x%x\n", blk_addr, blk_count);

		head_unit = blk_addr / erase_unit_sz;
		tail_unit = (blk_addr + blk_count - 1) / erase_unit_sz;

		if (tail_unit - head_unit <= 1)
		{
			dprintf(INFO, "SDHCI unit erase not required\n");
			return mmc_zero_out(dev, blk_addr, blk_count);
		}

		unaligned_blks = erase_unit_sz - (blk_addr % erase_unit_sz);

		if (unaligned_blks < erase_unit_sz)
		{
			dprintf(SPEW, "Handling unaligned head blocks\n");
			if (mmc_zero_out(dev, blk_addr, unaligned_blks))
				return 1;

			blk_addr += unaligned_blks;
			blk_count -= unaligned_blks;

			head_unit = blk_addr / erase_unit_sz;
			tail_unit = (blk_addr + blk_count - 1) / erase_unit_sz;

			if (tail_unit - head_unit <= 1)
			{
				dprintf(INFO, "SDHCI unit erase not required\n");
				return mmc_zero_out(dev, blk_addr, blk_count);
			}
		}

		unaligned_blks = blk_count % erase_unit_sz;
		blks_to_erase = blk_count - unaligned_blks;

		dprintf(SPEW, "Performing SDHCI erase: 0x%x:0x%x\n", blk_addr,(unsigned int)blks_to_erase);
		if (mmc_sdhci_erase((struct mmc_device *)dev, blk_addr, blks_to_erase * block_size
#if PLRTEST_ENABLE
					, 0U
#endif
					))
		{
			dprintf(CRITICAL, "MMC erase failed\n");
			return 1;
		}

		blk_addr += blks_to_erase;

		if (unaligned_blks)
		{
			dprintf(SPEW, "Handling unaligned tail blocks\n");
			if (mmc_zero_out(dev, blk_addr, unaligned_blks))
				return 1;
		}

	}
	else
	{
		if(ufs_erase((struct ufs_dev *)dev, addr, (len / block_size)))
		{
			dprintf(CRITICAL, "mmc_erase_card: UFS erase failed\n");
			return 1;
		}
	}

	return 0;
}

/*
 * Function: mmc get psn
 * Arg     : None
 * Return  : Returns the product serial number
 * Flow    : Get the PSN from card
 */
uint32_t mmc_get_psn(void)
{
	if (platform_boot_dev_isemmc())
	{
		struct mmc_card *card;

		card = get_mmc_card();

		return card->cid.psn;
	}
	else
	{
		void *dev;

		dev = target_mmc_device();

		return ufs_get_serial_num((struct ufs_dev *)dev);
	}
}

/*
 * Function: mmc get capacity
 * Arg     : None
 * Return  : Returns the density of the emmc card
 * Flow    : Get the density from card
 */
uint64_t mmc_get_device_capacity()
{
	if (platform_boot_dev_isemmc())
	{
		struct mmc_card *card;

		card = get_mmc_card();

		return card->capacity;
	}
	else
	{
		void *dev;

		dev = target_mmc_device();

		return ufs_get_dev_capacity((struct ufs_dev *)dev);
	}
}

/*
 * Function: mmc get blocksize
 * Arg     : None
 * Return  : Returns the block size of the storage
 * Flow    : Get the block size form the card
 */
uint32_t mmc_get_device_blocksize()
{
	if (platform_boot_dev_isemmc())
	{
		struct mmc_card *card;

		card = get_mmc_card();

		return card->block_size;
	}
	else
	{
		void *dev;

		dev = target_mmc_device();

		return ufs_get_page_size((struct ufs_dev *)dev);
	}
}

/*
 * Function: storage page size
 * Arg     : None
 * Return  : Returns the page size for the card
 * Flow    : Get the page size for storage
 */
uint32_t mmc_page_size()
{
	if (platform_boot_dev_isemmc())
	{
		return BOARD_KERNEL_PAGESIZE;
	}
	else
	{
		void *dev;

		dev = target_mmc_device();

		return ufs_get_page_size((struct ufs_dev *)dev);
	}
}

/*
 * Function: mmc device sleep
 * Arg     : None
 * Return  : Clean up function for storage
 * Flow    : Put the mmc card to sleep
 */
void mmc_device_sleep()
{
	void *dev;
	dev = target_mmc_device();

	if (platform_boot_dev_isemmc())
	{
		mmc_put_card_to_sleep((struct mmc_device *)dev);
	}
}

/*
 * Function     : mmc set LUN for ufs
 * Arg          : LUN number
 * Return type  : void
 */
void mmc_set_lun(uint8_t lun)
{
	void *dev;
	dev = target_mmc_device();

	if (!platform_boot_dev_isemmc())
	{
		((struct ufs_dev*)dev)->current_lun = lun;
	}
}

/*
 * Function     : mmc get LUN from ufs
 * Arg          : LUN number
 * Return type  : lun number for UFS and 0 for emmc
 */
uint8_t mmc_get_lun(void)
{
	void *dev;
	uint8_t lun=0;

	dev = target_mmc_device();

	if (!platform_boot_dev_isemmc())
	{
		lun = ((struct ufs_dev*)dev)->current_lun;
	}

	return lun;
}

void mmc_read_partition_table(uint8_t arg)
{
	void *dev;
	uint8_t lun = 0;
	uint8_t max_luns;

	dev = target_mmc_device();

	if(!platform_boot_dev_isemmc())
	{
		max_luns = ufs_get_num_of_luns((struct ufs_dev*)dev);

		ASSERT(max_luns);

		for(lun = arg; lun < max_luns; lun++)
		{
			mmc_set_lun(lun);

			if(partition_read_table())
			{
				dprintf(CRITICAL, "Error reading the partition table info for lun %d\n", lun);
			}
		}
		mmc_set_lun(0);
	}
	else
	{
		if(partition_read_table())
		{
			dprintf(CRITICAL, "Error reading the partition table info\n");
		}
	}
}

uint32_t mmc_write_protect(const char *ptn_name, int set_clr)
{
	void *dev = NULL;
	struct mmc_card *card = NULL;
	uint32_t block_size;
	unsigned long long  ptn = 0;
	uint64_t size;
	int index = -1;
#ifdef UFS_SUPPORT
	int ret = 0;
#endif

	dev = target_mmc_device();
	block_size = mmc_get_device_blocksize();

	if (platform_boot_dev_isemmc())
	{
		card = &((struct mmc_device *)dev)->card;

		index = partition_get_index(ptn_name);

		ptn = partition_get_offset(index);
		if(!ptn)
		{
			return 1;
		}

		/* Convert the size to blocks */
		size = partition_get_size(index) / block_size;

		/*
		 * For read only partitions the minimum size allocated on the disk is
		 * 1 WP GRP size. If the size of partition is less than 1 WP GRP size
		 * protect atleast one WP group.
		 */
		if (partition_read_only(index) && size < card->wp_grp_size)
		{
			/* Write protect api takes the size in bytes, convert size to bytes */
			size = card->wp_grp_size * block_size;
		}
		else
		{
			size *= block_size;
		}

		/* Set the power on WP bit */
		return mmc_set_clr_power_on_wp_user((struct mmc_device *)dev, (ptn / block_size), size, set_clr);
	}
	else
	{
#ifdef UFS_SUPPORT
		/* Enable the power on WP fo all LUNs which have WP bit is enabled */
		ret = dme_set_fpoweronwpen((struct ufs_dev*) dev);
		if (ret < 0)
		{
			dprintf(CRITICAL, "Failure to WP UFS partition\n");
			return 1;
		}
#endif
	}

	return 0;
}

#if PLRTEST_ENABLE
/*
 * Function: plr_get_mmc_card
 * Arg     : None
 * Return  : Pointer to mmc card structure
 * Flow    : Get the card pointer from the device structure
 */
static struct mmc_card *plr_get_mmc_card(int32_t dev_num)
{
	struct mmc_device *dev = (struct mmc_device*)find_mmc_device(dev_num);
	return &dev->card;
}

/*
 * Function: plr_mmc_get_blocksize
 * Arg     : None
 * Return  : Returns the block size of the storage
 * Flow    : Get the block size form the card
 */
static uint32_t plr_mmc_get_device_blocksize(int32_t dev_num)
{
	if (platform_boot_dev_isemmc())
	{
		struct mmc_card *card;

		card = plr_get_mmc_card(dev_num);

		return card->block_size;
	}
	return 0;
}

/*
 * Function: plr_mmc_get_eraseunit_size
 * Arg     : None
 * Return  : Returns the erase unit size of the storage
 * Flow    : Get the erase unit size from the card
 */
static uint32_t plr_mmc_get_eraseunit_size(int32_t dev_num)
{
	uint32_t erase_unit_sz = 0;

	if (platform_boot_dev_isemmc()) {
		struct mmc_device *dev;
		struct mmc_card *card;

		dev = (struct mmc_device*)find_mmc_device(dev_num);
		card = &dev->card;
		/*
		 * Calculate the erase unit size,
		 * 1. Based on emmc 4.5 spec for emmc card
		 * 2. Use SD Card Status info for SD cards
		 */
		if (MMC_CARD_MMC(card)) {
			/*
			 * Calculate the erase unit size as per the emmc specification v4.5
			 */
			if (dev->card.ext_csd[MMC_ERASE_GRP_DEF])
				erase_unit_sz = (MMC_HC_ERASE_MULT * dev->card.ext_csd[MMC_HC_ERASE_GRP_SIZE]) / MMC_BLK_SZ;
			else
				erase_unit_sz = (dev->card.csd.erase_grp_size + 1) * (dev->card.csd.erase_grp_mult + 1);
		}
		else
			erase_unit_sz = dev->card.ssr.au_size * dev->card.ssr.num_aus;
	}
	return erase_unit_sz;
}

/*
 * Function: Zero out blk_len blocks at the blk_addr by writing zeros. The
 *           function can be used when we want to erase the blocks not
 *           aligned with the mmc erase group.
 * Arg     : Block address & length
 * Return  : Returns 0
 * Flow    : Erase the card from specified addr
 */
static uint32_t plr_mmc_zero_out(int32_t dev_num, struct mmc_device* dev, uint32_t blk_addr, uint32_t num_blks)
{
	uint32_t *out;
	uint32_t block_size = plr_mmc_get_device_blocksize(dev_num);
	uint32_t erase_size = (block_size * num_blks);
	uint32_t scratch_size = target_get_max_flash_size();

	dprintf(INFO, "erasing 0x%x:0x%x\n", blk_addr, num_blks);

	if (erase_size <= scratch_size)
	{
		/* Use scratch address if the unaligned blocks */
		out = (uint32_t *) target_get_scratch_address();
	}
	else
	{
		dprintf(CRITICAL, "Erase Fail: Erase size: %u is bigger than scratch region:%u\n", erase_size, scratch_size);
		return 1;
	}

	memset((void *)out, 0, erase_size);

	/* Flush the data to memory before writing to storage */
	arch_clean_invalidate_cache_range((addr_t) out , erase_size);

	if (mmc_sdhci_write(dev, out, blk_addr, num_blks))
	{
		dprintf(CRITICAL, "failed to erase the partition: %x\n", blk_addr);
		return 1;
	}

	return 0;
}

/*
 * Function: plr_mmc_write
 * Arg     : Data address on card, data length, i/p buffer
 * Return  : 0 on Success, non zero on failure
 * Flow    : Write the data from in to the card
 */
uint32_t plr_mmc_write(int32_t dev_num, uint32_t start_sector, uint32_t len_sector, uint8_t *in)
{
	uint32_t val = 0;
	uint32_t block_size = 0;
	uint32_t write_size = 0;
	uint8_t *sptr = in;
	struct mmc_device *dev;
	uint32_t blkcnt = len_sector;

	dev = (struct mmc_device*)find_mmc_device(dev_num);

	block_size = plr_mmc_get_device_blocksize(dev_num);

	write_size = SDHCI_ADMA_MAX_TRANS_SZ/block_size;

	/*
	 * Flush the cache before handing over the data to
	 * storage driver
	 */
	arch_clean_invalidate_cache_range((addr_t)in, len_sector*block_size);

	if (platform_boot_dev_isemmc())
	{
		/* TODO: This function is aware of max data that can be
		 * tranferred using sdhci adma mode, need to have a cleaner
		 * implementation to keep this function independent of sdhci
		 * limitations
		 */
		statistic_mmc_request_start();
		while (len_sector > write_size) {

			val = mmc_sdhci_write(dev, (void *)sptr, start_sector, write_size);
			if (val)
			{
				if(val != -INTERNAL_POWEROFF_FLAG) {
					dprintf(CRITICAL, "Failed Writing block @ %x\n", start_sector);
				}
				statistic_mmc_request_done(STAT_MMC_WRITE, blkcnt);
				return val;
			}
			sptr += write_size*block_size;
			start_sector += write_size;
			len_sector -= write_size;
		}

		if (len_sector)
			val = mmc_sdhci_write(dev, (void *)sptr, start_sector, len_sector);

		if (val) {
			if(val != -INTERNAL_POWEROFF_FLAG) {
				dprintf(CRITICAL, "Failed Writing block @ %x\n", start_sector);
			}
		}
	}
	statistic_mmc_request_done(STAT_MMC_WRITE, blkcnt);

	return val;
}

/*
 * Function: plr_mmc_read
 * Arg     : Data address on card, o/p buffer & data length
 * Return  : 0 on Success, non zero on failure
 * Flow    : Read data from the card to out
 */
uint32_t plr_mmc_read(int32_t dev_num, uint32_t start_sector, uint8_t *out, uint32_t len_sector)
{
	uint32_t ret = 0;
	uint32_t block_size;
	uint32_t read_size = 0;
	struct mmc_device *dev;
	uint8_t *sptr = out;
	uint32_t blkcnt = len_sector;

	dev = (struct mmc_device*)find_mmc_device(dev_num);
	block_size = plr_mmc_get_device_blocksize(dev_num);

	read_size = SDHCI_ADMA_MAX_TRANS_SZ/block_size;

	/*
	 * dma onto write back memory is unsafe/nonportable,
	 * but callers to this routine normally provide
	 * write back buffers. Invalidate cache
	 * before read data from mmc.
         */
	arch_clean_invalidate_cache_range((addr_t)(out), len_sector*block_size);

	if (platform_boot_dev_isemmc())
	{
		statistic_mmc_request_start();
		/* TODO: This function is aware of max data that can be
		 * tranferred using sdhci adma mode, need to have a cleaner
		 * implementation to keep this function independent of sdhci
		 * limitations
		 */
		while (len_sector > read_size) {
			ret = mmc_sdhci_read(dev, (void *)sptr, start_sector, read_size);
			if (ret)
			{
				statistic_mmc_request_done(STAT_MMC_WRITE, blkcnt);
				dprintf(CRITICAL, "Failed Reading block @ %x\n", start_sector);
				return ret;
			}
			sptr += read_size*block_size;
			start_sector += read_size;
			len_sector -= read_size;
		}

		if (len_sector)
			ret = mmc_sdhci_read(dev, (void *)sptr, start_sector, len_sector);

		if (ret)
			dprintf(CRITICAL, "Failed Reading block @ %x\n", start_sector);
	}
	statistic_mmc_request_done(STAT_MMC_READ, blkcnt);
	return ret;
}

/*
 * Function: plr_mmc_erase_card
 * Arg     : Block address & length
 * Return  : Returns 0
 * Flow    : Erase the card from specified addr
 */
uint32_t plr_mmc_erase_card(int32_t dev_num, uint32_t start_sector, uint32_t len_sector)
{
	struct mmc_device *dev;
	uint32_t block_size;
	uint32_t unaligned_blks;
	uint32_t head_unit;
	uint32_t tail_unit;
	uint32_t erase_unit_sz;
	uint32_t blk_addr;
	uint32_t blk_count;
	uint64_t blks_to_erase;

	dev = (struct mmc_device*)find_mmc_device(dev_num);
	block_size = plr_mmc_get_device_blocksize(dev_num);

	if (platform_boot_dev_isemmc())
	{
		erase_unit_sz = plr_mmc_get_eraseunit_size(dev_num);
		dprintf(SPEW, "erase_unit_sz:0x%x\n", erase_unit_sz);

		blk_addr = start_sector;
		blk_count = len_sector;

		dprintf(INFO, "Erasing card: 0x%x:0x%x\n", blk_addr, blk_count);

		head_unit = blk_addr / erase_unit_sz;
		tail_unit = (blk_addr + blk_count - 1) / erase_unit_sz;

		if (tail_unit - head_unit <= 1)
		{
			dprintf(INFO, "SDHCI unit erase not required\n");
			return plr_mmc_zero_out(dev_num, dev, blk_addr, blk_count);
		}

		unaligned_blks = erase_unit_sz - (blk_addr % erase_unit_sz);

		if (unaligned_blks < erase_unit_sz)
		{
			dprintf(SPEW, "Handling unaligned head blocks\n");
			if (plr_mmc_zero_out(dev_num, dev, blk_addr, unaligned_blks))
				return 1;

			blk_addr += unaligned_blks;
			blk_count -= unaligned_blks;

			head_unit = blk_addr / erase_unit_sz;
			tail_unit = (blk_addr + blk_count - 1) / erase_unit_sz;

			if (tail_unit - head_unit <= 1)
			{
				dprintf(INFO, "SDHCI unit erase not required\n");
				return plr_mmc_zero_out(dev_num, dev, blk_addr, blk_count);
			}
		}

		unaligned_blks = blk_count % erase_unit_sz;
		blks_to_erase = blk_count - unaligned_blks;

		dprintf(SPEW, "Performing SDHCI erase: 0x%x:0x%llx\n", blk_addr, blks_to_erase);
		if (mmc_sdhci_erase((struct mmc_device *)dev, blk_addr, blks_to_erase * block_size, 1U))
		{
			dprintf(CRITICAL, "MMC erase failed\n");
			return 1;
		}

		blk_addr += blks_to_erase;

		if (unaligned_blks)
		{
			dprintf(SPEW, "Handling unaligned tail blocks\n");
			if (plr_mmc_zero_out(dev_num, dev, blk_addr, unaligned_blks))
				return 1;
		}

	}

	return 0;
}

/* PACKED CMD */
static uint32_t plr_mmc_packed_add_list(struct mmc_device *dev, uint32_t start, lbaint_t blkcnt, void *buf, uint8_t rw)
{
    struct mmc_req *req_info;
	struct mmc_card* card;
    enum mmc_packed_cmd packed_cmd = (rw == WRITE) ? MMC_PACKED_WRITE : MMC_PACKED_READ;
	card = &dev->card;

	if(!dev->config.packed_event_en)
        return MMC_PACKED_PASS;

    if (card->packed_info.packed_data == NULL)
        return MMC_PACKED_PASS;

    /* check request size */
    if (blkcnt > card->mmc_max_blk_cnt)
        return MMC_PACKED_INVALID;

    /* check max block size */
    if (card->packed_info.packed_blocks + blkcnt > card->mmc_max_blk_cnt) {
        if (card->packed_info.packed_num > 0)
            return MMC_PACKED_BLOCKS_OVER;
        else
            return MMC_PACKED_PASS;
    }

    /* check switching packed command */
    if (card->packed_info.packed_num > 0) {
        if (card->packed_info.packed_cmd != packed_cmd)
            return MMC_PACKED_CHANGE;
    }

    req_info = (struct mmc_req*)malloc(sizeof(struct mmc_req));
    memset (req_info, 0, sizeof(struct mmc_req));

    req_info->start = start;
    req_info->blkcnt = blkcnt;

    if (rw == WRITE)
        req_info->src = (const char*)buf;
    else
        req_info->dest = (char*)buf;


    list_initialize(&req_info->req_list);

    list_add_tail(&card->packed_info.packed_list, &req_info->req_list);
    if (card->packed_info.packed_num < 1) {
        card->packed_info.packed_start_sector = req_info->start;
        card->packed_info.packed_cmd = packed_cmd;
    }

    card->packed_info.packed_blocks += blkcnt;
    card->packed_info.packed_num++;

    /* check packed full state */
    if (rw == WRITE) {
        if (card->packed_info.packed_num >= dev->config.max_packed_writes)
            return MMC_PACKED_COUNT_FULL;
    }
    else {
        if (card->packed_info.packed_num >= dev->config.max_packed_reads)
            return MMC_PACKED_COUNT_FULL;
    }

    if (card->packed_info.packed_blocks >= card->mmc_max_blk_cnt)
        return MMC_PACKED_BLOCKS_FULL;

    return MMC_PACKED_ADD;
}

uint32_t plr_packed_add_list(int32_t dev_num, uint32_t start, lbaint_t blkcnt, void *buf, uint8_t rw)
{
	struct mmc_device *dev;

	dev = (struct mmc_device*)find_mmc_device(dev_num);
	if (!dev)
		return 0;

	return plr_mmc_packed_add_list(dev, start, blkcnt, buf, rw);
}

static void plr_mmc_make_packed_header(struct mmc_device *dev)
{
	int i = 1;
	struct mmc_req *req;
	struct mmc_card* card = &dev->card;
	uint32_t  *packed_cmd_hdr = card->packed_info.packed_cmd_hdr;
	packed_cmd_hdr[0] = (card->packed_info.packed_num << 16) |
		( ((card->packed_info.packed_cmd == MMC_PACKED_READ) ?
		   PACKED_CMD_RD : PACKED_CMD_WR ) << 8 ) |
		PACKED_CMD_VER;

	list_for_every_entry(&card->packed_info.packed_list, req, struct mmc_req, req_list) {
		packed_cmd_hdr[(i * 2)] = req->blkcnt;
		packed_cmd_hdr[((i * 2)) + 1] = req->start;
		i++;
	}
}

static void plr_mmc_init_packed_info(struct mmc_device *dev)
{
	struct mmc_card *card = &dev->card;
    memset (&card->packed_info, 0,
        ( sizeof(struct mmc_packed) - sizeof(card->packed_info.packed_data)));
    list_initialize(&card->packed_info.packed_list);
}

static uint32_t plr_mmc_packed_request(struct mmc_device *dev)
{
	struct mmc_card* card = &dev->card;
	uint32_t ret = 0;
	struct mmc_req *req;

	struct mmc_packed* packed_info = &card->packed_info;
	char *buf = packed_info->packed_data;

	if (packed_info->packed_blocks < 1 ||
			packed_info->packed_num < 1)
		return 0;

	if (!buf) {
		printf ("[%s] buf is NULL!\n", __func__);
		return 0;
	}

	statistic_mmc_request_start();
	if (packed_info->packed_num < 2) {
		req = list_peek_head_type(&packed_info->packed_list, struct mmc_req, req_list);

		/*
		 * Flush the cache before handing over the data to
		 * storage driver
		 */
		arch_clean_invalidate_cache_range((addr_t)buf, req->blkcnt*card->block_size + sizeof(packed_info->packed_cmd_hdr));

		if (packed_info->packed_cmd == MMC_PACKED_READ) {
			ret = mmc_sdhci_read(dev, buf, req->start, req->blkcnt);
//			printf("data = 0x%lx\n", *((uint32_t *)buf));
		} else {
			ret = mmc_sdhci_write(dev, (void*)(buf + sizeof(packed_info->packed_cmd_hdr)), req->start, req->blkcnt);
		}
		if(!ret) {
			ret = req->blkcnt;
		}
	}
	else {
		plr_mmc_make_packed_header(dev);

		if (card->packed_info.packed_cmd == MMC_PACKED_READ) {
			/*
			 * Flush the cache before handing over the data to
			 * storage driver
			 */
			arch_clean_invalidate_cache_range((addr_t)buf, (packed_info->packed_blocks+1)*card->block_size);

			req = list_peek_head_type(&packed_info->packed_list, struct mmc_req, req_list);
			ret = plr_mmc_sdhci_read_packed(dev, req->dest);
		}
		else {
			memcpy(buf, packed_info->packed_cmd_hdr, sizeof(packed_info->packed_cmd_hdr));
			/*
			 * Flush the cache before handing over the data to
			 * storage driver
			 */
			arch_clean_invalidate_cache_range((addr_t)buf, (packed_info->packed_blocks+1)*card->block_size);
			ret = plr_mmc_sdhci_write_packed(dev, buf);
		}

	}

	// Exclude a header size (1 sector)
	if (card->packed_info.packed_cmd == MMC_PACKED_READ) {
		statistic_mmc_request_done(STAT_MMC_READ, card->packed_info.packed_blocks);
	}
	else {
		statistic_mmc_request_done(STAT_MMC_WRITE, card->packed_info.packed_blocks);
	}

	while(!list_is_empty(&packed_info->packed_list)) {
		req = list_remove_head_type(&packed_info->packed_list, struct mmc_req, req_list);
		free(req);
	}

	plr_mmc_init_packed_info(dev);

	return ret;
}

uint32_t plr_packed_send_list(int32_t dev_num)
{
	struct mmc_device *dev;

	dev = (struct mmc_device*)find_mmc_device(dev_num);
	if (!dev)
		return 0;

	return plr_mmc_packed_request(dev);
}

void* plr_packed_create_buf(int32_t dev_num, void *buf)
{
	struct mmc_device *dev;
	struct mmc_card *card;
	uint buf_size = 0;

	dev = (struct mmc_device*)find_mmc_device(dev_num);
	if (!dev)
		return 0;

	card = &dev->card;

	if (buf) {
		card->packed_info.packed_data = (char*)buf;
		return card->packed_info.packed_data + sizeof(card->packed_info.packed_cmd_hdr);
	}

	buf_size = (card->mmc_max_blk_cnt * card->csd.write_blk_len) + sizeof(card->packed_info.packed_cmd_hdr);

	if (card->packed_info.packed_data == NULL) {
		card->packed_info.packed_data =
			(char*)malloc(buf_size);
	}

	if (!card->packed_info.packed_data) {
		printf ("[%s] malloc(%lu) failed!!!!!!!!\n", __func__, buf_size);
		return 0;
	}
	card->packed_info.packed_data_size = buf_size;

	// joys,2014.12.02 Add packed header offset
	return card->packed_info.packed_data + sizeof(card->packed_info.packed_cmd_hdr);
}

uint32_t plr_packed_delete_buf(int32_t dev_num)
{
	struct mmc_device *dev;
	struct mmc_card *card;
	struct mmc_req *req = NULL;

	dev = (struct mmc_device*)find_mmc_device(dev_num);
	if (!dev)
		return 0;

	card = &dev->card;

	if (card->packed_info.packed_data_size > 0 && card->packed_info.packed_data) {
		free(card->packed_info.packed_data);
		card->packed_info.packed_data = NULL;
		card->packed_info.packed_data_size = 0;
	}
	else
		card->packed_info.packed_data = NULL;

	while(!list_is_empty(&card->packed_info.packed_list)) {
		req = list_remove_head_type(&card->packed_info.packed_list, struct mmc_req, req_list);
		free(req);
	}
	plr_mmc_init_packed_info(dev);
	return 0;

}

struct mmc_packed* plr_get_packed_info(int32_t dev_num)
{
	struct mmc_device *dev;

	dev = (struct mmc_device*)find_mmc_device(dev_num);
	if (!dev)
		return NULL;

	return &dev->card.packed_info;
}

uint32_t plr_get_packed_count(int32_t dev_num)
{
	struct mmc_device *dev;

	dev = (struct mmc_device*)find_mmc_device(dev_num);
	if (!dev)
		return 0;

	return dev->card.packed_info.packed_num;
}

uint32_t plr_get_packed_max_sectors(int32_t dev_num, uint8_t rw)
{
	struct mmc_device *dev;
	struct mmc_card *card;

	dev = (struct mmc_device*)find_mmc_device(dev_num);
	if (!dev)
		return 0;

	card = &dev->card;

	uint32_t block_size = (rw == WRITE) ? card->csd.write_blk_len : card->csd.read_blk_len;

	if (card->packed_info.packed_data_size)
		return (card->packed_info.packed_data_size / block_size);
	else
		return card->mmc_max_blk_cnt;
}

int32_t plr_cache_flush(int32_t dev_num)
{
	struct mmc_device *dev;
	int ret = 0;

	dev = (struct mmc_device*)find_mmc_device(dev_num);

	statistic_mmc_request_start();
	ret = mmc_switch_cmd(&dev->host, &dev->card, MMC_ACCESS_WRITE, MMC_EXT_FLUSH_CACHE, 1);
	if (ret && ret != -INTERNAL_POWEROFF_FLAG) {
		ret = -PLR_EIO;
	}
	statistic_mmc_request_done(STAT_MMC_CACHE_FLUSH, 0);
	return ret;
}

int32_t plr_cache_ctrl(int32_t dev_num, uint8_t enable)
{
	struct mmc_device *dev;
	int ret = 0;

	dev = (struct mmc_device*)find_mmc_device(dev_num);

	ret = mmc_switch_cmd(&dev->host, &dev->card, MMC_ACCESS_WRITE, MMC_EXT_CACHE_CTRL, enable);
	if(ret){
		ret = -PLR_EIO;
	} else {
		dev->config.cache_ctrl = enable;
	}

	return ret;
}

int32_t emmc_set_internal_info(int32_t dev_num, void* internal_info)
{
	internal_info_t *rev_int_info = NULL;
	struct mmc_device *dev;
	struct mmc_card *card;

	dev = (struct mmc_device*)find_mmc_device(dev_num);

	if (!dev)
		return -1;

	card = &dev->card;

	// if NULL then initialize
	if (internal_info == NULL) {
		card->internal_info.action = INTERNAL_ACTION_NONE;
		memset(&card->internal_info.poff_info, 0, sizeof(poweroff_info_t));
	} else {
		rev_int_info = (internal_info_t*)internal_info;
		card->internal_info.action = rev_int_info->action;
		memcpy(&card->internal_info.poff_info, &rev_int_info->poff_info, sizeof(poweroff_info_t));
	}

//	printf_internal_info(&mmc->internal_info);

	return 0;
}

int32_t emmc_erase_for_poff(int32_t dev_num, uint32_t start_sector, uint32_t len, int type)
{
	struct mmc_device *dev;

	dev = (struct mmc_device*)find_mmc_device(dev_num);

	if (!dev)
		return -1;

	return plr_emmc_erase_for_poff(dev, start_sector, len, type);
}
#endif
