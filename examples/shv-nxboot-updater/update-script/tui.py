#!/usr/bin/env python3

############################################################################
# apps/examples/shv-nxboot-updater/update-script/gui.py
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

import argparse
import asyncio
import logging
import os

import rich
from rich.progress import Progress
from shvconfirm import ConfirmStep, shv_confirm
from shvflasher import FlashStep, shv_flasher

log_levels = (
    logging.DEBUG,
    logging.INFO,
    logging.WARNING,
    logging.ERROR,
    logging.CRITICAL,
)


def get_parser():
    """Parse passed arguments and return result."""
    parser = argparse.ArgumentParser(description="SHV NXboot Update Python script")
    parser.add_argument(
        "-v",
        action="count",
        default=0,
        help="Increase verbosity level of logging",
    )
    parser.add_argument(
        "-q",
        action="count",
        default=0,
        help="Decrease verbosity level of logging",
    )
    parser.add_argument(
        "-i",
        "--image",
        dest="image",
        type=str,
        default="nuttx.nximg",
        help="Image path",
    )
    parser.add_argument(
        "-m",
        "--mount",
        dest="target_mount",
        type=str,
        default="test/nuttxdevice",
        help="Target mount location on the SHV broker",
    )
    parser.add_argument(
        "-s",
        "--server",
        dest="shv_server",
        type=str,
        default="tcp://xyz@127.0.0.1:3755?password=xyz",
        help="SHV server/broker",
    )
    parser.add_argument(
        "-o",
        "--operation",
        type=str,
        default="flash",
        help="Requested operation of this script, supported options: flash, confirm. Default=flash",
    )
    return parser


async def do_flash(
    url: str, path_to_img: str, path_to_root: str, method_call_timeout: float
):
    queue: asyncio.Queue = asyncio.Queue()
    asyncio.create_task(
        shv_flasher(url, path_to_img, path_to_root, queue, method_call_timeout)
    )
    progress = None
    task_id = None
    while True:
        val: [FlashStep, int | str] = await queue.get()
        match val[0]:
            case FlashStep.INFO:
                rich.print(f"[cyan]INFO:[/cyan] {str(val[1])}")
            case FlashStep.WRITE_PROGRESS:
                if progress is None:
                    progress = Progress()
                    progress.start()
                    task_id = progress.add_task(
                        f"[cyan]Flashing {os.path.basename(path_to_img)}",
                        total=100,
                        completed=0,
                    )
                progress.update(task_id, completed=int(val[1]))
            case FlashStep.WRITE_COMPLETE:
                rich.print("[cyan]INFO:[/cyan] Image sent!")
                if progress is not None and task_id is not None:
                    progress.stop()
                    progress.remove_task(task_id)
                    progress = None
                    task_id = None
            case FlashStep.END_OK:
                if progress is not None and task_id is not None:
                    progress.stop()
                    progress.remove_task(task_id)
                rich.print("[green]Flashing operation successful![/green]")
                break
            case FlashStep.END_ERROR:
                if progress is not None:
                    progress.stop()
                    progress.remove_task(task_id)
                rich.print("[red]ERROR:[/red] {str(val[1])}")
                break


async def do_confirm(url: str, path_to_root: str, method_call_timeout: float):
    queue: asyncio.Queue = asyncio.Queue()
    task = asyncio.create_task(
        shv_confirm(url, path_to_root, queue, method_call_timeout)
    )
    while True:
        val: [ConfirmStep, int | str] = await queue.get()
        match val[0]:
            case ConfirmStep.INFO:
                rich.print(f"[cyan]INFO:[/cyan] {str(val[1])}")
            case ConfirmStep.END_OK:
                rich.print("[green]Confirm operation successful![/green]")
                break
            case ConfirmStep.END_ERROR:
                rich.print(f"[red]ERROR:[/red] {str(val[1])}")
                break
    await task


def tui_main() -> None:
    parser = get_parser()
    args = parser.parse_args()
    logging.basicConfig(
        level=log_levels[sorted([1 - args.v + args.q, 0, len(log_levels) - 1])[1]],
        format="[%(asctime)s] [%(levelname)s] - %(message)s",
    )
    if args.operation != "flash" and args.operation != "confirm":
        rich.print(f'[red]ERROR:[/red] unsupported "{args.operation}" operation')
        parser.print_help()
        return
    if args.operation == "flash" and not os.path.isfile(args.image):
        rich.print(f"[red]ERROR:[/red] {args.image} not found")
        return
    # In seconds
    METHOD_CALL_TIMEOUT = 5
    match args.operation:
        case "flash":
            asyncio.run(
                do_flash(
                    args.shv_server, args.image, args.target_mount, METHOD_CALL_TIMEOUT
                )
            )
        case "confirm":
            asyncio.run(
                do_confirm(args.shv_server, args.target_mount, METHOD_CALL_TIMEOUT)
            )


if __name__ == "__main__":
    tui_main()
