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
 * Name: TestNuttxMutexTest20
 ****************************************************************************/

void TestNuttxMutexTest20(FAR void **state)
{
    pthread_mutex_t mutex;
    int rc;

    /* Initialize a mutex object */
    rc = pthread_mutex_init(&mutex, NULL);
    syslog(LOG_INFO,"rc : %d\n", rc);
    assert_int_equal(rc, ENOERR);

    /* Acquire the mutex object using pthread_mutex_lock */
    if ((rc = pthread_mutex_lock(&mutex))){
        syslog(LOG_INFO,"rc: %d \n", rc);
        goto EXIT1;}

    sleep(1);

    /* Release the mutex object using pthread_mutex_unlock */
    if((rc = pthread_mutex_unlock(&mutex))){
        syslog(LOG_INFO,"rc: %d \n", rc);
        goto EXIT2;
    }

    /* Destroy the mutex object */
    if((rc = pthread_mutex_destroy(&mutex))){
        syslog(LOG_INFO,"rc: %d \n", rc);
        goto EXIT1;}

    return;

EXIT2:
    pthread_mutex_unlock(&mutex);
    assert_int_equal(1, 0);

EXIT1:
    pthread_mutex_destroy(&mutex);
    assert_int_equal(1, 0);
    return;
}
