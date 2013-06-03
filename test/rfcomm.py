# file: rfcomm.py

import sys
import bluetooth
import pyev
import subprocess
import fcntl
import os
import errno

class Connection(object):
        def __init__(self, loop, readfile, writefile):
            # save the args
            self.loop = loop
            self.readfile = readfile
            self.writefile = writefile
            # initialize a buffer to use in queuing data from read to write 
            self.buf = ""
            # turn on non-blocking i/o on the read file descriptors
            fl = fcntl.fcntl(readfile, fcntl.F_GETFL)
            fcntl.fcntl(readfile, fcntl.F_SETFL, fl | os.O_NONBLOCK)
            fl = fcntl.fcntl(writefile, fcntl.F_GETFL)
            fcntl.fcntl(writefile, fcntl.F_SETFL, fl | os.O_NONBLOCK)    
            # create the watchers
            self.read_watcher = pyev.Io(readfile.fileno(), pyev.EV_READ, loop, self.read_cb)
            self.write_watcher = pyev.Io(writefile.fileno(), pyev.EV_WRITE, loop, self.write_cb)
            # start reading
            self.read_watcher.start()

        def read_cb(self, watcher, revents):
            if revents & pyev.EV_READ:
                try:
                    buf = os.read(self.readfile.fileno(), 1024)
                except OSError, ex:
                    # if we get any error other than EAGAIN then we fatally die
                    if ex[0] != errno.EAGAIN:
                        print >> sys.stderr, "exiting due to error=%s" % (ex[1])
                        os._exit(1);
                    print >> sys.stderr, "read EAGAIN"
                if buf:
                    print >> sys.stderr, "received %d bytes of data" % (len(buf))
                    # if we were able to read data then we:
                    # 1. store the data
                    # 2. disable reading 
                    # 3. enable writing            
                    self.buf += buf
                    self.read_watcher.stop()
                    self.write_watcher.start()
                        
        def write_cb(self, watcher, revents):
            if revents & pyev.EV_WRITE:
                # write the data 
                try:
                    count = os.write(self.writefile.fileno(), self.buf)
                except OSError, ex:
                    if ex[0] != errno.EAGAIN:
                        print >> sys.stderr, "exiting due to error=%s" % (ex[1])
                        os._exit(1)
                    print >> sys.stderr, "write EAGAIN"
                if count:
                    print >> sys.stderr, "writing %d bytes of data" % (count)
                    # truncate the part of the buffer we wrote
                    self.buf = self.buf[count:]
                    # if there is no data to write then turn ourselves off
                    if (0 == len(self.buf)):
                        # turn ourselves off and the reader back on
                        self.write_watcher.stop()
                        self.read_watcher.start()

# validate args
if len(sys.argv) != 4:
    print >> sys.stderr, "usage: rfcomm.py <addr> <service> <program>"
    print >> sys.stderr, "   addr is a bluetooth address"
    print >> sys.stderr, "   service is the uuid of the rfcomm service"
    print >> sys.stderr, "   program is the command to spawn the process that will handle input/output"
    sys.exit(2)

# extract args
target = sys.argv[1]
service = sys.argv[2]
program = sys.argv[3]

# validate the address
if (False == bluetooth.is_valid_address(target)):
    print >> sys.stderr, "invalid address format"
    sys.exit(2)

# validate the uuid
if (False == bluetooth.is_valid_uuid(service)):
    print >> sys.stderr, "invalid service format"
    sys.exit(2)

# query for the service's rfcomm port
print >> sys.stderr, "Looking for service=" + service + " at address=" + target
sdp_services = bluetooth.find_service(address=target, uuid=service)
if (0 == len(sdp_services)):
    print >> sys.stderr, "Unable to resolve service=" + service + " at address=" + target 
    sys.exit(1)

# get the service 
sdp_service = sdp_services[0]

# store the channel + channel
channel = sdp_service["port"]
host = sdp_service["host"]

# print out the port
print >> sys.stderr, "found service=" + service + " at address=" + target + " on channel=" + str(channel)

# attach to the remote service
socket = bluetooth.BluetoothSocket(bluetooth.RFCOMM)
socket.connect((host, channel))

print >> sys.stderr, "connected to \"%s\" on %s" % (service, host)

print >> sys.stderr, "starting child process with command=\"%s\"" % (program)

# start the child process
process = subprocess.Popen(program, stdin=subprocess.PIPE, stdout=subprocess.PIPE, shell=True)

print >> sys.stderr, "child process started with PID=%s" % (process.pid)

# setup the pyev main loop
loop = pyev.default_loop()

print >> sys.stderr, "mirroring bluetooth socket to child process"

# setup the connections
readconnection = Connection(loop, process.stdout, socket)
writeconnection = Connection(loop, socket, process.stdin)

# run the loop 
loop.start()
