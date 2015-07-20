# TODO: Header
#
#

CRCL_ROOT=../../..

include $(CRCL_ROOT)/Testing/generic.mk

TARGETS = hello_world one_activity activate_loop

.SECONDARY:

clean:
	rm -f $(BUILD_DIR)/*

CFLAGS+= -O2

all:
	echo $(BUILD_DIR)

rec: $(BUILD_DIR)/rec

$(BUILD_DIR)/rec: rec.c
	$(CC) $(CFLAGS) -o $@ $<

$(BUILD_DIR)/non_rec: non_rec.c
	$(CC) $(CFLAGS) -o $@ $<

$(BUILD_DIR)/just_calling_rec: just_calling_rec.c
	$(CC) $(CFLAGS) -o $@ $<

$(BUILD_DIR)/non_rec_coro: non_rec_coro.c
	$(CC) $(CFLAGS) -g -o $@ $<

$(BUILD_DIR)/non_rec_coro_goto: non_rec_coro_goto.c
	$(CC) $(CFLAGS) -g -o $@ $<

$(BUILD_DIR)/non_rec_final_maybe: non_rec_final_maybe.c
	$(CC) $(CFLAGS) -g -o $@ $<

$(BUILD_DIR)/non_rec_coro2: non_rec_coro2.c
	$(CC) $(CFLAGS) -g -o $@ $<

$(BUILD_DIR)/non_rec_coro3: non_rec_coro3.c
	$(CC) $(CFLAGS) -g -o $@ $<

$(BUILD_DIR)/non_rec_coro4: non_rec_coro4.c
	$(CC) $(CFLAGS) -g -o $@ $<

$(BUILD_DIR)/non_rec.class: non_rec.java
	javac -d $(BUILD_DIR) non_rec.java

$(BUILD_DIR)/non_rec_hs: non_rec.hs
	ghc -outputdir $(BUILD_DIR) -o $(BUILD_DIR)/non_rec_hs --make non_rec.hs

include $(CRCL_ROOT)/Testing/cil_rules.mk
