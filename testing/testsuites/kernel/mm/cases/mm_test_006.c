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
 * Pre-processor Definitions
 ****************************************************************************/
/* Random size range, we will apply the memory size in this range */
#define MALLOC_MIN_SIZE 64
#define MALLOC_MAX_SIZE 8192

/****************************************************************************
 * Public Functions
 ****************************************************************************/
/****************************************************************************
 * Name: TestNuttxMm06
 ****************************************************************************/
void TestNuttxMm06(FAR void **state)
{
    int malloc_size, test_num = 1000;
    char check_character = 0x67; /* Memory write content check character */
    char *address_ptr = NULL;

    for (int i = 0; i < test_num; i++)
    {
        srand(i + gettid());
        /* Produces a random size */
        malloc_size = mmtest_get_rand_size(MALLOC_MIN_SIZE, MALLOC_MAX_SIZE);
        address_ptr = (char *)malloc(malloc_size * sizeof(char));
        assert_non_null(address_ptr);
        memset(address_ptr, check_character, malloc_size);

        /* Checking Content Consistency */
        for (int j = 0; j < malloc_size; j++)
        {
            if (address_ptr[j] != check_character)
            {
                free(address_ptr);
                fail_msg("check fail !\n");
            }
        }
        /* Free test memory */
        free(address_ptr);
    }
}