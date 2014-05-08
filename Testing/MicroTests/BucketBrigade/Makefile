# TODO: Header
# 
# 

CRCL_ROOT=../../..

include $(CRCL_ROOT)/Testing/generic.mk

TARGETS = bucket_brigade_setcontext bucket_brigade_setcontext_jmp bucket_brigade_libuv bucket_brigade_pthread bucket_brigade_silly_loop bucket_brigade_crcl

all: $(TARGETS)

include $(CRCL_ROOT)/Testing/cil_rules.mk

bucket_brigade_setcontext: bucket_brigade_setcontext.c
	gcc -g -O1 $(INCLUDE_DIRS) -o bucket_brigade_setcontext bucket_brigade_setcontext.c

bucket_brigade_setcontext_jmp: bucket_brigade_setcontext_jmp.c
	gcc -g -O1 $(INCLUDE_DIRS) -o bucket_brigade_setcontext_jmp bucket_brigade_setcontext_jmp.c

bucket_brigade_libuv: bucket_brigade_libuv.c
	gcc -g -O1 $(INCLUDE_DIRS) -o bucket_brigade_libuv bucket_brigade_libuv.c $(LIBUV_FLAGS) -pthread

bucket_brigade_pthread: bucket_brigade_pthread.c
	gcc -g -O1 -o bucket_brigade_pthread bucket_brigade_pthread.c -pthread

bucket_brigade_silly_loop: bucket_brigade_silly_loop.c
	gcc -g -O1 -o bucket_brigade_silly_loop bucket_brigade_silly_loop.c

bucket_brigade_crcl: bucket_brigade_crcl.c
	gcc -O1 $(FLAGS) -o bucket_brigade_crcl bucket_brigade_crcl.c -lcharcoal_sys $(LIBUV_FLAGS)

bucket_brigade.class: bucket_brigade.java
	javac bucket_brigade.java

clean:
	rm -rf $(TARGETS)
