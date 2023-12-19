/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <nuttx/config.h>
#include <stdlib.h>
#include <syslog.h>
#include <inttypes.h>
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
 * Name: TestNuttxMm03
 ****************************************************************************/
void TestNuttxMm03(FAR void **state)
{
    int i, flag = 0;
    char *pm1, *pm2;
    pm2 = pm1 = malloc(10);
    assert_non_null(pm2);
    for (i = 0; i < 10; i++)
        *pm2++ = 'X';

    pm2 = realloc(pm1, 5);
    pm1 = pm2;
    for (i = 0; i < 5; i++)
    {
        if (*pm2++ != 'X')
        {
            flag = 1;
        }
    }

    pm2 = realloc(pm1, 15);
    pm1 = pm2;
    for (i = 0; i < 5; i++)
    {
        if (*pm2++ != 'X')
        {
            flag = 1;
        }
    }

    free(pm1);
    assert_int_equal(flag, 0);
}