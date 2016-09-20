#! /usr/bin/env python

# Client and server for udp (datagram) echo.
#
# Usage: udpecho -s [port]            (to start a server)
# or:    udpecho -c host [port] <file (client)

import sys
import struct
import base64
from struct import *
from socket import *

ECHO_PORT = 10025
BUFSIZE = 1024


def main():
    if len(sys.argv) > 2:
        usage()
    else:
        server()


def usage():
    sys.stdout = sys.stderr
    print 'Usage: udpecho -s [port]            (server)'
    print 'or:    udpecho -c host [port] <file (client)'
    sys.exit(2)


def server():
    if len(sys.argv) > 1:
        port = eval(sys.argv[2])
    else:
        port = ECHO_PORT
    s = socket(AF_INET, SOCK_DGRAM)
    s.bind(('', port))
    print 'udp echo server ready'
    while 1:
        data, addr = s.recvfrom(BUFSIZE)
        print 'server received %r from %r' % (data, addr)
        response = format_response(data)
        s.sendto(response, addr)
    return


def format_response(inputData):
    retBytes = ''
    rqstTML, rqstID, rqstOP, rqstOPs, = unpack('!4B', inputData[0:4])
    print "rqstTML: " + str(rqstTML)
    print "rqstID: " + str(rqstID)
    print "rqstOP: " + str(rqstOP)
    print "rqstOPs: " + str(rqstOPs)
    retBytes += struct.pack('!B', 7)
    retBytes += struct.pack('!B', rqstID)
    retBytes += struct.pack('!B', 0)
    requestIntOne = unpack('!h', inputData[4:6])[0]
    requestIntTwo = unpack('!h', inputData[6:8])[0]
    print "requestIntOne: " + str(requestIntOne)
    print "requestIntTwo: " + str(requestIntTwo)
    resultInt = {
        0: lambda x, y: x + y,
        1: lambda x, y: x - y,
        2: lambda x, y: x | y,
        3: lambda x, y: x & y,
        4: lambda x, y: x >> y,
        5: lambda x, y: x << y
    }[rqstOP](requestIntOne, requestIntTwo)
    retBytes += struct.pack('!i', resultInt)
    return retBytes


main()

