# UDPPingerServer.py
# We will need the following module to generate randomized lost packets
#! /usr/bin/python

import random
from socket import *
import time


# Create a UDP socket
# Notice the use of SOCK_DGRAM for UDP packets
serverSocket = socket(AF_INET, SOCK_DGRAM)

# Assign IP address and port number to socket
serverSocket.bind(('127.0.0.1', 12000))

last_packet = 0  # saves the number of the last packet -- to pay attention to losses
recievedTime = 0

while True:

    # Generate random number in the range of 0 to 10
    rand = random.randint(0, 10)

    # Receive the client packet along with the address it is coming from
    message, address = serverSocket.recvfrom(1024)
    # Capitalize the message from the client
    message = message.upper()

    # Calculate the time difference - to check the speed of service
    now = time.time()
    # split the massage to get the time
    #split_arr = message.split(' ', 2)  # maxsplit = 2 (first 3 parts)
    #time_str = split_arr[2]  # the time in ctime format
    #msg_time = time.strptime(time_str)
    #differ = now - msg_time
    #print("The different is : ", differ)

    # split the massage to get the time in int
    get_array = message.decode().split(' ')
    rec_type = get_array[0].upper()
    # print the type
    recieved_seq = int(get_array[1])
    recieved_time = int(get_array[2])
    MaxSleep = 1.0
    MinSleep =0.2
    time.sleep(random.uniform(MinSleep, MaxSleep))
    # Report any lost packets - to check the reliability
    #curr_packet = int(split_arr[1])
   # if curr_packet - last_packet > 1:  # one or more packets are lost
       # for i in range(last_packet+1, curr_packet):
          #  print("packet ", i, " lost!")
   # last_packet = curr_packet

    # If rand is less is than 4, we consider the packet lost and do not respond
    if rand < 4:
        print ('Request timed out')
        continue

    # Otherwise, the server responds
    serverSocket.sendto(message, address)
    print ('Send: ' + message.decode())




   

