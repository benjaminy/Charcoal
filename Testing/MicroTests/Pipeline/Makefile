
CRCL_ROOT=../../..
BUILD_DIR=$(CRCL_ROOT)/Testing/Build
include $(CRCL_ROOT)/Source/charcoal_pre.mk

INCLUDE_DIRS += -I.

CFLAGS += -O2

crypto: crypto_common.h crypto_common.c crypto.c
	$(CC) -o $@ -I. -Wall -O2 crypto_common.c crypto.c -lcrypto

$(BUILD_DIR)/crypto_pipe: $(BUILD_DIR)/crypto_pipe$(CRCL_O_EXT) crypto_common.o
	$(CC) $(CFLAGS) -o $@ $^ $(CRCL_RUNTIME) $(ZLOG_LIB) $(LIBUV_FLAGS) -lcrypto

include $(CRCL_ROOT)/Source/charcoal_post.mk
include $(CRCL_ROOT)/Testing/charcoal_testing_post.mk
