# SPDX-License-Identifier: Apache-2.0
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
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""Exercise NuttX Bluetooth Hosts against a simulated radio environment."""

import os
import re
from pathlib import Path

import pytest
from bsworld.devices import BSController, ProcessInstance


@pytest.fixture
def host(topo):
    """Attach a newly built NuttX simulator to its own Python Controller."""

    def load(binary_env, label, address):
        value = os.environ.get(binary_env)
        if not value:
            pytest.fail(f"{binary_env} must identify the NuttX binary under test")
        binary = Path(value).resolve()
        if not binary.is_file() or not os.access(binary, os.X_OK):
            pytest.fail(f"NuttX binary is missing or not executable: {binary}")

        controller = topo.add_controller(
            controller_type=BSController, public_address=address
        )

        def command():
            return [
                binary,
                f"--sim-bsim-sid={controller.sim_id}",
                f"--sim-bsim-pid={controller.phy_id}",
                f"--sim-bsim-dev={controller.application_dev_id}",
                f"--bt-dev={controller.hci_sock}",
            ]

        return topo.add_process(
            ProcessInstance(
                command=command,
                controller=controller,
                label=label,
                shell=True,
                log_subdir=label,
            )
        )

    return load


async def test_btsak_discovers_advertiser(topo, host):
    """Use BTSak commands to advertise and discover a specific peer."""
    address = "C0:FF:EE:10:00:01"
    advertiser = host("BSWORLD_NUTTX_BTSAK_BIN", "advertiser", address)
    scanner = host("BSWORLD_NUTTX_BTSAK_BIN", "scanner", "C0:FF:EE:10:00:02")
    await topo.start()
    await advertiser.shell.run_many(("ifup bnep0", "bt bnep0 advertise start"))
    commands = ["ifup bnep0", "bt bnep0 scan start"]
    for _ in range(20):
        commands.extend(("sleep 1", "bt bnep0 scan get"))
    await scanner.shell.run_many(commands)
    await scanner.wait_log(rf"(?i)addr:\s+{re.escape(address)}\s+type: 0$", timeout=30)


async def test_nimble_connects_and_subscribes(topo, host):
    """Exercise the stock blecent/bleprph flow used by BSWorld's NuttX tests."""
    central = host("BSWORLD_NUTTX_NIMBLE_BIN", "central", "C0:FF:EE:20:01:01")
    peripheral = host("BSWORLD_NUTTX_NIMBLE_BIN", "peripheral", "C0:FF:EE:20:00:01")
    await topo.start()
    await central.shell.run_many(("ifup bnep0", "blecent"))
    await peripheral.shell.run_many(("ifup bnep0", "bleprph"))
    await central.wait_log(r"name\(complete\)=NuttX NimBLE PRPH", timeout=30)
    await central.wait_log(r"Connection established handle=\d+", timeout=30)
    await peripheral.wait_log(
        r"connection established; status=0 handle=\d+", timeout=30
    )
    await central.wait_log(
        r"Service discovery complete; status=0 conn_handle=\d+", timeout=30
    )
    await central.wait_log(
        r"Subscribe complete; status=0 conn_handle=\d+ attr_handle=\d+",
        timeout=30,
    )
