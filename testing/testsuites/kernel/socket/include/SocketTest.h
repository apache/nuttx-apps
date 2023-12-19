#ifndef __TEST_H
#define __TEST_H

#include <nuttx/config.h>
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <syslog.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
#define CLIENT_NUM 8

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/* test case function */
/* cases/net_socket_test_005.c ************************************************/
void TestNuttxNetSocketTest05(FAR void **state);
/* cases/net_socket_test_006.c ************************************************/
void TestNuttxNetSocketTest06(FAR void **state);
/* cases/net_socket_test_008.c ************************************************/
void TestNuttxNetSocketTest08(FAR void **state);
/* cases/net_socket_test_009.c ************************************************/
void TestNuttxNetSocketTest09(FAR void **state);
/* cases/net_socket_test_010.c ************************************************/
void TestNuttxNetSocketTest10(FAR void **state);
/* cases/net_socket_test_011.c ************************************************/
void TestNuttxNetSocketTest11(FAR void **state);

#endif
