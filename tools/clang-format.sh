#! /bin/bash
if [ -z "$TRAVIS_COMMIT_RANGE" ]; then
LIST_FILES="git ls-files"
CLANG_FORMAT="clang-format"
else
LIST_FILES="git diff --name-only ${TRAVIS_COMMIT_RANGE/.../..}"
CLANG_FORMAT="clang-format-6.0"
fi

echo $LIST_FILES
eval ${LIST_FILES}

eval ${LIST_FILES} | # These files are sourced from ARM
grep "\.[ch]\$" |
grep -v "^lib/cmsis" |
grep -v "^examples/embedding/mpconfigport.h" |  # This is a symlink
grep -v "^drivers/cc3000" |  # These files are sourced from TI
grep -v "^drivers/cc3100" |  # These files are sourced from TI
grep -v "^drivers/wiznet5k" |  # These files are sourced from WIZnet
grep -v "^extmod/crypto-algorithms" |  # These files are sourced from Brad Conte
grep -v "^lib/libm" |  # These files are sourced from Sun
grep -v "^lib/libm_dbl" |  # These files are sourced from Sun
grep -v "^ports/cc3200/hal" |  # These files are sourced from TI
grep -v "^ports/cc3200/FreeRTOS" |  # These files are sourced from FreeRTOS
grep -v "^ports/nrf/device" | # These files are sourced from Nordic
grep -v "^ports/stm32/usbdev" | # These files are sourced from ST
grep -v "^ports/stm32/usbhost" | # These files are sourced from ST
grep -v "^tools/tinytest" | # These files are sourced from TinyTest
xargs -r ${CLANG_FORMAT} -style=file $*

exit $?
