#! /usr/bin/env python

# Client and server for udp (datagram) echo.
#
# Usage: udpecho -s [port]            (to start a server)
# or:    udpecho -c host [port] <file (client)

import sys
import struct
import base64
import fcntl
from struct import *
from socket import *
groupID = 15
ECHO_PORT = 10025
BUFSIZE = 1024
myRingID = 0
nextSlaveIP = 0

def main():
    if len(sys.argv) > 2:
        usage()
    else:
        #server()
        host, aliaslist, lan_ip = socket.gethostbyname_ex(socket.gethostname())
        print host
        print aliaslist
        print lan_ip[0]


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
    nextSlaveIP = socket.gethostname()
    print "My address is {}".format(nextSlaveIP)
    #print([l for l in ([ip for ip in socket.gethostbyname_ex(socket.gethostname())[2] if not ip.startswith("127.")][:1], [[(s.connect(('8.8.8.8', 53)), s.getsockname()[0], s.close()) for s in [socket.socket(socket.AF_INET, socket.SOCK_DGRAM)]][0][1]]) if l][0][0])
    s = socket(AF_INET, SOCK_STREAM)
    s.bind(('', port))
    print 'tcp echo server ready'
    s.listen(1)
    while True:
        print 'tcp echo server waiting for requests'
        conn, addr = s.accept()
        print "Got Connection From: " + str(addr)
        data = s.recv(3)
        print 'server received %r from %r' % (data, addr)
        if requestIsValid(data):
            response = format_response(data)
            s.sendall(response)
        else:
            pass
        #response = format_response(data)
        
    return
def requestIsValid(recData):
    if((recData[0] == groupID) and (recData[1] == 0x12) and (recData[2] == 0x34)):
        return true
    else:
        return false
    

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
