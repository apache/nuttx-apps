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
    pthread_exit(NULL);
    return NULL;
}
/****************************************************************************
 * Name: TestNuttxPthreadTest03
 ****************************************************************************/

void TestNuttxPthreadTest03(FAR void **state)
{
    pthread_t aThread;
    pthread_t ptid;
    pthread_t a = 0;
    pthread_t b = 0;
    int tmp;
    pthread_attr_t aa = { 0 };
    int detachstate;
    UINT32 ret;

    ptid = pthread_self();
    syslog(LOG_INFO,"ptid: %d \n", ptid);
    assert_int_not_equal(ptid, 0);
    pthread_create(&aThread, NULL, ThreadF01, NULL);

    tmp = pthread_equal(a, b);
    syslog(LOG_INFO,"ret: %d\n", tmp);
    assert_int_not_equal(tmp, 0);

    pthread_attr_init(&aa);

    ret = pthread_attr_getdetachstate(&aa, &detachstate);
    syslog(LOG_INFO,"ret of getdetachstate: %d\n", ret);
    assert_int_equal(ret, 0);

    ret = pthread_attr_setdetachstate(&aa, PTHREAD_CREATE_DETACHED);
    syslog(LOG_INFO,"ret of setdetachstate: %d\n", ret);
    assert_int_equal(ret, 0);

    pthread_attr_destroy(&aa);

    ret = pthread_join(aThread, NULL);
    syslog(LOG_INFO,"ret of pthread_join: %d\n", ret);
    assert_int_equal(ret, 0);
}
