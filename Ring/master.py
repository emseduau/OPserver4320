#! /usr/bin/env python

# Client and server for udp (datagram) echo.
#
# Usage: udpecho -s [port]            (to start a server)
# or:    udpecho -c host [port] <file (client)

import sys
import struct
#import fcntl
from struct import *
from socket import *


class Master:
    def __init__(self):
        self.GROUP_ID = 15
        self.BUFF_SIZE = 20
        self.MAGIC_VALUE = 0x1234
        self.myRingID = 0
        self.nextSlaveRID = 1
        self.nextSlaveIP = 0


    def main(self):
        if len(sys.argv) > 2:
            self.usage()
        else:
            self.server()

    def usage(self):
        sys.stdout = sys.stderr
        print 'Usage: master [port]'
        sys.exit(2)

    def server(self):
        port = eval(sys.argv[1])
        host, alias_list, lan_ip = gethostbyname_ex(gethostname())
        self.nextSlaveIP = lan_ip[0]
        print "Master: Initialized with IP address of {} and port {}.".format(self.nextSlaveIP, port)
        s = socket(AF_INET, SOCK_STREAM)
        s.bind(('', port))
        print "Master: Ready."

        while True:
            s.listen(1)
            print 'Master: Waiting for a new request.'
            conn, addr = s.accept()
            print "Master: Got Connection From: " + str(addr)
            data = conn.recv(self.BUFF_SIZE)
            print "Master: Received these raw bytes:%s" % self.get_raw_bytes(data)
            extracted_id, extracted_magic = self.unpack_request(data)

            if self.request_is_valid(extracted_magic):
                print "Master: Decoded the values as: [groupID]: %i, [magicNum]: %i" % (extracted_id, extracted_magic)
                response = self.pack_response()
                self.nextSlaveRID += 1
                self.nextSlaveIP = addr[0]
                print "Master: Sending out raw bytes:%s" % (self.get_raw_bytes(response))
                conn.sendall(response)
                print "Master: Set new target with IP address %s." % self.nextSlaveIP
            else:
                print "Master: Malformed request found. Failed to add Slave."

        return

    def request_is_valid(self, extracted_magic):
        return extracted_magic == self.MAGIC_VALUE

    def get_raw_bytes(self, raw_data):
        hex_string = ""
        for raw_byte in raw_data:
            hex_string += " " + hex(ord(raw_byte))
        return hex_string

    def unpack_request(self, rec_data):
        extracted_id = struct.unpack('!B', rec_data[0])[0]
        extracted_magic = struct.unpack('!H', rec_data[1:3])[0]
        return extracted_id, extracted_magic

    def pack_response(self):
        ret_bytes = ''
        ret_bytes += struct.pack('!B', self.GROUP_ID)
        ret_bytes += struct.pack('!H', 0x1234)
        ret_bytes += struct.pack('!B', self.nextSlaveRID)
        ret_bytes += inet_aton(self.nextSlaveIP)
        return ret_bytes

mainObject = Master()
mainObject.main()
