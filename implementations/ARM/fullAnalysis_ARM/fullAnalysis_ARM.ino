/*
 * Arduino Heart Rate Analysis Toolbox
 *      Copyright (C) 2018 Paul van Gent
 *      
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License V3 as published by
 * the Free Software Foundation. The program is free for any non-commercial
 * usage and adaptation, granted you give the recipients of your code the same
 * open-source rights and license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */


/*
 * TO DO:
 *  - complete data logging functionality: log raw, peak data, processed data
 *  - implement peak interpolation, find max
 *  - clean code
 *  - optimise code
 */

 /*
  * TO DO LATER:
  * - test JSON writing speed, usefulness
  * - implement JSON logging if fast enough
  */



//includes
#include <math.h>
#include <arduinoFFT.h>
//#include <SD.h>


//declarations
int8_t hrpin = 0;
long t1, t2; //for performance timing purposes
int16_t led = 13; //status communication
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

/* 
 * declare data objects
 */

 //temp data objects for simulation of measurement
int16_t data0[2000] = {507,506,506,508,507,507,508,510,509,509,509,510,509,508,509,510,510,510,512,512,510,509,510,510,508,508,508,509,507,507,509,509,507,507,508,509,508,510,514,521,533,548,566,580,590,599,605,606,600,594,585,573,560,546,532,518,504,494,484,476,469,467,465,463,464,467,470,471,474,478,481,484,487,492,496,499,503,508,511,512,513,515,515,513,513,512,510,507,505,505,503,501,500,501,499,499,499,499,499,499,500,501,502,503,504,505,505,506,508,509,508,508,508,508,508,509,510,510,508,510,510,509,508,508,508,508,507,508,508,507,506,508,508,509,508,509,510,510,509,508,509,508,507,508,509,507,508,510,512,513,520,533,551,568,585,601,611,616,620,618,611,602,591,577,562,545,530,516,503,491,483,478,475,472,472,473,474,476,478,481,481,481,485,487,487,488,491,494,495,498,501,503,504,506,508,509,508,508,509,507,505,505,504,503,499,499,499,499,497,497,498,498,498,500,502,501,502,504,505,505,506,506,506,504,505,504,506,504,504,505,506,505,505,507,507,506,505,506,506,505,505,506,506,505,504,506,506,506,506,507,509,507,511,512,511,510,510,512,512,514,519,529,543,559,578,593,604,612,619,619,614,607,599,585,569,554,538,524,511,498,489,481,474,470,469,468,466,467,471,473,474,476,479,482,483,487,492,493,495,499,504,505,509,513,516,516,515,516,517,515,514,512,508,505,504,502,502,500,499,499,497,496,496,498,499,499,500,501,502,501,503,504,506,507,509,511,512,511,513,517,516,516,519,521,520,521,521,523,522,523,522,522,520,519,519,518,515,514,515,514,514,513,514,512,510,509,509,508,506,508,510,514,526,542,557,570,580,589,595,595,592,586,578,567,556,544,532,517,504,492,482,473,468,464,464,462,465,469,473,475,479,484,489,490,494,497,500,503,506,509,511,511,511,512,510,509,508,507,502,498,496,493,489,486,486,486,485,486,488,489,491,491,493,494,495,496,497,496,495,496,498,499,498,498,499,501,501,502,503,503,503,505,507,505,505,505,506,506,506,507,507,507,506,506,506,505,504,504,504,503,503,507,509,511,520,534,548,563,576,592,604,612,618,618,616,608,600,588,576,561,550,537,526,514,507,499,494,489,486,484,482,478,477,476,475,474,474,475,476,479,483,487,489,493,498,502,505,508,511,512,511,512,513,512,511,510,508,507,504,502,501,498,495,494,492,491,490,490,491,491,491,492,494,493,493,496,496,497,498,499,501,502,502,504,504,505,507,508,508,506,507,508,508,508,508,509,510,509,508,509,508,508,508,508,507,506,507,507,508,510,520,535,551,570,588,606,621,632,641,643,643,637,632,624,613,602,592,580,568,557,547,538,528,519,512,505,499,494,492,487,485,484,485,483,483,484,486,486,489,494,499,502,506,510,513,515,515,516,517,517,514,515,514,511,509,507,505,500,498,496,493,489,489,490,488,486,486,488,486,487,489,491,490,490,492,495,493,493,493,494,493,493,495,498,496,498,500,503,502,503,504,504,503,503,504,505,504,505,507,509,510,512,514,513,512,511,513,511,511,513,513,513,512,516,525,538,557,578,597,612,624,630,632,628,621,612,600,583,567,550,533,516,502,490,479,468,460,454,449,445,443,443,445,447,449,455,458,464,469,476,482,487,494,498,503,507,512,515,518,518,520,520,519,517,516,513,511,508,506,504,502,501,501,500,500,501,501,502,502,504,505,505,505,505,507,508,508,507,507,506,507,507,509,508,508,509,509,509,508,509,510,508,508,507,507,507,506,505,504,504,504,506,505,505,505,506,509,515,527,548,567,588,605,620,631,638,641,641,636,628,620,607,593,577,562,547,533,522,512,506,500,495,494,492,489,487,486,484,483,483,482,483,483,485,487,489,490,494,498,502,504,507,509,509,510,510,509,507,505,503,500,498,494,492,491,490,487,484,484,481,481,481,482,481,482,483,485,486,488,490,493,494,495,496,499,501,504,506,508,509,510,511,511,511,510,511,509,508,509,508,506,506,507,509,509,509,509,510,510,508,509,511,512,511,511,511,511,513,520,532,552,573,594,616,634,650,663,672,676,676,671,661,645,629,610,593,575,558,544,529,517,506,498,492,486,481,479,476,473,472,472,471,471,473,476,478,481,485,490,492,496,500,503,504,506,508,507,503,502,500,496,493,490,487,484,482,481,479,478,477,477,479,478,480,482,484,486,488,492,493,495,498,502,503,504,506,508,510,510,512,512,513,512,513,513,513,512,513,513,513,513,513,513,512,512,513,514,512,509,508,508,506,504,503,501,499,498,498,498,499,499,499,500,501,501,502,504,506,510,520,535,555,577,599,618,632,641,647,646,639,630,615,598,579,560,541,521,503,488,477,466,460,454,452,450,449,451,454,456,460,463,466,469,473,478,482,487,492,497,502,506,510,513,515,517,519,519,519,518,517,514,513,511,509,509,508,508,506,506,504,502,502,502,500,501,502,502,502,502,504,505,505,506,506,505,504,505,505,506,506,506,507,508,508,510,511,513,513,514,518,521,523,527,528,529,530,529,529,528,526,524,522,521,518,514,508,503,498,494,492,491,490,491,496,506,523,546,568,588,603,615,622,622,616,607,594,579,563,547,531,516,503,491,483,473,468,464,462,463,466,474,479,487,494,500,508,516,525,534,545,556,568,582,596,608,618,627,632,633,631,624,615,605,597,588,579,571,564,558,552,551,551,551,548,548,546,546,547,552,559,567,578,589,599,605,610,612,609,601,589,571,547,519,489,458,425,392,364,340,322,306,296,292,295,303,316,331,349,367,388,407,426,443,459,474,485,495,504,516,524,533,541,549,556,562,570,574,580,585,588,591,595,598,600,601,599,596,591,584,576,565,552,539,523,507,494,478,464,453,444,435,430,427,426,425,426,430,432,436,440,445,451,456,462,467,473,478,484,489,495,500,505,511,517,523,527,532,535,539,542,546,551,554,558,563,567,570,573,575,577,579,581,582,583,585,584,585,583,578,575,573,568,563,558,551,544,537,532,529,525,522,521,519,518,518,517,519,521,524,526,529,532,533,535,534,533,531,527,522,515,504,495,480,465,449,432,414,396,383,371,362,354,353,353,358,363,372,382,395,412,440,476,512,548,584,615,642,666,685,698,704,703,695,683,665,642,619,594,569,547,526,508,493,480,472,463,459,457,455,456,458,459,463,466,469,473,477,482,487,492,497,500,504,507,509,510,510,508,507,504,500,497,492,487,482,480,476,471,469,468,469,468,470,472,474,477,481,484,487,490,492,496,498,500,501,502,502,502,504,505,504,505,505,506,505,503,504,504,503,503,502,502,502,501,500,501,500,500,501,501,501,501,502,503,503,504,504,505,504,503,504,504,504,505,507,510,518,534,555,576,598,618,636,646,653,654,649,639,624,608,590,569,550,531,512,496,482,470,461,453,448,446,443,441,443,444,448,452,457,463,468,475,481,488,494,499,505,508,512,516,519,519,518,518,516,514,511,507,505,504,502,499,499,497,495,496,495,496,496,497,497,497,497,499,499,501,501,502,502,503,503,503,505,504,504,504,504,505,505,507,506,506,507,509,508,508,507,507,506,506,505,507,506,506,507,509,511,509,510,510,512,512,512,515,518,528,544,564,586,606,627,644,656,662,664,661,651,640,623,606,584,565,546,530,514,502,492,483,477,473,472,472,471,474,474,477,478,480,481,483,483,485,486,490,492,495,498,502,503,503,504,505,503,503,502,501,499,498,495,494,493,492,490,490,490,490,490,489,491,491,492,491,492,493,495,495,495,496,497,497,498,499,500,502,502,503,502,503,502,503,503,502,503,502,503,504,504,505,503,504,504,504,503,503,503,504,503,504,503,505,504,506,506,506,505,506,506,507,507,508,509,512,517,529,547,567,586,604,620,634,641,645,642,634,623,609,593,574,556,537,520,505,490,478,467,460,454,451,449,447,448,450,453,457,462,468,473,477,482,488,493,499,502,506,509,511,516,516,518,516,515,514,512,510,507,506,504,503,502,500,499,498,497,497,496,497,497,498,498,498,500,501,502,504,504,504,503,504,505,504,505,505,506,505,504,504,505,505,504,505,506,506,505,507,506,507,507,507,506,506,507,507,508,506,507,506,507,506,506,507,508,508,508,507,508,506,508,507,506,507,507,508,510,513,526,543,563,582,603,619,631,640,643,642,635,625,611,595,576,557,540,522,504,488,476,464,455,449,445,445,445,446,451,454,458,463,468,474,480,486,493,498,504,509,513,517,517,519,519,520,520,518,517,515,512,511,508,506,506,504,503,502,503,503,504,504,504,504,504,505,506,506,507,508,508,508,507,508,510,511,513,512,511,512,510,509,508,506,506,504,503,504,504,502,503,503,503,503,503,503,502,503,503,504,504,505,504,505,505,505,505,505,506,506,508,509,516,529,547,568,587,606,622,633,638,640,637,629,617,604,587,570,550,533,516,501,485,472,463,456,450,448,446,447,450,452,456,459,464,469,475,480,485,492,498,501,505,507,510,510};
int16_t data1[2000] = {481,485,491,494,499,503,507,508,509,511,511,510,509,509,508,507,506,506,503,501,501,503,501,500,501,501,500,501,502,503,503,504,505,505,504,503,504,505,506,507,509,510,508,508,508,507,506,505,504,503,501,502,503,501,502,502,505,504,504,506,507,507,507,507,509,509,509,511,514,520,532,549,568,583,598,611,618,620,619,615,607,594,581,567,552,535,521,507,496,483,476,469,462,457,455,454,452,452,456,460,463,467,472,477,482,488,495,502,506,511,516,519,520,521,522,521,519,518,516,513,510,509,507,507,504,502,502,500,498,499,501,500,501,502,503,504,503,503,505,505,504,506,507,506,505,507,506,506,504,505,503,502,502,503,504,503,503,505,506,505,507,508,509,510,511,513,513,512,513,516,520,529,544,560,575,590,603,613,618,619,617,612,601,589,577,563,548,533,521,509,497,490,483,477,473,470,467,467,464,465,465,468,470,474,476,480,482,486,491,496,499,503,508,510,511,514,517,516,514,514,513,509,508,507,506,504,503,503,503,501,500,501,501,499,499,500,501,500,500,501,501,500,502,503,504,503,506,507,509,509,510,511,510,509,509,508,507,506,507,506,504,502,503,504,503,502,503,503,504,504,505,505,505,506,507,508,509,511,517,527,540,558,576,592,607,618,626,627,624,619,611,598,583,569,554,538,524,511,500,490,481,474,469,464,462,462,463,463,464,465,468,469,471,477,479,481,484,489,492,495,497,501,503,503,503,502,501,499,497,496,491,488,488,487,484,484,484,486,487,488,491,494,496,497,498,502,501,502,505,507,506,506,508,508,506,507,507,508,506,506,507,507,506,505,506,505,503,505,507,506,506,507,509,509,510,512,514,514,515,515,517,516,516,518,517,516,513,512,512,508,506,507,505,502,502,503,503,502,506,513,526,541,559,578,593,604,613,618,617,611,605,594,582,565,548,534,517,501,488,477,465,456,451,448,445,443,446,448,451,455,461,467,472,478,484,491,495,500,505,509,511,514,516,518,516,516,516,513,508,504,499,493,486,481,476,472,469,471,472,472,472,477,480,482,484,490,496,498,500,505,508,509,510,514,517,516,519,522,523,522,523,524,521,518,518,519,517,514,513,514,512,510,509,510,507,505,505,506,505,502,504,505,505,505,506,508,507,507,506,505,504,503,503,503,501,501,502,505,507,517,533,549,565,579,592,600,603,605,603,595,585,574,562,547,531,518,503,488,477,467,461,454,449,447,447,447,448,451,456,460,465,472,477,480,484,489,494,496,498,500,502,502,503,504,504,502,502,503,502,501,502,504,502,504,505,506,505,504,505,507,506,504,503,504,503,502,502,504,504,504,507,508,507,508,510,509,509,511,514,515,515,516,515,515,514,512,513,512,509,508,508,507,506,507,506,507,506,508,508,507,506,506,507,506,505,506,507,508,508,513,519,530,546,564,579,592,602,609,611,607,600,592,580,565,551,537,521,504,489,480,470,463,458,455,453,452,453,456,458,462,468,474,478,483,489,495,498,501,505,509,509,510,512,513,513,514,515,516,515,514,514,512,509,509,510,509,508,509,510,509,507,510,510,510,509,509,509,508,506,505,506,505,504,506,507,507,507,508,509,509,510,513,516,518,520,523,524,524,524,525,524,521,520,520,518,514,514,513,513,512,513,514,514,513,515,515,513,511,512,512,511,510,509,509,508,507,507,506,505,505,510,521,534,551,568,583,595,605,611,611,604,596,588,574,558,543,530,515,501,489,480,472,465,462,462,461,461,465,468,469,472,477,483,487,492,500,506,510,517,523,528,529,530,533,533,530,529,530,528,523,520,515,511,507,503,502,500,496,496,494,494,491,492,494,494,494,495,498,498,500,501,503,503,503,504,505,504,504,506,506,506,506,507,508,507,506,508,507,506,503,504,503,501,502,504,503,502,503,505,505,505,506,507,508,507,509,511,510,510,510,512,512,512,512,513,512,511,510,510,508,509,509,512,513,517,529,543,560,576,591,602,606,609,608,601,591,578,564,548,531,515,502,490,479,471,466,462,458,459,460,462,463,467,472,474,478,483,489,493,495,500,503,505,508,509,511,510,512,514,512,512,510,511,510,509,508,508,508,507,506,506,505,504,504,503,502,502,503,504,505,506,507,508,508,507,508,510,510,509,510,510,510,510,509,509,508,509,510,510,507,507,507,508,507,507,507,507,506,508,508,508,507,507,509,508,507,509,509,508,507,508,510,509,506,508,509,509,509,512,518,525,539,557,575,588,600,607,610,608,603,594,582,567,552,538,521,505,493,483,472,466,462,460,458,457,457,460,463,466,471,477,481,486,491,496,499,503,508,511,513,514,517,517,517,517,517,516,514,514,511,510,506,505,504,502,501,500,501,501,501,501,502,503,503,505,507,508,507,508,510,510,510,510,510,509,508,508,507,507,506,507,506,505,506,508,509,507,506,506,507,505,505,506,505,505,505,506,506,507,509,511,511,511,514,514,512,512,511,511,510,509,511,514,518,529,544,563,579,594,604,612,615,613,608,598,585,570,556,538,520,505,493,482,471,464,461,458,457,459,462,465,468,473,478,482,486,489,494,495,497,499,501,502,504,505,508,509,509,510,510,510,510,510,508,507,506,506,506,505,504,503,503,502,502,502,502,502,503,504,504,503,505,506,505,504,505,505,504,503,505,504,503,503,504,506,506,507,509,508,508,508,509,509,507,506,507,509,507,507,509,507,506,506,506,506,504,506,508,510,508,509,511,509,509,508,509,509,508,510,512,514,516,524,537,554,569,586,598,607,611,612,610,601,589,576,559,542,527,512,499,486,476,468,465,460,459,460,459,459,463,468,471,475,480,485,490,494,498,501,502,505,507,509,509,510,512,512,512,512,512,512,511,510,510,509,507,507,508,510,511,514,516,519,520,523,525,525,523,522,521,518,516,515,513,510,507,504,500,495,490,487,484,481,481,481,482,482,484,488,493,495,498,503,506,508,510,513,516,518,521,522,524,524,523,524,523,521,520,519,516,514,512,510,509,508,507,507,506,504,505,505,506,508,516,529,543,561,577,589,595,600,601,595,586,575,561,544,528,514,501,489,479,472,467,464,462,464,467,470,472,479,484,488,493,499,503,506,509,512,515,516,516,517,517,517,518,518,519,518,519,519,519,518,516,516,514,512,510,509,507,505,505,505,505,505,504,505,505,504,503,503,502,502,502,501,502,501,502,504,504,505,507,508,509,511,512,512,512,510,511,512,512,512,514,514,515,514,516,515,513,513,513,513,512,512,513,514,515,516,519,520,520,520,521,520,520,520,518,516,515,514,515,515,520,531,544,557,568,576,583,585,584,581,574,564,551,539,525,511,499,489,479,471,464,462,459,456,458,460,463,465,471,476,481,486,494,501,505,509,515,520,523,529,534,540,542,544,547,550,551,553,556,558,561,562,563,563,560,558,553,549,543,539,536,531,527,527,525,524,522,522,522,522,521,522,523,525,526,531,534,538,539,543,545,544,544,542,538,532,525,517,511,500,489,479,466,454,444,435,428,422,419,418,417,416,418,422,426,429,435,441,449,459,472,491,511,535,557,579,594,606,613,617,614,607,598,585,568,551,534,517,499,485,474,465,457,452,450,449,447,449,452,456,460,467,474,482,489,497,505,513,518,524,529,531,534,537,538,539,539,539,539,537,535,532,530,526,523,520,516,509,504,499,494,489,483,481,479,478,477,478,479,477,478,480,481,481,484,486,487,488,491,494,496,497,499,501,502,503,505,507,507,507,509,510,512,513,516,518,518,520,522,523,521,520,521,519,519,518,517,514,510,508,507,504,500,498,498,497,494,496,498,501,506,515,532,551,571,590,606,617,625,629,629,622,613,602,588,572,555,538,521,503,489,477,469,460,456,454,453,452,454,458,461,463,467,472,476,479,483,488,491,495,499,504,507,510,515,519,520,520,522,522,520,519,520,519,516,515,516,517,515,513,513,513,510,510,509,509,507,506,507,507,506,505,506,505,504,505,506,504,504,505,505,504,505,505,507,507,508,509,512,511,510,511,512,511,511,511,511,509,509,510,508,507,507,508,508,506,506,506,504,503,504,504,503,503,504,504,504,504,505,506,505,505,507,511,515,524,539,557,571,584,594,602,604,601,597,588,576,563,549,535,520,505,494,485,475,469,466,463,461,462,465,468,470,473,477,481,484,488,491,494,498,500,503,506,507,509,512,511,512,512,512,510,510,510,510,509,508,506,506,504,505,505,505,503,504,506,507,507,507,509,508,508,510,510,509,509,509,509,508,508,511,511,512,510,511,512,511,511,512,511,510,510,511,510,509,510,511,511,510,510,511,512,511,511,512,511,511,510,511,511,509,511,511,512,511,510,509,508,509,510,514,520,528,542,557,570,580,587,591,591,587,581,571,558,546,534,522,510,500,492,484,476,472,469,466,464,465,467,470,470,472,477,479,483,487,492,496,498,503,507,509,510,512,513,512,512,514,513,511,509,508,507,506,505,505,505,504,503,504,505,503,504,505,506,506,506,507};

//data buffer
struct dataBuffer {
  int16_t bufferID = 0; //0 for active buffer 0, 1 for active buffer 1
  int16_t dataBuffer0[2000] = {0};
  int16_t dataBuffer0Status = 0; //0 for clean, 1 for dirty buffer
  int16_t dataBuffer1[2000] = {0};
  int16_t dataBuffer1Status = 0; //0 for clean, 1 for dirty buffer
  int16_t bufferCounter = 0;
};
 
//working data container
struct workingDataContainer {
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
struct hrdataContainer {
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


 

//Initialize struct objects
struct hrdataContainer hrData;
struct workingDataContainer workingData;
struct dataBuffer dataBuffers;

/*
 * Helper Functions
 */
void signalQuality(struct workingDataContainer &workingData, struct hrdataContainer &hrData) {
  
  if (workingData.correctRejectRatio >= 99.9) {
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
 
int getMeanInt(int16_t data[], int16_t datalen) {
  
  long sum = 0;
  for(int i = 0; i < datalen; i++) {
    sum += data[i]; 
  }
  return (sum / datalen); //return mean
}

int getMeanInt32(int data[], int datalen) {
  
  long sum = 0;
  for(int i = 0; i < datalen; i++) {
    sum += data[i]; 
  }
  return (sum / datalen); //return mean
}

float getMeanFloat(int16_t data[], float datalen) {
  
  float sum = 0;
  for(int i = 0; i < datalen; i++) {
    sum += data[i];
  }
  return (sum / datalen); //return mean
}

float getMean(float data[], float datalen) {
  
  float sum = 0;
  for(int i = 0; i < datalen; i++) {
    sum += data[i];
  }
  return (sum / datalen); //return mean
}

float getStdFloat(int16_t data[], float dataLen) {
  
  float sqSum = 0.0;
  float diff = 0.0;
  float dataMean = getMeanFloat(data, dataLen);
  for (int i = 0; i < dataLen; i++) {
    diff = data[i] - dataMean;
    sqSum += (diff * diff);
  }
  return sqrt(sqSum / dataLen);
}

float getMedian(int16_t data[], int16_t datalen) { //find median of array 'data[]' with length 'datalen'
  
  float temp;
  int i, j;
  for(i=0; i<datalen-1; i++) {
      for(j=i+1; j<datalen; j++) {
          if(data[j] < data[i]) {
              temp = data[i];
              data[i] = data[j];
              data[j] = temp;
          }
      }
  }

  if(datalen%2==0) {
    return((data[datalen/2] + data[datalen/2 - 1]) / 2.0); 
    } else {
    return data[datalen/2];
  }
}


int movAvg(int *ptrArr, long *ptrSum, int pos, int windowSize, int nextNum) { 
  
  //moving average function, adapted from BMCCormack/movingAvg.c on GitHub
  *ptrSum = *ptrSum - ptrArr[pos] + nextNum;
  ptrArr[pos] = nextNum;
  return *ptrSum / windowSize;
}

void callmovAvg(float factor, struct workingDataContainer &workingData) {
  
  int arrNum[workingData.windowSize] = {0};
  int pos = 0;
  long sum = 0;
  for (int i = 0; i < workingData.datalen; i++) {
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


/*
 * Interpolation functions
 */
void interpolateRR(struct workingDataContainer &workingData) {
  float rrSum = 0.0;
  //Serial.print("\n------\n");
  for (int i = 0; i < (workingData.beatCount-workingData.rejectedBeatCount)-1 ; i++)
  {
    workingData.rrListX[i] = rrSum;
    //Serial.printf("%f, %i\n", rrSum, workingData.rrList[i]);
    if (i < ((workingData.beatCount-workingData.rejectedBeatCount)-2)) {
      rrSum += (workingData.rrList[i]/1000.0);
    }
  }
  //Serial.print("\n------\n");

  workingData.fsRR = 128.0 / rrSum;
  workingData._length = (workingData.beatCount - workingData.rejectedBeatCount)-1;
  float factor = rrSum / 128.0;
  
  for (int i = 0; i < 128; i++) {
    workingData.interpolatedRR[i] = value(factor*i, workingData);
    //Serial.printf("%f, %f\n", (factor*i), value(factor*i, workingData));
  }
  //Serial.print("\n------\n");
}

float value(float x, struct workingDataContainer &workingData) {
  if( workingData.rrListX[0] > x ) { 
    return workingData.rrList[0]; 
  }
  else if ( workingData.rrListX[workingData._length-1] < x ) { 
    return workingData.rrList[workingData._length-1]; 
  }
  else {
    for(int i = 0; i < workingData._length; i++ )
    {
      int index = ( i + workingData._prev_point ) % workingData._length;
      
      if( workingData.rrListX[index] == x ) {
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

float calc(float x, int i, struct workingDataContainer &workingData) {
  if( i == 0 ) {
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

float hermite( float t, float p0, float p1, float m0, float m1, float x0, float x1 ) {
  return (hermite_00(t)*p0) + (hermite_10(t)*(x1-x0)*m0) + (hermite_01(t)*p1) + (hermite_11(t)*(x1-x0)*m1);
}
float hermite_00( float t ) { return (2*pow(t,3)) - (3*pow(t,2)) + 1;}
float hermite_10( float t ) { return pow(t,3) - (2*pow(t,2)) + t; }
float hermite_01( float t ) { return (3*pow(t,2)) - (2*pow(t,3)); }
float hermite_11( float t ) { return pow(t,3) - pow(t,2); }

float catmull_tangent(int i) {
  if( workingData.rrListX[i+1] == workingData.rrListX[i-1] ) {
  // Avoids division by 0
  return 0;
  } else {  
  return (workingData.rrList[i+1] - workingData.rrList[i-1]) / (workingData.rrListX[i+1] - workingData.rrListX[i-1]);    
  } 
}



/*
 * Pre-processing
 */
void enhancePeaks(struct workingDataContainer &workingData) {
   
  // Function to enhance R to noise ratio by cubing, then normalising back
  int hmean = getMeanInt32(workingData.heartData, workingData.datalen);
  int minim = hmean;
  int maxim = hmean;
  int curVal = 0;
  
  for(int i = 0; i < workingData.datalen; i++) {
    curVal = pow(workingData.heartData[i], 2);
    workingData.heartData[i] = curVal;
    if (curVal < minim) minim = curVal;
    if (curVal > maxim) maxim = curVal;
  } //Cube signal and determine min, max for scaling
  
  for (int i = 0; i < workingData.datalen; i++) { 
    workingData.heartData[i] = map(workingData.heartData[i], minim, maxim, 1, 1024);
  } //Scale back to between 1 and 1024 using map function
}


/*
 * Peak detection and fitting functions
 */
void detectPeaks(struct workingDataContainer &workingData) {
  
  workingData.beatCount = 0;
  int listPos = 0;
  int beatStart = 0;
  int beatEnd = 0;
  int curmax = 0;
  int beatPos = 0;
  
  for (int i = 0; i < workingData.datalen; i++) {
    if ((workingData.heartData[i] <= workingData.heartmovAvg[i]) && (beatStart == 0)) {
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
        for (int i = beatStart; i < beatEnd; i++) {
          if (workingData.heartData[i] > curmax) {
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
      } else {
        //edit this before release
        Serial.print(F("Encountered unknown condition in detectPeaks\n"));
        Serial.printf("i: %i, heartData: %i, movavg: %i\n", i, workingData.heartData[i], workingData.heartmovAvg[i]);
      }
  }
}

float fitPeaks(struct workingDataContainer &workingData) {
  
  float tstart = micros();
  int fittingLowestPos = 0;
  float lowestStdev = 1e20; //some large number
  float rrStdev = 0.0;
  float rrMean = 0.0;
  float estimatedBPM = 0.0;
  
  for (int i=0; i < workingData.mavgFactorsLen; i++) {
    callmovAvg(workingData.mavgFactors[i], workingData);//raise moving average
    detectPeaks(workingData); //do the peak detection
    calcRR(workingData);
    rrMean = getMeanFloat(workingData.rrList, float(workingData.beatCount-1));
    estimatedBPM = 60000.0 / rrMean;
    //updateMeasures(workingData, hrData);
    //calcMeasures(workingData, hrData);
    rrStdev = getStdFloat(workingData.rrList, float(workingData.beatCount-1));
    if ((rrStdev < lowestStdev) && (rrStdev > 0.001) && (estimatedBPM >= workingData.bpmMin) && (estimatedBPM <= workingData.bpmMax)) {
      lowestStdev = rrStdev;
      fittingLowestPos = i;
    }
  }
  
  Serial.printf("finished peak fitting with %i levels in %.2f miliseconds. Best fit was: %.2f\n", 
                workingData.mavgFactorsLen, (micros()-tstart)/1000.0, workingData.mavgFactors[fittingLowestPos]);
  return workingData.mavgFactors[fittingLowestPos];
}

int rejectPeaks(struct workingDataContainer &workingData) {

  workingData.lastRejectedRR = 0;
  workingData.rejectedBeatCount = 0;
  for (int i = 0; i < workingData.beatCount-1; i++) {
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

  for(int i = 0; i < (workingData.beatCount-1); i++) {
    //Serial.printf("Beat %i, with rrvalue %i, is: ", i, workingData.rrList[i]);
    if ((workingData.rrList[i] >= workingData.bpmMinRRdist) && (workingData.rrList[i] <= workingData.bpmMaxRRdist) && (abs(workingData.rrList[i]-rrMedian) <= 250)) {
      if ((i - workingData.lastRejectedRR == 1) && (i > 1)) {
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

  if ((workingData.beatListBin[0] == 1) && (workingData.beatListBin[1] == 0)) {
    workingData.beatListBin[0] = 0;
    workingData.rejectedBeatCount++;
  }
 
  //evaluate state of second-to-last beat and decide if the last beat should be rejected or accepted
  if (workingData.beatListBin[workingData.beatCount-2] == 1) {
    workingData.beatListBin[workingData.beatCount-1] = 1;
    //Serial.print("last beat is accepted\n");  
    } else {
      workingData.beatListBin[workingData.beatCount-1] = 0;
      //Serial.printf("last beat is rejected\n");
    } 
  
  Serial.print("Binary beatlist: ");
  for (int i = 0; i < workingData.beatCount; i++) {
    Serial.printf("%i, ", workingData.beatListBin[i]);
  }
  Serial.print("\n");

  int j = 0;
  for (int i = 0; i < workingData.beatCount-1; i++) {
    //loop over binary peaklist
    if ((workingData.beatListBin[i] == 1) && (workingData.beatListBin[i+1] == 1)) {
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


/*
 * Analysis
 */
void calcRR(struct workingDataContainer &workingData) {
  
  float rrInterval = 0;
  for (int i = 0; i < (workingData.beatCount - 1); i++) {
    rrInterval = workingData.beatList[i+1] - workingData.beatList[i];
    workingData.rrList[i] = ((rrInterval / workingData.fs) * 1000.0);
    //Serial.printf("RR interval: %f\n", ((rrInterval / workingData.fs) * 1000.0));
  }
}

void calcRRdiff(struct workingDataContainer &workingData) {
  
  for (int i = 0; i < (workingData.beatCount - 2); i++) {
    workingData.rrDiffList[i] = abs(workingData.rrList[i+1] - workingData.rrList[i]);
  }
}

void calcRRsqdiff(struct workingDataContainer &workingData) {
  
  for (int i = 0; i < (workingData.beatCount - 2); i++) {
    workingData.rrDiffsqList[i] = workingData.rrDiffList[i] * workingData.rrDiffList[i];
  }
}

void updateMeasures(struct workingDataContainer &workingData, struct hrdataContainer &hrData) {
  float rrmean = getMeanFloat(workingData.rrList, float(workingData.beatCount-1));
  int nn20 = 0;
  int nn50 = 0;
  for(int i = 0; i < (workingData.beatCount-2); i++) { 
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
  
  if (hrData.firstRun == 1) {
    for (int i = 1; i < 3; i++) {
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

void FFT_rrList(struct workingDataContainer &workingData) {
  for (int i = 0; i < workingData.rrInterpLen; i++) {
    workingData.vReal[i] = workingData.interpolatedRR[i]; //Append heartrate data to FFT array
    workingData.vImag[i] = 0.0; //Zero imaginary array to prevent issues from previous FFT  
  }
  //Do the FFT
  FFT.Compute(workingData.vReal, workingData.vImag, workingData.rrInterpLen,
              FFT_FORWARD);
  FFT.ComplexToMagnitude(workingData.vReal, workingData.vImag, workingData.rrInterpLen);
  workingData.vReal[0] = 0; //prevent the ultra-low-frequency from leaking into signal
}

void calcFreqMeasures(struct workingDataContainer &workingData, struct hrdataContainer &hrData) {
  
  hrData.LF_ = 0.0;
  hrData.HF_ = 0.0;

  //Use squared Fourier Transform to express spectral power
  //Note: Replace with 
  for (int i = 0; i < 64; i++) {
    workingData.vReal[i] = workingData.vReal[i] * workingData.vReal[i];
  }

  //Apply a Riemann Sum with trapezoidal rule to find area under FFT function
  float tStep = ((workingData.fsRR) / workingData.rrInterpLen);
  for (int i = 0; i < (workingData.rrInterpLen >> 1); i++) {
    if (((tStep * i) >= 0.04) && ((tStep * i) <= 0.15)) {
      hrData.LF_ += ((0.5 * tStep) * (workingData.vReal[i] + workingData.vReal[i+1]));
    } else if (((tStep * i) >= 0.16) && ((tStep * i) <= 0.5)) {
      hrData.HF_ += ((0.5 * tStep) * (workingData.vReal[i] + workingData.vReal[i+1]));
    } else if ((tStep * i) >= 0.6) break; //break soon after last used frequency bin is passed
  }
  hrData.LFHF_ = hrData.LF_ / hrData.HF_;
  calcBreath(workingData, hrData, tStep);
}


//The last thing remaining is to integrate the area under curve at the LF (0.04 – 0.15Hz) and HF (0.16 – 0.5Hz) frequency bands.





void calcBreath(struct workingDataContainer &workingData, struct hrdataContainer &hrData, float tStep) {
  float maxPos = 0;
  float curMax = 0;
  for (int i = 0; i < (workingData.rrInterpLen >> 1); i++) {
    if (workingData.vReal[i] > curMax) {
      maxPos = i;
      curMax = workingData.vReal[i];
    }
  }
  hrData.breath_ = (maxPos * tStep);
}

/*
 * Data logging functions
 */
void readSensor()
{
  if (dataBuffers.bufferID == 0) {
    dataBuffers.dataBuffer0[dataBuffers.bufferCounter] = analogRead(hrpin);
    dataBuffers.bufferCounter++;
  } else {
    dataBuffers.dataBuffer1[dataBuffers.bufferCounter] = analogRead(hrpin);
    dataBuffers.bufferCounter++;
  }
  Serial.println(dataBuffers.bufferCounter);
}

void flushBuffer(int16_t data[], struct workingDataContainer &workingData) {
  for (int i = 0; i < workingData.datalen; i++) {
    workingData.heartData[i] = data[i];
  }
}

void switchBuffers(struct dataBuffer &dataBuffers, struct workingDataContainer &workingData) {
  dataBuffers.bufferCounter = 0;
  if (dataBuffers.bufferID == 0) {
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


/*
 * Main functions
 */
void preProcessor(struct workingDataContainer &workingdata)
{
  //enhance peaks several times to accentuate the R-peak in the QRS complex
  enhancePeaks(workingData);
  enhancePeaks(workingData);
  enhancePeaks(workingData);
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

void processData(struct workingDataContainer &workingData, struct hrdataContainer &hrData) {
  float totalt1 = micros();
  //readSensor(workingData);
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

  /*for (int i = 0; i < workingData.datalen; i++){
    Serial.printf("%i,%i\n", workingData.heartData[i], workingData.heartmovAvg[i]);
    delay(2);
  }*/

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
  /*Serial.print("bpm buffer: ");
  for (int i = 0; i < 3; i++) {
    Serial.printf("%f,", hrData.bpm[i]);
  }*/

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
  stopWorking();
   
}

 //temp function
void stopWorking() {
  Serial.print("Stopped working!");
  digitalWrite(led, HIGH);
  while(1==1)
  {
    if(Serial.available() >= 1){
      Serial.read();
      break;
    }
    delay(500);
  }
}


void setup() 
{
  pinMode(led, OUTPUT);
  Serial.begin(250000);
  delay(200);
  sensorTimer.begin(readSensor, 10000);
}

void loop() 
{
  if (dataBuffers.bufferCounter > 1999) { 
    sensorTimer.end();
    switchBuffers(dataBuffers, workingData);
    processData(workingData, hrData);
    //flushResults(hrData, processedData);
  } //if 2000 items have been stuffed into buffer, switch buffer and process data

  if (Serial.available() > 0) {
    Serial.read();
    stopWorking();
  } //temp function to stop output
  
}
