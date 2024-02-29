#!/usr/bin/env python

# Copyright (C) 2021 Brad Colbert

import socket

# create an ipv4 (AF_INET) socket object using the tcp protocol (SOCK_STREAM)
client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

# connect the client
client.connect(('127.0.0.1', 5556))
client.send(b'search funny\n')

IMAGE_SIZE = 8807
while True:
    numbytes = 0
    # receive the response data (4096 is recommended buffer size)
    while numbytes < IMAGE_SIZE:
        response = client.recv(4096)
        numbytes += len(response)

        if numbytes == 0:
            break

        print('\trecv\'d %d bytes (%d)\n' % (len(response), numbytes))
        print('\trecv\'d %d bytes (%d)\n' % (len(response), numbytes))

    input("Press Enter to continue...")
    client.send(b'stop\n')
