#! /usr/bin/env python

# Client and server for udp (datagram) echo.
#
# Usage: udpecho -s [port]            (to start a server)
# or:    udpecho -c host [port] <file (client)

import sys
import struct
import datetime
from struct import *
from socket import *

ECHO_PORT = 10025
BUFSIZE = 1024


def main():
    if len(sys.argv) != 3:
        usage()
    else:
        client()


def usage():
    sys.stdout = sys.stderr
    print 'Usage: tcpClient serverName portnumber'
    sys.exit(2)


def client():
    port = eval(sys.argv[2])
    hostThing = sys.argv[1]
    #port = ECHO_PORT
    s = socket(AF_INET, SOCK_STREAM)
    s.bind(('', port))
    userContinue = 1
    requestID = 1
    print 'TCP CLIENT ready'
    while userContinue == 1:
        #data, addr = s.recvfrom(BUFSIZE)
        #print 'server received data from %r' % (addr)
        operandOne = eval(raw_input("Please input the first operand: "))
        operandTwo = eval(raw_input("Please input the second operand: "))
        print "Please input the opCode of your desired operation."
        print "0 -> +"
        print "1 -> -"
        print "2 -> |"
        print "3 -> &"
        print "4 -> >>"
        print "5 -> <<"
        opCode = eval(raw_input("OP Code: "))
        request = format_request(operandOne, operandTwo, opCode, requestID)
        requestTime = datetime.datetime.now()
        s.sendto(request, (hostThing, port))
        ourResponse = s.recv(7)
        responseTime = datetime.datetime.now()
        elapsedTime = responseTime - requestTime
        print "The server responded to request %i with %i." % (unpack('!B', ourResponse[1])[0], unpack('!h', ourResponse[3:7])[0])
        print "The total round-trip time was %i milliseconds." % int(elapsedTime.total_seconds() * 1000)
        userContinue = eval(raw_input("Would you like to continue? (1 for yes, 0 for no): "))
        requestID += 1
    return


def format_request(operandOne, operandTwo, opCode, requestID):
    retBytes = ''
    retBytes += struct.pack('!B', 8)
    retBytes += struct.pack('!B', requestID)
    retBytes += struct.pack('!B', opCode)
    retBytes += struct.pack('!B', 2)
    retBytes += struct.pack('!i', operandOne)
    retBytes += struct.pack('!i', operandTwo)
    return retBytes
main()

