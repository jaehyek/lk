LOCAL_DIR := $(GET_LOCAL_DIR)

INCLUDES += -I$(LK_TOP_DIR)/platform/msm_shared/include -I$(LK_TOP_DIR)/lib/zlib_inflate
INCLUDES += -I$(LK_TOP_DIR)/platform/lge_shared/include
DEFINES += ASSERT_ON_TAMPER=1

MODULES += lib/zlib_inflate

OBJS += \
	$(LOCAL_DIR)/aboot.o \
	$(LOCAL_DIR)/fastboot.o \
	$(LOCAL_DIR)/recovery.o \
	$(LOCAL_DIR)/lge_stage.o

ifeq ($(ENABLE_MDTP_SUPPORT),1)
OBJS += \
	$(LOCAL_DIR)/mdtp.o \
	$(LOCAL_DIR)/mdtp_ui.o \
	$(LOCAL_DIR)/mdtp_fuse.o
endif

ifeq ($(ENABLE_LGE_WITH_FASTBOOT_MENU), 1)
DEFINES += LGE_WITH_FASTBOOT_MENU_TEXT=1
OBJS += \
	$(LOCAL_DIR)/fastboot_menu.o
endif

ifeq ($(ENABLE_LGE_WITH_FTM), 1)
DEFINES += LGE_WITH_FASTBOOT_COMMAND
endif
