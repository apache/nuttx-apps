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

from shv import RpcUrl, SHVValueClient


async def shv_confirm(connection: str, path_to_root: str) -> None:
    url = RpcUrl.parse(connection)
    client = await SHVValueClient.connect(url)
    assert client is not None

    await client.call(f"{path_to_root}/fwStable", "confirm")

    print("FW confirmed.")
    await client.disconnect()
