/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <nuttx/config.h>
#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <syslog.h>
#include "SchedTest.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: TestNuttxSchedTestGroupSetup
 ****************************************************************************/
int TestNuttxSchedTestGroupSetup(void **state)
{
    // syslog(LOG_INFO, "This is goup setup !\n");
    return 0;
}

/****************************************************************************
 * Name: TestNuttxSchedTestGroupTearDown
 ****************************************************************************/
int TestNuttxSchedTestGroupTearDown(void **state)
{
    // syslog(LOG_INFO, "This is goup tearDown !\n");
    return 0;
}
