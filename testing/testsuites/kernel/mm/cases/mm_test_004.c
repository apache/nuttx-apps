/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <nuttx/config.h>
#include <stdlib.h>
#include <syslog.h>
#include <string.h>
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
 * Name: TestNuttxMm04
 ****************************************************************************/
void TestNuttxMm04(FAR void **state)
{
    int i;
    char *temp_ptr = NULL;

    for (i = 0; i < 10; i++)
    {
        temp_ptr = memalign(64, 1024 * sizeof(char));
        assert_non_null(temp_ptr);

        memset(temp_ptr, 0x33, 1024 * sizeof(char));
        free(temp_ptr);
    }
}