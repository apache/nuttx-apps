/****************************************************************************
 * apps/examples/canardv1/canard_main.c
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <canard.h>
#include <canard_dsdl.h>
#include <o1heap.h>

#include <sched.h>

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

#include <net/if.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#include <poll.h>

#include <nuttx/can.h>
#include <netpacket/can.h>

#include "socketcan.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Application constants */

#define APP_VERSION_MAJOR                        1
#define APP_VERSION_MINOR                        0
#define APP_NODE_NAME                            CONFIG_EXAMPLES_LIBCANARDV1_APP_NODE_NAME

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* Arena for memory allocation, used by the library */

#define O1_HEAP_SIZE CONFIG_EXAMPLES_LIBCANARDV1_NODE_MEM_POOL_SIZE

/* Temporary development UAVCAN topic service ID to publish/subscribe from */
#define PORT_ID                                  4421
#define TOPIC_SIZE                               512

O1HeapInstance *my_allocator;
static uint8_t uavcan_heap[O1_HEAP_SIZE]
__attribute__((aligned(O1HEAP_ALIGNMENT)));

/* Node status variables */

static bool g_canard_daemon_started;

static uint8_t my_message_transfer_id;

struct pollfd fd;

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: memallocate
 *
 * Description:
 *
 ****************************************************************************/

static void *memallocate(CanardInstance *const ins, const size_t amount)
{
  (void) ins;
  return o1heapAllocate(my_allocator, amount);
}

/****************************************************************************
 * Name: memfree
 *
 * Description:
 *
 ****************************************************************************/

static void memfree(CanardInstance *const ins, void *const pointer)
{
  (void) ins;
  o1heapFree(my_allocator, pointer);
}

/****************************************************************************
 * Name: getmonotonictimestampusec
 *
 * Description:
 *
 ****************************************************************************/

uint64_t getmonotonictimestampusec(void)
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
 * Name: process1hztasks
 *
 * Description:
 *   This function is called at 1 Hz rate from the main loop.
 *
 ****************************************************************************/

void process1hztasks(CanardInstance *ins, uint64_t timestamp_usec)
{
  CanardMicrosecond transmission_deadline =
                      getmonotonictimestampusec() + 1000 * 10;

  const CanardTransfer transfer =
    {
      .timestamp_usec = transmission_deadline,      /* Zero if transmission deadline is not limited. */
      .priority       = CanardPriorityNominal,
      .transfer_kind  = CanardTransferKindMessage,
      .port_id        = 1234,                       /* This is the subject-ID. */
      .remote_node_id = CANARD_NODE_ID_UNSET,       /* Messages cannot be unicast, so use UNSET. */
      .transfer_id    = my_message_transfer_id,
      .payload_size   = 47,
      .payload        = "\x2D\x00"
                        "Sancho, it strikes me thou art in great fear.",
    };

  ++my_message_transfer_id;  /* The transfer-ID shall be incremented after every transmission on this subject. */
  int32_t result = canardTxPush(ins, &transfer);

  if (result < 0)
    {
      /* An error has occurred: either an argument is invalid or we've
       * ran out of memory. It is possible to statically prove that an
       * out-of-memory will never occur for a given application if the
       * heap is sized correctly; for background, refer to the Robson's
       * Proof and the documentation for O1Heap.
       */

      fprintf(stderr, "Transmit error %ld\n", result);
    }
}

/****************************************************************************
 * Name: processTxRxOnce
 *
 * Description:
 *   Transmits all frames from the TX queue, receives up to one frame.
 *
 ****************************************************************************/

void processtxrxonce(CanardInstance *ins, canardsocketinstance *sock_ins,
                     int timeout_msec)
{
  int32_t result;

  /* Transmitting, Look at the top of the TX queue. */

  for (const CanardFrame *txf = NULL; (txf = canardTxPeek(ins)) != NULL; )
    {
      if (txf->timestamp_usec > getmonotonictimestampusec()) /* Check if the frame has timed out. */
        {
          if (socketcantransmit(sock_ins, txf) == 0)  /* Send the frame. Redundant interfaces may be used here. */
            {
              /* If the driver is busy, break and retry later. */

              break;
            }
        }

       canardTxPop(ins); /* Remove the frame from the queue after it's transmitted. */
       ins->memory_free(ins, (CanardFrame *)txf);
    }

  /* Poll receive */

  if (poll(&fd, 1, timeout_msec) <= 0)
    {
      return;
    }

  /* Receiving */

  CanardFrame received_frame;

  socketcanreceive(sock_ins, &received_frame);

  CanardTransfer receive;
  result = canardRxAccept(ins,
    &received_frame, /* The CAN frame received from the bus. */
    0,               /* If the transport is not redundant, use 0. */
    &receive);

  if (result < 0)
    {
      /* An error has occurred: either an argument is invalid or we've ran
       * out of memory. It is possible to statically prove that an
       * out-of-memory will never occur for a given application
       * if the heap is sized correctly; for background, refer to the
       * Robson's Proof and the documentation for O1Heap.
       * Reception of an invalid frame is NOT an error.
       */

      fprintf(stderr, "Receive error %ld\n", result);
    }
  else if (result == 1)
    {
      /* A transfer has been received, process it */

      printf("Receive UAVCAN port id%d TODO process me\n",
             receive.port_id);

      ins->memory_free(ins, (void *)receive.payload);
    }
  else
    {
      /* Nothing to do.
       * The received frame is either invalid or it's a non-last frame
       * of a multi-frame transfer.
       * Reception of an invalid frame is NOT reported as an error
       * because it is not an error.
       */
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
  int errval = 0;
  int can_fd = 0;
  int pub = 1;

  if (argc > 2)
    {
      for (int args = 2; args < argc; args++)
        {
          if (!strcmp(argv[args], "canfd"))
            {
              can_fd = 1;
            }

          if (!strcmp(argv[args], "pub"))
            {
              pub = 1;
            }

          if (!strcmp(argv[args], "sub"))
            {
              pub = 0;
            }
        }
    }

  my_allocator = o1heapInit(&uavcan_heap, O1_HEAP_SIZE);

  if (my_allocator == NULL)
    {
      printf("o1heapInit failed with size %d\n", O1_HEAP_SIZE);
      errval = 2;
      goto errout_with_dev;
    }

  CanardInstance ins = canardInit(&memallocate, &memfree);

  if (can_fd)
    {
      ins.mtu_bytes = CANARD_MTU_CAN_FD;
    }
  else
    {
      ins.mtu_bytes = CANARD_MTU_CAN_CLASSIC;
    }

  ins.node_id = (pub ? CONFIG_EXAMPLES_LIBCANARDV1_NODE_ID :
                       CONFIG_EXAMPLES_LIBCANARDV1_NODE_ID + 1);

  /* Open the CAN device for reading */

  canardsocketinstance sock_ins;
  socketcanopen(&sock_ins, CONFIG_EXAMPLES_LIBCANARDV1_DEV, can_fd);

  /* setup poll fd */

  fd.fd = sock_ins.s;
  fd.events = POLLIN;

  if (sock_ins.s < 0)
    {
      printf("canard_daemon: ERROR: open %s failed: %d\n",
         CONFIG_EXAMPLES_LIBCANARDV1_DEV, errno);
      errval = 2;
      goto errout_with_dev;
    }

  printf("canard_daemon: canard initialized\n");
  printf("start node (ID: %d Name: %s MTU: %d PUB: %d TOPIC_SIZE: %d)\n",
         ins.node_id, APP_NODE_NAME, ins.mtu_bytes, pub, TOPIC_SIZE);

  CanardRxSubscription heartbeat_subscription;
  (void) canardRxSubscribe(&ins,   /* Subscribe to messages uavcan.node.Heartbeat. */
     CanardTransferKindMessage,
     32085,  /* The fixed Subject-ID of the Heartbeat message type (see DSDL definition). */
     7,      /* The maximum payload size (max DSDL object size) from the DSDL definition. */
     CANARD_DEFAULT_TRANSFER_ID_TIMEOUT_USEC,
     &heartbeat_subscription);

  CanardRxSubscription my_subscription;
  (void) canardRxSubscribe(&ins,
     CanardTransferKindMessage,
     PORT_ID,    /* The Service-ID to subscribe to. */
     TOPIC_SIZE, /* The maximum payload size (max DSDL object size). */
     CANARD_DEFAULT_TRANSFER_ID_TIMEOUT_USEC,
     &my_subscription);

  g_canard_daemon_started = true;
  uint64_t next_1hz_service_at = getmonotonictimestampusec();

  for (; ; )
    {
      processtxrxonce(&ins, &sock_ins, 10);

      const uint64_t ts = getmonotonictimestampusec();

      if (ts >= next_1hz_service_at)
        {
          next_1hz_service_at += 1000000;
          process1hztasks(&ins, ts);
        }
    }

errout_with_dev:

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

int canardv1_main(int argc, FAR char *argv[])
{
  int ret;

  printf("canard_main: Starting canard_daemon\n");

  if (g_canard_daemon_started)
    {
      printf("canard_main: receive and send task already running\n");
      return EXIT_SUCCESS;
    }

  ret = task_create("canard_daemon",
                    CONFIG_EXAMPLES_LIBCANARDV1_DAEMON_PRIORITY,
                    CONFIG_EXAMPLES_LIBCANARDV1_DAEMON_STACK_SIZE,
                    canard_daemon, argv);

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
