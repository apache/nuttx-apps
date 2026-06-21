# ##############################################################################
# apps/testing/sig_sp_test/CMakeLists.txt
#
# SPDX-License-Identifier: Apache-2.0
#
# ##############################################################################

if(CONFIG_TESTING_SIG_SP_TEST)
  nuttx_add_application(
    NAME
    ${CONFIG_TESTING_SIG_SP_TEST_PROGNAME}
    PRIORITY
    ${CONFIG_TESTING_SIG_SP_TEST_PRIORITY}
    STACKSIZE
    ${CONFIG_TESTING_SIG_SP_TEST_STACKSIZE}
    MODULE
    ${CONFIG_TESTING_SIG_SP_TEST}
    SRCS
    sig_sp_test_main.c)
endif()
