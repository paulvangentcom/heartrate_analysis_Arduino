.. _implementations:


***************
Implementations
***************
Several implementations are available, depending on the goal you want to achieve. This page describes them in detail. If you're unsure which to pick, take a look at the :ref:`quickstart`.

Each implementation may be available for different chipsets. The requirements are always mentioned with each implementation. If you don't know what you have, take a look at the section "What board do I have?" in the quickstart to get help with that.

have different options and characteristics. This section describes them as best as possible. The implementations are split into AVR (Arduino, and other 8-bit Atmel chipsets), and ARM (Teensy and other boards using ARM (Cortex) or other 32-bit chipsets).

---------------------------

.. _simplelogger: 

Simple Logger
=============
The simple logger implementation functions as a basic data logging device with highly precise timing. It utilizes hardware interrupt timers so that the chosen sampling rate is reliably maintained throughout the logging process. This differs from for example logging solutions using linux-boards (such as Raspberry Pi), pc-based logging systems, or indeed many Arduino versions out there, where timing accuracy is often not ensured. `See here for more information on hardware timers <http://www.paulvangent.com/2018/03/28/hardware-interrupts-are-not-that-scary/>`_.

+-------------+-------------+-----------------------------------------------------+
| Board type  | Available?  | Notes                                               |
+=============+=============+=====================================================+
| Arduino     | Yes         | All except ATTiny based                             |
+-------------+-------------+-----------------------------------------------------+
| Teensy      | Yes         | All versions                                        |
+-------------+-------------+-----------------------------------------------------+
| Other       | Yes         | | Requires >700 bytes RAM                           |
|             |             | | SD version requires 512 bytes extra for buffering |
|             |             | | Sampling rate dependent on chip speed             |
|             |             | | Tested up to 2KHz on 16MHz 328p                   |
+-------------+-------------+-----------------------------------------------------+


Settable options available in the code:

.. code-block:: C

    // -------------------- User Settable Variables --------------------
    int8_t hrpin = 0; //Whatever analog pin the sensor is hooked up to
    int8_t scale_data = 1; /*Uses dynamic scaling of data when set to 1, not if set to 0 \
                           sampling speed over 1500Hz not recommended when scaling data 
                           on 8-bit AVR (e.g. Arduino)*/
    int8_t mode = 0; /*Speed mode. \
                  0 means the "sample_rate" speed will be used \
                  1 means custom. Custom sampling rate is set through Serial after connect.\
                  See documentation for details.  */
    int16_t sample_rate = 1500; /*should be 4Hz or more, over 2KHz on AVR not recommended.
                               When using adaptive scaling: over 1.5KHz not recommended on AVR.
                               Higher speeds attainable on 32-bit chipsets.
                               Please see documentation for suggested limits and theoretical limits*/       

- **hrpin**: the pin you connected the sensor to. By default it is set to 0, meaning Analog-0 (often called A0 on the board pinout).
- **scale_data**: Whether to use adaptive scaling, see :ref:`scaling`. Set to '1' to enable adaptive scaling, set to '0' to disable.
- **mode**: the logging mode, indicating if you want a predefined sampling rate, or want to set it at boot. If set to '0', whatever value is set in "sample_rate" will be adhered to.
- **sample_rate**: the sample rate to adhere to when mode is set to 0.


USB version
^^^^^^^^^^^

The **USB logger** starts when a serial connection is made to the device. It is meant to be used in connection with a computer to log heart rate. There is an example Python file supplied that shows how to do so using :code:`PySerial`. When set to mode 6, once a serial connection is established the logger will request a logging speed and wait for a reply, otherwise it will just start logging once a serial connection is made.


SD Version
^^^^^^^^^^

The **SD logger** starts as soon as power is applied to it. If no SD card is present or there is an error writing to the card, the default board light (pin 13) turns on and stays on. It flashes while writing data. "mode" is not available on SD version.

---------------------------

.. _peakfinder:

Peak Finder
===========
The Peak Finder implementation logs heart rate data, analysis it real-time to identify peaks, and returns the peak positions + RR-intervals. It can also be set to output the raw signal as well. On 8-bit AVR implementations it's limited to 100Hz (mostly due to limitations on RAM used for buffering). It uses adaptive scaling and error correction described in :ref:`algorithm functioning`.

+-------------+-------------+-----------------------------------------------------+
| Board type  | Available?  | Notes                                               |
+=============+=============+=====================================================+
| Arduino     | Yes         | All except ATTiny based                             |
+-------------+-------------+-----------------------------------------------------+
| Teensy      | Yes         | | All versions, implementation with                 |
|             |             | | settable sampling rate coming soon                |
+-------------+-------------+-----------------------------------------------------+
| Other       | Yes         | | Requires >900 bytes RAM                           |
|             |             | | SD version requires 512 bytes extra for buffering |
|             |             | | Sampling rate dependent on chip speed             |
+-------------+-------------+-----------------------------------------------------+


.. code-block:: C

    // -------------------- User Settable Variables --------------------
    int8_t hrpin = 0; //Whatever analog pin the sensor is hooked up to
    int8_t report_hr = 1; //if 1, reports raw heart rate and peak threshold data as well, else set to 0 (default 0)
    float max_bpm = 180; //The max BPM to be expected, used in error detection (default 180)
    float min_bpm = 45; //The min BPM to be expected, used in error detection (default 45)


- **hrpin**: the pin you connected the sensor to. By default it is set to 0, meaning Analog-0 (often called A0 on the board pinout).
- **report_hr**: Set this to '1' to have the logger also output the raw heart rate signal and moving average.
- **max_bpm**: The maximum BPM to expect, used as a first estimation of peak position accuracy.
- **min_bpm**: The minimum BPM to expect, used as a first estimation of peak position accuracy.

USB version
^^^^^^^^^^^

The **USB logger** AVR starts when a serial connection is made to the device (The ARM version starts when power is applied regardless of serial status). It is meant to be used in connection with a computer to log peak positions and RR-intervals (and raw heart rate if set to output). There is an example Python file supplied that shows how to do so using :code:`PySerial`. The peak finder runs at a fixed 100Hz rate. The next update will introduce settable sampling rate


SD Version
^^^^^^^^^^

The **SD logger** starts as soon as power is applied to it. If no SD card is present or there is an error writing to the card, the default board light (pin 13) turns on and stays on. It flashes while writing data.

---------------------------

.. _timeseriesanalysis:

Time Series Analysis
====================
This implementation is a basic heart rate analysis toolkit for both AVR and ARM chipsets. It functions like the peak detector, but will also output the described under :ref:`timeseries` every beat.

By default it will output only RR-interval of the last two peaks, and the absolute position in samples-since-start of the last detected peak.

Sample rate will be made settable in the next update.

.. code-block:: C

    // -------------------- User Settable Variables --------------------
    int8_t hrpin = 0; //Whatever analog pin the sensor is hooked up to
    int8_t Verbose = 0; //Whether to report measures + description (1) or just measures (0); See docs.
    int8_t report_hr = 0; //if 1, reports raw heart rate and peak threshold data as well, else set to 0 (default 0)
    int8_t thresholding = 0; //Whether to use thresholding, can cause incorrect rejections in conditions of high variability
    float max_bpm = 180; //The max BPM to be expected, used in error detection (default 180)
    float min_bpm = 45; //The min BPM to be expected, used in error detection (default 45)

    
- **hrpin**: the pin you connected the sensor to. By default it is set to 0, meaning Analog-0 (often called A0 on the board pinout).
- **Verbose**: If set to 0, variables are output in CSV format, a descriptive output is given including the variable names. 

    - CSV format = "bpm,ibi,sdnn,sdsd,rmssd,pnn20,pnn50"
    - Verbose looks like this:

.. code-block:: C

    1090,2679 //first is RR-value, second is peak position in samples-since-start
    bpm: 66.91
    ibi: 896.67
    sdnn: 87.69
    sdsd: 55.75
    rmssd: 96.69
    pnn20: 0.85
    pnn50: 0.65

**Note** that the SD logger does not have the :code:`Verbose` option.
   
- **report_hr**: Set this to '1' to have the logger also output the raw heart rate signal and moving average.
- **max_bpm**: The maximum BPM to expect, used as a first estimation of peak position accuracy.
- **min_bpm**: The minimum BPM to expect, used as a first estimation of peak position accuracy.

    
+-------------+-------------+-----------------------------------------------------+
| Board type  | Available?  | Notes                                               |
+=============+=============+=====================================================+
| Arduino     | Yes         | All Except ATTiny based                             |
+-------------+-------------+-----------------------------------------------------+
| Teensy      | Yes         | All versions                                        |
+-------------+-------------+-----------------------------------------------------+
| Other       | Yes         | | Requires >1050 bytes of RAM                       |
|             |             | | SD version requires 512 bytes extra for buffering |
|             |             | | Sampling rate fixed @100Hz for now                |
+-------------+-------------+-----------------------------------------------------+

USB version
^^^^^^^^^^^

The **USB logger** AVR starts when a serial connection is made to the device (The ARM version starts when power is applied regardless of serial status). It is meant to be used in connection with a computer. There is an example Python file supplied that shows how to do so using :code:`PySerial`. The peak finder runs at a fixed 100Hz rate. The next update will introduce settable sampling rate


SD Version
^^^^^^^^^^

The **SD logger** starts as soon as power is applied to it. If no SD card is present or there is an error writing to the card, the default board light (pin 13) turns on and stays on. It flashes while writing data.


.. _fullanalysis:

Full Implementation
===================
This implementation mirrors the full Python implementation on a Teensy (ARM Cortex-based) board and makes it real-time. The logger collects 20 seconds of heart rate data, and at the end of each measurement period outputs both the time-serie and frequency-series heart rate measures.

For now the sampling rate is fixed at 100Hz. An update is being worked on that will make it settable. The Frequency Measures that are output rely on a squared FFT to estimate the periodogram, which is not a good estimator. It gives an indication, but I would **not recommend** using the frequency measures for scientific use yet. In a future version Welch's method will be implemented.

+-------------+-------------+-----------------------------------------------------+
| Board type  | Available?  | Notes                                               |
+=============+=============+=====================================================+
| Arduino     | No          | Amount of RAM too limited for required buffers      |
+-------------+-------------+-----------------------------------------------------+
| Teensy      | Yes         | | All ARM-based versions except Teensy LC,          |
|             |             | | meaning 3.1, 3.2, 3.5, 3.6                        |
+-------------+-------------+-----------------------------------------------------+
| Other       | Yes         | | Requires >30 Kilobytes of RAM                     |
|             |             | | SD version requires 512 bytes extra for buffering |
|             |             | | Sampling rate fixed @100Hz for now                |
+-------------+-------------+-----------------------------------------------------+

USB version
^^^^^^^^^^^

The **USB logger** starts when power is applied regardless of serial status. It is meant to be used in connection with a computer. There is an example Python file supplied that shows how to do so using :code:`PySerial`. The analysis suite runs at a fixed 100Hz rate. A future update will introduce settable sampling rate


SD Version
^^^^^^^^^^

The **SD logger** starts as soon as power is applied to it. If no SD card is present or there is an error writing to the card, the default board light (pin 13) turns on and stays on. It flashes while writing data.
