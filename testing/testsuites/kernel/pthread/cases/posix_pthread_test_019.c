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
static void PthreadOnceF01(void)
{
    g_testPthreadCount++;

}
/****************************************************************************
 * Name: TestNuttxPthreadTest19
 ****************************************************************************/

void TestNuttxPthreadTest19(FAR void **state)
{
    UINT32 ret;
    pthread_once_t onceBlock = PTHREAD_ONCE_INIT;

    g_testPthreadCount = 0;

    ret = pthread_once(NULL, PthreadOnceF01);
    syslog(LOG_INFO,"ret: %d \n", ret);
    assert_int_equal(ret, EINVAL);

    ret = pthread_once(&onceBlock, NULL);
    syslog(LOG_INFO,"ret: %d \n", ret);
    assert_int_equal(ret, EINVAL);

    ret = pthread_once(&onceBlock, PthreadOnceF01);
    syslog(LOG_INFO,"ret: %d \n", ret);
    assert_int_equal(ret, 0);
    syslog(LOG_INFO,"g_testPthreadCount: %d \n", g_testPthreadCount);
    assert_int_equal(g_testPthreadCount, 1);

    ret = pthread_once(&onceBlock, PthreadOnceF01);
    syslog(LOG_INFO,"ret: %d \n", ret);
    syslog(LOG_INFO,"g_testPthreadCount: %d \n", g_testPthreadCount);
    assert_int_equal(ret, 0);
    assert_int_equal(g_testPthreadCount, 1);
}
