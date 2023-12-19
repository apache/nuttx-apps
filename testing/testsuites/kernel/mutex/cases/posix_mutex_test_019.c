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
static pthread_mutex_t *g_mtx;
static sem_t g_semA, g_semB;
static pthread_mutex_t g_mtxNull, g_mtxDef;
static int g_testMutexretval;
static UINT32 g_testMutexCount;
/****************************************************************************
 * Public Functions
 ****************************************************************************/
static void *TaskF01(void *arg)
{
    int ret;

    // TestBusyTaskDelay(20); // 20, Set the timeout of runtime.
    usleep(50000);
    g_testMutexCount++;

    if ((ret = pthread_mutex_lock(g_mtx))) {
        syslog(LOG_INFO,"ret: %d \n", ret);
        goto EXIT;
    }

    if ((ret = sem_post(&g_semA))) {
        syslog(LOG_INFO,"ret: %d \n", ret);
        goto EXIT;
    }

    if ((ret = sem_wait(&g_semB))) {
        syslog(LOG_INFO,"ret: %d \n", ret);
        goto EXIT;
    }

    if (g_testMutexretval != 0) { /* parent thread failed to unlock the mutex) */
        if ((ret = pthread_mutex_unlock(g_mtx))) {
            syslog(LOG_INFO,"ret: %d \n", ret);
            goto EXIT;
        }
    }

    g_testMutexCount++;
    return NULL;


EXIT:
    assert_int_equal(1, 0);
    return NULL;
}
/****************************************************************************
 * Name: TestNuttxMutexTest19
 ****************************************************************************/

void TestNuttxMutexTest19(FAR void **state)
{
    pthread_mutexattr_t mattr;
    pthread_t thr;

    pthread_mutex_t *tabMutex[2];
    int tabRes[2][3] = { {0, 0, 0}, {0, 0, 0} };

    int ret;
    void *thRet = NULL;

    int i;

    g_testMutexretval = 0;

    g_testMutexCount = 0;

    /* We first initialize the two mutexes. */
    if ((ret = pthread_mutex_init(&g_mtxNull, NULL))) {
        syslog(LOG_INFO,"ret: %d \n", ret);
        assert_int_equal(1, 0);
    }

    if ((ret = pthread_mutexattr_init(&mattr))) {
        syslog(LOG_INFO,"ret: %d \n", ret);
        assert_int_equal(1, 0);
    }

    if ((ret = pthread_mutex_init(&g_mtxDef, &mattr))) {
        syslog(LOG_INFO,"ret: %d \n", ret);
        assert_int_equal(1, 0);
    }

    if ((ret = pthread_mutexattr_destroy(&mattr))) {
        syslog(LOG_INFO,"ret: %d \n", ret);
        assert_int_equal(1, 0);
    }

    tabMutex[0] = &g_mtxNull;
    tabMutex[1] = &g_mtxDef;

    if ((ret = sem_init(&g_semA, 0, 0))) {
        syslog(LOG_INFO,"ret: %d \n", ret);
        assert_int_equal(1, 0);
    }

    if ((ret = sem_init(&g_semB, 0, 0))) {
        syslog(LOG_INFO,"ret: %d \n", ret);
        assert_int_equal(1, 0);
    }

    /* OK let's go for the first part of the test : abnormals unlocking */

    /* We first check if unlocking an unlocked mutex returns an uwErr. */
    ret = pthread_mutex_unlock(tabMutex[0]);
    syslog(LOG_INFO,"ret: %d \n", ret);
    assert_int_not_equal(ret, ENOERR);

    ret = pthread_mutex_unlock(tabMutex[1]);
    syslog(LOG_INFO,"ret: %d \n", ret);
    assert_int_not_equal(ret, ENOERR);

    /* Now we focus on unlocking a mutex lock by another thread */
    for (i = 0; i < 2; i++) { // 2, Set the timeout of runtime
        g_mtx = tabMutex[i];
        tabRes[i][0] = 0;
        tabRes[i][1] = 0;
        tabRes[i][2] = 0; // 2, buffer index.

        if ((ret = pthread_create(&thr, NULL, TaskF01, NULL))) {
            syslog(LOG_INFO,"ret: %d \n", ret);
            assert_int_equal(1, 0);
        }

        if (i == 0) {
            syslog(LOG_INFO,"g_testMutexCount: %d \n", g_testMutexCount);
            assert_int_equal(g_testMutexCount, 0);
        }
        if (i == 1) {
            syslog(LOG_INFO,"g_testMutexCount: %d \n", g_testMutexCount);
            assert_int_equal(g_testMutexCount, 2); // 2, Here, assert the g_testMutexCount.
        }

        if ((ret = sem_wait(&g_semA))) {
            syslog(LOG_INFO,"ret: %d \n", ret);
            assert_int_equal(1, 0);
        }

        g_testMutexretval = pthread_mutex_unlock(g_mtx);
        syslog(LOG_INFO,"g_testMutexretval: %d \n", g_testMutexretval);
        assert_int_equal(g_testMutexretval, EPERM);

        if (i == 0) {
            syslog(LOG_INFO,"g_testMutexCount: %d \n", g_testMutexCount);
            assert_int_equal(g_testMutexCount, 1);
        }
        if (i == 1) {
            syslog(LOG_INFO,"g_testMutexCount: %d \n", g_testMutexCount);
            assert_int_equal(g_testMutexCount, 3); // 3, Here, assert the g_testMutexCount.
        }

        if ((ret = sem_post(&g_semB))) {
            syslog(LOG_INFO,"ret: %d \n", ret);
            assert_int_equal(1, 0);
        }

        if ((ret = pthread_join(thr, &thRet))) {
            syslog(LOG_INFO,"ret: %d \n", ret);
            assert_int_equal(1, 0);
        }

        if (i == 0) {
            syslog(LOG_INFO,"g_testMutexCount: %d \n", g_testMutexCount);
            assert_int_equal(g_testMutexCount, 2); // 2, Here, assert the g_testMutexCount.
        }
        if (i == 1) {
            syslog(LOG_INFO,"g_testMutexCount: %d \n", g_testMutexCount);
            assert_int_equal(g_testMutexCount, 4); // 4, Here, assert the g_testMutexCount.
        }

        tabRes[i][0] = g_testMutexretval;
    }

    if (tabRes[0][0] != tabRes[1][0]) {
        assert_int_equal(1, 0);
    }

    /* We start with testing the NULL mutex features */
    (void)pthread_mutexattr_destroy(&mattr);
    ret = pthread_mutex_destroy(&g_mtxNull);
    syslog(LOG_INFO,"ret: %d \n", ret);
    assert_int_equal(ret, 0);

    ret = pthread_mutex_destroy(&g_mtxDef);
    syslog(LOG_INFO,"ret: %d \n", ret);
    assert_int_equal(ret, 0);

    ret = sem_destroy(&g_semA);
    syslog(LOG_INFO,"ret: %d \n", ret);
    assert_int_equal(ret, 0);

    ret = sem_destroy(&g_semB);
    syslog(LOG_INFO,"ret: %d \n", ret);
    assert_int_equal(ret, 0);
}
