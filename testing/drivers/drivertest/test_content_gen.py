#!/bin/python3
#############################################################################
# apps/testing/drivers/drivertest/cmocka_driver_uart/test_content_gen.py
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
#############################################################################

import argparse
import sys
import time
from random import choices, randint

try:
    import serial
except ImportError:
    print("Please Install pyserial first [pip install pyserial]")
    sys.exit(1)

DEFAULT_CONTENT = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ,./<>?;':\"[]{}\\|!@#$%^&*()-+_="


def parse_args():
    parser = argparse.ArgumentParser(
        description="Use this script to test whether the serial port is working properly. If the device path is not "
        "provided, the text of the test will be output manually, which requires the user to copy and "
        "paste manually to complete the test. If provided, it will be tested automatically."
    )
    parser.add_argument("dev", type=str, nargs="?", help="tty device", default="")
    parser.add_argument(
        "-t", "--turn", type=int, nargs="?", help="test turns [10]", default=10
    )
    parser.add_argument(
        "-l",
        "--length",
        type=int,
        nargs="?",
        help="each turn max length [100]",
        default=100,
    )

    return parser.parse_args()


def fake_symbol(k):
    return "".join(choices(DEFAULT_CONTENT, k=k))


def s_write(fd, content, end="#"):
    if fd:
        try:
            fd.write((content + end).encode())
        except Exception:
            fd.flush()
            fd.close()
    else:
        print(content, end=end)


def s_read(fd, size: int):
    if fd:
        try:
            return fd.read(size).decode()
        except Exception:
            fd.flush()
            fd.close()
    else:
        return input()


def make_test(device: str, default_content: str, turn: int, max_length: int):
    fd = None
    if len(device) > 0:
        fd = serial.Serial(device, 115200)
        if not fd.is_open:
            fd.open()
        print("FROM SERIAL:", s_read(fd, size=len(default_content)))
        fd.flushInput()
        fd.flushOutput()

    print("START WRITE:")
    time.sleep(1)
    s_write(fd, DEFAULT_CONTENT, end="")
    time.sleep(1)

    print("START BURST TEST:")
    fail_count = 0
    for i in range(turn):
        content_length = randint(1, max_length)
        content = fake_symbol(content_length)
        s_write(fd, f"{content_length}")
        s_write(fd, content, end="")

        back = s_read(fd, content_length)
        if back == content:
            s_write(fd, "pass")
            print(f"TURN[{i + 1}]: PASS")
        else:
            s_write(fd, "fail")
            fail_count += 1
            print(f"TURN[{i + 1}]: FAIL")
    s_write(fd, "0")
    if fail_count != 0:
        print(f"### FAIL {fail_count} TURNS ###")
    else:
        print("### ALL PASSED ###")


if __name__ == "__main__":
    args = parse_args()

    make_test(
        args.dev,
        default_content=DEFAULT_CONTENT,
        turn=args.turn,
        max_length=args.length,
    )
