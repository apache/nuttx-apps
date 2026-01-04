# ##############################################################################
# apps/examples/hsm_psicc2/CMakeLists.txt
#
# SPDX-License-Identifier: Apache-2.0
# ##############################################################################

if(CONFIG_EXAMPLES_SMF)
  nuttx_add_application(
    NAME
    ${CONFIG_EXAMPLES_SMF_PROGNAME}
    SRCS
    smf_main.c
    hsm_psicc2_thread.c
    STACKSIZE
    ${CONFIG_EXAMPLES_SMF_STACKSIZE}
    PRIORITY
    ${CONFIG_EXAMPLES_SMF_PRIORITY})
endif()
