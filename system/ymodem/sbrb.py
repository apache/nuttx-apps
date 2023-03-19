#!/bin/python3
# apps/system/ymodem/sbrb.py
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
import sys
import os
import io
import argparse
import termios
import binascii
import signal
import datetime

SOH = b'\x01' # start of 128-byte data packet
STX = b'\x02' # start of 1024-byte data packet
STC = b'\x03' # start of a customsize data packet
EOT = b'\x04' # end of transmission
ACK = b'\x06' # acknowledge
NAK = b'\x15' # negative acknowledge
CAN = b'\x18' # two of these in succession aborts transfer
CRC16 = b'\x43' # 'C' == 0x43, request 16-bit CRC

PACKET_SIZE = 128
PACKET_1K_SIZE = 1024
EAGAIN = 1
EINVAL = 2
EEOT   = 3
ERRORMAX = 200

def format_time(seconds):

    hours = seconds / 3600
    seconds %= 3600
    minutes = seconds / 60
    seconds %= 60

    time = "%02dh%02dm%02ds" % (int(hours), int(minutes), int(seconds))
    return time

def ymodem_stdread(size):
    fd = sys.stdin.fileno()
    old_settings = termios.tcgetattr(fd)

    try:
        new_settings = termios.tcgetattr(fd)
        new_settings[3] &= ~(termios.ICANON | termios.ECHO)
        termios.tcsetattr(fd, termios.TCSADRAIN, new_settings)
        sys.stdin.flush()
        data = sys.stdin.buffer.read(size)
        return data
    finally:
        termios.tcsetattr(fd, termios.TCSADRAIN, old_settings)

def ymodem_stdwrite(data):
    fd = sys.stdout.fileno()
    old_settings = termios.tcgetattr(fd)

    try:
        new_settings = termios.tcgetattr(fd)
        new_settings[3] &= ~(termios.ICANON | termios.ECHO)
        termios.tcsetattr(fd, termios.TCSADRAIN, new_settings)
        data = sys.stdout.buffer.write(data)
        sys.stdout.flush()
        return data
    finally:
        termios.tcsetattr(fd, termios.TCSADRAIN, old_settings)

def ymodem_stdprogress(data):
    sys.stderr.write(data)
    sys.stderr.flush()

def calc_crc16(data, crc = 0):

    crctable = [
        0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5, 0x60c6, 0x70e7,
        0x8108, 0x9129, 0xa14a, 0xb16b, 0xc18c, 0xd1ad, 0xe1ce, 0xf1ef,
        0x1231, 0x0210, 0x3273, 0x2252, 0x52b5, 0x4294, 0x72f7, 0x62d6,
        0x9339, 0x8318, 0xb37b, 0xa35a, 0xd3bd, 0xc39c, 0xf3ff, 0xe3de,
        0x2462, 0x3443, 0x0420, 0x1401, 0x64e6, 0x74c7, 0x44a4, 0x5485,
        0xa56a, 0xb54b, 0x8528, 0x9509, 0xe5ee, 0xf5cf, 0xc5ac, 0xd58d,
        0x3653, 0x2672, 0x1611, 0x0630, 0x76d7, 0x66f6, 0x5695, 0x46b4,
        0xb75b, 0xa77a, 0x9719, 0x8738, 0xf7df, 0xe7fe, 0xd79d, 0xc7bc,
        0x48c4, 0x58e5, 0x6886, 0x78a7, 0x0840, 0x1861, 0x2802, 0x3823,
        0xc9cc, 0xd9ed, 0xe98e, 0xf9af, 0x8948, 0x9969, 0xa90a, 0xb92b,
        0x5af5, 0x4ad4, 0x7ab7, 0x6a96, 0x1a71, 0x0a50, 0x3a33, 0x2a12,
        0xdbfd, 0xcbdc, 0xfbbf, 0xeb9e, 0x9b79, 0x8b58, 0xbb3b, 0xab1a,
        0x6ca6, 0x7c87, 0x4ce4, 0x5cc5, 0x2c22, 0x3c03, 0x0c60, 0x1c41,
        0xedae, 0xfd8f, 0xcdec, 0xddcd, 0xad2a, 0xbd0b, 0x8d68, 0x9d49,
        0x7e97, 0x6eb6, 0x5ed5, 0x4ef4, 0x3e13, 0x2e32, 0x1e51, 0x0e70,
        0xff9f, 0xefbe, 0xdfdd, 0xcffc, 0xbf1b, 0xaf3a, 0x9f59, 0x8f78,
        0x9188, 0x81a9, 0xb1ca, 0xa1eb, 0xd10c, 0xc12d, 0xf14e, 0xe16f,
        0x1080, 0x00a1, 0x30c2, 0x20e3, 0x5004, 0x4025, 0x7046, 0x6067,
        0x83b9, 0x9398, 0xa3fb, 0xb3da, 0xc33d, 0xd31c, 0xe37f, 0xf35e,
        0x02b1, 0x1290, 0x22f3, 0x32d2, 0x4235, 0x5214, 0x6277, 0x7256,
        0xb5ea, 0xa5cb, 0x95a8, 0x8589, 0xf56e, 0xe54f, 0xd52c, 0xc50d,
        0x34e2, 0x24c3, 0x14a0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
        0xa7db, 0xb7fa, 0x8799, 0x97b8, 0xe75f, 0xf77e, 0xc71d, 0xd73c,
        0x26d3, 0x36f2, 0x0691, 0x16b0, 0x6657, 0x7676, 0x4615, 0x5634,
        0xd94c, 0xc96d, 0xf90e, 0xe92f, 0x99c8, 0x89e9, 0xb98a, 0xa9ab,
        0x5844, 0x4865, 0x7806, 0x6827, 0x18c0, 0x08e1, 0x3882, 0x28a3,
        0xcb7d, 0xdb5c, 0xeb3f, 0xfb1e, 0x8bf9, 0x9bd8, 0xabbb, 0xbb9a,
        0x4a75, 0x5a54, 0x6a37, 0x7a16, 0x0af1, 0x1ad0, 0x2ab3, 0x3a92,
        0xfd2e, 0xed0f, 0xdd6c, 0xcd4d, 0xbdaa, 0xad8b, 0x9de8, 0x8dc9,
        0x7c26, 0x6c07, 0x5c64, 0x4c45, 0x3ca2, 0x2c83, 0x1ce0, 0x0cc1,
        0xef1f, 0xff3e, 0xcf5d, 0xdf7c, 0xaf9b, 0xbfba, 0x8fd9, 0x9ff8,
        0x6e17, 0x7e36, 0x4e55, 0x5e74, 0x2e93, 0x3eb2, 0x0ed1, 0x1ef0,
    ]

    crc = 0x0
    for char in bytearray(data):
        crctbl_idx = ((crc >> 8) ^ char) & 0xff
        crc = ((crc << 8) ^ crctable[crctbl_idx]) & 0xffff
    return crc & 0xffff

class ymodem:
    def __init__(self, read = ymodem_stdread, write = ymodem_stdwrite,
                 progress = ymodem_stdprogress, timeout = 100, debug = '',
                 customsize = 0):
        self.read = read;
        self.write = write;
        self.timeout = timeout
        self.progress = progress
        self.customsize = customsize
        if debug != '':
            self.debugon = True
            self.debugfd = open(debug,"w+")
        else:
            self.debugon = False

    def debug(self, data):
        if self.debugon:
            self.debugfd.write(data)
            self.debugfd.flush()

    def init_pkt(self):
        self.head = SOH
        self.seq0 = b'\x00'
        self.seq1 = b'\xff'
        self.data = b''
        self.crch = b''
        self.crcl = b''

    def send_pkt(self):
        crc16 = calc_crc16(self.data)
        self.crch = bytes([(crc16 >> 8) & 0xff])
        self.crcl = bytes([crc16 & 0xff])
        pkt = self.head + self.seq0 + self.seq1 + self.data + self.crch + self.crcl

        self.write(pkt)

    def add_seq(self):
        seq0 = bytearray(self.seq0)
        if (seq0[0] == 0xff):
            seq0[0] = 0x00
        else:
            seq0[0] += 1

        seq1 = bytearray(self.seq1)
        if (seq1[0] == 0x00):
            seq1[0] = 0xff
        else:
            seq1[0] -= 1

        self.seq0 = bytes(seq0)
        self.seq1 = bytes(seq1)

    def get_pkt_size(self):
        if self.head == SOH:
            return PACKET_SIZE
        elif self.head == STX:
            return PACKET_1K_SIZE
        elif self.head == STC:
            return self.customsize

        return PACKET_SIZE

    def recv_cmd(self, cmd):
        chunk = self.read(1)
        if chunk == NAK:
            return -EAGAIN
        if chunk != cmd:
            self.debug("should be " + binascii.hexlify(cmd).decode('utf-8'))
            self.debug("but receive " + binascii.hexlify(chunk).decode('utf-8') + "\n")
            return -EINVAL

        return 0

    def send(self, filelist):
        err = 0
        need_sendfile_num = len(filelist)
        cnt = 0
        now = datetime.datetime.now()
        base = float(int(now.timestamp() * 1000)) / 1000
        totolbytes = 0

        while need_sendfile_num != 0:

            now = datetime.datetime.now()
            start = float(int(now.timestamp() * 1000)) / 1000
            while err < 10:
                self.write(CRC16)
                chunk = self.read(1)
                if chunk == CRC16:
                    break
                else:
                    err+= 1

            if err == 10:
                return False;

            self.init_pkt()
            self.head = SOH
            filename = os.path.basename(filelist[cnt])

            self.progress("name:" + filename)
            self.data = filename.encode("utf-8")
            self.data = self.data + bytes([0x00] * 1);
            filesize = os.path.getsize(filelist[cnt])
            sendfilesize = 0
            self.progress(" filesize:%d\n" % (filesize))
            self.data = self.data + str(filesize).encode("utf-8")
            self.data = self.data.ljust(self.get_pkt_size(), b'\x00')
            self.send_pkt()

            ret = self.recv_cmd(ACK)
            if ret == -EAGAIN:
                continue
            elif ret == -EINVAL:
                return ret

            ret = self.recv_cmd(CRC16)
            if ret == -EAGAIN:
                continue
            elif ret == -EINVAL:
                return ret

            self.add_seq()
            readfd = open(filelist[cnt], "rb")
            self.progress("     ")
            while sendfilesize < filesize:

                if sendfilesize + 128 >= filesize:
                    self.head = SOH
                elif self.customsize != 0:
                    self.head = STC
                else:
                    self.head = STX

                self.data = readfd.read(self.get_pkt_size())
                sendbytes = len(self.data)
                self.data = self.data.ljust(self.get_pkt_size(), b'\x00')

                self.send_pkt()
                ret = self.recv_cmd(ACK)
                if ret == -EAGAIN:
                    continue
                elif ret == -EINVAL:
                    return ret

                self.add_seq()
                sendfilesize += sendbytes
                totolbytes += sendbytes
                self.progress("\r")
                self.progress("%2.1f%%" % (float(sendfilesize) / filesize * 100))
                self.progress(" %d:%d" % (sendfilesize, filesize))
                now = datetime.datetime.now()
                usedtime = float(int(now.timestamp() * 1000) / 1000) - start
                realspeed = (sendfilesize / 1024 / usedtime)
                left = (filesize - sendfilesize)/ 1024 / (realspeed)
                self.progress(" left:" + format_time(left))

            err = 0
            while True:
                if err == 2:
                    return -EINVAL
                err += 1

                self.write(EOT)
                ret = self.recv_cmd(ACK)
                if ret == -EAGAIN:
                    continue
                elif ret < 0:
                    return ret
                ret = self.recv_cmd(CRC16)
                if ret == -EAGAIN:
                    continue
                elif ret < 0:
                    return ret
                break

            need_sendfile_num = need_sendfile_num - 1
            readfd.close()
            now = datetime.datetime.now()
            time = float(int(now.timestamp() * 1000) / 1000)
            time = time - start
            self.progress("\ntime used:%.1fs" % time)
            self.progress(" speed %.1fkB/s\n" % (float(sendfilesize) / 1024 / time))
            if need_sendfile_num != 0:
                cnt += 1
                continue

            err = 0
            while True:
                if err == 2:
                    return -EINVAL
                err += 1
                self.head = SOH
                self.seq0 = b'\x00'
                self.seq1 = b'\xff'
                self.data = bytes([0x00] * self.get_pkt_size())
                self.send_pkt()

                ret = self.recv_cmd(ACK)
                if ret == -EAGAIN:
                    continue
                elif ret < 0:
                    return ret
                break

        now = datetime.datetime.now()
        time = float(int(now.timestamp() * 1000) / 1000)
        totaltime = time - base
        arvgspeed = float(totolbytes) / 1024 / totaltime
        self.progress("\n all time:%.2fs average speed:%.2fkB/s" %(totaltime, arvgspeed))

    def recv_packet(self):
        chunk = self.read(1)
        if chunk == SOH:
            self.packetsize = PACKET_SIZE
        elif chunk == STX:
            self.packetsize = PACKET_1K_SIZE
        elif chunk == STC:
            self.packetsize = self.customsize
        elif chunk == NAK:
            return -EAGAIN
        elif chunk == EOT:
            return -EEOT
        else:
            self.debug("recv EBADMSG" + str(chunk) + "\n")
            return -EINVAL

        seq0 = self.read(1)
        seq1 = self.read(1)

        self.data = self.read(self.packetsize)

        crch = self.read(1)
        crcl = self.read(1)

        if seq0 != self.seq0:
            self.debug("recv bad seq0" + binascii.hexlify(seq0).decode('utf-8') + "\n")
            return -EINVAL
        if seq1 != self.seq1:
            self.debug("recv bad seq1" + binascii.hexlify(seq1).decode('utf-8') + "\n")
            return -EINVAL

        crc16 = calc_crc16(self.data)

        crch_b = (crc16 >> 8).to_bytes(1, byteorder='little')
        if crch != crch_b:
            self.debug("recv bad crch_b" + binascii.hexlify(crch_b).decode('utf-8') + "\n")
            self.debug("recv bad crch" + binascii.hexlify(crch).decode('utf-8') + "\n")
            return -EINVAL

        crcl_b = (crc16 & 0xff).to_bytes(1, byteorder='little')
        if crcl != crcl_b:
            self.debug("recv bad crcl_b" + binascii.hexlify(crcl_b).decode('utf-8') + "\n")
            self.debug("recv bad crcl" + binascii.hexlify(crcl).decode('utf-8') + "\n")
            return -EINVAL

        self.add_seq()
        return 0

    def recv(self):

        err = 0
        start_recv = False
        now = datetime.datetime.now()
        base = float(int(now.timestamp() * 1000)) / 1000
        totolbytes = 0
        self.write(CRC16)
        while True:
            now = datetime.datetime.now()
            start = float(int(now.timestamp() * 1000)) / 1000
            self.init_pkt()
            ret = self.recv_packet()
            if ret < 0:
                if err > ERRORMAX:
                    return -1
                err += 1
                continue

            self.debug("recv frist packet\n")
            filename = bytes.decode(self.data.split(b"\x00")[0], "utf-8")
            if not filename:
                if start_recv:
                    self.debug("recv last packet\n")
                    break

                self.debug("recv a none file\n")
                err += 1
                continue

            start_recv = True
            self.progress("name:" + filename + " ")
            size_str = bytes.decode(self.data.split(b"\x00")[1], "utf-8")
            filesize = int(size_str)
            self.progress("size:%d" % (filesize) + "\n")

            self.write(ACK)
            self.write(CRC16)
            fd = open(filename, "wb+")
            writensize = 0
            while writensize < filesize:
                ret = self.recv_packet()
                if ret < 0:
                    self.debug("recv a bad data packet\n")
                    if err > ERRORMAX:
                        return -1
                    err += 1
                    continue

                size = 0
                if self.packetsize > filesize - writensize:
                    self.debug("last data packet\n")
                    size = self.packetsize - (filesize - writensize)
                else:
                    size = self.packetsize

                data = self.data[0:size]
                fd.write(data)
                writensize += size

                self.progress("\r%.2f%%" % (float(writensize) / filesize * 100))
                self.progress(" %d:%d" % (writensize, filesize))
                now = datetime.datetime.now()
                usedtime = float(int(now.timestamp() * 1000) / 1000) - start
                realspeed = (writensize / 1024 / usedtime)
                left = (filesize - writensize)/ 1024 / (realspeed)
                self.progress(" left:" + format_time(left))

                self.write(ACK)

            ret = self.recv_packet()
            if ret == -EEOT:
                self.debug("recv EOT cmd\n")
            elif ret < 0:
                self.debug("recv error packet")
                return -EINVAL

            self.write(ACK)
            self.write(CRC16)
            now = datetime.datetime.now()
            time = float(now.timestamp() * 1000) / 1000
            time = time - start
            self.progress("\ntime used:%.1fs" % time)
            self.progress(" speed %.1fkB/s\n" % (float(filesize) / 1024 / time))
            totolbytes += filesize

            fd.close()

        now = datetime.datetime.now()
        time = float(int(now.timestamp() * 1000) / 1000)
        totaltime = time - base
        arvgspeed = float(totolbytes) / 1024 / totaltime
        self.progress("\n all time:%.2fs average speed:%.2fkB/s" %(totaltime, arvgspeed))


if __name__ == "__main__":

    parser = argparse.ArgumentParser()
    parser.add_argument('filelist',
                        help = "if filelist is valid, that is sb, else is rb",
                        nargs = '*')

    parser.add_argument('-k', "--kblocksize",
                        help = "This opthin can set a customsize block size to transfer",
                        type = int,
                        default = 0)

    parser.add_argument('--debug',
                        help = "This opthin is save debug log on host",
                        default='')

    args = parser.parse_args()

    sys.stdout = io.open(sys.stdout.fileno(), 'w', buffering=32768)
    sbrb = ymodem(debug = args.debug, customsize = args.kblocksize * 1024)
    if len(args.filelist) == 0:
        sbrb.progress("recv mode\n")
        sbrb.recv()
    else:
        sbrb.progress("send mode\n")
        sbrb.send(args.filelist)
