LOCAL_DIR := $(GET_LOCAL_DIR)

INCLUDES += -I$(LOCAL_DIR)

CFLAGS += -Wno-sign-compare

#MODULES += \
	lib/libc \
	lib/debug \
	lib/heap

OBJS += \
	$(LOCAL_DIR)/plr_calibration.o \
	$(LOCAL_DIR)/plr_case_info.o \
	$(LOCAL_DIR)/plr_common.o \
	$(LOCAL_DIR)/plr_deviceio.o \
	$(LOCAL_DIR)/plr_err_handling.o \
	$(LOCAL_DIR)/plr_hooking_lg.o \
	$(LOCAL_DIR)/plr_io_statistics.o \
	$(LOCAL_DIR)/plr_lk_wrapper.o \
	$(LOCAL_DIR)/plr_main.o \
	$(LOCAL_DIR)/plr_pattern.o \
	$(LOCAL_DIR)/plr_precondition.o \
	$(LOCAL_DIR)/plr_protocol.o \
	$(LOCAL_DIR)/plr_rand_sector.o \
	$(LOCAL_DIR)/plr_rtc64.o \
	$(LOCAL_DIR)/plr_tftp.o \
	$(LOCAL_DIR)/plr_tick_conversion.o \
	$(LOCAL_DIR)/plr_verify.o \
	$(LOCAL_DIR)/plr_verify_log.o \
	$(LOCAL_DIR)/plr_verify_report.o \
	$(LOCAL_DIR)/plr_write_rand.o \
	$(LOCAL_DIR)/test_case/plr_daxx0000.o \
	$(LOCAL_DIR)/test_case/plr_daxx0001.o \
	$(LOCAL_DIR)/test_case/plr_daxx0002.o \
	$(LOCAL_DIR)/test_case/plr_daxx0003_4.o \
	$(LOCAL_DIR)/test_case/plr_daxx0008.o \
	$(LOCAL_DIR)/test_case/plr_dpin0000.o \
	$(LOCAL_DIR)/test_case/plr_dpin0001.o \
	$(LOCAL_DIR)/test_case/plr_dpin0002.o \
	$(LOCAL_DIR)/test_case/plr_dpin0003.o \
	$(LOCAL_DIR)/test_case/plr_dpin0004.o \
	$(LOCAL_DIR)/test_case/plr_dpin0005.o \
	$(LOCAL_DIR)/test_case/plr_dpin0006.o \
	$(LOCAL_DIR)/test_case/plr_dpin0007.o \
	$(LOCAL_DIR)/test_case/plr_dpin0008.o \
	$(LOCAL_DIR)/test_case/plr_dpin0009_12.o

