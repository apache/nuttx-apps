/****************************************************************************
 * apps/examples/xrcedds/xrcedds_pub_main.c
 *
 * SPDX-License-Identifier: Apache-2.0
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

#include <nuttx/config.h>

#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <uxr/client/client.h>
#include <ucdr/microcdr.h>

#include "hello_world.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define STREAM_HISTORY 8
#define BUFFER_SIZE    (UXR_CONFIG_UDP_TRANSPORT_MTU * STREAM_HISTORY)

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  uxrUDPTransport transport;
  uxrSession session;
  uxrStreamId reliable_out;
  uint8_t out_stream_buffer[BUFFER_SIZE];
  uint8_t in_stream_buffer[BUFFER_SIZE];
  uxrObjectId participant_id;
  uxrObjectId topic_id;
  uxrObjectId publisher_id;
  uxrObjectId datawriter_id;
  const char *participant_xml;
  const char *topic_xml;
  const char *publisher_xml;
  const char *datawriter_xml;
  uint16_t participant_req;
  uint16_t topic_req;
  uint16_t publisher_req;
  uint16_t datawriter_req;
  uint8_t status[4];
  uint16_t requests[4];
  bool connected;
  uint32_t count;
  char *ip;
  char *port;
  uint32_t max_topics;

  /* Parse the command line. */

  if (argc < 3 || atoi(argv[2]) == 0)
    {
      printf("usage: %s ip port [<max_topics>]\n", argv[0]);
      return 0;
    }

  ip = argv[1];
  port = argv[2];
  max_topics = argc == 4 ? atoi(argv[3]) : UINT32_MAX;

  /* Create the UDP transport. */

  if (!uxr_init_udp_transport(&transport, UXR_IPv4, ip, port))
    {
      printf("Error at create transport.\n");
      return 1;
    }

  /* Create the session. */

  uxr_init_session(&session, &transport.comm, 0xaaaabbbb);
  if (!uxr_create_session(&session))
    {
      printf("Error at create session.\n");
      return 1;
    }

  /* Create the reliable streams. */

  reliable_out = uxr_create_output_reliable_stream(&session,
                                                   out_stream_buffer,
                                                   BUFFER_SIZE,
                                                   STREAM_HISTORY);
  uxr_create_input_reliable_stream(&session, in_stream_buffer,
                                   BUFFER_SIZE, STREAM_HISTORY);

  /* Create the DDS entities. */

  participant_id = uxr_object_id(0x01, UXR_PARTICIPANT_ID);
  participant_xml = "<dds>"
                    "<participant>"
                    "<rtps>"
                    "<name>default_xrce_participant</name>"
                    "</rtps>"
                    "</participant>"
                    "</dds>";
  participant_req = uxr_buffer_create_participant_xml(&session, reliable_out,
                                                      participant_id, 0,
                                                      participant_xml,
                                                      UXR_REPLACE);

  topic_id = uxr_object_id(0x01, UXR_TOPIC_ID);
  topic_xml = "<dds>"
              "<topic>"
              "<name>HelloWorldTopic</name>"
              "<dataType>HelloWorld</dataType>"
              "</topic>"
              "</dds>";
  topic_req = uxr_buffer_create_topic_xml(&session, reliable_out, topic_id,
                                          participant_id, topic_xml,
                                          UXR_REPLACE);

  publisher_id = uxr_object_id(0x01, UXR_PUBLISHER_ID);
  publisher_xml = "";
  publisher_req = uxr_buffer_create_publisher_xml(&session, reliable_out,
                                                  publisher_id,
                                                  participant_id,
                                                  publisher_xml,
                                                  UXR_REPLACE);

  datawriter_id = uxr_object_id(0x01, UXR_DATAWRITER_ID);
  datawriter_xml = "<dds>"
                   "<data_writer>"
                   "<topic>"
                   "<kind>NO_KEY</kind>"
                   "<name>HelloWorldTopic</name>"
                   "<dataType>HelloWorld</dataType>"
                   "</topic>"
                   "</data_writer>"
                   "</dds>";
  datawriter_req = uxr_buffer_create_datawriter_xml(&session, reliable_out,
                                                    datawriter_id,
                                                    publisher_id,
                                                    datawriter_xml,
                                                    UXR_REPLACE);

  /* Send the create entities message and wait for its status. */

  requests[0] = participant_req;
  requests[1] = topic_req;
  requests[2] = publisher_req;
  requests[3] = datawriter_req;

  if (!uxr_run_session_until_all_status(&session, 1000, requests, status, 4))
    {
      printf("Error at create entities: participant: %d topic: %d "
             "publisher: %d datawriter: %d\n",
             status[0], status[1], status[2], status[3]);
      return 1;
    }

  /* Wait for the Agent to match the DataWriter before sending. */

  sleep(3);

  /* Publish samples until the requested number is reached or the session
   * is disconnected.
   */

  connected = true;
  count = 0;

  while (connected && count < max_topics)
    {
      struct hello_world_s topic;
      ucdrBuffer ub;
      uint32_t topic_size;

      topic.index = ++count;
      strlcpy(topic.message, "Hello DDS world!",
              sizeof(topic.message));

      topic_size = hello_world_topic_size(&topic, 0);

      uxr_prepare_output_stream(&session, reliable_out, datawriter_id, &ub,
                                topic_size);
      hello_world_serialize_topic(&ub, &topic);

      printf("Send topic: %s, id: %" PRIu32 "\n", topic.message,
             topic.index);
      connected = uxr_run_session_time(&session, 1000);
    }

  /* Delete the session and close the transport. */

  uxr_delete_session(&session);
  uxr_close_udp_transport(&transport);

  return 0;
}
