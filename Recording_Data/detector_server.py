import tornado.httpserver
import tornado.websocket
import tornado.ioloop
import tornado.web
import socket
import multiprocessing
import time
import glob
import math
import random
import thread
import sys
import serial 
from datetime import datetime
'''
This is a Websocket server that forwards signals from the detector to any client connected.
It requires Tornado python library to work properly.
Please run `pip install tornado` with python of version 2.7.9 or greater to install tornado.
Run it with `python detector-server.py`
Written by Pawel Przewlocki (pawel.przewlocki@ncbj.gov.pl).
Based on http://fabacademy.org/archives/2015/doc/WebSocketConsole.html
''' 

clients = [] ##list of clients connected
queue = multiprocessing.Queue() #queue for events forwarded from the device

def signal_handler(signal, frame):
        print('You pressed Ctrl+C!')
        bg.comport.close()     
        #file.close() 
        sys.exit(0)

def serial_ports():
    """ Lists serial port names

        :raises EnvironmentError:
            On unsupported or unknown platforms
        :returns:
            A list of the serial ports available on the system
    """
    if sys.platform.startswith('win'):
        ports = ['COM%s' % (i + 1) for i in range(256)]
    elif sys.platform.startswith('linux') or sys.platform.startswith('cygwin'):
        # this excludes your current terminal "/dev/tty"
        ports = glob.glob('/dev/tty[A-Za-z]*')
    elif sys.platform.startswith('darwin'):
        ports = glob.glob('/dev/tty.*')
    else:
        raise EnvironmentError('Unsupported platform')
        sys.exit(0)
    result = []
    for port in ports:
        try:
            s = serial.Serial(port)
            s.close()
            result.append(port)
	    print(port)
        except (OSError, serial.SerialException):
            pass
    return result

##Background process for data collection
class DataCollectionProcess(multiprocessing.Process):
    def __init__(self, queue):
        #multiprocessing.Process.__init__(self)
        self.queue = queue
        port_list=serial_ports()
	print(port_list[1])
        ArduinoPort=port_list[1]
        self.comport = serial.Serial(ArduinoPort) # open the COM Port
        self.comport.baudrate = 9600          # set Baud rate
        self.comport.bytesize = 8             # Number of data bits = 8
        self.comport.parity   = 'N'           # No parity
        self.comport.stopbits = 1 

    def close(self):
        self.comport.close()
        
    def nextTime(self, rate):
        return -math.log(1.0 - random.random()) / rate
    
    '''
    def run(self):
        counter = 0
        while True:
            data = self.comport.readline()
	    self.queue.put(str(datetime.now())+" "+data)
    '''

def RUN(bg):
    print 'RUN!!'
    while True:
        data = bg.comport.readline()
        bg.queue.put(str(datetime.now())+" "+data)
    

##Tornado handler of WebSocket connections 
class WSHandler(tornado.websocket.WebSocketHandler):
    def __init__ (self, application, request, **kwargs):
        super(WSHandler, self).__init__(application, request, **kwargs)
        self.sending = False

    def open(self):
        print 'New connection opened from ' + self.request.remote_ip
        clients.append(self)
        print '%d clients connected' % len(clients)
      
    def on_message(self, message):
        print 'message received:  %s' % message
        if message == 'StartData':
            self.sending = True
        if message == 'StopData':
            self.sending = False
 
    def on_close(self):
        self.sending = False
        clients.remove(self)
        print 'Connection closed from ' + self.request.remote_ip
        print '%d clients connected' % len(clients)
 
    def check_origin(self, origin):
        return True
        
##Callback checking the queue for pending messages, and relaying all to connected clients
def checkQueue():
    while not queue.empty():
        message = queue.get()
        ##sys.stdout.write('#')
        for client in clients:
            if client.sending:
                client.write_message(message)
 
   

if __name__ == "__main__":
    #bg process
    
    bg = DataCollectionProcess(queue) 
    #bg.daemon = True
    #bg.start()
    thread.start_new_thread(RUN,(bg,)) 
    #p=multiprocessing.Process(target=RUN)
    #p.start()
    #server stuff
    application = tornado.web.Application(
        handlers=[(r'/', WSHandler)]
    )
    http_server = tornado.httpserver.HTTPServer(application)
    port = 9090
    http_server.listen(port)
    myIP = socket.gethostbyname(socket.gethostname())
    print 'CosmicWatch detector server started at %s:%d' % (myIP, port)
    print 'You can now connect to your device using http://cosmicwatch.pl/'
    mainLoop = tornado.ioloop.IOLoop.instance()
    #in the main loop fire queue check each 100ms
    scheduler = tornado.ioloop.PeriodicCallback(checkQueue, 100, io_loop = mainLoop)
    scheduler.start()
    #start the loop
    mainLoop.start()
