/* (c) 2015, The Linux Foundation. All rights reserved.
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

#include <debug.h>
#include <platform/iomap.h>
#include <reg.h>
#include <target.h>
#include <platform.h>
#include <uart_dm.h>
#include <mmc.h>
#include <platform/gpio.h>
#include <dev/keys.h>
#include <spmi_v2.h>
#include <pm8x41.h>
#include <pm8x41_hw.h>
#include <board.h>
#include <baseband.h>
#include <hsusb.h>
#include <scm.h>
#include <platform/gpio.h>
#include <platform/gpio.h>
#include <platform/irqs.h>
#include <platform/clock.h>
#include <platform/timer.h>
#include <crypto5_wrapper.h>
#include <partition_parser.h>
#include <stdlib.h>
#include <rpm-smd.h>
#include <spmi.h>
#include <sdhci_msm.h>
#include <clock.h>

#include "target/display.h"

#ifdef TARGET_USES_CHARGERLOGO
#include <lge_ftm.h>
#endif

#ifdef LGE_WITH_SMEM_PON_STATUS
#include "lge_smem.h"
#endif
#if LONG_PRESS_POWER_ON
#include <shutdown_detect.h>
#endif

#ifdef LGE_WITH_COMMON
#include <lge_target_common.h>
#include "lge_bootmode.h"
#endif

#ifdef QPNP_HAPTIC_SUPPORT
#include <vibrator.h>
#endif

#define PMIC_ARB_CHANNEL_NUM    0
#define PMIC_ARB_OWNER_ID       0
#define TLMM_VOL_UP_BTN_GPIO    85

#ifdef QPNP_HAPTIC_SUPPORT
#define VIBRATE_TIME	150
#endif

#define FASTBOOT_MODE           0x77665500
#define RECOVERY_MODE           0x77665502
#define PON_SOFT_RB_SPARE       0x88F

#define CE1_INSTANCE            1
#define CE_EE                   1
#define CE_FIFO_SIZE            64
#define CE_READ_PIPE            3
#define CE_WRITE_PIPE           2
#define CE_READ_PIPE_LOCK_GRP   0
#define CE_WRITE_PIPE_LOCK_GRP  0
#define CE_ARRAY_SIZE           20

#ifdef LGE_WITH_SERIAL_NUMBER
#define HWIO_QFPROM_CORR_SERIAL_NUM_ADDR              0x5C008
#endif

struct mmc_device *dev;
#if PLRTEST_ENABLE
struct mmc_device *dev_emmc;
struct mmc_device *dev_sdcard;
#endif

static uint32_t mmc_pwrctl_base[] =
	{ MSM_SDC1_BASE, MSM_SDC2_BASE };

static uint32_t mmc_sdhci_base[] =
	{ MSM_SDC1_SDHCI_BASE, MSM_SDC2_SDHCI_BASE };

static uint32_t  mmc_sdc_pwrctl_irq[] =
	{ SDCC1_PWRCTL_IRQ, SDCC2_PWRCTL_IRQ };

void target_early_init(void)
{
#if WITH_DEBUG_UART
	uart_dm_init(2, 0, BLSP1_UART1_BASE);
#endif
}

#if PLRTEST_ENABLE
static void set_sdc_power_ctrl();

/* GPIO NUMBER */
#define GPIO_EMMC_IO_SW		26
#define GPIO_EMMC_VCC_SW	27
#define GPIO_EMMC_RESET_N	35
#define GPIO_EMMC_IO_BUCK	63
#define GPIO_EMMC_VCC_BUCK	64

#define GPIO_EXT_INT_28_6	16
#define GPIO_EXT_INT_28_7	17

/* GPIO_DIRECTION */
#define GPIO_STATE_LOW 0
#define GPIO_STATE_HIGH 2
static void config_mmc_power()
{
	gpio_tlmm_config(GPIO_EMMC_IO_BUCK, 0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_8MA, GPIO_DISABLE);
	gpio_tlmm_config(GPIO_EMMC_VCC_BUCK, 0, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA, GPIO_DISABLE);
	gpio_tlmm_config(GPIO_EMMC_IO_SW, 0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA, GPIO_DISABLE);
	gpio_tlmm_config(GPIO_EMMC_VCC_SW, 0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA, GPIO_DISABLE);
}

static void turn_on_mmc()
{
	gpio_set_dir(GPIO_EMMC_VCC_BUCK, GPIO_STATE_HIGH);
	gpio_set_dir(GPIO_EMMC_IO_BUCK, GPIO_STATE_HIGH);
	thread_sleep(10);
	gpio_set_dir(GPIO_EMMC_VCC_SW, GPIO_STATE_HIGH);
	gpio_set_dir(GPIO_EMMC_IO_SW, GPIO_STATE_HIGH);
	thread_sleep(10);
}

/*
static void reset_mmc()
{
	gpio_tlmm_config(GPIO_EMMC_RESET_N, 0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA, GPIO_DISABLE);
	gpio_set_dir(GPIO_EMMC_RESET_N, GPIO_STATE_HIGH);
	thread_sleep(10000);
}
*/

void turn_on_and_reinit_mmc()
{
	turn_on_mmc();
	mmc_card_reinit(dev_emmc);
}

void turn_off_mmc()
{
	gpio_set_dir(GPIO_EMMC_VCC_SW, GPIO_STATE_LOW);
	gpio_set_dir(GPIO_EMMC_IO_SW, GPIO_STATE_LOW);
	thread_sleep(10);
	gpio_set_dir(GPIO_EMMC_VCC_BUCK, GPIO_STATE_LOW);
	gpio_set_dir(GPIO_EMMC_IO_BUCK, GPIO_STATE_LOW);
	thread_sleep(10);
}

static void set_gpk0()
{
	/* gpio config */
	gpio_tlmm_config(GPIO_EXT_INT_28_6, 0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_6MA, GPIO_DISABLE);
	gpio_set_dir(GPIO_EXT_INT_28_6, GPIO_STATE_HIGH);

	gpio_tlmm_config(GPIO_EXT_INT_28_7, 0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_6MA, GPIO_DISABLE);
	gpio_set_dir(GPIO_EXT_INT_28_7, GPIO_STATE_LOW);

	thread_sleep(10);
}
#endif

static void set_sdc_power_ctrl()
{
	/* Drive strength configs for sdc pins */
	struct tlmm_cfgs sdc1_hdrv_cfg[] =
	{
		{ SDC1_CLK_HDRV_CTL_OFF,  TLMM_CUR_VAL_16MA, TLMM_HDRV_MASK, 0},
		{ SDC1_CMD_HDRV_CTL_OFF,  TLMM_CUR_VAL_10MA, TLMM_HDRV_MASK, 0},
		{ SDC1_DATA_HDRV_CTL_OFF, TLMM_CUR_VAL_10MA, TLMM_HDRV_MASK , 0},
	};

	/* Pull configs for sdc pins */
	struct tlmm_cfgs sdc1_pull_cfg[] =
	{
		{ SDC1_CLK_PULL_CTL_OFF,  TLMM_NO_PULL, TLMM_PULL_MASK, 0},
		{ SDC1_CMD_PULL_CTL_OFF,  TLMM_PULL_UP, TLMM_PULL_MASK, 0},
		{ SDC1_DATA_PULL_CTL_OFF, TLMM_PULL_UP, TLMM_PULL_MASK, 0},
	};

	struct tlmm_cfgs sdc1_rclk_cfg[] =
	{
		{ SDC1_RCLK_PULL_CTL_OFF, TLMM_PULL_DOWN, TLMM_PULL_MASK, 0},
	};

	/* Set the drive strength & pull control values */
	tlmm_set_hdrive_ctrl(sdc1_hdrv_cfg, ARRAY_SIZE(sdc1_hdrv_cfg));
	tlmm_set_pull_ctrl(sdc1_pull_cfg, ARRAY_SIZE(sdc1_pull_cfg));
	tlmm_set_pull_ctrl(sdc1_rclk_cfg, ARRAY_SIZE(sdc1_rclk_cfg));
}

void target_sdc_init()
{
	struct mmc_config_data config;

#if PLRTEST_ENABLE
	config_mmc_power();
	turn_on_mmc();
	set_gpk0();
#endif

	/* Set drive strength & pull ctrl values */
	set_sdc_power_ctrl();

	/* Try slot 1*/
	config.slot          = 1;
	config.bus_width     = DATA_BUS_WIDTH_8BIT;
	config.max_clk_rate  = MMC_CLK_192MHZ;
	config.sdhc_base     = mmc_sdhci_base[config.slot - 1];
	config.pwrctl_base   = mmc_pwrctl_base[config.slot - 1];
	config.pwr_irq       = mmc_sdc_pwrctl_irq[config.slot - 1];
	config.hs400_support = 1;

#if PLRTEST_ENABLE
	if (!(dev_emmc = mmc_init(&config)))
	{
		dprintf(CRITICAL, "eMMC init failed!");
		ASSERT(0);
	}

	/* Try slot 2 */
	config.slot          = 2;
	config.max_clk_rate  = MMC_CLK_200MHZ;
	config.sdhc_base     = mmc_sdhci_base[config.slot - 1];
	config.pwrctl_base   = mmc_pwrctl_base[config.slot - 1];
	config.pwr_irq       = mmc_sdc_pwrctl_irq[config.slot - 1];
	config.hs400_support = 0;

	if (!(dev_sdcard = mmc_init(&config))) {
		dprintf(CRITICAL, "sdcard init failed!\n");
		ASSERT(0);
	}

	dev = dev_sdcard;
#else
	if (!(dev = mmc_init(&config))) {
	/* Try slot 2 */
		config.slot          = 2;
		config.max_clk_rate  = MMC_CLK_200MHZ;
		config.sdhc_base     = mmc_sdhci_base[config.slot - 1];
		config.pwrctl_base   = mmc_pwrctl_base[config.slot - 1];
		config.pwr_irq       = mmc_sdc_pwrctl_irq[config.slot - 1];
		config.hs400_support = 0;

		if (!(dev = mmc_init(&config))) {
			dprintf(CRITICAL, "mmc init failed!");
			ASSERT(0);
		}
	}
#endif
}

#if PLRTEST_ENABLE
/* plrtest_target_mmc_device() is called from mmc operation code like as read/write.
 * dev_num = 0 : SD Card
 * dev_num = 1 : eMMC
 */
#define SDCARD_DEV_NUM	0
#define EMMC_DEV_NUM	1

void *find_mmc_device(uint32_t dev_num)
{
	if (dev_num == SDCARD_DEV_NUM)
		return (void *) dev_sdcard;
	else if (dev_num == EMMC_DEV_NUM)
		return (void *) dev_emmc;
	return (void *) NULL;
}
#endif

void *target_mmc_device()
{
	return (void *) dev;
}

#ifdef LGE_WITH_FASTBOOT_MENU
uint32_t target_power_key(void)
{
	return pm8x41_get_pwrkey_is_pressed();
}
#endif

/* Return 1 if vol_up pressed */
int target_volume_up(void)
{
	uint8_t status = 0;

	struct pm8x41_gpio gpio;

	/* Configure the GPIO */
	gpio.direction = PM_GPIO_DIR_IN;
	gpio.function = 0;
	gpio.pull = PM_GPIO_PULL_UP_30;
	gpio.vin_sel = 2;

	pm8x41_gpio_config(5, &gpio);

	/* Wait for the pmic gpio config to take effect */
	thread_sleep(1);

	/* Get status of GPIO_5 */
	pm8x41_gpio_get(5, &status);

	return !status; /* active low */
}

/* Return 1 if vol_down pressed */
uint32_t target_volume_down()
{
#ifdef LGE_WITH_SELECT_MODE
	if (pm8x41_resin_status())
		return 1;
	else
		return 0;
#else
	/* Volume down button tied in with PMIC RESIN. */
	return pm8x41_resin_status();
#endif
}

void target_keystatus()
{
	keys_init();

	if(target_volume_down())
		keys_post_event(KEY_VOLUMEDOWN, 1);

	if(target_volume_up())
		keys_post_event(KEY_VOLUMEUP, 1);
}

/* Configure PMIC and Drop PS_HOLD for shutdown */
void shutdown_device()
{
	dprintf(CRITICAL, "Going down for shutdown.\n");

	/* Configure PMIC for shutdown */
	pm8x41_reset_configure(PON_PSHOLD_SHUTDOWN);

	/* Drop PS_HOLD for MSM */
	writel(0x00, MPM2_MPM_PS_HOLD);

	mdelay(5000);

	dprintf(CRITICAL, "shutdown failed\n");

	ASSERT(0);
}


void target_init(void)
{
	dprintf(INFO, "target_init()\n");

	spmi_init(PMIC_ARB_CHANNEL_NUM, PMIC_ARB_OWNER_ID);

	target_keystatus();

	target_sdc_init();

/*	if (partition_read_table())
	{
		dprintf(CRITICAL, "Error reading the partition table info\n");
		ASSERT(0);
	}
*/
#ifdef LGE_WITH_COMMON
//	target_common_init();
#endif

#if LONG_PRESS_POWER_ON
	shutdown_detect();
#endif

#ifdef LGE_QCT_HW_CRYPTO
	target_crypto_init_params();
#else
	if (target_use_signed_kernel())
		target_crypto_init_params();
#endif

#if SMD_SUPPORT
	rpm_smd_init();
#endif

#ifdef QPNP_HAPTIC_SUPPORT
	/* turn on vibrator to indicate that phone is booting up to end user */
	vib_timed_turn_on(VIBRATE_TIME);
#endif
}

void target_serialno(unsigned char *buf)
{
	uint32_t serialno;
	if (target_is_emmc_boot()) {
		serialno = mmc_get_psn();
#ifdef LGE_WITH_SERIAL_NUMBER
		snprintf((char *)buf, 17, "%08x%08x",
			*REG32(HWIO_QFPROM_CORR_SERIAL_NUM_ADDR), serialno);
#else
		snprintf((char *)buf, 13, "%x", serialno);
#endif
	}
}

unsigned board_machtype(void)
{
	return LINUX_MACHTYPE_UNKNOWN;
}

/* Detect the target type */
void target_detect(struct board_data *board)
{
	/* This is already filled as part of board.c */
}

/* Detect the modem type */
void target_baseband_detect(struct board_data *board)
{
	uint32_t platform;

	platform = board->platform;

	switch(platform) {
	case MSM8952:
	case MSM8956:
	case MSM8976:
		board->baseband = BASEBAND_MSM;
		break;
	default:
		dprintf(CRITICAL, "Platform type: %u is not supported\n",platform);
		ASSERT(0);
	};
}

unsigned target_baseband()
{
	return board_baseband();
}

unsigned check_reboot_mode(void)
{
	uint32_t restart_reason = 0;

	/* Read reboot reason and scrub it */
	restart_reason = readl(RESTART_REASON_ADDR);
	writel(0x00, RESTART_REASON_ADDR);

	return restart_reason;
}

unsigned check_hard_reboot_mode(void)
{
	uint8_t hard_restart_reason = 0;
	uint8_t value = 0;

	/* Read reboot reason and scrub it
	  * Bit-5, bit-6 and bit-7 of SOFT_RB_SPARE for hard reset reason
	  */
	value = pm8x41_reg_read(PON_SOFT_RB_SPARE);
	hard_restart_reason = value >> 5;
	pm8x41_reg_write(PON_SOFT_RB_SPARE, value & 0x1f);

	return hard_restart_reason;
}

int set_download_mode(enum dload_mode mode)
{
#ifdef LGE_WITH_CRASH_HANDLER
	int ret = 0;
	ret = scm_dload_mode(mode);

	pm8x41_clear_pmic_watchdog();

	return ret;
#else
	return -1;
#endif
}

int emmc_recovery_init(void)
{
	return _emmc_recovery_init();
}

void reboot_device(unsigned reboot_reason)
{
	uint8_t reset_type = 0;
	uint32_t ret = 0;

	/* Need to clear the SW_RESET_ENTRY register and
	 * write to the BOOT_MISC_REG for known reset cases
	 */
	if(reboot_reason != DLOAD)
		scm_dload_mode(NORMAL_MODE);

	writel(reboot_reason, RESTART_REASON_ADDR);
#ifdef LGE_WITH_CRASH_HANDLER
	if (reboot_reason == DLOAD)
		writel(0x6d630600, RESTART_REASON_ADDR);
#endif

	/* For Reboot-bootloader and Dload cases do a warm reset
	 * For Reboot cases do a hard reset
	 */
	if((reboot_reason == FASTBOOT_MODE) || (reboot_reason == DLOAD) || (reboot_reason == RECOVERY_MODE))
		reset_type = PON_PSHOLD_WARM_RESET;
	else
		reset_type = PON_PSHOLD_HARD_RESET;

	pm8x41_reset_configure(reset_type);

	ret = scm_halt_pmic_arbiter();
	if (ret)
		dprintf(CRITICAL , "Failed to halt pmic arbiter: %d\n", ret);

	/* Drop PS_HOLD for MSM */
	writel(0x00, MPM2_MPM_PS_HOLD);

	mdelay(5000);

	dprintf(CRITICAL, "Rebooting failed\n");
}

#if USER_FORCE_RESET_SUPPORT
/* Return 1 if it is a force resin triggered by user. */
uint32_t is_user_force_reset(void)
{
	uint8_t poff_reason1 = pm8x41_get_pon_poff_reason1();
	uint8_t poff_reason2 = pm8x41_get_pon_poff_reason2();

	dprintf(SPEW, "poff_reason1: %d\n", poff_reason1);
	dprintf(SPEW, "poff_reason2: %d\n", poff_reason2);
	if (pm8x41_get_is_cold_boot() && (poff_reason1 == KPDPWR_AND_RESIN ||
							poff_reason2 == STAGE3))
		return 1;
	else
		return 0;
}
#endif

#define SMBCHG_USB_RT_STS 0x21310
#define USBIN_UV_RT_STS BIT(0)
#ifdef TARGET_USES_CHARGERLOGO
extern unsigned boot_into_chargerlogo;
#endif
unsigned target_pause_for_battery_charge(void)
{
#ifdef TARGET_USES_CHARGERLOGO
	uint8_t pon_reason = pm8x41_get_pon_reason();
	uint8_t poff_reason1 = pm8x41_get_pon_poff_reason1();
	uint8_t poff_reason2 = pm8x41_get_pon_poff_reason2();
	uint8_t is_cold_boot = pm8x41_get_is_cold_boot();

	dprintf(INFO, "%s : pon_reason is %d cold_boot:%d\n", __func__,
		pon_reason, is_cold_boot);

	dprintf(INFO, "PON_PON_REASON1:0x%x\n", pon_reason);
	dprintf(INFO, "PON_POFF_REASON1:0x%x\n", poff_reason1);
	dprintf(INFO, "PON_POFF_REASON2:0x%x\n", poff_reason2);

	#define USBIN_SRC_DET_STS_RT_STS 0x4
	#define WARM_RST_REASON2_AFP     0x8
	/* chargerlogo entering condition
	 * 1. PON_PON_REASON1 != HARD_RESET &&
	 * 2. PON_PON_REASON1 != KPDPWR_N &&
	 * 3. PON_PON_REASON1 = PON1 and USBIN ||
	 * 4. PON_POFF_REASON2 = UVLO and PON_PON_REASON1 = SMPL|PON1 and USBIN
	 */
	if ((!(pon_reason & PWR_ON_EVENT_KPDPWR_N)) &&
		(!(pon_reason & PWR_ON_EVENT_HARD_RESET))) {
		if (!!(pon_reason & PWR_ON_EVENT_PON1) ||
			  (!!(poff_reason2 & PWR_OFF2_EVENT_UVLO) &&
			  !!(pon_reason & (PWR_ON_EVENT_PON1|PWR_ON_EVENT_SMPL)))) {
			if (boot_into_chargerlogo == 1 &&
					bootmode_check_pif_detect() != 1 &&
					get_qem() != 1 &&
					get_charger_skip() != 1 )
				return 1;
		}
	}
	return 0;
#else
	uint8_t pon_reason = pm8x41_get_pon_reason();
	uint8_t is_cold_boot = pm8x41_get_is_cold_boot();
	bool usb_present_sts = !(USBIN_UV_RT_STS &
				pm8x41_reg_read(SMBCHG_USB_RT_STS));
	dprintf(INFO, "%s : pon_reason is:0x%x cold_boot:%d usb_sts:%d\n", __func__,
		pon_reason, is_cold_boot, usb_present_sts);
	/* In case of fastboot reboot,adb reboot or if we see the power key
	* pressed we do not want go into charger mode.
	* fastboot reboot is warm boot with PON hard reset bit not set
	* adb reboot is a cold boot with PON hard reset bit set
	*/
	if (is_cold_boot &&
			(!(pon_reason & HARD_RST)) &&
			(!(pon_reason & KPDPWR_N)) &&
			usb_present_sts)
		return 1;
	else
		return 0;
#endif
}

void target_uninit(void)
{
#if PLRTEST_ENABLE
	mmc_put_card_to_sleep(dev_emmc);
	mmc_put_card_to_sleep(dev_sdcard);
	sdhci_mode_disable(&dev_emmc->host);
	sdhci_mode_disable(&dev_sdcard->host);
#endif
	mmc_put_card_to_sleep(dev);
	sdhci_mode_disable(&dev->host);
	if (crypto_initialized())
		crypto_eng_cleanup();

	if (target_is_ssd_enabled())
		clock_ce_disable(CE1_INSTANCE);

#if SMD_SUPPORT
	rpm_smd_uninit();
#endif
}

#ifdef LGE_WITH_FASTBOOT_MENU
int target_is_production(void)
{
	return 1;
}
#endif

void target_usb_init(void)
{
	uint32_t val;

	/* Select and enable external configuration with USB PHY */
	ulpi_write(ULPI_MISC_A_VBUSVLDEXTSEL | ULPI_MISC_A_VBUSVLDEXT, ULPI_MISC_A_SET);

	/* Enable sess_vld */
	val = readl(USB_GENCONFIG_2) | GEN2_SESS_VLD_CTRL_EN;
	writel(val, USB_GENCONFIG_2);

	/* Enable external vbus configuration in the LINK */
	val = readl(USB_USBCMD);
	val |= SESS_VLD_CTRL;
	writel(val, USB_USBCMD);
}

void target_usb_stop(void)
{
	/* Disable VBUS mimicing in the controller. */
	ulpi_write(ULPI_MISC_A_VBUSVLDEXTSEL | ULPI_MISC_A_VBUSVLDEXT, ULPI_MISC_A_CLEAR);
}

static uint8_t splash_override;
/* Returns 1 if target supports continuous splash screen. */
int target_cont_splash_screen()
{
	uint8_t splash_screen = 0;
	if (!splash_override) {
		switch (board_hardware_id()) {
		case HW_PLATFORM_MTP:
		case HW_PLATFORM_SURF:
		case HW_PLATFORM_P1V_ATT_US:
			splash_screen = 1;
			break;
		default:
			splash_screen = 0;
			break;
		}
		dprintf(SPEW, "Target_cont_splash=%d\n", splash_screen);
	}
	return splash_screen;
}

void target_force_cont_splash_disable(uint8_t override)
{
        splash_override = override;
}

/* Do any target specific intialization needed before entering fastboot mode */
void target_fastboot_init(void)
{
	if (target_is_ssd_enabled()) {
		clock_ce_enable(CE1_INSTANCE);
		target_load_ssd_keystore();
	}
}

void target_load_ssd_keystore(void)
{
	uint64_t ptn;
	int      index;
	uint64_t size;
	uint32_t *buffer = NULL;

	if (!target_is_ssd_enabled())
		return;

	index = partition_get_index("ssd");

	ptn = partition_get_offset(index);
	if (ptn == 0){
		dprintf(CRITICAL, "Error: ssd partition not found\n");
		return;
	}

	size = partition_get_size(index);
	if (size == 0) {
		dprintf(CRITICAL, "Error: invalid ssd partition size\n");
		return;
	}

	buffer = memalign(CACHE_LINE, ROUNDUP(size, CACHE_LINE));
	if (!buffer) {
		dprintf(CRITICAL, "Error: allocating memory for ssd buffer\n");
		return;
	}

	if (mmc_read(ptn, buffer, size)) {
		dprintf(CRITICAL, "Error: cannot read data\n");
		free(buffer);
		return;
	}

	clock_ce_enable(CE1_INSTANCE);
	scm_protect_keystore(buffer, size);
	clock_ce_disable(CE1_INSTANCE);
	free(buffer);
}

crypto_engine_type board_ce_type(void)
{
	return CRYPTO_ENGINE_TYPE_HW;
}

/* Set up params for h/w CE. */
void target_crypto_init_params()
{
	struct crypto_init_params ce_params;

	/* Set up base addresses and instance. */
	ce_params.crypto_instance  = CE1_INSTANCE;
	ce_params.crypto_base      = MSM_CE1_BASE;
	ce_params.bam_base         = MSM_CE1_BAM_BASE;

	/* Set up BAM config. */
	ce_params.bam_ee               = CE_EE;
	ce_params.pipes.read_pipe      = CE_READ_PIPE;
	ce_params.pipes.write_pipe     = CE_WRITE_PIPE;
	ce_params.pipes.read_pipe_grp  = CE_READ_PIPE_LOCK_GRP;
	ce_params.pipes.write_pipe_grp = CE_WRITE_PIPE_LOCK_GRP;

	/* Assign buffer sizes. */
	ce_params.num_ce           = CE_ARRAY_SIZE;
	ce_params.read_fifo_size   = CE_FIFO_SIZE;
	ce_params.write_fifo_size  = CE_FIFO_SIZE;

	/* BAM is initialized by TZ for this platform.
	 * Do not do it again as the initialization address space
	 * is locked.
	 */
	ce_params.do_bam_init      = 0;

	crypto_init_params(&ce_params);
}

unsigned long long target_check_pon_reason(void)
{
	smem_power_on_status_type pon_status;

	pon_status.reasons = 0;

	if (smem_get_pon_status_info(&pon_status) == 0) {
		dprintf(CRITICAL, "PON_PON_PBL_STATUS:0x%x\n", pm8x41_reg_read(0x0807));
		dprintf(CRITICAL, "PON_PON_REASON1:0x%llx\n", pon_status.reasons & 0xFF);
		dprintf(CRITICAL, "PON_WARM_RESET_REASON1:0x%llx\n",
				(pon_status.reasons >> 16) & 0xFF);
		dprintf(CRITICAL, "PON_WARM_RESET_REASON2:0x%llx\n",
				(pon_status.reasons >> 24) & 0xFF);
		dprintf(CRITICAL, "PON_POFF_REASON1:0x%llx\n", (pon_status.reasons >> 32) & 0xFF);
		dprintf(CRITICAL, "PON_POFF_REASON2:0x%llx\n", (pon_status.reasons >> 40) & 0xFF);
		dprintf(CRITICAL, "PON_SOFT_RESET_REASON1:0x%llx\n",
				(pon_status.reasons >> 48) & 0xFF);
		dprintf(CRITICAL, "PON_SOFT_RESET_REASON2:0x%llx\n",
				(pon_status.reasons >> 56) & 0xFF);
		dprintf(CRITICAL, "TEMP_ALARM_STATUS1:0x%x\n", pm8x41_reg_read(0x2408));
	} else {
		dprintf(CRITICAL, "ERROR: unable to read smem for pon\n");
	}

	return pon_status.reasons;
}

int target_get_pon_usb_chg(void)
{
	unsigned long long pon;

	pon = target_check_pon_reason();

	return (pon & 0xff) & PWR_ON_EVENT_USB_CHG;
}

#ifdef LGE_PM_FACTORY_CABLE
void lge_pm_check_factory_reset(unsigned *pon_addr, unsigned *poff_addr)
{
	unsigned long long fr;
	unsigned smem_status = 0;

	smem_status = smem_read_alloc_entry(SMEM_POWER_ON_STATUS_INFO,
				&fr, sizeof(fr));

	if (smem_status) {
		dprintf(CRITICAL,
				"ERROR: unable to read shared memory for power on reason\n");
	}

	*pon_addr = fr & 0xFF;
	*poff_addr = (fr>>32) & 0xFF;

	dprintf(INFO, "PON_PON_REASON1:%#llx\n", fr & 0xFF);
	dprintf(INFO, "PON_POFF_REASON1:%#llx\n", (fr>>32) & 0xFF);
}
#endif

