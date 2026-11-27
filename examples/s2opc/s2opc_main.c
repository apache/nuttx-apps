/****************************************************************************
 * apps/examples/s2opc/s2opc_main.c
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

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "libs2opc_common_config.h"
#include "libs2opc_server.h"
#include "libs2opc_server_config.h"
#include "libs2opc_server_config_custom.h"
#include "sopc_address_space.h"
#include "sopc_toolkit_config_constants.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define S2OPC_APPLICATION_URI "urn:S2OPC:localhost"
#define S2OPC_PRODUCT_URI     "urn:nuttx:s2opc"

/****************************************************************************
 * External Data
 ****************************************************************************/

extern const bool sopc_embedded_is_const_addspace;
extern SOPC_AddressSpace_Node SOPC_Embedded_AddressSpace_Nodes[];
extern const uint32_t SOPC_Embedded_AddressSpace_nNodes;
extern SOPC_Variant SOPC_Embedded_VariableVariant[];
extern const uint32_t SOPC_Embedded_VariableVariant_nb;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: s2opc_set_address_space
 ****************************************************************************/

static SOPC_ReturnStatus s2opc_set_address_space(void)
{
  FAR SOPC_AddressSpace *address_space;
  SOPC_ReturnStatus status;

  if (!sopc_embedded_is_const_addspace)
    {
      return SOPC_STATUS_NOK;
    }

  address_space = SOPC_AddressSpace_CreateReadOnlyNodes(
    SOPC_Embedded_AddressSpace_nNodes, SOPC_Embedded_AddressSpace_Nodes,
    SOPC_Embedded_VariableVariant_nb, SOPC_Embedded_VariableVariant);
  if (address_space == NULL)
    {
      return SOPC_STATUS_OUT_OF_MEMORY;
    }

  status = SOPC_ServerConfigHelper_SetAddressSpace(address_space);
  if (status != SOPC_STATUS_OK)
    {
      SOPC_AddressSpace_Delete(address_space);
    }

  return status;
}

/****************************************************************************
 * Name: s2opc_configure_server
 ****************************************************************************/

static SOPC_ReturnStatus s2opc_configure_server(FAR const char *endpoint_url)
{
  FAR SOPC_Endpoint_Config *endpoint;
  FAR SOPC_SecurityConfig *security;
  SOPC_ReturnStatus status;

  status = SOPC_ServerConfigHelper_Initialize();
  if (status == SOPC_STATUS_OK)
    {
      status = SOPC_ServerConfigHelper_SetApplicationDescription(
        S2OPC_APPLICATION_URI, S2OPC_PRODUCT_URI, "NuttX S2OPC server",
        NULL, OpcUa_ApplicationType_Server);
    }

  endpoint = NULL;
  if (status == SOPC_STATUS_OK)
    {
      endpoint = SOPC_ServerConfigHelper_CreateEndpoint(endpoint_url, true);
      if (endpoint == NULL)
        {
          status = SOPC_STATUS_OUT_OF_MEMORY;
        }
    }

  security = NULL;
  if (status == SOPC_STATUS_OK)
    {
      security = SOPC_EndpointConfig_AddSecurityConfig(
        endpoint, SOPC_SecurityPolicy_None);
      if (security == NULL)
        {
          status = SOPC_STATUS_OUT_OF_MEMORY;
        }
    }

  if (status == SOPC_STATUS_OK)
    {
      status = SOPC_SecurityConfig_SetSecurityModes(
        security, SOPC_SecurityModeMask_None);
    }

  if (status == SOPC_STATUS_OK)
    {
      status = SOPC_SecurityConfig_AddUserTokenPolicy(
        security, &SOPC_UserTokenPolicy_Anonymous);
    }

  if (status == SOPC_STATUS_OK)
    {
      status = s2opc_set_address_space();
    }

  return status;
}

/****************************************************************************
 * Name: s2opc_run_server
 ****************************************************************************/

static SOPC_ReturnStatus s2opc_run_server(FAR const char *endpoint_url)
{
  SOPC_Log_Configuration log_config;
  SOPC_ReturnStatus status;

  log_config = SOPC_Common_GetDefaultLogConfiguration();
  log_config.logSystem = SOPC_LOG_SYSTEM_NO_LOG;

  status = SOPC_CommonHelper_Initialize(&log_config, NULL);
  if (status == SOPC_STATUS_OK)
    {
      status = s2opc_configure_server(endpoint_url);
    }

  if (status == SOPC_STATUS_OK)
    {
      printf("S2OPC server: %s\n", endpoint_url);
      printf("Press Ctrl-C to stop\n");
      status = SOPC_ServerHelper_Serve(true);
    }

  SOPC_ServerConfigHelper_Clear();
  SOPC_CommonHelper_Clear();
  return status;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  FAR const char *endpoint_url = CONFIG_EXAMPLES_S2OPC_ENDPOINT_URL;
  SOPC_ReturnStatus status;

  if (argc > 2)
    {
      fprintf(stderr, "Usage: %s [endpoint-url]\n", argv[0]);
      return EXIT_FAILURE;
    }

  if (argc > 1)
    {
      endpoint_url = argv[1];
    }

  status = s2opc_run_server(endpoint_url);
  if (status != SOPC_STATUS_OK)
    {
      fprintf(stderr, "s2opc: server failed: %d\n", status);
      return EXIT_FAILURE;
    }

  return EXIT_SUCCESS;
}
