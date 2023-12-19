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
 * Name: TestNuttxMm02
 ****************************************************************************/
void TestNuttxMm02(FAR void **state)
{
    int i, flag = 0;
    char *pm1, *pm2;
    unsigned long memsize;

    memsize = mmtest_get_memsize();
    pm2 = pm1 = calloc(memsize, 1);
    assert_non_null(pm2);

    for (i = 0; i < memsize; i++)
    {
        if (*pm2++ != 0)
        {
            flag = 1;
        }
    }

    pm2 = pm1;
    for (i = 0; i < memsize; i++)
        *pm2++ = 'X';
    pm2 = pm1;
    for (i = 0; i < memsize; i++)
    {
        if (*pm2++ != 'X')
        {
            flag = 1;
        }
    }
    free(pm1);
    assert_int_equal(flag, 0);
}