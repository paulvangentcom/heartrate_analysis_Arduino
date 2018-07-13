# AVR implementations
# Arduino Heart Rate Analysis Toolkit

This readme file describe the AVR implementations of several functional heart rate analysis sketches.

**Unsure about which one to pick?** This depends on what you are using it for, and how it's connected. See our **QuickStart Section**

### Index:
- Simple logger
- Simple logger with outlier filtering
- Logger with peak position output
- Raw data logger with full functionality

------
## Simple logger
The simple logger does exactly that: it logs data from a (heart rate) sensor. Hardware timer interrupts are used to ensure timing accuracy ([see here for more info](http://www.paulvangent.com/2018/03/28/hardware-interrupts-are-not-that-scary/)). This approach ensures that data will be logged (almost) exactly at the specified sampling rate, with an error well below a microsecond.

The logger can be set to a fixed samplerate-on-boot, or 'custom mode', by changing the mode in line 7 of the Arduino sketch to 6:

```C
int16_t mode = 6; //Speed mode. \
              0 for 100Hz, 1 for 200Hz, 2 for 250Hz, \
              3 for 500Hz, 4 for 1000Hz 5 for 2000Hz, \
              and 6 for custom. Custom mode is set through Serial.\
              See documentation for details.              
```

Mode 0-5 specifies an exact sample rate (100Hz, 200Hz, 250Hz, 500Hz, 1000Hz, 2000Hz). Whenever a serial connection to the device is made it will start outputting data at the set sampling rate. Setting the mode to 6 will activate the custom sampling rate mode. Upon connecting to the device, the first integer sent to the device will be interpreted as the requested sampling rate. An example in Python:

```python
import serial

if __name__ == '__main__':
    #We connect to device (on COM3 in this example)
    #BAUDrate is 250000, timeout we set to 5sec)
    ser = serial.Serial('COM3', 250000, timeout=5)
    
    #After opening the serial port the logger will wake up
    #We wait for logger to send the 'ready' signal
    while ser.inWaiting() < 1: 
        pass
        
    #If logger sends back the 'all clear' signal
    if ser.readline().decode().strip() == 'Logger ready!':
        ser.write(str.encode(samplerate))
        
        #Now the serial buffer starts filling up. You need to catch this data
        #See the python file packed with the sketch in the Examples folder for a working implementation
```

**Special note on timer accuracy**.
When using a custom sampling rate, the used sampling rate will not in all cases be equal to the requested sampling rate. To understand why, consider that the hardware timer thresholds are calculated using this formula:

![interrupt formula]()

If your chosen sampling rate results in a non-integer using this formula, keep in mind that the AVR will round this to the closest integer and use that as a timer threshold. In the vast majority of cases this is not an issue at all. However, if your application is absolutely reliant on an exactly specified sampling rate (if you're unsure: **it is not**), please use the above formula to verify your chosen value works.

<example>

