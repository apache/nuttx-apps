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
import sys
from typing import Any

from PyQt6.QtCore import Qt, QThread
from PyQt6.QtWidgets import (
    QApplication,
    QComboBox,
    QDialog,
    QGridLayout,
    QLabel,
    QLineEdit,
    QProgressBar,
    QPushButton,
)
from shvconfirm import shv_confirm
from shvflasher import shv_flasher

log_levels = (
    logging.DEBUG,
    logging.INFO,
    logging.WARNING,
    logging.ERROR,
    logging.CRITICAL,
)

PROGRESS_STYLE = """
QProgressBar{
    border: 2px solid grey;
    border-radius: 5px;
    text-align: center
}

QProgressBar::chunk {
    background-color: green;
}
"""


def parse_args() -> argparse.Namespace:
    """Parse passed arguments and return result."""
    parser = argparse.ArgumentParser(
        description="GUI application for NuttX firmware flash over SHV"
    )
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
    return parser.parse_args()


async def flashThreadAsync(
    url: str, img: str, path_to_root, progress_bar: QProgressBar
) -> None:
    queue: asyncio.Queue = asyncio.Queue()
    task = asyncio.create_task(shv_flasher(url, img, path_to_root, queue))

    while True:
        progressVal = await queue.get()
        progress_bar.setValue(progressVal)

        if progressVal == 100:
            break

    await task


class flashThread(QThread):
    def __init__(
        self,
        parent: Any,
        url: str,
        img: str,
        path_to_root: str,
        progress_bar: QProgressBar,
    ) -> None:
        QThread.__init__(self, parent)
        self.url = url
        self.img = img
        self.path_to_root = path_to_root
        self.progress_bar = progress_bar

    def run(self) -> None:
        asyncio.run(
            flashThreadAsync(self.url, self.img, self.path_to_root, self.progress_bar)
        )


class confirmThread(QThread):
    def __init__(self, parent: Any, url: str, path_to_root: str) -> None:
        QThread.__init__(self, parent)
        self.url = url
        self.path_to_root = path_to_root

    def run(self) -> None:
        asyncio.run(shv_confirm(self.url, self.path_to_root))


class ShvFlasherGui(QDialog):
    def __init__(self, image=None, target_mount=None, shv_server=None) -> None:
        super().__init__()
        self.setWindowModality(Qt.WindowModality.ApplicationModal)
        self.setWindowTitle("NuttX Firmware Flasher over SHV")

        lbl_shv_url = QLabel("SHV RCP URL")
        self.set_interface(shv_server)

        lbl_image = QLabel("Image path")
        self.image = QLineEdit(image if image is not None else "", self)

        lbl_target_mount = QLabel("Device mount")
        self.target_mount = QLineEdit(
            target_mount if target_mount is not None else "", self
        )

        self.progress_bar = QProgressBar(self)
        self.progress_bar.setValue(0)
        self.progress_bar.setStyleSheet(PROGRESS_STYLE)

        pb_flash = QPushButton("FLASH")
        pb_confirm = QPushButton("CONFIRM")
        pb_exit = QPushButton("EXIT")
        grid = QGridLayout()

        grid.addWidget(lbl_shv_url, 0, 0)
        grid.addWidget(self.interface, 0, 1)
        grid.addWidget(lbl_image, 1, 0)
        grid.addWidget(self.image, 1, 1)
        grid.addWidget(lbl_target_mount, 2, 0)
        grid.addWidget(self.target_mount, 2, 1)
        grid.addWidget(self.progress_bar, 3, 0, 1, 3)
        grid.addWidget(pb_flash, 4, 0)
        grid.addWidget(pb_confirm, 4, 1)
        grid.addWidget(pb_exit, 4, 2)

        pb_flash.clicked.connect(self.do_flash)
        pb_confirm.clicked.connect(self.do_confirm)
        pb_exit.clicked.connect(self.reject)

        self.setLayout(grid)

    def set_interface(self, shv_server=None) -> None:
        self.interface = QComboBox()
        self.interface.setEditable(True)
        # tcp://user@localhost:3755?password=pass
        self.interface.addItems([shv_server if shv_server is not None else ""])

    def do_flash(self) -> None:
        rpc_url = str(self.interface.currentText())
        img_path = str(self.image.text())
        path_to_root = str(self.target_mount.text())
        workerFlash = flashThread(
            self, rpc_url, img_path, path_to_root, self.progress_bar
        )
        workerFlash.start()

    def do_confirm(self) -> None:
        rpc_url = str(self.interface.currentText())
        path_to_root = str(self.target_mount.text())
        worker = confirmThread(self, rpc_url, path_to_root)
        worker.start()


def main() -> None:
    args = parse_args()
    logging.basicConfig(
        level=log_levels[sorted([1 - args.v + args.q, 0, len(log_levels) - 1])[1]],
        format="[%(asctime)s] [%(levelname)s] - %(message)s",
    )

    _ = QApplication(sys.argv)

    dialog = ShvFlasherGui(
        image=args.image, target_mount=args.target_mount, shv_server=args.shv_server
    )
    dialog.exec()


if __name__ == "__main__":
    main()
