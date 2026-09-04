/****************************************************************************
 * apps/testing/nettest/others/test_others_chksum.c
 ****************************************************************************/

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <cmocka.h>

#include <nuttx/mm/iob.h>
#include <nuttx/net/netdev.h>

#include "test_others.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void test_others_chksum(FAR void **state)
{
  struct iob_s iob1;
  struct iob_s iob2;
  struct iob_s iob3;

  uint8_t data1[] = {0xaa, 0xbb, 0xcc};
  uint8_t data2[] = {0xdd, 0xee};
  uint8_t data[] = {0xaa, 0xbb, 0xcc, 0xdd, 0xee};

  uint16_t chained_sum;
  uint16_t reference_sum;

  memset(&iob1, 0, sizeof(iob1));
  memset(&iob2, 0, sizeof(iob2));
  memset(&iob3, 0, sizeof(iob3));

  /* First IOB: odd number of bytes */

  memcpy(iob1.io_data, data1, sizeof(data1));
  iob1.io_len = sizeof(data1);
  iob1.io_offset = 0;
  iob1.io_flink = &iob2;

  /* Second IOB: zero-length */

  iob2.io_data[0] = 0xdd;
  iob2.io_len = 0;
  iob2.io_offset = 0;
  iob2.io_flink = &iob3;

  /* Third IOB: remaining bytes */

  memcpy(iob3.io_data, data2, sizeof(data2));
  iob3.io_len = sizeof(data2);
  iob3.io_offset = 0;
  iob3.io_flink = NULL;

  chained_sum = chksum_iob(0, &iob1, 0);
  reference_sum = chksum(0, data, sizeof(data));

  assert_int_equal(chained_sum, reference_sum);
}
