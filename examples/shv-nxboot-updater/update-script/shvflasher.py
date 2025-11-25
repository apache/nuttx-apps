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

from shv import RpcUrl, SHVBytes, SHVValueClient


async def shv_flasher(
    connection: str, name: str, path_to_root: str, queue: asyncio.Queue | None
) -> None:
    url = RpcUrl.parse(connection)
    client = await SHVValueClient.connect(url)
    assert client is not None
    node_name = f"{path_to_root}/fwUpdate"
    node_name_dotdevice = f"{path_to_root}/.device"

    res = await client.call(node_name, "stat")

    maxfilesize = res[1]
    maxwrite = res[5]
    print(f"Received maximum enabled write size {maxwrite}.")
    print(f"Received maximum file size is {maxfilesize}")
    print(f"Started uploading new firmware {name}... this may take some time.")
    size = 0
    with open(name, mode="rb") as f:
        # first, compute the CRC from the zlib library
        # turns out, NuttX uses the same polynomial

        if queue:
            f.seek(0, io.SEEK_END)
            size = f.tell()
            print(f"File's size is {size}")
            f.seek(0, io.SEEK_SET)
            transfers = size / maxwrite

        i = 0
        crc = 0
        while data := f.read(maxwrite):
            print("Uploading chunk n.", i + 1)
            crc = zlib.crc32(data, crc)
            offset = i * maxwrite
            res = await client.call(node_name, "write", [offset, SHVBytes(data)])
            i += 1
            if queue:
                currProgress = (int)((i * 100) / transfers)
                queue.put_nowait(currProgress)

    print("Flashing completed!")

    # now get the CRC from the device and reset the device, if OK
    res = await client.call(node_name, "crc", [0, size])
    # just to be sure, make it unsigned
    res = res & 0xFFFFFFFF
    # the result of the CRC is signed, actually, so make reinterpret it as unsigned
    print(f"Calculated CRC: {hex(crc)} and received: {hex(res)}")

    if res == crc:
        print("Checksum match, perform reset!")
        res = await client.call(node_name_dotdevice, "reset")
    else:
        print("Checksum mismatch!")

    await client.disconnect()
