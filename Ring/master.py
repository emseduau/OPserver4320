#! /usr/bin/env python

# Client and server for udp (datagram) echo.
#
# Usage: udpecho -s [port]            (to start a server)
# or:    udpecho -c host [port] <file (client)

import sys
import struct
import threading
# import fcntl
from struct import *
from socket import *


class Master:
    def __init__(self):
        self.MASTER_ID = 15
        self.BUFF_SIZE = 71
        self.MAGIC_VALUE = 0x1234
        self.myRingID = 0
        self.nextSlaveRID = 1
        self.nextSlaveIP = 0
        self.sendTicket = 0
        self.udp_listen = 0

    def main(self):
        if len(sys.argv) > 2:
            self.usage()
        else:
            self.server()

    def usage(self):
        sys.stdout = sys.stderr
        print 'Usage: master [port]'
        sys.exit(2)

    def calculate_checksum(self, ret_bytes):
        running_total = 0
        for little_byte in ret_bytes:
            #print running_total
            running_total += struct.unpack('!B', little_byte[0])[0]
            #print running_total
            while running_total >= 256:
                #print "Its happening."
                running_total = (running_total & 255) + (running_total >> 8)

        print "{0:b}".format(running_total)
        return running_total ^ 0xFF

    def raw_sum(self, ret_bytes):
        running_total = 0
        for little_byte in ret_bytes:
            #print running_total
            running_total += struct.unpack('!B', little_byte[0])[0]
            #print running_total
            while running_total >= 256:
                #print "Its happening."
                running_total = (running_total & 255) + (running_total >> 8)
        return running_total





    def form_datagram(self, target_RID, user_message):
        formedDatagram = ''
        formedDatagram += struct.pack('!B', self.MASTER_ID)
        formedDatagram += struct.pack('!H', 0x1234)
        formedDatagram += struct.pack('!B', 255)
        formedDatagram += struct.pack('!B', target_RID)
        formedDatagram += struct.pack('!B', self.myRingID)
        for little_byte in map(ord, user_message):
            formedDatagram += struct.pack('!B', little_byte)
        # formedDatagram += userMessage
        formedDatagram += struct.pack('!B', self.calculate_checksum(formedDatagram))
        return formedDatagram

    def send_message_prompt(self):
        while True:
            targetRID = eval(raw_input("Please input the ring ID of the recipient: "))
            userMessage = raw_input("Please input the message you want to send: ")
            formedDatagram = self.form_datagram(targetRID, userMessage)
            self.sendTicket.sendto(formedDatagram,  (self.nextSlaveIP, self.calculate_target_port()))

    def verify_checksum(self, data):
        return self.raw_sum(data) == 0xFF

    def i_am_recipient(self, data):
        return struct.unpack('!B', data[4])[0] == self.myRingID

    def recv_handle_message(self):
        while True:
            data, users_msg = self.sendTicket.recvfrom(self.BUFF_SIZE)
            if self.verify_checksum(data):
                if self.i_am_recipient(data):
                    self.recv_print_message(data)
                    print "HAX"
                else:
                    self.recv_forward_message(data)
                    print "forWard"
            else:
                print "BIG ERROR FOUND!"

    def recv_forward_message(self, data):
        self.sendTicket.sendto(data, (self.nextSlaveIP, self.calculate_target_port()))

    def recv_print_message(self, data):
        print "haahhaha len %s " % (len(data))
        print "This node just received the message: %s!" % (data[6: (len(data) - 1)])

    def calculate_my_port(self):
        return 10010 + self.MASTER_ID * 5 + self.myRingID - 1

    def calculate_target_port(self):
        return 10010 + self.MASTER_ID * 5 + self.nextSlaveRID

    def accept_joins(self, s):
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

    def server(self):
        port = eval(sys.argv[1])
        host, alias_list, lan_ip = gethostbyname_ex(gethostname())
        self.nextSlaveIP = lan_ip[0]
        print "Master: Initialized with IP address of {} and port {}.".format(self.nextSlaveIP, port)
        s = socket(AF_INET, SOCK_STREAM)
        s.bind(('', port))
        print "Master: Ready."
        self.udp_listen = self.calculate_my_port()
        self.sendTicket = socket(AF_INET, SOCK_DGRAM)
        self.sendTicket.bind(('', self.udp_listen))
        accept_thread = threading.Thread(target=self.accept_joins, args=(s,))
        send_thread = threading.Thread(target=self.send_message_prompt)
        recv_thread = threading.Thread(target=self.recv_handle_message)
        #self.accept_joins(s)#####################################
        #self.send_message_prompt()###############################
        #self.recv_handle_message()###############################
        accept_thread.start()
        recv_thread.start()
        send_thread.start()
        send_thread.join()
        recv_thread.join()
        accept_thread.join()

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
        ret_bytes += struct.pack('!B', self.MASTER_ID)
        ret_bytes += struct.pack('!H', 0x1234)
        ret_bytes += struct.pack('!B', self.nextSlaveRID)
        ret_bytes += inet_aton(self.nextSlaveIP)
        return ret_bytes

mainObject = Master()
mainObject.main()
#calcString = ''
#calcString += struct.pack('!B', 1)
#calcString += struct.pack('!B', 255)
#print mainObject.calculate_checksum(calcString)
