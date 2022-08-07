/****************************************************************************
 * apps/examples/canardv0/canard_main.c
 *
 *   Copyright (C) 2016 ETH Zuerich. All rights reserved.
 *   Author: Matthias Renner <rennerm@ethz.ch>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/can/can.h>
#include <canard.h>
#include <canard_nuttx.h>       /* CAN backend driver for nuttx, distributed
                                 * with Libcanard */

#include <sys/ioctl.h>
#include <sched.h>

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Application constants */

#define APP_VERSION_MAJOR                        1
#define APP_VERSION_MINOR                        0
#define APP_NODE_NAME                            CONFIG_EXAMPLES_LIBCANARDV0_APP_NODE_NAME
#define GIT_HASH                                 0xb28bf6ac

/* Some useful constants defined by the UAVCAN specification.
 * Data type signature values can be easily obtained with the script
 * show_data_type_info.py
 */

#define UAVCAN_NODE_STATUS_MESSAGE_SIZE          7
#define UAVCAN_NODE_STATUS_DATA_TYPE_ID          341
#define UAVCAN_NODE_STATUS_DATA_TYPE_SIGNATURE   0x0f0868d0c1a7c6f1

#define UAVCAN_NODE_HEALTH_OK                    0
#define UAVCAN_NODE_HEALTH_WARNING               1
#define UAVCAN_NODE_HEALTH_ERROR                 2
#define UAVCAN_NODE_HEALTH_CRITICAL              3

#define UAVCAN_NODE_MODE_OPERATIONAL             0
#define UAVCAN_NODE_MODE_INITIALIZATION          1

#define UAVCAN_GET_NODE_INFO_RESPONSE_MAX_SIZE   ((3015 + 7) / 8)
#define UAVCAN_GET_NODE_INFO_DATA_TYPE_SIGNATURE 0xee468a8121c46a9e
#define UAVCAN_GET_NODE_INFO_DATA_TYPE_ID        1

#define UNIQUE_ID_LENGTH_BYTES                   16

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* Library instance.
 * In simple applications it makes sense to make it static, but it is not
 * necessary.
 */

static CanardInstance canard;

/* Arena for memory allocation, used by the library */

static uint8_t canard_memory_pool
               [CONFIG_EXAMPLES_LIBCANARDV0_NODE_MEM_POOL_SIZE];

static uint8_t unique_id[UNIQUE_ID_LENGTH_BYTES] =
{ 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x01
};

/* Node status variables */

static uint8_t node_health = UAVCAN_NODE_HEALTH_OK;
static uint8_t node_mode = UAVCAN_NODE_MODE_INITIALIZATION;
static bool g_canard_daemon_started;

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: getMonotonicTimestampUSec
 *
 * Description:
 *
 ****************************************************************************/

uint64_t getMonotonicTimestampUSec(void)
{
  struct timespec ts;

  memset(&ts, 0, sizeof(ts));
  if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0)
    {
      abort();
    }

  return ts.tv_sec * 1000000ULL + ts.tv_nsec / 1000ULL;
}

/****************************************************************************
 * Name: makeNodeStatusMessage
 *
 * Description:
 *
 ****************************************************************************/

void makeNodeStatusMessage(uint8_t buffer[UAVCAN_NODE_STATUS_MESSAGE_SIZE])
{
  static uint32_t started_at_sec = 0;

  memset(buffer, 0, UAVCAN_NODE_STATUS_MESSAGE_SIZE);

  if (started_at_sec == 0)
    {
      started_at_sec = (uint32_t) (getMonotonicTimestampUSec() / 1000000U);
    }

  const uint32_t uptime_sec =
    (uint32_t) ((getMonotonicTimestampUSec() / 1000000U) - started_at_sec);

  /* Here we're using the helper for demonstrational purposes; in this
   * simple case it could be preferred to encode the values manually.
   */

  canardEncodeScalar(buffer, 0, 32, &uptime_sec);
  canardEncodeScalar(buffer, 32, 2, &node_health);
  canardEncodeScalar(buffer, 34, 3, &node_mode);
}

/****************************************************************************
 * Name: onTransferReceived
 *
 * Description:
 *   This callback is invoked by the library when a new message or request
 *   or response is received.
 *
 ****************************************************************************/

static void onTransferReceived(CanardInstance *ins,
                               CanardRxTransfer *transfer)
{
  if ((transfer->transfer_type == CanardTransferTypeRequest) &&
      (transfer->data_type_id == UAVCAN_GET_NODE_INFO_DATA_TYPE_ID))
    {
      printf("GetNodeInfo request from %d\n", transfer->source_node_id);

      uint8_t buffer[UAVCAN_GET_NODE_INFO_RESPONSE_MAX_SIZE];
      memset(buffer, 0, UAVCAN_GET_NODE_INFO_RESPONSE_MAX_SIZE);

      /* NodeStatus */

      makeNodeStatusMessage(buffer);

      /* SoftwareVersion */

      buffer[7] = APP_VERSION_MAJOR;
      buffer[8] = APP_VERSION_MINOR;
      buffer[9] = 1;            /* Optional field flags, VCS commit is set */

      /* uint32_t u32 = GIT_HASH;
       * canardEncodeScalar(buffer, 80, 32, &u32);
       */

      /* Image CRC skipped */

      /* HardwareVersion */

      /* Major skipped */

      /* Minor skipped */

      memcpy(&buffer[24], unique_id, UNIQUE_ID_LENGTH_BYTES);

      /* Certificate of authenticity skipped */

      /* Name */

      const size_t name_len = strlen(APP_NODE_NAME);
      memcpy(&buffer[41], APP_NODE_NAME, name_len);

      const size_t total_size = 41 + name_len;

      /* Transmitting; in this case we don't have to release the payload
       * because it's empty anyway.
       */

      const int resp_res =
        canardRequestOrRespond(ins,
                               transfer->source_node_id,
                               UAVCAN_GET_NODE_INFO_DATA_TYPE_SIGNATURE,
                               UAVCAN_GET_NODE_INFO_DATA_TYPE_ID,
                               &transfer->transfer_id,
                               transfer->priority,
                               CanardResponse,
                               &buffer[0],
                               (uint16_t) total_size);
      if (resp_res <= 0)
        {
          fprintf(stderr, "Could not respond to GetNodeInfo; error %d\n",
                  resp_res);
        }
    }
}

/****************************************************************************
 * Name: shouldAcceptTransfer
 *
 * Description:
 *   This callback is invoked by the library when it detects beginning of a
 *   new transfer on the bus that can be received by the local node.
 *   If the callback returns true, the library will receive the transfer.
 *   If the callback returns false, the library will ignore the transfer.
 *   All transfers that are addressed to other nodes are always ignored.
 *
 ****************************************************************************/

static bool shouldAcceptTransfer(const CanardInstance * ins,
                                 uint64_t * out_data_type_signature,
                                 uint16_t data_type_id,
                                 CanardTransferType transfer_type,
                                 uint8_t source_node_id)
{
  if (canardGetLocalNodeID(ins) == CANARD_BROADCAST_NODE_ID)
    {
      /* If we're in the process of allocation of dynamic node ID, accept
       * only relevant transfers.
       */
    }
  else
    {
      if ((transfer_type == CanardTransferTypeRequest) &&
          (data_type_id == UAVCAN_GET_NODE_INFO_DATA_TYPE_ID))
        {
          *out_data_type_signature =
           UAVCAN_GET_NODE_INFO_DATA_TYPE_SIGNATURE;
          return true;
        }
    }

  return false;
}

/****************************************************************************
 * Name: process1HzTasks
 *
 * Description:
 *   This function is called at 1 Hz rate from the main loop.
 *
 ****************************************************************************/

void process1HzTasks(uint64_t timestamp_usec)
{
  /* Purging transfers that are no longer transmitted. This will occasionally
   * free up some memory.
   */

  canardCleanupStaleTransfers(&canard, timestamp_usec);

  /* Printing the memory usage statistics. */

    {
      const CanardPoolAllocatorStatistics stats =
        canardGetPoolAllocatorStatistics(&canard);
      const unsigned peak_percent =
        100U * stats.peak_usage_blocks / stats.capacity_blocks;

#ifdef CONFIG_DEBUG_CAN
      printf("Memory pool stats: capacity %u blocks, usage %u blocks,"
             " peak usage %u blocks (%u%%)\n",
         stats.capacity_blocks, stats.current_usage_blocks,
         stats.peak_usage_blocks, peak_percent);
#endif

      /* The recommended way to establish the minimal size of the memory pool
       * is to stress-test the application and record the worst case memory
       * usage.
       */

      if (peak_percent > 70)
        {
          puts("WARNING: ENLARGE MEMORY POOL");
        }
    }

  /* Transmitting the node status message periodically. */

    {
      uint8_t buffer[UAVCAN_NODE_STATUS_MESSAGE_SIZE];
      makeNodeStatusMessage(buffer);

      static uint8_t transfer_id;

      const int bc_res =
        canardBroadcast(&canard, UAVCAN_NODE_STATUS_DATA_TYPE_SIGNATURE,
                        UAVCAN_NODE_STATUS_DATA_TYPE_ID, &transfer_id,
                        CANARD_TRANSFER_PRIORITY_LOW,
                        buffer, UAVCAN_NODE_STATUS_MESSAGE_SIZE);
      if (bc_res <= 0)
        {
          fprintf(stderr, "Could not broadcast node status; error %d\n",
                  bc_res);
        }
    }

    {
      static uint8_t transfer_id;
      uint8_t payload[1];
      uint8_t dest_id = 2;
      const int resp_res =
        canardRequestOrRespond(&canard, dest_id,
                               UAVCAN_GET_NODE_INFO_DATA_TYPE_SIGNATURE,
                               UAVCAN_GET_NODE_INFO_DATA_TYPE_ID,
                               &transfer_id,
                               CANARD_TRANSFER_PRIORITY_LOW, CanardRequest,
                               payload, 0);
      if (resp_res <= 0)
        {
          fprintf(stderr, "Could not request GetNodeInfo; error %d\n",
                  resp_res);
        }
    }

  node_mode = UAVCAN_NODE_MODE_OPERATIONAL;
}

/****************************************************************************
 * Name: processTxRxOnce
 *
 * Description:
 *   Transmits all frames from the TX queue, receives up to one frame.
 *
 ****************************************************************************/

void processTxRxOnce(CanardNuttXInstance * nuttxcan, int timeout_msec)
{
  const CanardCANFrame *txf;

  /* Transmitting */

  for (txf = NULL; (txf = canardPeekTxQueue(&canard)) != NULL; )
    {
      const int tx_res = canardNuttXTransmit(nuttxcan, txf, 0);
      if (tx_res < 0)           /* Failure - drop the frame and report */
        {
          canardPopTxQueue(&canard);
          fprintf(stderr,
                  "Transmit error %d, frame dropped, errno '%s'\n",
                  tx_res, strerror(errno));
        }
      else if (tx_res > 0)      /* Success - just drop the frame */
        {
          canardPopTxQueue(&canard);
        }
      else                      /* Timeout - just exit and try again later */
        {
          break;
        }
    }

  /* Receiving */

  CanardCANFrame rx_frame;
  const uint64_t timestamp = getMonotonicTimestampUSec();
  const int rx_res = canardNuttXReceive(nuttxcan, &rx_frame, timeout_msec);

  if (rx_res < 0)               /* Failure - report */
    {
      fprintf(stderr, "Receive error %d, errno '%s'\n", rx_res,
              strerror(errno));
    }
  else if (rx_res > 0)          /* Success - process the frame */
    {
      canardHandleRxFrame(&canard, &rx_frame, timestamp);
    }
  else
    {
      ;                         /* Timeout - nothing to do */
    }
}

/****************************************************************************
 * Name: canard_daemon
 *
 * Description:
 *
 ****************************************************************************/

static int canard_daemon(int argc, char *argv[])
{
  static CanardNuttXInstance canardnuttx_instance;
#ifdef CONFIG_DEBUG_CAN
  struct canioc_bittiming_s bt;
#endif
  int errval = 0;
  int ret;

  /* Initialization of the CAN hardware is performed by external, board-
   * specific logic to running this test.
   */

  /* Open the CAN device for reading */

  ret = canardNuttXInit(&canardnuttx_instance,
                        CONFIG_EXAMPLES_LIBCANARDV0_DEVPATH);
  if (ret < 0)
    {
      printf("canard_daemon: ERROR: open %s failed: %d\n",
             CONFIG_EXAMPLES_LIBCANARDV0_DEVPATH, errno);
      errval = 2;
      goto errout_with_dev;
    }

#ifdef CONFIG_DEBUG_CAN
  /* Show bit timing information if provided by the driver.  Not all CAN
   * drivers will support this IOCTL.
   */

  ret =
    ioctl(canardNuttXGetDeviceFileDescriptor(&canardnuttx_instance),
          CANIOC_GET_BITTIMING, (unsigned long)((uintptr_t)&bt));
  if (ret < 0)
    {
      printf("canard_daemon: Bit timing not available: %d\n", errno);
    }
  else
    {
      printf("canard_daemon: Bit timing:\n");
      printf("canard_daemon:    Baud: %lu\n", (unsigned long)bt.bt_baud);
      printf("canard_daemon:   TSEG1: %u\n", bt.bt_tseg1);
      printf("canard_daemon:   TSEG2: %u\n", bt.bt_tseg2);
      printf("canard_daemon:     SJW: %u\n", bt.bt_sjw);
    }
#endif

  canardInit(&canard, canard_memory_pool, sizeof(canard_memory_pool),
             onTransferReceived, shouldAcceptTransfer, (void *)(12345));
  canardSetLocalNodeID(&canard, CONFIG_EXAMPLES_LIBCANARDV0_NODE_ID);
  printf("canard_daemon: canard initialized\n");
  printf("start node (ID: %d Name: %s)\n",
         CONFIG_EXAMPLES_LIBCANARDV0_NODE_ID,
         APP_NODE_NAME);

  g_canard_daemon_started = true;
  uint64_t next_1hz_service_at = getMonotonicTimestampUSec();

  for (; ; )
    {
      processTxRxOnce(&canardnuttx_instance, 10);

      const uint64_t ts = getMonotonicTimestampUSec();

      if (ts >= next_1hz_service_at)
        {
          next_1hz_service_at += 1000000;
          process1HzTasks(ts);
        }
    }

errout_with_dev:
  canardNuttXClose(&canardnuttx_instance);

  g_canard_daemon_started = false;
  printf("canard_daemon: Terminating!\n");
  fflush(stdout);
  return errval;
}

/****************************************************************************
 * Name: canard_main
 *
 * Description:
 *
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  int ret;

  printf("canard_main: Starting canard_daemon\n");
  if (g_canard_daemon_started)
    {
      printf("canard_main: receive and send task already running\n");
      return EXIT_SUCCESS;
    }

  ret = task_create("canard_daemon",
                    CONFIG_EXAMPLES_LIBCANARDV0_DAEMON_PRIORITY,
                    CONFIG_EXAMPLES_LIBCANARDV0_STACKSIZE, canard_daemon,
                    NULL);
  if (ret < 0)
    {
      int errcode = errno;
      printf("canard_main: ERROR: Failed to start canard_daemon: %d\n",
             errcode);
      return EXIT_FAILURE;
    }

  printf("canard_main: canard_daemon started\n");
  return EXIT_SUCCESS;
}
