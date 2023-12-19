/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <nuttx/config.h>
#include <stdio.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <syslog.h>
#include <errno.h>
#include <stdlib.h>
#include "PthreadTest.h"

UINT32 g_testPthreadCount;
UINT32 g_testPthreadTaskMaxNum = 128;
/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: TestPthreadSelf
 ****************************************************************************/
pthread_t TestPthreadSelf(void)
{
    pthread_t tid = pthread_self();
    return tid;
}

/****************************************************************************
 * Name: TestNuttxSyscallTestGroupTearDown
 ****************************************************************************/
