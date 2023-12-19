/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <nuttx/config.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <stdint.h>
#include <semaphore.h>
#include "PthreadTest.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
static sem_t re_PthreadF01;
/****************************************************************************
 * Public Functions
 ****************************************************************************/
static void *PthreadF01(void *argument)
{
    g_testPthreadCount++;
    sem_wait(&re_PthreadF01);

    return argument;
}
/****************************************************************************
 * Name: TestNuttxPthreadTest18
 ****************************************************************************/

void TestNuttxPthreadTest18(FAR void **state)
{
    pthread_attr_t attr;
    pthread_t newTh;
    UINT32 ret;
    UINTPTR temp;
    int policy;
    struct sched_param param;
    struct sched_param param2 = { 2 }; // 2, init

    g_testPthreadCount = 0;

    ret = sem_init(&re_PthreadF01, 0, 0);
    syslog(LOG_INFO,"ret: %d \n", ret);
    assert_int_equal(ret, 0);

    ret = pthread_attr_init(&attr);
    syslog(LOG_INFO,"ret: %d \n", ret);
    assert_int_equal(ret, 0);

    ret = pthread_create(&newTh, NULL, PthreadF01, (void *)9); // 9, test param of the function.
    syslog(LOG_INFO,"ret: %d \n", ret);
    assert_int_equal(ret, 0);
    usleep(1000);
    // LOS_TaskDelay(1);
    syslog(LOG_INFO,"g_testPthreadCount: %d \n", g_testPthreadCount);
    assert_int_equal(g_testPthreadCount, 1);

    ret = pthread_setschedparam(newTh, -1, &param);
    syslog(LOG_INFO,"ret: %d \n", ret);
    assert_int_equal(ret, EINVAL);

    ret = pthread_setschedparam(newTh, 100, &param); // 4, test for invalid param.
    syslog(LOG_INFO,"ret: %d \n", ret);
    assert_int_equal(ret, EINVAL);

    #if CONFIG_RR_INTERVAL > 0
        param.sched_priority = 31; // 31, init
        ret = pthread_setschedparam(newTh, SCHED_RR, &param);
        syslog(LOG_INFO,"ret: %d \n", ret);
        assert_int_equal(ret, 0);
    #endif

    ret = pthread_getschedparam(newTh, NULL, &param2);
    syslog(LOG_INFO,"ret: %d \n", ret);
    syslog(LOG_INFO,"param2.sched_priority: %d \n", param2.sched_priority);
    assert_int_equal(ret, EINVAL);
    assert_int_equal(param2.sched_priority, 2); // 2, here assert the result.

    ret = pthread_getschedparam(newTh, &policy, NULL);
    syslog(LOG_INFO,"ret: %d \n", ret);
    assert_int_equal(ret, EINVAL);

    ret = pthread_getschedparam(newTh, &policy, &param2);
    syslog(LOG_INFO,"ret: %d \n", ret);
    syslog(LOG_INFO,"param2.sched_priority: %d \n", param2.sched_priority);
    assert_int_equal(ret, 0);
    #if CONFIG_RR_INTERVAL > 0
        assert_int_equal(param2.sched_priority, 31); // 31, here assert the result.
    #endif

    ret = sem_post(&re_PthreadF01);
    syslog(LOG_INFO,"ret of sem_post: %d \n", ret);
    assert_int_equal(ret, 0);
    ret = pthread_join(newTh, (void *)&temp);
    syslog(LOG_INFO,"ret: %d \n", ret);
    syslog(LOG_INFO,"temp: %ld \n", temp);
    assert_int_equal(ret, 0);
    assert_int_equal(temp, 9); // 9, here assert the result.

    #if CONFIG_RR_INTERVAL > 0
        ret = pthread_setschedparam(newTh + 9, SCHED_RR, &param); // 9, test for invalid param.
        syslog(LOG_INFO,"ret: %d \n", ret);
        assert_int_equal(ret, ESRCH);
    #endif

    ret = pthread_getschedparam(newTh + 8, &policy, &param2); // 8, test for invalid param.
    syslog(LOG_INFO,"ret: %d \n", ret);
    assert_int_equal(ret, ESRCH);

    ret = pthread_attr_destroy(&attr);
    syslog(LOG_INFO,"ret: %d \n", ret);
    assert_int_equal(ret, 0);

    ret = sem_destroy(&re_PthreadF01);
    syslog(LOG_INFO,"ret: %d \n", ret);
    assert_int_equal(ret, 0);
}
