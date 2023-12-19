/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <nuttx/config.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <stdint.h>

#include "MutexTest.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: TestNuttxMutexTest01
 ****************************************************************************/

void TestNuttxMutexTest01(FAR void **state)
{
    pthread_mutexattr_t mta;
    int rc;

    /* Initialize a mutex attributes object */
    rc = pthread_mutexattr_init(&mta);
    syslog(LOG_INFO,"rc : %d\n", rc);
    assert_int_equal(rc, 0);

    rc = pthread_mutexattr_destroy(&mta);
    syslog(LOG_INFO,"rc : %d\n", rc);
    assert_int_equal(rc, 0);
}
