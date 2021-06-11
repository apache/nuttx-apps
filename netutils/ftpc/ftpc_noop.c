/****************************************************************************
 * apps/netutils/ftpc/ftpc_noop.c
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

#include "ftpc_config.h"

#include "netutils/ftpc.h"

#include "ftpc_internal.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Public Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: ftpc_noop
 *
 * Description:
 *   No operation command.  Using NOOP allows us to make sure that commands
 *   are passed over the control connection without changing the status of
 *   any data transaction or server status.  This is useful for (1)
 *   maintaining connections during long IDLE times and (2) It can also be
 *   used as a harmless way of detecting timeouts.
 *
 ****************************************************************************/

int ftpc_noop(SESSION handle)
{
  return ftpc_cmd((FAR struct ftpc_session_s *)handle, "NOOP");
}
