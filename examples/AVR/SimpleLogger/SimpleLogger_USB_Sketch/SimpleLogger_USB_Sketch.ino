/*
 * Arduino Heart Rate Analysis Toolbox - AVR Simple USB Logger
 *      Copyright (C) 2018 Paul van Gent
 *      
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License V3 as published by
 * the Free Software Foundation. The program is free for any non-commercial
 * usage and adaptation, granted you give the recipients of your code the same
 * open-source rights and license.
 * 
 * Please add the following citation to any work utilising one or more of the
 * implementation from this project:
 * 
 * <Add JORS paper reference once published>
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

// -------------------- User Settable Variables --------------------
int8_t hrpin = 0; //Whatever analog pin the sensor is hooked up to
int8_t mode = 6; //Speed mode. \
              0 for 100Hz, 1 for 200Hz, 2 for 250Hz, \
              3 for 500Hz, 4 for 1000Hz 5 for 2000Hz, \
              and 6 for custom. Custom mode is set through Serial.\
              See documentation for details.              
// -------------------- End User Settable Variables --------------------
//Don't change values from here on unless you know what you're doing
long fs;
long timerValue;

// -------------------- Data Struct Definition--------------------
struct dataBuffers 
{
  //initialise two buffers of 50 each
  int16_t hrdata0[50] = {0};
  int16_t hrdata1[50] = {0};
  int16_t bufferPointer = 0; //buffer index to write values to
  int16_t bufferMarker = 0; //0 for buffer 0, 1 for buffer 1
  int16_t buffer0State = 0; //0 if clean, 1 if dirty
  int16_t buffer1State = 0;
};

// -------------------- Data Struct Init --------------------
struct dataBuffers dataBuf;

// -------------------- Functions --------------------
void readSensors(struct dataBuffers &dataBuf)
{ //read the sensors, put in correct buffer
  if (dataBuf.bufferMarker == 0) 
  {
    dataBuf.hrdata0[dataBuf.bufferPointer] = analogRead(hrpin);
  } else 
  {
    dataBuf.hrdata1[dataBuf.bufferPointer] = analogRead(hrpin);
  }
  dataBuf.bufferPointer++;
}

void getOCR(long fs)
{ // Calculate timer compare flag value
  timerValue = ((F_CPU / (64 * fs)) - 1);
  if(timerValue >= 65535 || timerValue < 1)
  {
    Serial.println("Error, timer value incorrect.");
    Serial.print("Sample rate should be faster than 4Hz. You requested: "); 
    Serial.println(fs);
    Serial.println("Speeds above 2KHz are untested and not recommended");
    Serial.println("=================");
    Serial.println("connection closed. Restart Serial connection to reset logger");
    delay(100);
    exit(0);
  }  
}

void setTimerInterrupts(int8_t mode)
{ // set timer compare value
  switch(mode)
  {
    case 0:
      timerValue = 2499;
      break;
    case 1:
      timerValue = 1249;
      break;
    case 2:
      timerValue = 999;
      break;
    case 3:
      timerValue = 499;
      break;
    case 4:
      timerValue = 249;
      break;
    case 5:
      timerValue = 124;
      break;
    case 6:
      Serial.println("Logger ready!");
      while(Serial.available() == 0){}
      fs = Serial.parseInt();
      Serial.flush();
      getOCR(fs);
      break;
  };
  //set-up the interrupts on the 328p
  cli();
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1  = 0;
  TCCR1B |= (1 << WGM12);
  TCCR1B |= (1 << CS01) | (1 << CS00);
  TIMSK1 |= (1 << OCIE1A); 
  OCR1A = timerValue;
  sei();
}


ISR(TIMER1_COMPA_vect)
{ //define interrupt service routine
  readSensors(dataBuf);
}

// -------------------- Setup --------------------
void setup() 
{
  //start serial
  Serial.begin(250000);  
  setTimerInterrupts(mode);
}

// -------------------- Main Loop --------------------
void loop()
{
  if ((dataBuf.bufferPointer >=  49) && (dataBuf.bufferMarker == 0)) 
  { //time to switch buffer0 to buffer1
    if(dataBuf.buffer1State == 1)  //check if buffer1 is dirty before switching
    {
      Serial.println("buffer0 overflow"); //report error if dirty
      delay(20); //give the processor some time to finish serial print before halting
      exit(0); //halt processor
    } else 
    { //if switching is possible
      dataBuf.buffer0State = 1; //mark buffer0 dirty
    }
    dataBuf.bufferMarker = 1; //set buffer flag to buffer1
    dataBuf.bufferPointer = 0; //reset datapoint bufferPointer
    for (int i = 0; i < 49; i++) { //transmit contents of buffer0
      Serial.println(dataBuf.hrdata0[i]);
    }
    dataBuf.buffer0State = 0; //release buffer0 after data tranmission, mark as clean
    //here follows same as above, except with reversed buffer order
  } else if ((dataBuf.bufferPointer >= 49) && (dataBuf.bufferMarker == 1)) 
  {
    if(dataBuf.buffer0State == 1)
    {
      Serial.println("buffer1 overflow");
      delay(20);
      exit(0);
    } else 
    {
      dataBuf.buffer1State = 1;
    }
    dataBuf.bufferMarker = 0;
    dataBuf.bufferPointer = 0;
    for (int i = 0; i < 49; i++) 
    {
      Serial.println(dataBuf.hrdata1[i]);
    }
    dataBuf.buffer1State = 0;
  }
}
