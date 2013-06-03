# file: proto.py

import sys
import v1_pb2
import struct
import time


def read_message(handle):
    # read the length of the response
    input = handle.read(2)
    tuple = struct.unpack('!H', input)
    length = tuple[0]
    print >> sys.stderr, "reading " + str(length) + " bytes"
    input = handle.read(length)

    # decode the message
    message = v1_pb2.ResponseOrNotification()
    message.ParseFromString(input)

    if (message.type == v1_pb2.ResponseOrNotification.RESPONSE):
        print >> sys.stderr, "type=" + str(message.response.type) 
        print >> sys.stderr, "success=" + str(message.response.success) 
    elif (message.type == v1_pb2.ResponseOrNotification.NOTIFICATION):
        print >> sys.stderr, "type=" + str(message.notification.type)
    
    # done
    return message 

def write_message(handle, message):
    # serialize the request
    output = message.SerializeToString()
    print >> sys.stderr, "writing " + str(len(output)) + " bytes"
    # write the request
    handle.write(struct.pack('!H', len(output)))
    handle.write(output);
    handle.flush()

# setup the input/output handles
inputhandle = sys.stdin
outputhandle = sys.stdout

# setup the channel request object
channelrequest = v1_pb2.Request()
channelrequest.type = v1_pb2.QUERYAUDIOCHANNELS
channelrequest.queryaudiochannels.SetInParent()

# write the request
write_message(outputhandle, channelrequest)

# read the response
channelresponse = read_message(inputhandle)

# setup the level request object
levelrequest = v1_pb2.Request()
levelrequest.type = v1_pb2.SETLEVEL
levelrequest.setlevel.type = v1_pb2.PEAK

# write the request
write_message(outputhandle, levelrequest)

# read the response
levelresponse = read_message(inputhandle)

# loop forever reading notifications
while True:
    # read the notification
    notification = read_message(inputhandle)

