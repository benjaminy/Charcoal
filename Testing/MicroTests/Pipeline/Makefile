
CRCL_ROOT=../../..
BUILD_DIR=$(CRCL_ROOT)/Testing/Build
include $(CRCL_ROOT)/Source/charcoal_pre.mk


crypto: crypto_common.h crypto_common.c crypto.c
	$(CC) -o $@ -I. -Wall -O2 crypto_common.c crypto.c -lcrypto

include $(CRCL_ROOT)/Source/charcoal_post.mk
include $(CRCL_ROOT)/Testing/charcoal_testing_post.mk
