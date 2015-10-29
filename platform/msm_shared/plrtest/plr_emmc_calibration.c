#include "plr_common.h"
#include "plr_emmc_internal.h"
#include "plr_calibration.h"
#include "plr_emmc_internal_state.h"
#include "plr_lk_wrapper.h"
#include <mmc_wrapper.h>

// joys,2015.06.22
#define PLR_TIMEOUT 0x1FFFFFFF
#define COMMAND_TIMEOUT (0x1000000)

//
// static vaiables for calibratioin
// @sj 150702

static tick_s _ticks = {-1, -1, -1};	// and other meanningful ticks
extern s64 _ticks_sod;		// start of dma

/***************************************************************************************************//**
*
* @brief	Internal calibration
*
*******************************************************************************************************/
static int timing_calibration_while_transfer(struct sdhci_host *host, uint cmd_arg, uint blocks)
{
//	uint32_t 	int_status = 0;
	int 		timeout = PLR_TIMEOUT;
	uint32_t	busy_state;
	_ticks_sod = (u32)get_tick_count64();						// the time DMA transfer start
//	uint32_t	high_cnt = 0;;
//	uint32_t	low_cnt = 0;

	do {
		busy_state = REG_READ32(host, SDHCI_PRESENT_STATE_REG);
		busy_state &= SDHCI_STATE_WR_TRANS_ACT_MASK;
/*
		if(!(timeout&0xFFF) || timeout == PLR_TIMEOUT) {
			printf("write_active = 0x%lx\n", busy_state);
		}
*/
//		int_status = REG_READ16(host, SDHCI_NRML_INT_STS_REG);

//		if (int_status  & SDHCI_INT_STS_TRANS_COMPLETE) {
		if (!busy_state) {

			_ticks.eod = (s32)(get_tick_count64() - (u64)_ticks_sod);

			if( calib_get_last_calib_phase() == PROCESS_PHA_DATA_TRANS)
			{
				calib_calc_ticks_min_max(blocks, &_ticks);		// @sj 150702
			}

			break;
		}
	} while (timeout--);

	return 0;
}

static int timing_calibration_while_busy(struct sdhci_host *host, uint cmd_arg, uint blocks)
{
	uint32_t busy_state = 0;
	int timeout = PLR_TIMEOUT;

	do {
		busy_state = REG_READ32(host, SDHCI_PRESENT_STATE_REG);

		busy_state &= SDHCI_STATE_LEVEL_DAT0_MASK;

		if (busy_state) {
			_ticks.eob = (s32)(get_tick_count64() - (u64)_ticks_sod);

			if( calib_get_last_calib_phase() == PROCESS_PHA_WRITE_BUSY)
			{
				if(!IS_CACHEON_TEST) {
					calib_calc_ticks_min_max(blocks, &_ticks);		// @sj 150702
				}
			}

			break;
		}

	} while (timeout--);

	return 0;
}

static int timing_calibration_cache_flush(struct sdhci_host *host, uint cmd_arg, uint blocks)
{
	uint32_t busy_state = 0;
	int timeout = PLR_TIMEOUT;

	do {
		busy_state = REG_READ32(host, SDHCI_PRESENT_STATE_REG);
		busy_state &= SDHCI_STATE_LEVEL_DAT0_MASK;

/*
		if(!(timeout&0xFFF) || timeout == PLR_TIMEOUT) {
			printf("busy_state = 0x%lx\n", busy_state);
		}
*/
		if (busy_state) {
			_ticks.eof = (s32)(get_tick_count64() - (u64)_ticks_sod);

			if( calib_get_last_calib_phase() == PROCESS_PHA_CACHE_FLUSH )
			{
				uint last_request_len = calib_get_cached_write_size_before_flush();		// @sj 150702 the way of know data size in cache flush
				calib_calc_ticks_min_max(last_request_len, &_ticks);					// @sj
			}

			break;
		}

	} while (timeout--);

	return 0;
}

/***************************************************************************************************//**
*
* @brief	Attach a call function
*
*******************************************************************************************************/

int process_attach_calibration(struct processing_manage *pro_manage, struct sdhci_host *host)
{
	int ret = 0;

	// TBD ...
	struct mmc_card *card = MMC_CARD(host);
	internal_info_t *int_info = &card->internal_info;

	switch (int_info->action)
	{
		// Timing Calibration
		case INTERNAL_ACTION_TIMING_CALIBRATION:
			pro_manage->processing_phase_request[PROCESS_PHA_DATA_TRANS]  		= timing_calibration_while_transfer;
			pro_manage->processing_phase_request[PROCESS_PHA_WRITE_BUSY] 		= timing_calibration_while_busy;
			pro_manage->processing_phase_request[PROCESS_PHA_CACHE_FLUSH] 		= timing_calibration_cache_flush;
			pro_manage->processing_phase_request[PROCESS_PHA_ERASE] 			= NULL;
			pro_manage->processing_phase_request[PROCESS_PHA_SANITIZE] 			= NULL;
			break;

		default:
			ret = -1;
			break;
	}

	return ret;
}

