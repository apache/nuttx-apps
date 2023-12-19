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
static void *PthreadF01(void *arg)
{
    g_testPthreadCount++;

    pthread_exit(NULL);

    return (void *)9; // 9, here set value about the return status.
}
/****************************************************************************
 * Name: TestNuttxPthreadTest09
 ****************************************************************************/

void TestNuttxPthreadTest09(FAR void **state)
{
    pthread_t newTh;
    UINT32 ret;
    UINTPTR temp = 1;
    // _pthread_data *joinee = NULL;

    g_testPthreadCount = 0;
    g_testPthreadTaskMaxNum = 128;

    ret = pthread_create(&newTh, NULL, PthreadF01, NULL);
    syslog(LOG_INFO,"ret: %d \n", ret);
    assert_int_equal(ret, 0);
    usleep(1000);
    // LOS_TaskDelay(1);
    syslog(LOG_INFO,"g_testPthreadCount: %d \n", g_testPthreadCount);
    assert_int_equal(g_testPthreadCount, 1);

    ret = pthread_join(g_testPthreadTaskMaxNum, (void *)&temp);
    syslog(LOG_INFO,"ret: %d \n", ret);
    syslog(LOG_INFO,"temp: %ld \n", temp);
    assert_int_equal(ret, ESRCH);
    assert_int_equal(temp, 1);

    ret = pthread_join(LOSCFG_BASE_CORE_TSK_CONFIG + 1, (void *)&temp);
    syslog(LOG_INFO,"ret: %d \n", ret);
    assert_int_equal(ret, ESRCH);
    syslog(LOG_INFO,"temp: %ld \n", temp);
    assert_int_equal(temp, 1);

    ret = pthread_detach(g_testPthreadTaskMaxNum);
    syslog(LOG_INFO,"ret: %d \n", ret);
    assert_int_equal(ret, ESRCH);

    ret = pthread_join(newTh, NULL);
    syslog(LOG_INFO,"ret: %d \n", ret);
    assert_int_equal(ret, 0);

    sleep(1);
    ret = pthread_detach(newTh);
    syslog(LOG_INFO,"ret: %d \n", ret);
    assert_int_equal(ret, ESRCH);

    ret = pthread_join(newTh, NULL);
    syslog(LOG_INFO,"ret: %d \n", ret);
    assert_int_equal(ret, ESRCH);
}
