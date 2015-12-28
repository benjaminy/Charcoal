# TODO: Header
#
#

CRCL_ROOT=../../..
BUILD_DIR=$(CRCL_ROOT)/Testing/Build
include $(CRCL_ROOT)/Source/charcoal_pre.mk

TARGETS = bucket_brigade_setcontext bucket_brigade_setcontext_jmp bucket_brigade_libuv bucket_brigade_pthread bucket_brigade_silly_loop bucket_brigade_crcl bucket_brigade

CFLAGS+= -O2
# CFLAGS+= -g -pg

.SECONDARY:

all: $(TARGETS)

clean:
	rm -rf $(addprefix $(BUILD_DIR)/, $(addsuffix *, $(TARGETS)))

include $(CRCL_ROOT)/Source/charcoal_post.mk

bucket_brigade_setcontext: bucket_brigade_setcontext.c
	gcc -g -O1 $(INCLUDE_DIRS) -o bucket_brigade_setcontext bucket_brigade_setcontext.c

bucket_brigade_setcontext_jmp: bucket_brigade_setcontext_jmp.c
	gcc -g -O1 $(INCLUDE_DIRS) -o bucket_brigade_setcontext_jmp bucket_brigade_setcontext_jmp.c

bucket_brigade_libuv: $(BUILD_DIR)/bucket_brigade_libuv

$(BUILD_DIR)/bucket_brigade_libuv: bucket_brigade_libuv.c
	$(CC) $(CFLAGS) $(INCLUDE_DIRS) -o  $@ $< $(LIBUV_FLAGS) -pthread

$(BUILD_DIR)/bucket_brigade_pthread: bucket_brigade_pthread.c
	$(CC) $(CFLAGS) -o $@ $< -pthread

bucket_brigade_silly_loop: bucket_brigade_silly_loop.c
	gcc -g -O1 -o bucket_brigade_silly_loop bucket_brigade_silly_loop.c

bucket_brigade_crcl: bucket_brigade_crcl.c
	gcc -O1 $(FLAGS) -o bucket_brigade_crcl bucket_brigade_crcl.c -lcharcoal_sys $(LIBUV_FLAGS)

bucket_brigade.class: bucket_brigade.java
	javac bucket_brigade.java

include $(CRCL_ROOT)/Testing/charcoal_testing_post.mk
