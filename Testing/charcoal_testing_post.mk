
# Generic rule for building a single file program
$(BUILD_DIR)/% : $(BUILD_DIR)/%$(CRCL_O_EXT)
	$(CC) $(CFLAGS) -o $@ $< $(CRCL_RUNTIME) $(ZLOG_LIB) $(LIBUV_FLAGS)

# Generic rule for building in the build directory
%: $(BUILD_DIR)/%
	echo "Built \"$@\" by building \"$<\""
