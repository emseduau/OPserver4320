#! /usr/bin/env python

# Client and server for udp (datagram) echo.
#
# Usage: udpecho -s [port]            (to start a server)
# or:    udpecho -c host [port] <file (client)
import sys
import struct
from struct import *
from socket import *


class Slave:
    def __init__(self):
        self.myRID = 777
        pass

    def main(self):
        if len(sys.argv) != 3:
            self.usage()
        else:
            self.client()

    def usage(self):
        sys.stdout = sys.stderr
        print 'Usage: slave serverName portnumber'
        sys.exit(2)

    def client(self):
        port = eval(sys.argv[2])
        master_ip = sys.argv[1]
        host, alias_list, lan_ip = gethostbyname_ex(gethostname())
        user_continue = 1
        print "Slave: Initialized with IP address of {}".format(lan_ip[0])
        print 'Slave: Ready.'


        while user_continue == 1:
            s = socket(AF_INET, SOCK_STREAM)
            s.connect((master_ip, port))
            request = self.pack_request()
            print "Slave: Sending out raw bytes:%s" % (self.get_raw_bytes(request))
            s.sendall(request)
            our_response = s.recv(8)
            print "Slave: Received these raw bytes:%s" % (self.get_raw_bytes(our_response))
            reply_group_id, reply_magic_value, ring_id, next_slave_ip = self.unpack_reply(our_response)
            print "Slave: Decoded the values as:"
            print "       [groupID]:   %i " % reply_group_id
            print "       [ringID]:    %i " % ring_id
            print "       [nextSlave]: %s " % next_slave_ip
            s.close()
            user_continue = eval(raw_input("Slave: Request another Slave from this address? (1 for yes, 0 for no): "))

        return

    def get_raw_bytes(self, raw_data):
        hex_string = ""

        for raw_byte in raw_data:
            hex_string += " " + hex(ord(raw_byte))

        return hex_string

    def pack_request(self):
        ret_bytes = ''
        ret_bytes += struct.pack('!B', 15)
        ret_bytes += struct.pack('!H', 0x1234)
        return ret_bytes

    def unpack_reply(self, raw_response):
        group_id = unpack('!B', raw_response[0])[0]
        magic_value = unpack('!H', raw_response[1:3])[0]
        ring_id = unpack('!B', raw_response[3])[0]
        next_slave_ip = inet_ntoa(raw_response[4:8])
        return group_id, magic_value, ring_id, next_slave_ip

mainObject = Slave()
mainObject.main()