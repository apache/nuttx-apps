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
    pthread_exit((void *)2); // 2, here set value of the exit status.
    return NULL;
}
/****************************************************************************
 * Name: TestNuttxPthreadTest04
 ****************************************************************************/

void TestNuttxPthreadTest04(FAR void **state)
{
    pthread_t mainTh, newTh;
    UINT32 ret;
    UINTPTR temp;

    if (pthread_create(&newTh, NULL, ThreadF01, NULL) != 0) {
        printf("Error creating thread\n");
        assert_int_equal(1, 0);
    }

    usleep(1000);
    // LOS_TaskDelay(1);
    /* Obtain the thread ID of this main function */
    mainTh = TestPthreadSelf();
    /* Compare the thread ID of the new thread to the main thread.
     * They should be different.  If not, the test fails. */
    if (pthread_equal(newTh, mainTh) != 0) {
        printf("Test FAILED: A new thread wasn't created\n");
        assert_int_equal(1, 0);
    }

    usleep(1000);
    // TestExtraTaskDelay(1);
    ret = pthread_join(newTh, (void *)&temp);
    syslog(LOG_INFO,"ret: %d \n", ret);
    syslog(LOG_INFO,"temp: %ld \n", temp);
    assert_int_equal(ret, 0);
    assert_int_equal(temp, 2); // 2, here assert the result.
}
