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
        self,
        path: str,
        result: str,
        version: str,
        header_size: int,
        identifier: int,
    ) -> None:
        self.path = path
        self.result = result
        self.size = os.stat(path).st_size
        self.version = semantic_version.Version(version)
        self.header_size = header_size
        self.identifier = identifier
        self.crc = 0
        self.extd_hdr_ptr = 0

    def __repr__(self) -> str:
        repr = (
            "<NxImage\n"
            f"  path:        {self.path}\n"
            f"  result:      {self.result}\n"
            f"  fsize:       {self.size}\n"
            f"  version:     {self.version}\n"
            f"  header_size: {self.header_size:x}\n"
            f"  identifier:  {self.identifier:x}\n"
            f"  crc:         {self.crc:x}\n"
            f">"
        )
        return repr

    def add_header(self) -> None:
        with open(self.path, "r+b") as src, open(self.result, "w+b") as dest:
            dest.write(b"\x4e\x58\x4f\x53")
            dest.write(struct.pack("<H", 0))
            dest.write(struct.pack("<H", self.header_size))
            dest.write(struct.pack("<I", 0xFFFFFFFF))
            dest.write(struct.pack("<I", self.size))
            dest.write(struct.pack("<Q", self.identifier))
            dest.write(struct.pack("<I", self.extd_hdr_ptr))
            dest.write(struct.pack("<H", self.version.major))
            dest.write(struct.pack("<H", self.version.minor))
            dest.write(struct.pack("<H", self.version.patch))
            if not self.version.prerelease:
                dest.write(struct.pack("@94s", b"\x00"))
            else:
                dest.write(
                    struct.pack("@94s", bytes(self.version.prerelease[0], "utf-8"))
                )
            dest.write(bytearray(b"\xff") * (self.header_size - 128))
            while data := src.read(io.DEFAULT_BUFFER_SIZE):
                dest.write(data)

        with open(self.result, "r+b") as f:
            f.seek(12)
            while data := f.read(io.DEFAULT_BUFFER_SIZE):
                self.crc = zlib.crc32(data, self.crc)
            f.seek(8)
            f.write(struct.pack("<I", self.crc))


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
        "--identifier",
        type=lambda x: int(x, 0),
        default=0x0,
        help="Platform identifier. An image is rejected if its identifier doesn't match the one set in bootloader.",
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
        args.PATH,
        args.RESULT,
        args.version,
        args.header_size,
        args.identifier,
    )
    image.add_header()
    if args.v:
        print(image)


if __name__ == "__main__":
    main()
