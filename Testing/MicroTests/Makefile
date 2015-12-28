# TODO: Header
# 
# 

CRCL_ROOT=../..
BUILD_DIR=$(CRCL_ROOT)/Testing/Build
include $(CRCL_ROOT)/Source/charcoal_pre.mk

TARGETS = simple_test simple_test_noyield simple_yield_exe  unyielding_test_exe activity_create timeout_test_exe simple_unyield_test simple_unyield_test_noyield

timer_create_test: timer_create_test.c
	gcc -O1 -o timer_create_test timer_create_test.c -lrt

multi_dns_crcl_c: multi_dns_crcl.c
	gcc -g -o multi_dns_crcl $(FLAGS) multi_dns_crcl.c -lcharcoal_sys $(LIBUV_FLAGS)

$(BUILD_DIR)/multi_dns_crcl: $(BUILD_DIR)/multi_dns$(CIL_C_EXT)
	$(CC) -o $(BUILD_DIR)/multi_dns_crcl $< $(FLAGS) $(LIBUV_FLAGS)

simple_unyield_test:
	gcc -g -o simple_unyield_test $(FLAGS) -DYIELDING simple_unyield_test.c -lm

simple_unyield_test_noyield:
	gcc -g -o simple_unyield_test_noyield $(FLAGS) simple_unyield_test.c -lm 
simple_test:
	gcc -g -o simple_test $(FLAGS) -DYIELDING simple_test.c -lm

simple_test_noyield:
	gcc -g -o simple_test_noyield $(FLAGS) simple_test.c -lm

checks:
	gcc -Wall -o check check_default_stack_size.c -lpthread
	./check

simple_yield:
	gcc -g -o simple_yield_exe $(FLAGS) simple_yield.c
	./simple_yield_exe

activity_create:
	gcc -g -o activity_create $(FLAGS) activity_create.c
	./activity_create

unyielding_test:
	gcc -g -o unyielding_test_exe $(FLAGS) unyielding_test.c

timeout:
	gcc -g -o timeout_test_exe $(FLAGS) timeout_test.c

clean:
	rm -rf $(TARGETS)

include $(CRCL_ROOT)/Source/charcoal_post.mk
include $(CRCL_ROOT)/Testing/charcoal_testing_post.mk
