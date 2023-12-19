/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <nuttx/config.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/socket.h>
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
 * Name: TestNuttxNetSocketTest05
 ****************************************************************************/

void TestNuttxNetSocketTest05(FAR void **state)
{
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    uint32_t hl = ntohl(0x12345678);
    syslog(LOG_INFO,"hl %#"PRIx32, hl);
    assert_int_equal(hl, 0x78563412);

    uint32_t nl = htonl(0x12345678);
    syslog(LOG_INFO,"nl %#"PRIx32, nl);
    assert_int_equal(nl, 0x78563412);

    uint16_t hs = ntohs(0x1234);
    syslog(LOG_INFO,"hs %#"PRIx16, hs);
    assert_int_equal(hs, 0x3412);

    uint16_t ns = htons(0x1234);
    syslog(LOG_INFO,"ns %#"PRIx16, ns);
    assert_int_equal(ns, 0x3412);
#else
    uint32_t hl = ntohl(0x12345678);
    syslog(LOG_INFO,"hl %#"PRIx32, hl);
    assert_int_equal(hl, 0x12345678);

    uint32_t nl = htonl(0x12345678);
    syslog(LOG_INFO,"nl %#"PRIx32, nl);
    assert_int_equal(nl, 0x12345678);

    uint16_t hs = ntohs(0x1234);
    syslog(LOG_INFO,"hs %#"PRIx16, hs);
    assert_int_equal(hs, 0x1234);

    uint16_t ns = htons(0x1234);
    syslog(LOG_INFO,"ns %#"PRIx16, ns);
    assert_int_equal(ns, 0x1234);
#endif
}
