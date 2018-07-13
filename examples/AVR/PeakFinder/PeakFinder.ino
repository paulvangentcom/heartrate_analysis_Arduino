/*
 * Arduino Heart Rate Analysis Toolbox - Peak Finder
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

//#include "spline.h"

// -------------------- User Settable Variables --------------------
int8_t hrpin = 0; //Whatever analog pin the sensor is hooked up to
int t1, t_end;



int largestVal = 0;
int largestValPos = 0;
int clippingcount = 0;
int8_t clipFlag = 0;
int clipStart = 0;
int8_t clipEnd = 0;
int lastVal = 0;


// -------------------- Define Data Structs --------------------
struct workingDataContainer
{
  long absoluteCount = 0;
  
  //buffers
  int16_t curVal = 0;
  int16_t datalen = 100;
  int16_t hrData[100] = {0};
  int8_t buffPos = 0;
  
  //movavg variables
  int16_t windowSize = 75; //windowSize in samples
  int16_t hrMovAvg[100] = {0};
  int8_t oldestValuePos = 1;
  long movAvgSum = 0;

  //peak variables
  int16_t ROI[40] = {0};
  int16_t ROI_interp[40] = {0};
  int8_t ROIPos = 0;
  int8_t peakFlag = 0;
  int8_t ROI_overflow = 0;
  long curPeak = 0;
  long curPeakEnd = 0;
  long lastPeak = 0;

  //peak validation variables
  int8_t initFlag = 0; //use for initialisation
  int16_t lastRR = 0;
  int16_t curRR = 0;
  int16_t recent_RR[20] = {0};
  int8_t RR_pos = 0;
};

struct workingDataContainer workingData;

// -------------------- Debugging Helper Functions --------------------
int freeRam () {
  extern int __heap_start, *__brkval; 
  int v; 
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}

// -------------------- Define Helper Functions --------------------
int findMax(int arr[], int arrLen, struct workingDataContainer &workingData)
{
  largestVal = 0;
  largestValPos = 0;
  clippingcount = 0;
  clipFlag = 0;
  clipStart = 0;
  clipEnd = 0;
  lastVal = 0;
  
  for(int i = 0; i<arrLen; i++)
  {
    //Serial.print(arr[i]);
    //Serial.print(", ");
    if((abs(lastVal - arr[i]) <= 15) && (arr[i] > 925))
    {
      if(clipFlag == 0)
      {
        clipFlag = 1;
        clipStart = i;
      } else {
        clippingcount++;
      }
    } else {
      if(clipFlag == 1)
      {
        clipEnd = i;
      }
    }
    
    lastVal = arr[i];    
    
    if(arr[i] > largestVal) 
    {
      largestVal = arr[i];
      largestValPos = i;
    }

    if(clippingcount > 3)
    {
      largestValPos = (clipStart + (clipEnd - clipStart)) / 2;
    }
  }

  

  /*Serial.println();
  Serial.print(arrLen);
  Serial.print(", ");
  Serial.print(largestValPos);
  Serial.print(", ");
  Serial.print(clippingcount);
  Serial.print(", ");
  Serial.print(clipStart);
  Serial.print(", ");
  Serial.println(clipEnd);*/
  
  return workingData.curPeakEnd - (arrLen - largestValPos);
}

int findPeakTime(int maxPos, int arrLen)
{
  //function to find the absolute position in time 
}

// -------------------- Define Main Functions --------------------
void checkForPeak(struct workingDataContainer &workingData)
{
  if(workingData.hrData[workingData.buffPos] >= workingData.hrMovAvg[workingData.buffPos])
  {
    if(workingData.ROIPos >= 40){
      workingData.ROI_overflow = 1;
    } else {
      //Serial.println(1);
      workingData.peakFlag = 1;
      workingData.ROI[workingData.ROIPos] = workingData.curVal;
      workingData.ROIPos++;
      workingData.ROI_overflow = 0;
    }
  } else {
      //Serial.println(0);
  }
  
  //Serial.print(", ");
  
  if((workingData.hrData[workingData.buffPos] <= workingData.hrMovAvg[workingData.buffPos])
  && (workingData.peakFlag == 1))
  {
    if(workingData.ROI_overflow == 1)
    {
      workingData.ROI_overflow = 0;
    } else {
      //solve for peak
      workingData.curPeakEnd = workingData.absoluteCount;
      workingData.lastPeak = workingData.curPeak;
      workingData.curPeak = findMax(workingData.ROI, workingData.ROIPos, workingData);
      workingData.recent_RR[workingData.RR_pos] = workingData.curPeak;
      workingData.RR_pos++;
      if(workingData.RR_pos >= 10)
      {
        for(int i = 0; i<10; i++)
        {
          Serial.print(workingData.recent_RR[i]);
          Serial.print(", ");
        }
        Serial.println();
        delay(200);
        //exit(0);
      }
      //Serial.println(workingData.curPeak);
      //add peak to struct
    }
    workingData.peakFlag = 0;
    workingData.ROIPos = 0;
  }
}

// -------------------- Define Timer Interrupts --------------------
void setInterrupt()
{
  cli();
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1  = 0;
  TCCR1B |= (1 << WGM12);
  TCCR1B |= (1 << CS01) | (1 << CS00);
  TIMSK1 |= (1 << OCIE1A); 
  OCR1A = 2499;
  sei();
}

void readSensors(struct workingDataContainer &workingData)
{
  workingData.curVal = analogRead(hrpin); //read latest sensor value
  workingData.movAvgSum += workingData.curVal; //update total sum by adding recent value
  workingData.movAvgSum -= workingData.hrData[workingData.oldestValuePos];  //as well as subtracting oldest value
  workingData.hrMovAvg[workingData.buffPos] = workingData.movAvgSum / workingData.windowSize; //compute moving average
  workingData.hrData[workingData.buffPos] = workingData.curVal; //store sensor value
}

ISR(TIMER1_COMPA_vect)
{ 
  //define interrupt service routine
  //timed sensor read + mov avg calc routine at 152 uSec on 16 MHz
  readSensors(workingData);

  //check peak
  checkForPeak(workingData);

  Serial.print(workingData.hrData[workingData.buffPos]);
  Serial.print(", ");
  Serial.println(workingData.hrMovAvg[workingData.buffPos]);
  //Serial.println(freeRam());

  //update buffer pointers
  workingData.buffPos++; //update buffer position pointers
  workingData.oldestValuePos++;
  
  if(workingData.buffPos >= 100) workingData.buffPos = 0;
  if(workingData.oldestValuePos >= 100) workingData.oldestValuePos = 0;

  workingData.absoluteCount++;
}

// -------------------- Setup --------------------
void setup()
{
  //start serial
  Serial.begin(250000);

  //start timer interrupts
  setInterrupt();

}

// -------------------- Main Loop --------------------
void loop()
{
  
}

