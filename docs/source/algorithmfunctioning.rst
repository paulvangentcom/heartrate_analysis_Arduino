.. _algorithm functioning:

*********************
Algorithm functioning
*********************

This section describes the details of the algorithm functionality. 

Pre-processing
==============
Depending on which implementation you select, several pre-processing steps can be applied.

Adaptive Input Amplitude Scaling
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
This feature is available on all implementations. It can be turned on by setting the flag "adaptivescale" in the "user settable variables" section to 0. The 8-bit AVR boards (Arduino) will be restricted in sampling rate when using autoscaling, since it relies on 16- and 32-bit computations, which are quite slow on 8-bit systems. The ARM (Teensy, 32-bit) boards can handle high sampling rates without issue.

The adaptive scaling funcions in periods of 2 seconds. At the end of each 2-second period the signal minimum and maximum are determined over the preceding period. The signal in the next period is scaled using these values. This accomodates changes in signal amplitude. In the PPG signal, amplitude changes like this occur at low frequency due to changes in for example blood pressure, or vasoconstriction in response to (internal or external) stressors. Adaptive scaling also accomodates instances where reduced blood perfusion leads to a lower signal amplitude baseline, such as in the elderly or long-term smokers.

While not as powerful as the adaptive moving average used in the other implementations, the adaptive signal scaling is still quite powerful as shown on the figure below:

.. image:: images/AVR_PeakDetection_source.jpg

*Image showing result of the real-time AVR implementation in different conditions. In the first part (I.), the signal is measured at the fingertip. A period of sensor disconnect occurs (II.) while moving the sensor to a new measurement location. In the final segment (III.) the signal is measured on the cheek, where signal amplitude is much weaker. The adaptive scaling kicks in after the first two low-amplitude beats (IV.) to stabilize amplitude and allow further analysis.*

The following image shows a signal recorded at the top of the wrist, a location where typically PPG sensors record a very low amplitude signal. Two recordings are displayed below, one made without adaptive scaling (top) and one with (bottom).

.. image:: images/wrist_scaling.jpeg

As you can see it makes quite a difference. Keep in mind that it is not a magic bullet: noise will scale as well, meaning that the lower the amplitude of the original signal, generally the more noise you'll find in the scaled recording.






Peak detection & Error Correction
=================================
Two types of peak detection are available, one real-time using adaptive scaling, and one using an adaptive threshold similar to the Python implementation. The latter one is only available on ARM (Teensy) boards due to RAM demands made by the required buffers.

Adaptive Scaling
~~~~~~~~~~~~~~~~
When comparing the AVR implementations to the full ARM or Python implementations, the first thing to note is that there is not adaptive moving average in the AVR version. The main reason is that AVR chips have very limited RAM, so the use of buffers required for the adaptive moving average is not feasible. In stead, the AVR implmentation implements adaptive signal scaling as discussed above. 

In the case of adaptive scaling, peak detection functions with a fixed moving average that is computed on-the-fly for each datapoint read from the sensor. Whenever the signal exceeds the moving average, it is stored in a "region of interest" buffer until it dips below the moving average again. The peak is then identified in the region of interest and marked. 

Error-detection functions in three stages:

1. The RR-interval with the previously marked peak is computed and evaluated whether it falls into to the expected BPM range (settable under "user settable variables")
2. The RR-interval is compared to the previous RR-interval. It is not allowed to deviate more than 350ms, otherwise it is ignored.
3. Thresholds are computed based on the mean of the last 20 RR-intervals. Thresholds are determined as **RR_mean +/- (30% of RR_mean, with minimum value of 300)** (+ or - for upper and lower threshold, respectively). If the RR-interval exceeds one of the thresholds, it is ignored.


Adaptive Threshold
~~~~~~~~~~~~~~~~~~


  
