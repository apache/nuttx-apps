#!/usr/bin/env python3
############################################################################
# apps/boot/nxboot/tools/nximage.py
#
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
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
# License for the specific language governing permissions and limitations
# under the License.
#
############################################################################
"""Python script that prepares the NuttX image to be used with NX bootloader"""

import argparse
import io
import os
import struct
import zlib

import semantic_version


class NxImage:
    def __init__(
        self, path: str, result: str, version: str, header_size: int, primary: bool
    ) -> None:
        self.path = path
        self.result = result
        self.size = os.stat(path).st_size
        self.version = semantic_version.Version(version)
        self.header_size = header_size
        self.primary = primary
        self.crc = 0

        with open(path, "rb") as f:
            while data := f.read(io.DEFAULT_BUFFER_SIZE):
                self.crc = zlib.crc32(data, self.crc)

    def __repr__(self):
        repr = (
            "<NxImage\n"
            f"  path:        {self.path}\n"
            f"  result:      {self.result}\n"
            f"  fsize:       {self.size}\n"
            f"  version:     {self.version}\n"
            f"  header_size: {self.header_size}\n"
            f"  primary:     {self.primary}\n"
            f"  crc:         {self.crc}\n"
            f">"
        )
        return repr

    def add_header(self):
        with open(self.path, "r+b") as src, open(self.result, "w+b") as dest:
            if self.primary:
                dest.write(b"\xb1\xab\xa0\xac")
            else:
                dest.write(b"\x4e\x58\x4f\x53")
            dest.write(struct.pack("<I", self.size))
            if self.primary:
                dest.write(struct.pack("<I", 0xFFFFFFFF))
            else:
                dest.write(struct.pack("<I", self.crc))
            dest.write(struct.pack("<H", self.version.major))
            dest.write(struct.pack("<H", self.version.minor))
            dest.write(struct.pack("<H", self.version.patch))
            if not self.version.prerelease:
                dest.write(struct.pack("@110s", b"\x00"))
            else:
                dest.write(
                    struct.pack("@110s", bytes(self.version.prerelease[0], "utf-8"))
                )
            dest.write(bytearray(b"\xff") * (self.header_size - 128))
            while data := src.read(io.DEFAULT_BUFFER_SIZE):
                dest.write(data)


def parse_args() -> argparse.Namespace:
    """Parse passed arguments and return result."""
    parser = argparse.ArgumentParser(description="Tool for Nuttx Bootloader")
    parser.add_argument(
        "--version",
        default="0.0.0",
        help="Image version according to Semantic Versioning 2.0.0.",
    )
    parser.add_argument(
        "--header_size",
        type=lambda x: int(x, 0),
        default=0x200,
        help="Size of the image header.",
    )
    parser.add_argument(
        "--primary",
        action="store_true",
        help="Primary image intended to be uploaded directly to primary memory.",
    )
    parser.add_argument(
        "-v",
        action="store_true",
        help="Verbose output. This prints information abourt the created image.",
    )
    parser.add_argument(
        "PATH",
        default="nuttx.bin",
        help="Path to the NuttX image.",
    )
    parser.add_argument(
        "RESULT",
        default="nuttx.img",
        help="Path where the resulting NuttX image is stored.",
    )
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    image = NxImage(
        args.PATH, args.RESULT, args.version, args.header_size, args.primary
    )
    image.add_header()
    if args.v:
        print(image)


if __name__ == "__main__":
    main()
