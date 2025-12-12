############################################################################
# apps/examples/shv-nxboot-updater/update-script/shvflasher.py
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
import io
import zlib
from enum import Enum

from shv import SHVBytes
from shv.rpcapi.valueclient import SHVValueClient
from shv.rpcurl import RpcUrl


class FlashStep(Enum):
    INFO = (0,)
    WRITE_PROGRESS = (1,)
    WRITE_COMPLETE = (2,)
    END_OK = (3,)
    END_ERROR = 4


async def _shv_flasher_cleanup(
    msg: str, queue: asyncio.Queue, client: SHVValueClient | None
):
    queue.put_nowait([FlashStep.END_ERROR, msg])
    if client is not None:
        await client.disconnect()


async def shv_flasher(
    connection: str,
    name: str,
    path_to_root: str,
    queue: asyncio.Queue,
    method_timeout: float,
):
    try:
        url = RpcUrl.parse(connection)
    except ValueError:
        await _shv_flasher_cleanup("The supplied URL is invalid!", queue, None)
        return
    queue.put_nowait([FlashStep.INFO, "Connecting to the server"])
    try:
        client = await asyncio.wait_for(
            SHVValueClient.connect(url), timeout=method_timeout
        )
    except Exception:
        await _shv_flasher_cleanup(
            "Could not connect to the broker. "
            + "Is the broker running or is the url OK?",
            queue,
            None,
        )
        return
    if client is None:
        await _shv_flasher_cleanup("Could not connect to the broker.", queue, None)
        return

    node_name = f"{path_to_root}/fwUpdate"
    node_name_dotdevice = f"{path_to_root}/.device"
    try:
        res = await asyncio.wait_for(
            client.call(node_name, "stat"), timeout=method_timeout
        )
    except TimeoutError:
        await _shv_flasher_cleanup(
            "Timeout when calling the stat method.", queue, client
        )
        return
    except Exception:
        await _shv_flasher_cleanup("Failed calling the stat method.", queue, client)
        return

    maxfilesize = res[1]
    maxwrite = res[5]
    size = 0
    queue.put_nowait(
        [FlashStep.INFO, f"Received maximum enabled write size {maxwrite}."]
    )
    queue.put_nowait([FlashStep.INFO, f"Received maximum file size is {maxfilesize}"])
    queue.put_nowait([FlashStep.INFO, f"Started uploading new firmware {name}."])
    with open(name, mode="rb") as f:
        # first, compute the CRC from the zlib library
        # turns out, NuttX uses the same polynomial

        f.seek(0, io.SEEK_END)
        size = f.tell()
        queue.put_nowait([FlashStep.INFO, f"File's size is {size}"])
        f.seek(0, io.SEEK_SET)
        transfers = size / maxwrite

        i = 0
        crc = 0
        while data := f.read(maxwrite):
            crc = zlib.crc32(data, crc)
            offset = i * maxwrite
            try:
                res = await asyncio.wait_for(
                    client.call(node_name, "write", [offset, SHVBytes(data)]),
                    timeout=method_timeout,
                )
            except TimeoutError:
                await _shv_flasher_cleanup(
                    "Timeout when calling the write method", queue, client
                )
                return
            except Exception:
                await _shv_flasher_cleanup(
                    "Failed calling the write method.", queue, client
                )
                return
            i += 1
            currProgress = (int)((i * 100) / transfers)
            queue.put_nowait([FlashStep.WRITE_PROGRESS, currProgress])

    queue.put_nowait([FlashStep.WRITE_COMPLETE, "Flashing completed!"])

    # Now get the CRC from the device and reset the device, if OK.
    # The crc can take a while to calculate, so set it to 20 seconds.
    try:
        res = await asyncio.wait_for(
            client.call(node_name, "crc", [0, size]), timeout=20
        )
    except TimeoutError:
        queue.put_nowait([FlashStep.END_ERROR, "Timeout when calling the crc method."])
        await client.disconnect()
        return
    except Exception:
        queue.put_nowait([FlashStep.END_ERROR, "Failed calling the crc method."])
        await client.disconnect()
        return
    # just to be sure, make it unsigned
    res = res & 0xFFFFFFFF
    # the result of the CRC is signed, actually, so make reinterpret it as unsigned
    queue.put_nowait(
        [FlashStep.INFO, f"Calculated CRC {hex(crc)} and received {hex(res)}"]
    )

    if res == crc:
        queue.put_nowait([FlashStep.INFO, "Performing reset on the target device."])
        try:
            res = await asyncio.wait_for(
                client.call(node_name_dotdevice, "reset"), timeout=method_timeout
            )
        except TimeoutError:
            queue.put_nowait(
                [FlashStep.END_ERROR, "Timeout when calling the reset method."]
            )
            await client.disconnect()
            return
        except Exception:
            queue.put_nowait([FlashStep.END_ERROR, "Failed calling the reset method."])
            await client.disconnect()
            return

        await client.disconnect()
        queue.put_nowait([FlashStep.END_OK, 0])

    else:
        await client.disconnect()
        queue.put_nowait([FlashStep.END_ERROR, "CRC Mismatch!"])
