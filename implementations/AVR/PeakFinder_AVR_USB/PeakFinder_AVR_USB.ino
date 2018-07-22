/*
 * Arduino Heart Rate Analysis Toolbox - Peak Finder
 *      Copyright (C) 2018 Paul van Gent
 *      
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License V3 as published by
 * the Free Software Foundation. The program is free for any commercial and
 * non-commercial usage and adaptation, granted you give the recipients 
 * of your code the same open-source rights and license.
 * 
 * You can read the full license granted to you here:
 * https://www.gnu.org/licenses/gpl-3.0.en.html
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
int8_t report_hr = 1; //if 1, reports raw heart rate and peak threshold data as well, else set to 0 (default 0)
float max_bpm = 180; //The max BPM to be expected, used in error detection (default 180)
float min_bpm = 45; //The min BPM to be expected, used in error detection (default 45)


// -------------------- Non-Settable Variables --------------------
// Seriously, don't touch
int largestVal = 0;
int largestValPos = 0;
int clippingcount = 0;
int8_t clipFlag = 0;
int clipStart = 0;
int8_t clipEnd = 0;
int lastVal = 0;
int16_t max_RR = (60.0 / min_bpm) * 1000.0;
int16_t min_RR = (60.0 / max_bpm) * 1000.0;


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
  int16_t windowSize = 60; //windowSize in samples
  int16_t hrMovAvg[100] = {0};
  int8_t oldestValuePos = 1;
  long movAvgSum = 0;
  int16_t rangeLow = 0;
  int16_t rangeLowNext = 1024;
  int16_t rangeHigh = 1023;
  int16_t rangeHighNext = 1;
  int16_t rangeCounter = 0;

  //peak variables
  int16_t ROI[60] = {0};
  //int16_t ROI_interp[40] = {0};
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
  int16_t RR_mean = 0;
  int16_t RR_sum = 0;
  int8_t RR_pos = 0;
  int16_t lower_threshold = 0;
  int16_t upper_threshold = 1;
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
    if((abs(lastVal - arr[i]) <= 5) && (arr[i] > 1020))
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
    
  return workingData.curPeakEnd - (arrLen - largestValPos);
}

void getMeanRR(struct workingDataContainer &workingData)
{ //returns the mean of the RR_list array.
  workingData.RR_sum = 0;
  for(int i = 0; i<20; i++)
  {
    workingData.RR_sum += workingData.recent_RR[i];
  }
  workingData.RR_mean = workingData.RR_sum / 20;  
}

int16_t map_int(int16_t x, int16_t in_min, int16_t in_max, int16_t out_min, int16_t out_max)
{
  return (x - in_min) * (out_max - out_min + 1) / (in_max - in_min + 1) + out_min;
}

void establish_range(struct workingDataContainer &workingData)
{
  if(workingData.rangeCounter <= 200)
  {
    //update upcoming ranges
    if(workingData.rangeLowNext > workingData.curVal) workingData.rangeLowNext = workingData.curVal;
    if(workingData.rangeHighNext < workingData.curVal) workingData.rangeHighNext = workingData.curVal;
    workingData.rangeCounter++;
  } else {
    //set range, minimum range should be bigger than 50
    //otherwise set to default of (0, 1024)
    if((workingData.rangeHighNext - workingData.rangeLowNext) > 50)
    {
      //update range
      workingData.rangeLow = workingData.rangeLowNext;
      workingData.rangeHigh = workingData.rangeHighNext;
      workingData.rangeLowNext = 1024;
      workingData.rangeHighNext = 1;      
    } else {
      //reset range to default
      workingData.rangeLow = 0;
      workingData.rangeHigh = 1024;
    }
    workingData.rangeCounter = 0;
  }
}

// -------------------- Define Main Functions --------------------
void readSensors(struct workingDataContainer &workingData)
{
  workingData.curVal = analogRead(hrpin); //read latest sensor value

  establish_range(workingData);
  workingData.curVal = map(workingData.curVal, workingData.rangeLow, workingData.rangeHigh, 1, 1024);
  if(workingData.curVal < 0) workingData.curVal = 0;
  //if(workingData.curVal > 1023) workingData.curVal = 1023;
  
  workingData.movAvgSum += workingData.curVal; //update total sum by adding recent value
  workingData.movAvgSum -= workingData.hrData[workingData.oldestValuePos];  //as well as subtracting oldest value
  workingData.hrMovAvg[workingData.buffPos] = workingData.movAvgSum / workingData.windowSize; //compute moving average
  workingData.hrData[workingData.buffPos] = workingData.curVal; //store sensor value
  
}

void checkForPeak(struct workingDataContainer &workingData)
{
  if(workingData.hrData[workingData.buffPos] >= workingData.hrMovAvg[workingData.buffPos])
  {
    if(workingData.ROIPos >= 60){
      workingData.ROI_overflow = 1;
      if(report_hr) Serial.print(",");
      return;
    } else {
      workingData.peakFlag = 1;
      workingData.ROI[workingData.ROIPos] = workingData.curVal;
      workingData.ROIPos++;
      workingData.ROI_overflow = 0;
      if(report_hr) Serial.print(",");
      return;
    }
  }
  
  if((workingData.hrData[workingData.buffPos] <= workingData.hrMovAvg[workingData.buffPos])
  && (workingData.peakFlag == 1))
  {
    if(workingData.ROI_overflow == 1)
    {
      workingData.ROI_overflow = 0;
    } else {
      //solve for peak
      workingData.lastRR = workingData.curRR;
      workingData.curPeakEnd = workingData.absoluteCount;
      workingData.lastPeak = workingData.curPeak;
      workingData.curPeak = findMax(workingData.ROI, workingData.ROIPos, workingData);
      workingData.curRR = (workingData.curPeak - workingData.lastPeak) * 10;
      //Serial.println(workingData.curPeak);
      //add peak to struct
    }
    workingData.peakFlag = 0;
    workingData.ROIPos = 0;

    //error detection run, timed at ????????????????????

    if(workingData.curRR > max_RR || workingData.curRR < min_RR)
    {
      if(report_hr) Serial.print(",");
      return; //break if outside of BPM bounds anyway
    } else if(workingData.initFlag != 0)
    {
      validatePeak(workingData);
    } else {
      updatePeak(workingData);
    }
  } else if (report_hr) Serial.print(",");
}

void validatePeak(struct workingDataContainer &workingData)
{
  //validate peaks by thresholding, only update if within thresholded band
  if(workingData.initFlag != 0)
  {
    getMeanRR(workingData);
    if((workingData.RR_mean* 0.3) <= 300){
      workingData.lower_threshold = workingData.RR_mean - 300;
      workingData.upper_threshold = workingData.RR_mean + 300;
    } else{
      workingData.lower_threshold = workingData.RR_mean - (0.3 * workingData.RR_mean);
      workingData.upper_threshold = workingData.RR_mean + (0.3 * workingData.RR_mean);
    }
    
    if(workingData.curRR < workingData.upper_threshold &&
    workingData.curRR > workingData.lower_threshold &&
    abs(workingData.curRR - workingData.lastRR) < 350)
    {
      updatePeak(workingData);
    } else {
      if(report_hr) Serial.print(",");
    }
  }
}

void updatePeak(struct workingDataContainer &workingData)
{
  //updates peak positions, adds RR-interval to recent intervals
  workingData.recent_RR[workingData.RR_pos] = workingData.curRR;
  workingData.RR_pos++;
  
  if(workingData.RR_pos >= 20)
  {
    workingData.RR_pos = 0;
    workingData.initFlag = 1;
  }

  if(!report_hr)
  {
    Serial.print(workingData.curRR);
    Serial.print(",");
    Serial.println(workingData.curPeak);
  } else {
    Serial.print(workingData.curRR);
    Serial.print(",");
    Serial.print(workingData.curPeak);
  }
}

// -------------------- Define Timer Interrupts --------------------
void setInterrupt()
{
  //function to set hardware timer interrupts at 100Hz.
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

ISR(TIMER1_COMPA_vect)
{ 
  /* define interrupt service routine
  * timed total interrupt routine at max 250 microsec
  * more than fast enough for 100Hz.
  * higher sampling rate not recommended due to increased RAM requirements
  */
  readSensors(workingData);

  //check if peak is present, update variables if so
  checkForPeak(workingData);
  
  //report raw signal if requested
  if(report_hr) 
  {
    Serial.print(",");
    Serial.print(workingData.hrMovAvg[workingData.buffPos]);
    Serial.print(",");
    Serial.println(workingData.curVal);
  }
  //update buffer position pointers
  workingData.buffPos++;
  workingData.oldestValuePos++;

  //reset buffer pointers if at end of buffer
  if(workingData.buffPos >= 100) workingData.buffPos = 0;
  if(workingData.oldestValuePos >= 100) workingData.oldestValuePos = 0;

  //increment total sample counter (used for RR determination)
  workingData.absoluteCount++;
}

// -------------------- Setup --------------------
void setup()
{
  //start serial
  Serial.begin(250000);
  Serial.println("Welcome, starting up..");
  
  //start timer interrupts
  setInterrupt();
}

// -------------------- Main Loop --------------------
void loop()
{
  
}

