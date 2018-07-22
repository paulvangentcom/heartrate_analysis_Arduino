/*
 * Arduino Heart Rate Analysis Toolbox - ARM Full Analysis
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
 
// -------------------- Includes --------------------
#include <math.h>
#include <arduinoFFT.h>c
//#include <SD.h>

// -------------------- User Settable Variables --------------------
int8_t hrpin = 0; //Whatever analog pin the sensor is hooked up to
int16_t led = 13; //status communication, ddfault is 13 (on-board led)
int8_t reportRaw = 0; //Whether to also print the raw heart rate data (1=true)


// -------------------- Non-Settable Variables --------------------
//declarations
long t1, t2; //for performance timing purposes
//File rawData;
//File processedData;
IntervalTimer sensorTimer;

/*
 * FFT implemented is based on work from Enrique Condes
 * https://github.com/kosme/arduinoFFT
 */
arduinoFFT FFT = arduinoFFT(); //create FFT object
//#define SCL_INDEX 0x00
//#define SCL_TIME 0x01
//#define SCL_FREQUENCY 0x02


//data buffer
struct dataBuffer 
{
  int16_t bufferID = 0; //0 for active buffer 0, 1 for active buffer 1
  int16_t dataBuffer0[2000] = {0};
  int16_t dataBuffer0Status = 0; //0 for clean, 1 for dirty buffer
  int16_t dataBuffer1[2000] = {0};
  int16_t dataBuffer1Status = 0; //0 for clean, 1 for dirty buffer
  int16_t bufferCounter = 0;
};
 
//working data container
struct workingDataContainer 
{
  //Settable parameters. Refer to documentation before changing these.
  float fs = 100.0; //sampling rate
  float hrw = 1.0; // window size in seconds
  int windowSize = fs * hrw; //window size in samples
  float bpmMin = 40.0; //minimum bpm allowed, for error detection purposes
  float bpmMax = 130.0; //maximum bpm allowed, for error detection purposes
  float bpmMinRRdist = (60.0/ bpmMax) * 1024.0; //reverse max and min, since higher hr equals shorter RR intervals
  float bpmMaxRRdist = (60.0 / bpmMin) * 1024.0;
  float mavgFactors[7] = {1.1, 1.2, 1.3, 1.4, 1.5, 1.75, 2.0};
  int mavgFactorsLen = 7;
  
  //Initialize arrays to hold sensor data and calculated measures
  int heartData[2000] = {0};
  int16_t heartmovAvg[2000] = {0};
  double vReal[128] = {0.0};
  double vImag[128] = {0.0};
  int16_t beatList[80] = {0}; //max 35 beats detected in 10 sec window (bpmmax = 150 = 25beats/10sec, max 10 rejected beats before break, total possible = 35
  int16_t beatListBin[80] = {0};
  float rrListX[69] = {0};
  float interpolatedRR[128] = {0};
  float fsRR = 0.0;
  int16_t rrList[69] = {0}; //rr intervals list is one less than number of beats-max rejected
  int16_t rrDiffList[68] = {0}; //rr diff list is one less than number or rr intervals-max rejected
  int16_t rrDiffsqList[68] = {0}; //rr diff squared list is one less than number of rr intervals-max rejected
  int16_t rejectedBeatList[10] = {0}; //array to hold rejected beats. length 10: more rejected beats? stop analysis
  int16_t newrrList[69] = {0}; //array to hold correct rr intervals;

  //Define buffers for aggregated data
  int16_t rrBufPosition = 0; //keep track of which buffer to fill with next data batch
  int16_t rrList_buf[60] = {0}; //keep data of last 3 runs for breathing analysis
  
  //Define variables required during analysis
  int16_t heartMean = 0;
  int16_t datalen = 2000;
  int16_t rrInterpLen = 128;
  float correctRejectRatio = 0.0;

  //Define variables for interpolation
  int16_t _length = 16;
  int16_t _prev_point = 0;
  float riemannSum = 0.0;

  //Define various counters to do the bookkeeping
  int16_t beatCount = 0;
  int16_t rejectedBeatCount = 0;
  int16_t i = 0;
  int16_t lastRejectedRR = 0;
};

//HR variables
struct hrdataContainer 
{
  int16_t curLoc = 0; //0-2 depending on buffer location to fill next
  int16_t prevLoc = 0; //previous buffer location, for datalogging value lookup
  int16_t firstRun = 1; //1 if first run (fill entire buffer), 0 otherwise
  float bpm[3] = {0.0};
  float ibi[3] = {0.0};
  float sdnn[3] = {0.0};
  float sdsd[3] = {0.0};
  float rmssd[3] = {0.0};
  float pnn20[3] = {0.0};
  float pnn50[3] = {0.0};
  //int16_t breath[3] = {0};

  //float bpm_ = 0.0;
  //float ibi_ = 0.0;
  //float sdnn_ = 0.0;
  //float sdsd_ = 0.0;
  //float rmssd_ = 0.0;
  //float pnn20_ = 0.0;
  //float pnn50_ = 0.0;
  float LF_ = 0.0;
  float HF_ = 0.0;
  float LFHF_ = 0.0;
  float breath_ = 0.0;
  int confidenceLevel = 0; 
  /* 
   * confidence is based on the accepted vs rejected peaks ratio. Values:
   * 0: error in analysis, too many incorrect peaks, no peaks detected, too much noise
   * 1: excellent, 100% of peaks accepted
   * 2. good, >= 75% of peaks accepted
   * 3. medium, >= 50% of peaks accepted
   * 4. poor, < 50% of peaks accepted
   */ 

 };


 

// -------------------- Define Data Structs --------------------
struct hrdataContainer hrData;
struct workingDataContainer workingData;
struct dataBuffer dataBuffers;

// -------------------- Helper Functions --------------------
void signalQuality(struct workingDataContainer &workingData, struct hrdataContainer &hrData) 
{
  if (workingData.correctRejectRatio >= 99.9) 
  {
    hrData.confidenceLevel = 1;
    } else if ((75.0 <= workingData.correctRejectRatio) && (workingData.correctRejectRatio < 99.9)) {
      hrData.confidenceLevel = 2;
    } else if ((50.0 <= workingData.correctRejectRatio) && (workingData.correctRejectRatio < 75.0)) {
      hrData.confidenceLevel = 3;
    } else if ((0.1 <= workingData.correctRejectRatio) && (workingData.correctRejectRatio < 50.0)) {
      hrData.confidenceLevel = 4; 
    } else {
      hrData.confidenceLevel = 0;
  }
}
 
int getMeanInt(int16_t data[], int16_t datalen) 
{
  long sum = 0;
  for(int i = 0; i < datalen; i++) 
  {
    sum += data[i]; 
  }
  return (sum / datalen); //return mean
}

int getMeanInt32(int data[], int datalen) 
{
  long sum = 0;
  for(int i = 0; i < datalen; i++) 
  {
    sum += data[i]; 
  }
  return (sum / datalen); //return mean
}

float getMeanFloat(int16_t data[], float datalen) 
{
  float sum = 0;
  for(int i = 0; i < datalen; i++) 
  {
    sum += data[i];
  }
  return (sum / datalen); //return mean
}

float getMean(float data[], float datalen) 
{  
  float sum = 0;
  for(int i = 0; i < datalen; i++) 
  {
    sum += data[i];
  }
  return (sum / datalen); //return mean
}

float getStdFloat(int16_t data[], float dataLen) 
{  
  float sqSum = 0.0;
  float diff = 0.0;
  float dataMean = getMeanFloat(data, dataLen);
  for (int i = 0; i < dataLen; i++) 
  {
    diff = data[i] - dataMean;
    sqSum += (diff * diff);
  }
  return sqrt(sqSum / dataLen);
}

float getMedian(int16_t data[], int16_t datalen) 
{ //find median of array 'data[]' with length 'datalen'  
  float temp;
  int i, j;
  for(i=0; i<datalen-1; i++) 
  {
      for(j=i+1; j<datalen; j++) 
      {
          if(data[j] < data[i]) 
          {
              temp = data[i];
              data[i] = data[j];
              data[j] = temp;
          }
      }
  }

  if(datalen%2==0) 
  {
    return((data[datalen/2] + data[datalen/2 - 1]) / 2.0); 
    } else {
    return data[datalen/2];
  }
}

int movAvg(int *ptrArr, long *ptrSum, int pos, int windowSize, int nextNum) 
{   
  //moving average function, adapted from BMCCormack/movingAvg.c on GitHub
  *ptrSum = *ptrSum - ptrArr[pos] + nextNum;
  ptrArr[pos] = nextNum;
  return *ptrSum / windowSize;
}

void callmovAvg(float factor, struct workingDataContainer &workingData) 
{  
  int arrNum[workingData.windowSize] = {0};
  int pos = 0;
  long sum = 0;
  for (int i = 0; i < workingData.datalen; i++) 
  {
    workingData.heartmovAvg[i] = movAvg(arrNum, &sum, pos, workingData.windowSize, workingData.heartData[i]) * factor;
    pos++;
    if(pos >= workingData.windowSize) pos = 0;
  }
  
  int heartMeanRaised = workingData.heartMean * factor;
  for (int i = 0; i < workingData.windowSize; i++)
  { 
    workingData.heartmovAvg[i] = heartMeanRaised;
  } //Pad first n=window size values with mean
}

// -------------------- Interpolation Functions --------------------
void interpolateRR(struct workingDataContainer &workingData) 
{
  float rrSum = 0.0;
  //Serial.print("\n------\n");
  for (int i = 0; i < (workingData.beatCount-workingData.rejectedBeatCount)-1 ; i++)
  {
    workingData.rrListX[i] = rrSum;
    if (i < ((workingData.beatCount-workingData.rejectedBeatCount)-2)) 
    {
      rrSum += (workingData.rrList[i]/1000.0);
    }
  }

  workingData.fsRR = 128.0 / rrSum;
  workingData._length = (workingData.beatCount - workingData.rejectedBeatCount)-1;
  float factor = rrSum / 128.0;
  
  for (int i = 0; i < 128; i++) 
  {
    workingData.interpolatedRR[i] = value(factor*i, workingData);
  }
}

float value(float x, struct workingDataContainer &workingData) 
{
  if( workingData.rrListX[0] > x ) 
  { 
    return workingData.rrList[0]; 
  }
  else if ( workingData.rrListX[workingData._length-1] < x ) { 
    return workingData.rrList[workingData._length-1]; 
  }
  else {
    for(int i = 0; i < workingData._length; i++ )
    {
      int index = ( i + workingData._prev_point ) % workingData._length;
      
      if( workingData.rrListX[index] == x ) 
      {
        workingData._prev_point = index;
        return workingData.rrList[index];
      } else if( (workingData.rrListX[index] < x) && (x < workingData.rrListX[index+1]) ) {
        workingData._prev_point = index;
        return calc( x, index, workingData );
      }
    }    
  }
  return 0; 
}

float calc(float x, int i, struct workingDataContainer &workingData) 
{
  if( i == 0 ) 
  {
    return workingData.rrList[1];
  } else if( i == workingData._length-2 ) {
    return workingData.rrList[workingData._length-2];
  } else {
    float t = (x-workingData.rrListX[i]) / (workingData.rrListX[i+1]-workingData.rrListX[i]);
    float m0 = (i==0 ? 0 : catmull_tangent(i) );
    float m1 = (i==workingData._length-1 ? 0 : catmull_tangent(i+1) );
    return hermite( t, workingData.rrList[i], workingData.rrList[i+1], m0, m1, workingData.rrListX[i], workingData.rrListX[i+1]);        
  }
}

float hermite( float t, float p0, float p1, float m0, float m1, float x0, float x1 ) 
{
  return (hermite_00(t)*p0) + (hermite_10(t)*(x1-x0)*m0) + (hermite_01(t)*p1) + (hermite_11(t)*(x1-x0)*m1);
}
float hermite_00( float t ) { return (2*pow(t,3)) - (3*pow(t,2)) + 1;}
float hermite_10( float t ) { return pow(t,3) - (2*pow(t,2)) + t; }
float hermite_01( float t ) { return (3*pow(t,2)) - (2*pow(t,3)); }
float hermite_11( float t ) { return pow(t,3) - pow(t,2); }

float catmull_tangent(int i) 
{
  if( workingData.rrListX[i+1] == workingData.rrListX[i-1] ) 
  {
  // Avoids division by 0
  return 0;
  } else {  
  return (workingData.rrList[i+1] - workingData.rrList[i-1]) / (workingData.rrListX[i+1] - workingData.rrListX[i-1]);    
  } 
}

// -------------------- Pre-processing --------------------
void enhancePeaks(struct workingDataContainer &workingData) 
{
  // Function to enhance R to noise ratio by cubing, then normalising back
  int hmean = getMeanInt32(workingData.heartData, workingData.datalen);
  int minim = hmean;
  int maxim = hmean;
  int curVal = 0;
  
  for(int i = 0; i < workingData.datalen; i++) 
  {
    curVal = pow(workingData.heartData[i], 2);
    workingData.heartData[i] = curVal;
    if (curVal < minim) minim = curVal;
    if (curVal > maxim) maxim = curVal;
  } //Cube signal and determine min, max for scaling
  
  for (int i = 0; i < workingData.datalen; i++) 
  { 
    workingData.heartData[i] = map(workingData.heartData[i], minim, maxim, 1, 1024);
  } //Scale back to between 1 and 1024 using map function
}

// -------------------- Peak detection and -fitting functions --------------------
void detectPeaks(struct workingDataContainer &workingData) 
{  
  workingData.beatCount = 0;
  int listPos = 0;
  int beatStart = 0;
  int beatEnd = 0;
  int curmax = 0;
  int beatPos = 0;
  
  for (int i = 0; i < workingData.datalen; i++) 
  {
    if ((workingData.heartData[i] <= workingData.heartmovAvg[i]) && (beatStart == 0)) 
    {
        listPos ++;
      } else if ((workingData.heartData[i] > workingData.heartmovAvg[i]) && (beatStart == 0)) {
        beatStart = i;
        listPos ++;
      } else if ((workingData.heartData[i] > workingData.heartmovAvg[i]) && (beatStart != 0)) {
        listPos ++;
      } else if ((workingData.heartData[i] <= workingData.heartmovAvg[i]) && (beatStart != 0)) {
        beatEnd = i;
        curmax = 0;
        beatPos = 0;
        for (int i = beatStart; i < beatEnd; i++) 
        {
          if (workingData.heartData[i] > curmax) 
          {
            beatPos = i;
            curmax = workingData.heartData[i];
          }
        }
        workingData.beatList[workingData.beatCount] = beatPos;
        workingData.beatCount ++;
        if (workingData.beatCount > 35) break;
        listPos ++;
        beatStart = 0;
        beatEnd = 0;
    }
  }
}

float fitPeaks(struct workingDataContainer &workingData) 
{
  float tstart = micros();
  int fittingLowestPos = 0;
  float lowestStdev = 1e20; //some large number
  float rrStdev = 0.0;
  float rrMean = 0.0;
  float estimatedBPM = 0.0;
  
  for (int i=0; i < workingData.mavgFactorsLen; i++) 
  {
    callmovAvg(workingData.mavgFactors[i], workingData);//raise moving average
    detectPeaks(workingData); //do the peak detection
    calcRR(workingData);
    rrMean = getMeanFloat(workingData.rrList, float(workingData.beatCount-1));
    estimatedBPM = 60000.0 / rrMean;
    //updateMeasures(workingData, hrData);
    //calcMeasures(workingData, hrData);
    rrStdev = getStdFloat(workingData.rrList, float(workingData.beatCount-1));
    if ((rrStdev < lowestStdev) && (rrStdev > 0.001) && (estimatedBPM >= workingData.bpmMin) && (estimatedBPM <= workingData.bpmMax)) 
    {
      lowestStdev = rrStdev;
      fittingLowestPos = i;
    }
  }
  
  Serial.printf("finished peak fitting with %i levels in %.2f miliseconds. Best fit was: %.2f\n", 
                workingData.mavgFactorsLen, (micros()-tstart)/1000.0, workingData.mavgFactors[fittingLowestPos]);
  return workingData.mavgFactors[fittingLowestPos];
}

int rejectPeaks(struct workingDataContainer &workingData) 
{
  workingData.lastRejectedRR = 0;
  workingData.rejectedBeatCount = 0;

  for (int i = 0; i < workingData.beatCount-1; i++) 
  {
    workingData.newrrList[i] = workingData.rrList[i];
  } //transfer rr list to second list to calculate median

  float rrMedian = getMedian(workingData.newrrList, workingData.beatCount-1);
  /*
   * PASS 1: initial rejection
   * Flow:
   * - Reject peaks based on thresholded RR-values
   * - For each RR: if incorrect: reject left peak
   * - At end: if last RR correct: accept last peak, otherwise reject
   * - Rebuild rrList to include only intervals between non-rejected peaks
   * - Update counters
   * - Calculate estimated confidence level: 
   */

  for(int i = 0; i < (workingData.beatCount-1); i++) 
  {
    //Serial.printf("Beat %i, with rrvalue %i, is: ", i, workingData.rrList[i]);
    if ((workingData.rrList[i] >= workingData.bpmMinRRdist) && (workingData.rrList[i] <= workingData.bpmMaxRRdist) && (abs(workingData.rrList[i]-rrMedian) <= 250)) 
    {
      if ((i - workingData.lastRejectedRR == 1) && (i > 1)) 
      {
        //Serial.print("rejected!\n");
        workingData.beatListBin[i] = 0;
        workingData.rejectedBeatCount++;
        } else {
          workingData.beatListBin[i] = 1;
          //Serial.printf("accepted!\n");
        } 
    } else {
        //Serial.printf("rejected!\n");
        workingData.lastRejectedRR = i;
        workingData.beatListBin[i] = 0;
        workingData.rejectedBeatCount++; 
      }
  }

  if ((workingData.beatListBin[0] == 1) && (workingData.beatListBin[1] == 0)) 
  {
    workingData.beatListBin[0] = 0;
    workingData.rejectedBeatCount++;
  }
 
  //evaluate state of second-to-last beat and decide if the last beat should be rejected or accepted
  if (workingData.beatListBin[workingData.beatCount-2] == 1) 
  {
    workingData.beatListBin[workingData.beatCount-1] = 1;
    //Serial.print("last beat is accepted\n");  
    } else {
      workingData.beatListBin[workingData.beatCount-1] = 0;
      //Serial.printf("last beat is rejected\n");
    } 
  
  Serial.print("Binary beatlist: ");
  for (int i = 0; i < workingData.beatCount; i++) 
  {
    Serial.printf("%i, ", workingData.beatListBin[i]);
  }
  Serial.print("\n");

  int j = 0;
  for (int i = 0; i < workingData.beatCount-1; i++) 
  {
    //loop over binary peaklist
    if ((workingData.beatListBin[i] == 1) && (workingData.beatListBin[i+1] == 1)) 
    {
      //Serial.printf("accepted rr between peaks %i and %i\n", i, i+1);
      //Serial.printf("RR interal before: %i\n", workingData.rrList[i]);
      workingData.rrList[i-j] = workingData.rrList[i];
      //Serial.printf("i-j = %i\n", i-j);
      //Serial.printf("RR interal after: %i\n", workingData.rrList[i]);
    } else {
        //Serial.printf("rejected rr between peaks %i and %i\n", i, i+1);
        workingData.rejectedBeatList[j] = workingData.beatList[i];
        j++;
    }
  } //update rrlist to include only intervals between correctly detected peaks

  //Calculate quality before updating beatcounter
  workingData.correctRejectRatio = 100.0 - ((100.0 * workingData.rejectedBeatCount) / workingData.beatCount);
  workingData.beatCount -= workingData.rejectedBeatCount; //update beatcounter
  
  return 0; //return zero to indicate less than 10 rejected beats
}

// -------------------- Analysis Functions --------------------
void calcRR(struct workingDataContainer &workingData) 
{
  float rrInterval = 0;
  for (int i = 0; i < (workingData.beatCount - 1); i++) 
  {
    rrInterval = workingData.beatList[i+1] - workingData.beatList[i];
    workingData.rrList[i] = ((rrInterval / workingData.fs) * 1000.0);
    //Serial.printf("RR interval: %f\n", ((rrInterval / workingData.fs) * 1000.0));
  }
}

void calcRRdiff(struct workingDataContainer &workingData) 
{  
  for (int i = 0; i < (workingData.beatCount - 2); i++) 
  {
    workingData.rrDiffList[i] = abs(workingData.rrList[i+1] - workingData.rrList[i]);
  }
}

void calcRRsqdiff(struct workingDataContainer &workingData) 
{
  for (int i = 0; i < (workingData.beatCount - 2); i++) 
  {
    workingData.rrDiffsqList[i] = workingData.rrDiffList[i] * workingData.rrDiffList[i];
  }
}

void updateMeasures(struct workingDataContainer &workingData, struct hrdataContainer &hrData) 
{
  float rrmean = getMeanFloat(workingData.rrList, float(workingData.beatCount-1));
  int nn20 = 0;
  int nn50 = 0;
  for(int i = 0; i < (workingData.beatCount-2); i++) 
  { 
    if (workingData.rrDiffList[i] >= 20.0) nn20++;
    if (workingData.rrDiffList[i] >= 50.0) nn50++;
  } //find RR-intervals larger than 20ms and 50ms
  
  //function to update actual measures based on buffer values
  //update measures
  hrData.bpm[hrData.curLoc] = 60000.0 / (rrmean);
  hrData.ibi[hrData.curLoc] = rrmean;
  hrData.sdnn[hrData.curLoc] = getStdFloat(workingData.rrList, float(workingData.beatCount-1));
  hrData.sdsd[hrData.curLoc] = getStdFloat(workingData.rrDiffList, float(workingData.beatCount-2));
  hrData.rmssd[hrData.curLoc] = pow((getMeanFloat(workingData.rrDiffsqList, float(workingData.beatCount-2))), 0.5); //get square root of the mean of the squared successive differences
  hrData.pnn20[hrData.curLoc] = (float(nn20) / float(workingData.beatCount-2.0)); //calculate proportion of intervals > 20ms
  hrData.pnn50[hrData.curLoc] = (float(nn50) / float(workingData.beatCount-2.0)); //calculate proportion of intervals > 50ms
  //other measures go here
  
  if (hrData.firstRun == 1) 
  {
    for (int i = 1; i < 3; i++) 
    {
      hrData.bpm[i] = hrData.bpm[0];
      hrData.ibi[i] = hrData.ibi[0];
      hrData.sdnn[i] = hrData.sdnn[0];
      hrData.sdsd[i] = hrData.sdsd[0];
      hrData.rmssd[i] = hrData.rmssd[0];
      hrData.pnn20[i] = hrData.pnn20[0];
      hrData.pnn50[i] = hrData.pnn50[0];
      hrData.firstRun = 0;
    }
  } //fill entire buffer upon first run
  
  //Serial.printf("Buffer location is: %i\n", hrData.curLoc);
  hrData.prevLoc = hrData.curLoc;
  hrData.curLoc++; //update buffer location
  if (hrData.curLoc == 3) hrData.curLoc = 0; //reset to first buffer index
}

void FFT_rrList(struct workingDataContainer &workingData) 
{
  for (int i = 0; i < workingData.rrInterpLen; i++) 
  {
    workingData.vReal[i] = workingData.interpolatedRR[i]; //Append heartrate data to FFT array
    workingData.vImag[i] = 0.0; //Zero imaginary array to prevent issues from previous FFT  
  }
  //Do the FFT
  FFT.Compute(workingData.vReal, workingData.vImag, workingData.rrInterpLen,
              FFT_FORWARD);
  FFT.ComplexToMagnitude(workingData.vReal, workingData.vImag, workingData.rrInterpLen);
  workingData.vReal[0] = 0; //prevent the ultra-low-frequency from leaking into signal
}

void calcFreqMeasures(struct workingDataContainer &workingData, struct hrdataContainer &hrData) 
{  
  hrData.LF_ = 0.0;
  hrData.HF_ = 0.0;

  //Use squared Fourier Transform to express spectral power
  //Note: Replace with 
  for (int i = 0; i < 64; i++) 
  {
    workingData.vReal[i] = workingData.vReal[i] * workingData.vReal[i];
  }

  //Apply a Riemann Sum with trapezoidal rule to find area under FFT function
  float tStep = ((workingData.fsRR) / workingData.rrInterpLen);
  for (int i = 0; i < (workingData.rrInterpLen >> 1); i++) 
  {
    if (((tStep * i) >= 0.04) && ((tStep * i) <= 0.15)) 
    {
      hrData.LF_ += ((0.5 * tStep) * (workingData.vReal[i] + workingData.vReal[i+1]));
    } else if (((tStep * i) >= 0.16) && ((tStep * i) <= 0.5)) {
      hrData.HF_ += ((0.5 * tStep) * (workingData.vReal[i] + workingData.vReal[i+1]));
    } else if ((tStep * i) >= 0.6) break; //break soon after last used frequency bin is passed
  }
  hrData.LFHF_ = hrData.LF_ / hrData.HF_;
  calcBreath(workingData, hrData, tStep);
}

void calcBreath(struct workingDataContainer &workingData, struct hrdataContainer &hrData, float tStep) 
{
  float maxPos = 0;
  float curMax = 0;
  for (int i = 0; i < (workingData.rrInterpLen >> 1); i++) 
  {
    if (workingData.vReal[i] > curMax) 
    {
      maxPos = i;
      curMax = workingData.vReal[i];
    }
  }
  hrData.breath_ = (maxPos * tStep);
}

// -------------------- Data Logging Functions --------------------
void readSensor()
{
  if (dataBuffers.bufferID == 0) 
  {
    dataBuffers.dataBuffer0[dataBuffers.bufferCounter] = analogRead(hrpin);
    dataBuffers.bufferCounter++;
  } else {
    dataBuffers.dataBuffer1[dataBuffers.bufferCounter] = analogRead(hrpin);
    dataBuffers.bufferCounter++;
  }
}

void flushBuffer(int16_t data[], struct workingDataContainer &workingData) 
{
  for (int i = 0; i < workingData.datalen; i++) 
  {
    workingData.heartData[i] = data[i];
  }
}

void switchBuffers(struct dataBuffer &dataBuffers, struct workingDataContainer &workingData) 
{
  dataBuffers.bufferCounter = 0;
  if (dataBuffers.bufferID == 0) 
  {
    dataBuffers.bufferID = 1;
    if(dataBuffers.dataBuffer0Status != 0) Serial.print("Overflow in buffer 0!\n");
    //flushData(dataBuffers.dataBuffer0, workingData.datalen, fileName);
    flushBuffer(dataBuffers.dataBuffer0, workingData);
    dataBuffers.dataBuffer0Status = 0;
  } else {
    dataBuffers.bufferID = 0;
    if(dataBuffers.dataBuffer1Status != 0) Serial.print("Overflow in buffer 1!\n");
    //flushData(dataBuffers.dataBuffer1, workingData.datalen, fileName);
    flushBuffer(dataBuffers.dataBuffer1, workingData);
    dataBuffers.dataBuffer1Status = 0;
  } 
}

// -------------------- Main Functions --------------------
void preProcessor(struct workingDataContainer &workingdata)
{
  //enhance peaks several times to accentuate the R-peak in the QRS complex
  enhancePeaks(workingData);
  enhancePeaks(workingData);
  //enhancePeaks(workingData);
  workingData.heartMean = getMeanInt32(workingData.heartData, workingData.datalen); //get mean of heart rate signal
}

void hrProcessor(struct workingDataContainer &workingData, struct hrdataContainer &hrData) 
{
  float bestFitFactor = fitPeaks(workingData);
  callmovAvg(bestFitFactor, workingData);
  detectPeaks(workingData);
  calcRR(workingData);
  Serial.printf("Found %i peaks at the following locations:\n", workingData.beatCount);
  for (int i = 0; i < workingData.beatCount; i++)
  {
    Serial.printf("%i, ", workingData.beatList[i]);
  }
  Serial.print("\n");
  if(rejectPeaks(workingData) == 0) //if less than 10 rejected, continue.
  {
    calcRRdiff(workingData);
    calcRRsqdiff(workingData);
    updateMeasures(workingData, hrData);
    interpolateRR(workingData);
    FFT_rrList(workingData);
    calcFreqMeasures(workingData, hrData);
    signalQuality(workingData, hrData);
  }
  else
  {
    hrData.confidenceLevel = 0; //set confidence level to 
    Serial.print("too many incorrect peaks detected\n");
  }
  
  //Print results to Serial.
  if (workingData.rejectedBeatCount > 0) 
  {
    Serial.printf("Rejected %i peaks at: ", workingData.rejectedBeatCount);
    if (workingData.rejectedBeatList[0] == 0)
    {
      workingData.i = 1; //set i to 1 if first beat was correctly detected
      for (int i = 1; i < workingData.rejectedBeatCount+1; i++)
      {
        Serial.printf("%i, ", workingData.rejectedBeatList[i]);
      }
      Serial.print("\n");
    } else {
      workingData.i = 0; //set i to 0 if first beat was incorrectly detected
      for (int i = 0; i < workingData.rejectedBeatCount; i++)
      {
        Serial.printf("%i, ", workingData.rejectedBeatList[i]);
      }
      Serial.print("\n");
    }
  }
}

void processData(struct workingDataContainer &workingData, struct hrdataContainer &hrData) 
{
  float totalt1 = micros();
  
  t1 = micros();

  preProcessor(workingData);
  Serial.printf("Finished preprocessing in %i uSec\n", micros()-t1);

  t1 = micros();
  hrProcessor(workingData, hrData);
  Serial.printf("Finished main analysis loop in %.2f miliseconds\n", (micros()-t1)/1000.0);

  Serial.print("RR list: ");
  for (int i = 0; i < (workingData.beatCount-workingData.rejectedBeatCount)-1 ; i++)
  {
    Serial.printf("%i, ", workingData.rrList[i]);
  }
  Serial.print("\n");

  Serial.print(F("\n---------------------------\nMeasures are:\n"));
  Serial.printf("BPM: %f.\n", hrData.bpm[hrData.prevLoc]);
  Serial.printf("IBI: %f.\n", hrData.ibi[hrData.prevLoc]);
  Serial.printf("SDNN: %f.\n", hrData.sdnn[hrData.prevLoc]);
  Serial.printf("SDSD: %f.\n", hrData.sdsd[hrData.prevLoc]);
  Serial.printf("RMSSD: %f.\n", hrData.rmssd[hrData.prevLoc]);
  Serial.printf("pNN20: %f.\n", hrData.pnn20[hrData.prevLoc]);
  Serial.printf("pNN50: %f.\n", hrData.pnn50[hrData.prevLoc]);
  Serial.printf("HF: %f\n", hrData.HF_);
  Serial.printf("LF: %f\n", hrData.LF_);
  Serial.printf("LF/HF: %f\n", hrData.LFHF_);
  Serial.printf("Breathing: %f Hz\n", hrData.breath_);
  Serial.printf("Signal quality is: %i\n", hrData.confidenceLevel);

  Serial.println("regular RR:");
  for(int i=0; i<workingData.beatCount-1; i++)
  {
    Serial.printf("%i, ", workingData.rrList[i]);
  }

  Serial.println("inteprolated RR:");
  for(int i = 0; i<128; i++)
  {
    Serial.printf("%f, ", workingData.interpolatedRR[i]);
  }
  
  Serial.print(F("\n---------------------------\n"));
  
  Serial.printf("Ending loop function. Full runtime: %.2f miliseconds\n", (micros()-totalt1)/1000.0);
  //stopWorking();

  if(reportRaw)
  {
    for (int i = 0; i < workingData.datalen; i++)
    {
      Serial.printf("%i,%i\n", workingData.heartData[i], workingData.heartmovAvg[i]);
    }
  }
  Serial.print(F("\n---------------------------\n"));
}

 //temp function
void stopWorking() 
{
  Serial.print("Stopped working!");
  digitalWrite(led, HIGH);
  while(1==1)
  {
    if(Serial.available() >= 1)
    {
      Serial.read();
      break;
    }
    delay(500);
  }
}

// -------------------- Setup Function --------------------
void setup() 
{
  pinMode(led, OUTPUT);
  Serial.begin(250000);
  delay(200);
  sensorTimer.begin(readSensor, 10000);
}

// -------------------- Loop Function --------------------
void loop() 
{
  if (dataBuffers.bufferCounter > 1999) 
  { 
    //sensorTimer.end();
    switchBuffers(dataBuffers, workingData);
    processData(workingData, hrData);
    //flushResults(hrData, processedData);
  } //if 2000 items have been stuffed into buffer, switch buffer and process data

  if (Serial.available() > 0) 
  {
    Serial.read();
    stopWorking();
  } 
}
