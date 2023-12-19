#ifndef PTHREAD_TEST_H
#define PTHREAD_TEST_H

#include <nuttx/config.h>
#include <errno.h>
#include <syslog.h>
#include <stdio.h>
#include <pthread.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include "sched.h"
#include "semaphore.h"
#include "unistd.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
#define MUTEX_TEST_NUM 100
#define ENOERR 0
typedef unsigned int UINT32;

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/


/* test case function */
/* cases/posix_mutex_test_001.c ************************************************/
void TestNuttxMutexTest01(FAR void **state);
/* cases/posix_mutex_test_019.c ************************************************/
void TestNuttxMutexTest19(FAR void **state);
/* cases/posix_mutex_test_020.c ************************************************/
void TestNuttxMutexTest20(FAR void **state);
#endif
