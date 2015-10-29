LOCAL_DIR := $(GET_LOCAL_DIR)

INCLUDES += -I$(LOCAL_DIR)/include
INCLUDES += -I$(LK_TOP_DIR)/app/aboot
INCLUDES += -I$(LK_TOP_DIR)/platform/lge_shared/include

ENABLE_LGE_WITH_COMMON := 1

ifeq ($(ENABLE_LGE_WITH_COMMON),1)
DEFINES += LGE_WITH_COMMON
OBJS += $(LOCAL_DIR)/lge_target_common.o
endif

ifeq ($(ENABLE_LGE_WITH_SELECT_MODE),1)
OBJS += $(LOCAL_DIR)/lge_target_select_mode.o
endif

ifeq ($(ENABLE_LGE_WITH_FTM), 1)
OBJS += $(LOCAL_DIR)/lge_target_ftm.o
endif

ifeq ($(ENABLE_LGE_WITH_DEVICE_TREE), 1)
OBJS += $(LOCAL_DIR)/lge_target_dev_tree.o
endif

DEFINES += LGE_PM_FACTORY_CABLE=1

# WITH_LGE_DOWNLOAD
OBJS += $(LOCAL_DIR)/lge_target_laf.o

