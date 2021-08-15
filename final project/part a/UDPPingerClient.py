#! /usr/bin/python 
import socket 
import time
import math

serverIp = '127.0.0.1'
serverPort = 12000
clientIp = '127.0.0.1'
clientPort = 1024
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
server_addr = ('localhost', 12000)
sock.settimeout(1)  # wait up to one second for a reply
sock.bind((clientIp, clientPort))

# parameters for the final report:
actionNumber = 1
totalPackets = 0
minRTT = math.inf
maxRTT = - math.inf
sumRTT = 0
numRTT = 0  # the number of massages received
numLosts = 0

def waitForResponse():
  global actionNumber
  while True:
    try:
      message, address = sock.recvfrom(serverPort)
      return message
    except Exception:
      actionNumber +=   1
      return ('Request timed out')

def sendmessage(message,wait=False):
   sock.sendto(message.encode(), (serverIp, serverPort))
   if wait == False:
     return
   else:
     return waitForResponse()

def gettime():
  return int(round(time.time() * 1000))


for i in range(1, 11):
  start = time.time()
  message = 'Ping ' + str(i) + ' ' + str(gettime())
  message_got = sendmessage(message, True)
  if isinstance(message_got, str):
    print ('Request timed out')
    numLosts +=1
    continue
  else:
    message_size = len(message_got)
     # split the massage to get the time in int
    get_array = message_got.decode().split(' ')
    rec_type = get_array[0].upper()   # return type of what we got
    # print the type
    rec_action = int(get_array[1]) # the number of the action
    rec_time = int(get_array[2])
    RTT = gettime() - rec_time
  if RTT > 1000: 
    continue
  if rec_type == 'PING':
    print ("Packet Number "+ str(rec_action)+ ":" + str(message_size) + " bytes recieved from " + serverIp + ':' + str(serverPort)    + ' RTT =' + str(RTT))
    actionNumber +=  1
  elif rec_type == 'ERROR':
    recieved_message = get_array[3]
    print (message_got)
  if RTT < minRTT:
    minRTT = RTT
  if RTT > maxRTT:
    maxRTT = RTT
    sumRTT += RTT
    numRTT += 1

  last = message_got
  totalPackets = totalPackets + 1

print("---------------REPORT-----------------")
print("Minimum RTT: ", minRTT)
print("Maximum RTT: ", maxRTT)
if numRTT == 0:
   print("Average RTT: 0 massages received!")
else:
    print("Average RTT: ", sumRTT/numRTT)
    print("Packet Loss Rate: ", (numLosts/10)*100, "%")  # in percentage
    sock.close()





