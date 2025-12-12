############################################################################
# apps/examples/shv-nxboot-updater/update-script/shvconfirm.py
#
# Licensed to the Apache Software Foundation (ASF) under one or more
# contributor license agreements.  See the NOTICE file distributed with
# this work for additional information regarding copyright ownership.  The
# ASF licenses this file to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance with the
# License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
# License for the specific language governing permissions and limitations
# under the License.
#
############################################################################

import asyncio
from enum import Enum

from shv.rpcapi.valueclient import SHVValueClient
from shv.rpcurl import RpcUrl


class ConfirmStep(Enum):
    INFO = 0
    END_OK = 1
    END_ERROR = 2


async def _shv_confirm_cleanup(
    msg: str, queue: asyncio.Queue, client: SHVValueClient | None
):
    queue.put_nowait([ConfirmStep.END_ERROR, msg])
    if client is not None:
        await client.disconnect()


async def shv_confirm(
    connection: str, path_to_root: str, queue: asyncio.Queue, method_timeout: float
):
    try:
        url = RpcUrl.parse(connection)
    except ValueError:
        await _shv_confirm_cleanup("The supplied URL is invalid!", queue, None)
        return

    queue.put_nowait([ConfirmStep.INFO, "Connecting to the server"])
    try:
        client = await asyncio.wait_for(
            SHVValueClient.connect(url), timeout=method_timeout
        )
    except Exception:
        await _shv_confirm_cleanup(
            "Could not connect to the broker. "
            + "Is the broker running or is the url OK?",
            queue,
            None,
        )
        return
    if client is None:
        await _shv_confirm_cleanup("Could not connect to the broker.", queue, None)
        return

    queue.put_nowait([ConfirmStep.INFO, "Confirming the running firmware."])
    try:
        await asyncio.wait_for(
            client.call(f"{path_to_root}/fwStable", "confirm"), timeout=method_timeout
        )
    except TimeoutError:
        await _shv_confirm_cleanup(
            "Timeout when calling the confirm method.", queue, client
        )
        return
    except Exception:
        await _shv_confirm_cleanup("Failed calling the confirm method.", queue, client)
        return

    await client.disconnect()
    queue.put_nowait([ConfirmStep.END_OK, 0])
