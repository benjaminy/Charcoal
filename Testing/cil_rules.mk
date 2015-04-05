.PRECIOUS: %$(CRCL_DOT_H_EXT)

# Prepend the Charcoal runtime headers
$(BUILD_DIR)/%$(CRCL_DOT_H_EXT): %.crcl
	cat $(INSTALL_DIR)/include/application_pre_header.h > $@
	echo "#line 1 " \"$<\" >> $@
	cat $< >> $@

# Do C preprocesing.  Cil doesn't do this itself.
%$(CRCL_CPP_EXT): %$(CRCL_DOT_H_EXT)
	$(CC) -E $(INCLUDE_DIRS) -o $@ -x c $<

# Invoke Cil to translate Charcoal to plain C.
%$(CIL_C_EXT): %$(CRCL_CPP_EXT)
	$(CIL_DIR)/bin/cilly.native --tr coroutinify --out $@ $<

# Generic rule for building a single file program
$(BUILD_DIR)/% : $(BUILD_DIR)/%.crcl.cil.c
	$(CC) -o $@ $< $(CRCL_RUNTIME) $(ZLOG_LIB) $(LIBUV_FLAGS)

# Generic rule for building in the build directory
%: $(BUILD_DIR)/%
	echo "Built \"$@\" by building \"$<\""
