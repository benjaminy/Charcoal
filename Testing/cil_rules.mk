# Prepend the Charcoal runtime headers
$(BUILD_DIR)/%$(CRCL_DOT_H_EXT): %.crcl
	echo "#define __CHARCOAL_CIL" > $@
	echo "#include <charcoal.h>" >> $@
	echo "#include <charcoal_runtime_coroutine.h>" >> $@
	echo "#line 1 " \"$<\" >> $@
	cat $< >> $@

# Do C preprocesing.  Cil doesn't do this itself.
%$(CRCL_CPP_EXT): %$(CRCL_DOT_H_EXT)
	$(CC) -E $(INCLUDE_DIRS) -o $@ -x c $<

# Invoke Cil to translate Charcoal to plain C.
%$(CIL_C_EXT): %$(CRCL_CPP_EXT)
	echo "BLAH"
	$(CIL_DIR)/bin/cilly.native --out $@ $<

%: $(BUILD_DIR)/%
	echo "Find it in $(BUILD_DIR)"
