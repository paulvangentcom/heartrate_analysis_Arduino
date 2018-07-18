import serial
import os

def connect(port):
    '''Function to start serial connection'''
    ser = serial.Serial(port, 250000, timeout=2)
    return ser

def write_logfile(ser, filepath, samplerate):
    #write the
    ser.write(str.encode(str(samplerate))) 

    first_entry = ser.readline().decode().strip()
    try:
        int(first_entry) #see if we indeed get integers back, used to catch an exception message
    except: #if we don't get an integer back, it's an error message.
        print('error setting sampling rate on device. It returned message: \n\n------------------')
        while ser.inWaiting() != 0: #Report and return the error message from the serial buffer
            print('%s\n' %ser.readline().decode().strip())
        print('------------------')
        quit() #end execution

    with open(filepath, 'w') as f: #if all seems fine
        while True: #loop indefinitely
            if ser.inWaiting() > 1: #whenever there's something in the buffer...
                f.write('%s\n' %ser.readline().decode().strip()) #read it and write to the log file

if __name__ == '__main__':
    #specify the sampling rate
    samplerate = 100

    #start a serial connection on windows with COM3 port, BAUDrate of logger is 250,000
    ser = connect('COM3')

    #on linux, depending on where the logger device is mounted, you would do something like:
    #ser = connect('/dev/ttyUSB0')

    #wait for logger to send the 'ready' signal
    while ser.inWaiting() < 1:
        pass

    #retrieve the reported logger state from the buffer
    state = ser.readline().decode().strip()
    if state == 'Logger ready!': #See if it is what we expect
        #start the 'write to logfile' function
        write_logfile(ser, 'output.csv', samplerate)
    else: #if we get something unexpected, report the error
        print('error, cannot connect to device.')