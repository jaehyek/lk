# top level project rules for the MSM8952 project
#
LOCAL_DIR := $(GET_LOCAL_DIR)

TARGET := msm8952_p1v_att_us

MODULES += app/aboot

ifeq ($(TARGET_BUILD_VARIANT),user)
DEBUG := 0
DEFINES += WITH_DEBUG_UART=0
else
DEBUG := 1
DEFINES += WITH_DEBUG_UART=1
endif

EMMC_BOOT := 1

ENABLE_SMD_SUPPORT := 1
#ENABLE_PWM_SUPPORT := true
#ENABLE_KILLSWITCH_UNLOCK_SUPPORT := 1

#DEFINES += WITH_DEBUG_DCC=1
DEFINES += WITH_DEBUG_LOG_BUF=1
DEFINES += WITH_DEBUG_BOOTCHART=1
DEFINES += WITH_DEBUG_FBCON=1
DEFINES += DEVICE_TREE=1
#DEFINES += MMC_BOOT_BAM=1
DEFINES += CRYPTO_BAM=1
DEFINES += WITH_LGE_KSWITCH

DEFINES += SPMI_CORE_V2=1
DEFINES += ABOOT_IGNORE_BOOT_HEADER_ADDRS=1
DEFINES += LGE_PM_BATT_ID
DEFINES += WITH_LGE_DOWNLOAD=1
DEFINES += TARGET_USES_CHARGERLOGO=1
DEFINES += LGE_P1V=1

DEFINES += BAM_V170=1

#Enable the feature of long press power on
DEFINES += LONG_PRESS_POWER_ON=0

DEFINES += WITH_LGE_SHOW_ATT_SN_IN_HW_KEY_CTL_MODE
DEFINES += WITH_LGE_SHOW_ATT_SKU_IN_HW_KEY_CTL_MODE

#Disable thumb mode
ENABLE_THUMB := false

ENABLE_QPNP_HAPTIC_SUPPORT := true

ifeq ($(ENABLE_QPNP_HAPTIC_SUPPORT),true)
DEFINES += PON_VIB_SUPPORT=1
DEFINES += QPNP_HAPTIC_SUPPORT=1
endif

ENABLE_SDHCI_SUPPORT := 1

ifeq ($(ENABLE_SDHCI_SUPPORT),1)
DEFINES += MMC_SDHCI_SUPPORT=1
endif

#enable power on vibrator feature
#ENABLE_PON_VIB_SUPPORT := true

ifeq ($(EMMC_BOOT),1)
DEFINES += _EMMC_BOOT=1
endif

ifeq ($(ENABLE_PON_VIB_SUPPORT),true)
DEFINES += PON_VIB_SUPPORT=1
endif

DEFINES += LGE_WITH_LOAD_IMAGE_FROM_EMMC
DEFINES += LGE_WITH_SPLASH_SCREEN_IMG=1
#DEFINES += LGE_WITH_QUICKCOVER_IMAGE
DEFINES += LGE_MIPI_DSI_TOVIS_ILI7807B_FHD_VIDEO_PANEL
DEFINES +=LGE_WITH_PANEL_PRE_INIT
#DEFINES += LGE_WITH_PANEL_POST_INIT

ifeq ($(ENABLE_SMD_SUPPORT),1)
DEFINES += SMD_SUPPORT=1
endif

ifeq ($(ENABLE_MDTP_SUPPORT),1)
DEFINES += MDTP_SUPPORT=1
DEFINES += MDTP_EFUSE_ADDRESS=0x0C858250 # QFPROM_RAW_QC_SPARE_REG_LSB_ADDR
DEFINES += MDTP_EFUSE_START=0
endif

#SCM call before entering DLOAD mode
DEFINES += PLATFORM_USE_SCM_DLOAD=1

CFLAGS += -Werror

DEFINES += USE_TARGET_HS200_DELAY=1

ENABLE_LGE_WITH_FASTBOOT_MENU := 1

ifeq ($(ENABLE_LGE_WITH_FASTBOOT_MENU), 1)
DEFINES += LGE_WITH_FASTBOOT_MENU
endif

DEFINES += LGE_WITH_FACTORY_RSET_METHOD=1

ENABLE_KILLSWITCH_UNLOCK_SUPPORT=1
