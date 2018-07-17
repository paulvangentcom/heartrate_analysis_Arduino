.. _algorithm functioning:

*********************
Algorithm functioning
*********************

This section describes the details of the algorithm functionality. 

Pre-processing
==============


Peak detection
==============
<..>

When comparing the AVR implementations to the Teensy or Python implementations, the first thing to note is that there is not adaptive moving average in the AVR version. The main reason is that AVR chips have very limited RAM, so the use of buffers required for the adaptive moving average is not feasible. In stead, the AVR implmentation implements adaptive signal scaling. The adaptive scaling funcions in periods of 2 seconds. At the end of each 2-second period the signal minima and maxima are determined over the preceding period. The signal in the next period is scaled using these values. This accomodates changes in signal amplitude. In the PPG signal these changes occur at low frequency, due to changes in for example blood pressure or vasoconstriction in response to external stressors. Adaptive scaling also accomodates instances where reduced blood perfusion leads to a lower signal amplitude baseline, such as in the elderly or long-term smokers.

While not as powerful as the adaptive moving average used in the other implementations, the adaptive signal scaling is still quite powerful as shown on the figure below:

.. image:: images/AVR_PeakDetection_source.jpg

*Image showing result of the real-time AVR implementation in different conditions. In the first part (I.), the signal is measured at the fingertip. A period of sensor disconnect occurs (II.) while moving the sensor to a new measurement location. In the final segment (III.) the signal is measured on the cheek, where signal amplitude is much weaker. The adaptive scaling kicks in after the first two low-amplitude beats (IV.) to stabilize amplitude and allow further analysis.*


Peak rejection
==============

  




References
==========
