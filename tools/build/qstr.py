import ninja

# The logic for qstr regeneration is:
# - if anything in QSTR_GLOBAL_DEPENDENCIES is newer, then process all source files ($^)
# - else, if list of newer prerequisites ($?) is not empty, then process just these ($?)
# - else, process all source files ($^) [this covers "make -B" which can set $? to empty]
# $(HEADER_BUILD)/qstr.i.last: $(SRC_QSTR) $(SRC_QSTR_PREPROCESSOR) $(QSTR_GLOBAL_DEPENDENCIES) | $(HEADER_BUILD)/mpversion.h
#     $(STEPECHO) "GEN $@"
#     $(Q)grep -lE "(MP_QSTR|translate)" $(if $(filter $?,$(QSTR_GLOBAL_DEPENDENCIES)),$^,$(if $?,$?,$^)) | xargs $(CPP) $(QSTR_GEN_EXTRA_CFLAGS) $(CFLAGS) $(SRC_QSTR_PREPROCESSOR) >$(HEADER_BUILD)/qstr.i.last;

# $(HEADER_BUILD)/qstr.split: $(HEADER_BUILD)/qstr.i.last $(PY_SRC)/makeqstrdefs.py
#     $(STEPECHO) "GEN $@"
#     $(Q)$(PYTHON3) $(PY_SRC)/makeqstrdefs.py split $(HEADER_BUILD)/qstr.i.last $(HEADER_BUILD)/qstr $(QSTR_DEFS_COLLECTED)
#     $(Q)touch $@

# $(QSTR_DEFS_COLLECTED): $(HEADER_BUILD)/qstr.split $(PY_SRC)/makeqstrdefs.py
#     $(STEPECHO) "GEN $@"
#     $(Q)$(PYTHON3) $(PY_SRC)/makeqstrdefs.py cat $(HEADER_BUILD)/qstr.i.last $(HEADER_BUILD)/qstr $(QSTR_DEFS_COLLECTED)


# $(HEADER_BUILD)/qstrdefs.preprocessed.h: $(PY_QSTR_DEFS) $(QSTR_DEFS) $(QSTR_DEFS_COLLECTED) mpconfigport.h $(MPCONFIGPORT_MK) $(PY_SRC)/mpconfig.h | $(HEADER_BUILD)
#     $(STEPECHO) "GEN $@"
#     $(Q)cat $(PY_QSTR_DEFS) $(QSTR_DEFS) $(QSTR_DEFS_COLLECTED) | $(SED) 's/^Q(.*)/"&"/' | $(CPP) $(CFLAGS) - | $(SED) 's/^"\(Q(.*)\)"/\1/' > $@

# # qstr data
# $(HEADER_BUILD)/qstrdefs.enum.h: $(PY_SRC)/makeqstrdata.py $(HEADER_BUILD)/qstrdefs.preprocessed.h
#     $(STEPECHO) "GEN $@"
#     $(Q)$(PYTHON3) $(PY_SRC)/makeqstrdata.py $(HEADER_BUILD)/qstrdefs.preprocessed.h > $@

#     $(HEADER_BUILD)/qstr.i.last: $(SRC_QSTR) $(SRC_QSTR_PREPROCESSOR) $(QSTR_GLOBAL_DEPENDENCIES) | $(HEADER_BUILD)/mpversion.h
#     $(STEPECHO) "GEN $@"
#     $(Q)grep -lE "(MP_QSTR|translate)" $(if $(filter $?,$(QSTR_GLOBAL_DEPENDENCIES)),$^,$(if $?,$?,$^)) | xargs $(CPP) $(QSTR_GEN_EXTRA_CFLAGS) $(CFLAGS) $(SRC_QSTR_PREPROCESSOR) >$(HEADER_BUILD)/qstr.i.last;

# $(HEADER_BUILD)/qstr.split: $(HEADER_BUILD)/qstr.i.last $(PY_SRC)/makeqstrdefs.py
#     $(STEPECHO) "GEN $@"
#     $(Q)$(PYTHON3) $(PY_SRC)/makeqstrdefs.py split $(HEADER_BUILD)/qstr.i.last $(HEADER_BUILD)/qstr $(QSTR_DEFS_COLLECTED)
#     $(Q)touch $@

# $(QSTR_DEFS_COLLECTED): $(HEADER_BUILD)/qstr.split $(PY_SRC)/makeqstrdefs.py
#     $(STEPECHO) "GEN $@"
#     $(Q)$(PYTHON3) $(PY_SRC)/makeqstrdefs.py cat $(HEADER_BUILD)/qstr.i.last $(HEADER_BUILD)/qstr $(QSTR_DEFS_COLLECTED)

#  ag genhdr/qstrdefs.preprocessed.h build-steps.txt                                                                                                                        Sun Apr 12 13:02:20 2020
# 19:GEN build-feather_m0_express/genhdr/qstrdefs.preprocessed.h
# 20:cat ../../py/qstrdefs.h qstrdefsport.h build-feather_m0_express/genhdr/qstrdefs.collected.h | sed 's/^Q(.*)/"&"/' | arm-none-eabi-gcc -E -DCIRCUITPY_FULL_BUILD=1 -DCIRCUITPY_MINIMAL_BUILD=0 -DCIRCUITPY_DEFAULT_BUILD=1 -DCIRCUITPY_ALWAYS_BUILD=1 -DCIRCUITPY_ANALOGIO=1 -DCIRCUITPY_AUDIOBUSIO=1 -DCIRCUITPY_AUDIOIO=1 -DCIRCUITPY_AUDIOIO_COMPAT=1 -DCIRCUITPY_AUDIOPWMIO=0 -DCIRCUITPY_AUDIOCORE=1 -DCIRCUITPY_AUDIOMIXER=0 -DCIRCUITPY_AUDIOMP3=0 -DCIRCUITPY_BITBANGIO=0 -DCIRCUITPY_BLEIO=0 -DCIRCUITPY_BOARD=1 -DCIRCUITPY_BUSIO=1 -DCIRCUITPY_DIGITALIO=1 -DCIRCUITPY_DISPLAYIO=1 -DCIRCUITPY_FREQUENCYIO=0 -DCIRCUITPY_GAMEPAD=1 -DCIRCUITPY_GAMEPADSHIFT=0 -DCIRCUITPY_I2CSLAVE=0 -DCIRCUITPY_MATH=1 -DCIRCUITPY__EVE=0 -DCIRCUITPY_MICROCONTROLLER=1 -DCIRCUITPY_NEOPIXEL_WRITE=1 -DCIRCUITPY_NETWORK=0 -DCIRCUITPY_NVM=1 -DCIRCUITPY_OS=1 -DCIRCUITPY_PIXELBUF=1 -DCIRCUITPY_PULSEIO=1 -DCIRCUITPY_PS2IO=0 -DCIRCUITPY_RANDOM=1 -DCIRCUITPY_ROTARYIO=1 -DCIRCUITPY_RTC=1 -DCIRCUITPY_SAMD=0 -DCIRCUITPY_STAGE=0 -DCIRCUITPY_STORAGE=1 -DCIRCUITPY_STRUCT=1 -DCIRCUITPY_SUPERVISOR=1 -DCIRCUITPY_TIME=1 -DCIRCUITPY_TOUCHIO_USE_NATIVE=1 -DCIRCUITPY_TOUCHIO=1 -DCIRCUITPY_UHEAP=0 -DCIRCUITPY_USB_HID=1 -DCIRCUITPY_USB_MIDI=1 -DCIRCUITPY_PEW=0 -DCIRCUITPY_USTACK=0 -DCIRCUITPY_BITBANG_APA102=0 -DCIRCUITPY_REQUIRE_I2C_PULLUPS=1 -DCIRCUITPY_SERIAL_BLE=0 -DCIRCUITPY_BLE_FILE_SERVICE=0 -DCIRCUITPY_SERIAL_UART=0 -DCIRCUITPY_ENABLE_MPY_NATIVE=0 -DINTERNAL_FLASH_FILESYSTEM=0 -DQSPI_FLASH_FILESYSTEM=0 -DSPI_FLASH_FILESYSTEM=1 -DEXTERNAL_FLASH_DEVICES="S25FL216K, GD25Q16C" -DEXTERNAL_FLASH_DEVICE_COUNT=2 -DUSB_AVAILABLE -DLONGINT_IMPL_MPZ -Os -DNDEBUG -DCFG_TUSB_MCU=OPT_MCU_SAMD21 -DCFG_TUD_MIDI_RX_BUFSIZE=128 -DCFG_TUD_CDC_RX_BUFSIZE=128 -DCFG_TUD_MIDI_TX_BUFSIZE=128 -DCFG_TUD_CDC_TX_BUFSIZE=128 -DCFG_TUD_MSC_BUFSIZE=512 -finline-limit=60 -flto -flto-partition=none -I. -I../.. -I../lib/mp-readline -I../lib/timeutils -Iasf4/samd21 -Iasf4/samd21/hal/include -Iasf4/samd21/hal/utils/include -Iasf4/samd21/hri -Iasf4/samd21/hpl/core -Iasf4/samd21/hpl/gclk -Iasf4/samd21/hpl/pm -Iasf4/samd21/hpl/port -Iasf4/samd21/hpl/rtc -Iasf4/samd21/hpl/tc -Iasf4/samd21/include -Iasf4/samd21/CMSIS/Include -Iasf4_conf/samd21 -Iboards/feather_m0_express -Iboards/ -Iperipherals/ -Ifreetouch -I../../lib/tinyusb/src -I../../supervisor/shared/usb -Ibuild-feather_m0_express -Wall -Werror -std=gnu11 -nostdlib -fsingle-precision-constant -fno-strict-aliasing -Wdouble-promotion -Wno-endif-labels -Wstrict-prototypes -Werror-implicit-function-declaration -Wfloat-equal -Wundef -Wshadow -Wwrite-strings -Wsign-compare -Wmissing-format-attribute -Wno-deprecated-declarations -Wnested-externs -Wunreachable-code -Wcast-align -Wno-error=lto-type-mismatch -D__SAMD21G18A__ -ffunction-sections -fdata-sections -fshort-enums -DCIRCUITPY_SOFTWARE_SAFE_MODE=0x0ADABEEF -DCIRCUITPY_CANARY_WORD=0xADAF00 -DCIRCUITPY_SAFE_RESTART_WORD=0xDEADBEEF --param max-inline-insns-single=500 -DFFCONF_H=\"lib/oofatfs/ffconf.h\"  -mthumb -mabi=aapcs-linux -mcpu=cortex-m0plus -msoft-float -mfloat-abi=soft -DSAMD21 - | sed 's/^"\(Q(.*)\)"/\1/' > build-feather_m0_express/genhdr/qstrdefs.preprocessed.h
# 23:python3 ../../py/makeqstrdata.py --compression_filename build-feather_m0_express/genhdr/compression.generated.h --translation build-feather_m0_express/genhdr/en_US.mo build-feather_m0_express/genhdr/qstrdefs.preprocessed.h > build-feather_m0_express/genhdr/qstrdefs.generated.h
# 24:python3 ../../py/makeqstrdata.py build-feather_m0_express/genhdr/qstrdefs.preprocessed.h > build-feather_m0_express/genhdr/qstrdefs.enum.h

def build(sources, cc, cflags):
    # sources = $(PY_QSTR_DEFS) $(QSTR_DEFS) $(QSTR_DEFS_COLLECTED)
    steps = []
    cflags = " ".join(cflags)
    print(cflags)
    # First we preprocess source files in a NO_QSTR mode.
    steps.append(f"rule qstr_src_pp\n  command = {cc} -E -DNO_QSTR {cflags} $in > $out\n\n")
    for source in sources:
        steps.append("build qstr/pp/{}: qstr_src_pp ../{} | version/genhdr/mpversion.h\n\n".format(source, source))
    # Next, each preprocessed file is transformed into simple macros.
    steps.append(f"rule qstr_src_out\n  command = python3 ../py/makeqstrdefs.py process $in | python3 ../tools/build/only_if_different.py $out\n\n")
    for source in sources:
        steps.append("build qstr/out/{}: qstr_src_out qstr/pp/{}\n\n".format(source, source))

    # steps.append("rule qstr_pp\n  command = cat $in | sed 's/^Q(.*)/\"&\"/' | {} {} -E - | sed 's/^\"\\(Q(.*)\\)\"/\\1/' > $out\n\n".format(cc, cflags))
    # steps.append("build qstr/genhdr/qstrdefs.preprocessed.h: qstr_pp {}\n\n".format(" ".join(["../" + f for f in sources])))
    # steps.append("rule qstr_enum\n  command = python3 ../py/makeqstrdata.py $in > $out\n\n")
    # steps.append("build qstr/genhdr/qstrdefs.enum.h: qstr_enum qstr/genhdr/qstrdefs.preprocessed.h\n\n")

    return steps
