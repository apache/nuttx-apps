/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <nuttx/config.h>
#include <stdio.h>
#include <syslog.h>
#include <sched.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <semaphore.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <setjmp.h>
#include <cmocka.h>
#include "SchedTest.h"

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: schedTask01Routine
 ****************************************************************************/
static int schedTask01Routine(int argc, char *argv[])
{
    char *str = NULL;
    str = (char *)malloc(sizeof(char) * 1024);
    assert_non_null(str);
    memset(str, 'A', 1024);
    free(str);
    return 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: TestNuttxSchedTask01
 ****************************************************************************/
void TestNuttxSchedTask01(FAR void **state)
{
    pid_t pid;
    int status;

    pid = task_create("schedTask01Routine", TASK_PRIORITY, DEFAULT_STACKSIZE, schedTask01Routine, NULL);
    assert_true(pid > 0);
    waitpid(pid, &status, 0);
}