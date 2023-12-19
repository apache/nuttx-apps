/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <nuttx/config.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <inttypes.h>

#include "SocketTest.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/


/****************************************************************************
 * Name: TestNuttxNetSocketTest06
 ****************************************************************************/

void TestNuttxNetSocketTest06(FAR void **state)
{
    struct in_addr in;
    int ret = inet_pton(AF_INET, "300.10.10.10", &in);
    syslog(LOG_INFO,"ret: %d", ret);
    assert_int_equal(ret, 0);

    ret = inet_pton(AF_INET, "10.11.12.13", &in);
    syslog(LOG_INFO,"ret: %d", ret);
    assert_int_equal(ret, 1);
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    syslog(LOG_INFO,"in.s_addr: %#"PRIx32, in.s_addr);
    assert_int_equal(in.s_addr, 0x0d0c0b0a);
#else
    syslog(LOG_INFO,"in.s_addr: %#"PRIx32, in.s_addr);
    assert_int_equal(in.s_addr, 0x0a0b0c0d);
#endif
    /*Currently nuttx does not support the following interfaces inet_lnaof, inet_netof, inet_makeaddr*/
    // // host order
    // in_addr_t lna = inet_lnaof(in);
    // syslog(LOG_INFO,"lna: %#"PRIx32, lna);
    // assert_int_equal(lna, 0x000b0c0d);

    // // host order
    // in_addr_t net = inet_netof(in);
    // syslog(LOG_INFO,"net: %#"PRIx32, net);
    // assert_int_equal(net, 0x0000000a);

    // in = inet_makeaddr(net, lna);
// #if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
//     syslog(LOG_INFO,"in.s_addr: %#"PRIx32, in.s_addr);
//     assert_int_equal(in.s_addr, 0x0d0c0b0a);
// #else
//     syslog(LOG_INFO,"in.s_addr: %#"PRIx32, in.s_addr);
//     assert_int_equal(in.s_addr, 0x0a0b0c0d);
// #endif

    in_addr_t net = inet_network("300.10.10.10");
    syslog(LOG_INFO,"net: %#"PRIx32, net);
    assert_int_equal((int32_t)net, -1);

    // host order
    net = inet_network("10.11.12.13");
    syslog(LOG_INFO,"host order net: %#"PRIx32, net);
    assert_int_equal(net, 0x0a0b0c0d);

    const char *p = inet_ntoa(in);
    syslog(LOG_INFO,"inet_ntoa p: %s", p);
    assert_int_equal(strcmp(p, "10.11.12.13"), 0);

    char buf[32];
    p = inet_ntop(AF_INET, &in, buf, sizeof(buf));
    syslog(LOG_INFO,"inet_ntop p: %s", p);
    assert_string_equal(p, buf);
    assert_int_equal(strcmp(p, "10.11.12.13"), 0);

}
