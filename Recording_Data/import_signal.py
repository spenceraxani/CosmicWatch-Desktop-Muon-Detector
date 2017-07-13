import serial 
import time
import glob
import sys
import signal
#import numpy as np
#import json
from datetime import datetime
from multiprocessing import Process
def signal_handler(signal, frame):
        print('You pressed Ctrl+C!')
        ComPort.close()     
        file.close() 
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
        except (OSError, serial.SerialException):
            pass
    return result


#if __name__ == '__main__':
    
#This file is used to save real-time data from the detector. You will have to change the variable ComPort to the 
#   name of the USB port it is plugged into. If the Arduino is not recognized by your computer, make sure you have
#   installed the drivers for the Arduino.

print('\n             Welcome to:   ')
print('CosmicWatch: The Desktop Muon Detector\n')

port_list = serial_ports()
if len(port_list) > 1:
    print('Available serial ports:\n')
    for i in range(len(port_list)):
        print('['+str(i+1)+'] ' + str(port_list[i]))
    print('[h] help\n')
    ArduinoPort = raw_input("Select Arduino Port:")
    if ArduinoPort == 'h':
        print('\n===================== help =======================')
        print('1. Is your Arduino connected to the serial USB port?\n')
        print('2. Check that you have the correct drivers installed:\n')
        print('\tMacOS: CH340 driver')
        print('\tWindows: no dirver needed')
        print('\tLinux: no driver needed')
        sys.exit()
else :
    ArduinoPort = 1
print("The selected port is:")
print(str(port_list[int(ArduinoPort)-1])+'\n')
fname = raw_input("Enter file name (eg. /Users/HAL9000/Desktop/test.txt):")
id = raw_input("Enter device ID:")
print("Taking data ...")
print("Press ctl+c to terminate process")

signal.signal(signal.SIGINT, signal_handler)
#ComPort = serial.Serial('/dev/cu.wchusbserialfa130') # open the COM Port
ComPort = serial.Serial(port_list[int(ArduinoPort)-1]) # open the COM Port

ComPort.baudrate = 9600          # set Baud rate
ComPort.bytesize = 8             # Number of data bits = 8
ComPort.parity   = 'N'           # No parity
ComPort.stopbits = 1    

file = open(fname, "w",0)

counter = 0
while True:
    data = ComPort.readline()    # Wait and read data 
    if counter == 0:
        file.write("###################################################################################\n")
        file.write("### CosmicWatch: The Desktop Muon Detector \n")
        file.write("### Questions? saxani@mit.edu \n")
        file.write("### Device ID: "+str(id)+"\n")
        file.write("### Comp_date Comp_time Event Ardn_time[ms] ADC_value[0-1023] SiPM[mV] Deadtime[ms]\n")
        file.write("###################################################################################\n")
    file.write(str(datetime.now())+" "+data)
    counter +=1
    
ComPort.close()     
file.close()  
