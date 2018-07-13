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

// -------------------- Fast Running Median Class --------------------
template <typename T, uint8_t N, T default_value> 

class FastRunningMedian {
  public:
    FastRunningMedian() {
      _buffer_ptr = N;
      _window_size = N;
      _median_ptr = N/2;
  
      // Init buffers
      uint8_t i = _window_size;
      while( i > 0 ) {
        i--;
        _inbuffer[i] = default_value;
        _sortbuffer[i] = default_value;
      }
    };
  
    T getMedian() {
      // buffers are always sorted.
      return _sortbuffer[_median_ptr];
    }
  
    
    void addValue(T new_value) {
      // comparision with 0 is fast, so we decrement _buffer_ptr
      if (_buffer_ptr == 0)
        _buffer_ptr = _window_size;
      
      _buffer_ptr--;
      
      T old_value = _inbuffer[_buffer_ptr]; // retrieve the old value to be replaced
      if (new_value == old_value)       // if the value is unchanged, do nothing
        return;
      
      _inbuffer[_buffer_ptr] = new_value;  // fill the new value in the cyclic buffer
        
      // search the old_value in the sorted buffer
      uint8_t i = _window_size;
      while(i > 0) {
        i--;
        if (old_value == _sortbuffer[i])
          break;
      }
      
      // i is the index of the old_value in the sorted buffer
      _sortbuffer[i] = new_value; // replace the value 
  
      // the sortbuffer is always sorted, except the [i]-element..
      if (new_value > old_value) {
        //  if the new value is bigger than the old one, make a bubble sort upwards
        for(uint8_t p=i, q=i+1; q < _window_size; p++, q++) {
          // bubble sort step
          if (_sortbuffer[p] > _sortbuffer[q]) {
            T tmp = _sortbuffer[p];
            _sortbuffer[p] = _sortbuffer[q];
            _sortbuffer[q] = tmp;
          } else {
            // done ! - found the right place
            return;
          }
        }
      } else {
        // else new_value is smaller than the old one, bubble downwards
        for(int p=i-1, q=i; q > 0; p--, q--) {
          if (_sortbuffer[p] > _sortbuffer[q]) {
            T tmp = _sortbuffer[p];
            _sortbuffer[p] = _sortbuffer[q];
            _sortbuffer[q] = tmp;
          } else {
            // done !
            return;
          }
        }
      }
    }
    
  private:
    // Pointer to the last added element in _inbuffer
    uint8_t _buffer_ptr;
    // sliding window size
    uint8_t _window_size;
    // position of the median value in _sortbuffer
    uint8_t _median_ptr;
  
    // cyclic buffer for incoming values
    T _inbuffer[N];
    // sorted buffer
    T _sortbuffer[N];
};

// -------------------- Fast Running Median Class Init --------------------
FastRunningMedian<unsigned int, 50, 0> newMedian;

// -------------------- User Settable Variables --------------------
int8_t hrpin = 0; //Whatever analog pin the sensor is hooked up to

//Don't change values from here on unless you know what you're doing
int8_t mode = 0; //Speed mode. Only 100Hz available now.
long fs;
long timerValue;
int16_t sensorVal;
int16_t output;

// -------------------- Data Struct Definition--------------------
struct dataBuffers 
{
  //initialise two buffers of 50 each
  int16_t hrdata0[50] = {0};
  int16_t hrdata1[50] = {0};
  int16_t medDiff[50] = {0};
  int16_t filtResult0[50] = {0};
  int16_t filtResult1[50] = {0};
  int16_t mad = 0;
  int16_t currentMedian = 0;
  int16_t bufferPointer = 0; //buffer index to write values to
  int16_t filtPointer = 24;
  int16_t bufferMarker = 0; //0 for buffer 0, 1 for buffer 1
  int16_t buffer0State = 0; //0 if clean, 1 if dirty
  int16_t buffer1State = 0;
};

// -------------------- Data Struct Init --------------------
struct dataBuffers dataBuf;

// -------------------- Functions --------------------
void readSensors(int output, struct dataBuffers &dataBuf)
{ //read the sensors, put in correct buffer
  sensorVal = analogRead(hrpin);
 
  if (dataBuf.bufferMarker == 0) 
  {
    dataBuf.hrdata0[dataBuf.bufferPointer] = sensorVal;
    output = dataBuf.hrdata0[dataBuf.filtPointer] - hampelfilt(sensorVal, dataBuf);
    if(output > 0) output = dataBuf.hrdata0[dataBuf.filtPointer];
    dataBuf.filtResult0[dataBuf.filtPointer] = output;
  } else 
  {
    dataBuf.hrdata1[dataBuf.bufferPointer] = sensorVal;
    output = dataBuf.hrdata1[dataBuf.filtPointer] - hampelfilt(sensorVal, dataBuf);
    if(output > 0) output = dataBuf.hrdata0[dataBuf.filtPointer];
    dataBuf.filtResult1[dataBuf.filtPointer] = output;
  }
  
  dataBuf.filtPointer++;
  if(dataBuf.filtPointer >= 50) dataBuf.filtPointer = 0;
  dataBuf.bufferPointer++;
}

int getMeanInt(int16_t data[], int16_t datalen) {
  
  long sum = 0;
  for(int i = 0; i < datalen; i++) {
    sum += data[i]; 
  }
  return (sum / datalen); //return mean
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
  readSensors(output, dataBuf);
}

// -------------------- Define Filter --------------------
int hampelfilt(int sensorVal, struct dataBuffers &dataBuf)
{
  newMedian.addValue(sensorVal);
  dataBuf.currentMedian = newMedian.getMedian();

  if(dataBuf.bufferMarker == 0)
  {
    for(int i = 0; i<50; i++)
    {
      dataBuf.medDiff[i] = abs(dataBuf.hrdata0[i] - dataBuf.currentMedian);
    }
    dataBuf.mad = getMeanInt(dataBuf.medDiff, 50);
    if(dataBuf.hrdata0[dataBuf.filtPointer] > (dataBuf.currentMedian + (2 * dataBuf.mad)))
    {
      return dataBuf.currentMedian;
    } else {
      return dataBuf.hrdata0[dataBuf.filtPointer];
    }
    return 0;
  } else
  {
    for(int i = 0; i<50; i++)
    {
      dataBuf.medDiff[i] = abs(dataBuf.hrdata1[i] - dataBuf.currentMedian);
    }
    dataBuf.mad = getMeanInt(dataBuf.medDiff, 50);
    if(dataBuf.hrdata1[dataBuf.filtPointer] > (dataBuf.currentMedian + (2 * dataBuf.mad)))
    {
      return dataBuf.currentMedian;
    } else {
      return dataBuf.hrdata1[dataBuf.filtPointer];
    }
    return 0;    
  }
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
      Serial.print(dataBuf.hrdata0[i]);
      Serial.print(", ");
      Serial.println(dataBuf.filtResult0[i]);
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
      Serial.print(dataBuf.hrdata1[i]);
      Serial.print(", ");
      Serial.println(dataBuf.filtResult1[i]);
    }
    dataBuf.buffer1State = 0;
  }
}
