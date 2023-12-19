/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <nuttx/config.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <stdint.h>
#include<time.h>
#include "PthreadTest.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/
static void *ThreadF01(void *arg)
{
    sleep(500);

    /* Shouldn't reach here.  If we do, then the pthread_cancel()
     * function did not succeed. */

    pthread_exit((void *)6);
    return NULL;
}
/****************************************************************************
 * Name: TestNuttxPthreadTest05
 ****************************************************************************/

void TestNuttxPthreadTest05(FAR void **state)
{
    pthread_t newTh;
    UINT32 ret;
    UINTPTR temp;
    clock_t start, finish;
    double duration;

    start = clock();
    if (pthread_create(&newTh, NULL, ThreadF01, NULL) < 0) {
        syslog(LOG_INFO,"Error creating thread\n");
        assert_int_equal(1, 0);
    }

    usleep(1000);
    // LOS_TaskDelay(1);
    /* Try to cancel the newly created thread.  If an error is returned,
     * then the thread wasn't created successfully. */
    if (pthread_cancel(newTh) != 0) {
        syslog(LOG_INFO,"Test FAILED: A new thread wasn't created\n");
        assert_int_equal(1, 0);
    }
    ret = pthread_join(newTh, (void *)&temp);
    finish = clock();
    duration = (double)(finish - start)  / CLOCKS_PER_SEC * 1000;
    syslog(LOG_INFO,"duration: %f \n", duration);
    assert_int_equal(duration < 500, 1);
    syslog(LOG_INFO,"ret: %d \n", ret);
    syslog(LOG_INFO,"temp: %ld \n", temp);
    assert_int_equal(ret, 0);
    assert_int_not_equal(temp, 6);
}
