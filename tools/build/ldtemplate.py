
def build(template_ld_file, board):
        # Generate a linker script (.ld file) from a template, for those builds that use it.
    GENERATED_LD_FILE = $(BUILD)/$(notdir $(patsubst %.template.ld,%.ld,$(LD_TEMPLATE_FILE)))
    #
    # ld_defines.pp is generated from ld_defines.c. See py/mkrules.mk.
    # Run gen_ld_files.py over ALL *.template.ld files, not just LD_TEMPLATE_FILE,
    # because it may include other template files.
    $(GENERATED_LD_FILE): $(BUILD)/ld_defines.pp boards/*.template.ld
        $(STEPECHO) "GEN $@"
        $(Q)$(PYTHON3) $(TOP)/tools/gen_ld_files.py --defines $< --out_dir $(BUILD) boards/*.template.ld
