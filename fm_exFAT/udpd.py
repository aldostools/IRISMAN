#!/usr/bin/python

import socket
UDP_IP = "0.0.0.0"
UDP_PORT = 18194
sock = socket.socket(socket.AF_INET, # Internet
                  socket.SOCK_DGRAM) # UDP
sock.bind((UDP_IP, UDP_PORT))
while True:
    data, addr = sock.recvfrom(UDP_PORT)
    print data, 
