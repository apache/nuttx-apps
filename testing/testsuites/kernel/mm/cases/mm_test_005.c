/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <nuttx/config.h>
#include <stdlib.h>
#include <syslog.h>
#include <inttypes.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <stdint.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include "MmTest.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/
/****************************************************************************
 * Name: TestNuttxMm05
 ****************************************************************************/
void TestNuttxMm05(FAR void **state)
{
    int i, flag = 0;
    char *ptr, *temp_ptr = NULL;

    ptr = temp_ptr = zalloc(1024 * sizeof(char));
    assert_non_null(temp_ptr);

    for (i = 0; i < 1024; i++)
    {
        if (*temp_ptr++ != 0)
        {
            flag = 1;
        }
    }
    free(ptr);
    assert_int_equal(flag, 0);
}