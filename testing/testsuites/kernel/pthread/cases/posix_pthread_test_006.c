/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <nuttx/config.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <stdint.h>

#include "PthreadTest.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/
static void *ThreadF01(void *arg)
{
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

    sleep(1);

    pthread_exit((void *)0);
    return NULL;
}
/****************************************************************************
 * Name: TestNuttxPthreadTest06
 ****************************************************************************/

void TestNuttxPthreadTest06(FAR void **state)
{
    UINT32 ret;
    void *temp = NULL;
    pthread_t a;

    /* SIGALRM will be sent in 5 seconds. */

    /* Create a new thread. */
    if (pthread_create(&a, NULL, ThreadF01, NULL) != 0) {
        printf("Error creating thread\n");
        assert_int_equal(1, 0);
    }

    usleep(1000);
    // LOS_TaskDelay(1);
    pthread_cancel(a);
    /* If 'main' has reached here, then the test passed because it means
     * that the thread is truly asynchronise, and main isn't waiting for
     * it to return in order to move on. */

    ret = pthread_join(a, &temp);
    syslog(LOG_INFO,"ret: %d \n", ret);
    assert_int_equal(ret, 0);
}
