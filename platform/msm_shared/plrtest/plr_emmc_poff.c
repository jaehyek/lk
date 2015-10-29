#include "plr_common.h"
#include "plr_emmc_poff.h"
#include "plr_calibration.h"
#include "plr_emmc_internal_state.h"
#include "plr_lk_wrapper.h"
#include <mmc_wrapper.h>

// joys,2015.06.22
#define PLR_TIMEOUT 0x1FFFFFFF
//
// static vaiables for calibratioin
// @sj 150702

extern s64 _ticks_sod;		// start of dma

/***************************************************************************************************//**
*
* @brief	Internal poweroff
*
*******************************************************************************************************/

// DMA transfer section --------------------------------------------------------------------
static int poff_delay_while_transfer(struct sdhci_host *host, uint cmd_arg, uint blocks)
{
	struct mmc_card *card = MMC_CARD(host);
	poweroff_info_t *poff_info = &card->internal_info.poff_info;

	mdelay((u32)poff_info->poff_value);
	do_internal_power_control(EMMC_DEV_NUM, 0);

	plr_info_highlight("\nPower off while DMA transferring. delay: %07llu ms size : %04d KB sector : 0x%07X\n",
		poff_info->poff_value, blocks / 2, cmd_arg);

	g_plr_state.poweroff_pos = cmd_arg;
	card->internal_info.last_poff_lsn = cmd_arg;

	return -(INTERNAL_POWEROFF_FLAG);
}

static int poff_timing_while_transfer(struct sdhci_host *host, uint cmd_arg, uint blocks)
{
	u64 				ticcnt = 0;
	int 				timeout = PLR_TIMEOUT;
//	uint32_t			int_status = 0;
	uint32_t	busy_state;
	struct mmc_card *card = MMC_CARD(host);
	poweroff_info_t *poff_info = &card->internal_info.poff_info;

	do {
		busy_state = REG_READ32(host, SDHCI_PRESENT_STATE_REG);
		busy_state &= SDHCI_STATE_WR_TRANS_ACT_MASK;

//		int_status = REG_READ16(host, SDHCI_NRML_INT_STS_REG);

//		if (int_status & SDHCI_INT_STS_TRANS_COMPLETE)
//			break;
		if(!busy_state)
			break;

		ticcnt = get_tick_count64() - (u64)_ticks_sod;

		if (ticcnt >= poff_info->poff_value)
		{
			do_internal_power_control(EMMC_DEV_NUM, 0);
			plr_info_highlight("\nPower off while DMA transferring : %07u us size : %04d KB sector : 0x%07X\n",
				get_tick2usec(ticcnt), blocks / 2, cmd_arg);
			g_plr_state.poweroff_pos = cmd_arg;
			card->internal_info.last_poff_lsn = cmd_arg;

			return -(INTERNAL_POWEROFF_FLAG);
		}

	} while (timeout--);

	return 0;
}

static int poweroff_while_transfer(struct sdhci_host *host, uint cmd_arg, uint blocks)
{
	int ret = 0;
	struct mmc_card *card = MMC_CARD(host);
	poweroff_info_t *poff_info = &card->internal_info.poff_info;

	switch (poff_info->poff_type)
	{
		case POFF_TYPE_DELAY:
			ret = poff_delay_while_transfer(host, cmd_arg, blocks);
			break;

		case POFF_TYPE_TIMING:
			ret = poff_timing_while_transfer(host, cmd_arg, blocks);
			break;

		default:
			break;
	}

	return ret;
}

// eMMC busy section -----------------------------------------------------------------------
static int poff_delay_while_busy(struct sdhci_host *host, uint cmd_arg, uint blocks)
{
	struct mmc_card *card = MMC_CARD(host);
	poweroff_info_t *poff_info = &card->internal_info.poff_info;

	mdelay((u32)poff_info->poff_value);
	do_internal_power_control(EMMC_DEV_NUM, 0);

	plr_info_highlight("\nPower off while busy checking. delay: %07llu ms size : %04d KB sector : 0x%07X\n",
		poff_info->poff_value, blocks / 2, cmd_arg);

	g_plr_state.poweroff_pos = cmd_arg;
	card->internal_info.last_poff_lsn = cmd_arg;

	return -(INTERNAL_POWEROFF_FLAG);
}

static int poff_timing_while_busy(struct sdhci_host *host, uint cmd_arg, uint blocks)
{
	u64 ticcnt = 0;
	uint32_t busy_state = 0;
	int timeout = PLR_TIMEOUT;
	struct mmc_card *card = MMC_CARD(host);
	poweroff_info_t *poff_info = &card->internal_info.poff_info;

	do {

		busy_state = REG_READ32(host, SDHCI_PRESENT_STATE_REG);
		busy_state &= SDHCI_STATE_LEVEL_DAT0_MASK;

		ticcnt = get_tick_count64() - (u64)_ticks_sod;

		if (busy_state == 0) {
			//SJD_DEBUG_PRINTF("(tick, poff_info->poff_value) = (%lld, %lld)\n", tick, poff_info->poff_value);
			if (ticcnt >= poff_info->poff_value) {
				do_internal_power_control(EMMC_DEV_NUM, 0);

				plr_info_highlight("\nPower off while busy: %07u us size : %04d KB sector : 0x%07X\n",
						get_tick2usec(ticcnt),	blocks / 2,  cmd_arg);


				g_plr_state.poweroff_pos = cmd_arg;
				card->internal_info.last_poff_lsn = cmd_arg;

				return -(INTERNAL_POWEROFF_FLAG);
			}
		}
		else {
			break;
		}

	} while (timeout--);

	return 0;
}

static int poweroff_while_busy(struct sdhci_host *host, uint cmd_arg, uint blocks)
{
	int ret = 0;
	struct mmc_card *card = MMC_CARD(host);
	poweroff_info_t *poff_info = &card->internal_info.poff_info;

	switch (poff_info->poff_type)
	{
		case POFF_TYPE_DELAY:
			ret = poff_delay_while_busy(host, cmd_arg, blocks);
			break;

		case POFF_TYPE_TIMING:
			ret = poff_timing_while_busy(host, cmd_arg, blocks);
			break;

		default:
			break;
	}

	return ret;

}

// Cache flush busy section -----------------------------------------------------------------------
static int poff_delay_cache_flush(struct sdhci_host *host, uint cmd_arg, uint blocks)
{
	struct mmc_card *card = MMC_CARD(host);
	poweroff_info_t *poff_info = &card->internal_info.poff_info;

	mdelay((u32)poff_info->poff_value);
	do_internal_power_control(EMMC_DEV_NUM, 0);

	plr_info_highlight("\nPower off while cache flush. delay: %07llu ms\n",
		poff_info->poff_value);

	g_plr_state.poweroff_pos = 0;
	card->internal_info.last_poff_lsn = 0;

	return -(INTERNAL_POWEROFF_FLAG);;
}

static int poff_timing_cache_flush(struct sdhci_host *host, uint cmd_arg, uint blocks)
{
	u64 				ticcnt = 0;
	uint32_t 			busy_state = 0;
	int 				timeout = PLR_TIMEOUT;
	struct mmc_card *card = MMC_CARD(host);
	poweroff_info_t *poff_info = &card->internal_info.poff_info;

	do {

		busy_state = REG_READ32(host, SDHCI_PRESENT_STATE_REG);
		busy_state &= SDHCI_STATE_LEVEL_DAT0_MASK;

		ticcnt = get_tick_count64() - (u64)_ticks_sod;

		if (busy_state == 0) {
			//SJD_DEBUG_PRINTF("(tick, poff_info->poff_value) = (%lld, %lld)\n", tick, poff_info->poff_value);

			if (ticcnt >= poff_info->poff_value) {
				do_internal_power_control(EMMC_DEV_NUM, 0);

				plr_info_highlight("\nPower off while cache flush: %07u us\n", get_tick2usec(ticcnt));

				g_plr_state.poweroff_pos = 0;
				card->internal_info.last_poff_lsn = 0;

				return -(INTERNAL_POWEROFF_FLAG);
			}
		}
		else {
			break;
		}

	} while (timeout--);

	return 0;
}

static int poweroff_cache_flush(struct sdhci_host *host, uint cmd_arg, uint blocks)
{
	int ret = 0;
	struct mmc_card *card = MMC_CARD(host);
	poweroff_info_t *poff_info = &card->internal_info.poff_info;

	switch (poff_info->poff_type)
	{
		case POFF_TYPE_DELAY:
			ret = poff_delay_cache_flush(host, cmd_arg, blocks);
			break;

		case POFF_TYPE_TIMING:
			ret = poff_timing_cache_flush(host, cmd_arg, blocks);
			break;

		default:
			break;
	}

	return ret;
}

// Busy after erase section -----------------------------------------------------------------------
static int poff_delay_erase(struct sdhci_host *host, uint cmd_arg, uint blocks)
{
	char erase_type[8] = {0, };
	struct mmc_card *card = MMC_CARD(host);
	poweroff_info_t *poff_info = &card->internal_info.poff_info;

	mdelay((u32)poff_info->poff_value);
	do_internal_power_control(EMMC_DEV_NUM, 0);

	if (cmd_arg == NORMAL_ERASE) {
		sprintf (erase_type, "%s", "Erase");
	}
	else if (cmd_arg == TRIM) {
		sprintf (erase_type, "%s", "Trim");
	}
	else if (cmd_arg == DISCARD) {
		sprintf (erase_type, "%s", "Discard");
	}
	else {
		sprintf (erase_type, "%s", "Erase..");
	}

	plr_info_highlight("\nPower off while %s. delay: %07llu ms\n",
		erase_type, poff_info->poff_value);


	g_plr_state.poweroff_pos = 0;
	card->internal_info.last_poff_lsn = 0;

	return -(INTERNAL_POWEROFF_FLAG);;
}

static int poff_timing_erase(struct sdhci_host *host, uint cmd_arg, uint blocks)
{
	plr_info("[%s] does not yet support!\n", __func__);
	return 0;
}

static int poweroff_erase(struct sdhci_host *host, uint cmd_arg, uint blocks)
{
	int ret = 0;
	struct mmc_card *card = MMC_CARD(host);
	poweroff_info_t *poff_info = &card->internal_info.poff_info;

	switch (poff_info->poff_type)
	{
		case POFF_TYPE_DELAY:
			ret = poff_delay_erase(host, cmd_arg, blocks);
			break;

		case POFF_TYPE_TIMING:
			ret = poff_timing_erase(host, cmd_arg, blocks);
			break;

		default:
			break;
	}

	return ret;
}

// Busy after sanitize section -----------------------------------------------------------------------
static int poff_delay_sanitize(struct sdhci_host *host, uint cmd_arg, uint blocks)
{
	struct mmc_card *card = MMC_CARD(host);
	poweroff_info_t *poff_info = &card->internal_info.poff_info;

	mdelay((u32)poff_info->poff_value);
	do_internal_power_control(EMMC_DEV_NUM, 0);

	plr_info_highlight("\nPower off while sanitize. delay: %07llu ms\n",
			poff_info->poff_value);

	g_plr_state.poweroff_pos = 0;
	card->internal_info.last_poff_lsn = 0;

	return -(INTERNAL_POWEROFF_FLAG);;
}

static int poff_timing_sanitize(struct sdhci_host *host, uint cmd_arg, uint blocks)
{
	plr_info("[%s] does not yet support!\n", __func__);
	return 0;
}

static int poweroff_sanitize(struct sdhci_host *host, uint cmd_arg, uint blocks)
{
	int ret = 0;
	struct mmc_card *card = MMC_CARD(host);
	poweroff_info_t *poff_info = &card->internal_info.poff_info;

	switch (poff_info->poff_type)
	{
		case POFF_TYPE_DELAY:
			ret = poff_delay_sanitize(host, cmd_arg, blocks);
			break;

		case POFF_TYPE_TIMING:
			ret = poff_timing_sanitize(host, cmd_arg, blocks);
			break;

		default:
			break;
	}

	return ret;
}

/***************************************************************************************************//**
*
* @brief	Attach a call function
*
*******************************************************************************************************/

int process_attach_poweroff(struct processing_manage *pro_manage, struct sdhci_host *host)
{
	pro_manage->processing_phase_request[PROCESS_PHA_DATA_TRANS] 		= poweroff_while_transfer;
	pro_manage->processing_phase_request[PROCESS_PHA_WRITE_BUSY] 		= poweroff_while_busy;
	pro_manage->processing_phase_request[PROCESS_PHA_CACHE_FLUSH] 		= poweroff_cache_flush;
	pro_manage->processing_phase_request[PROCESS_PHA_ERASE] 			= poweroff_erase;
	pro_manage->processing_phase_request[PROCESS_PHA_SANITIZE] 			= poweroff_sanitize;

	return 0;
}

