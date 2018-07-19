.. _implementations:


***************
Implementations
***************
Several implementations are available, depending on the goal you want to achieve. This page describes them in detail. If you're unsure which to pick, take a look at the :ref:`quickstart`.

Each implementation may be available for different chipsets. The requirements are always mentioned with each implementation. If you don't know what you have, take a look at the section "What board do I have?" in the quickstart to get help with that.

have different options and characteristics. This section describes them as best as possible. The implementations are split into AVR (Arduino, and other 8-bit Atmel chipsets), and ARM (Teensy and other boards using ARM (Cortex) or other 32-bit chipsets).


Simple Logger
=============
The simple logger implementation functions as a basic data logging device with highly precise timing. It utilizes hardware interrupt timers so that the chosen sampling rate is reliably maintained throughout the logging process. This differs from for example logging solutions using linux-boards (such as Raspberry Pi) or pc-based logging systems, where timing accuracy is often not ensured.

+-------------+-------------+-----------------------------------------+
| Board type  | Functional? | Notes                                   |
+=============+=============+=========================================+
| Arduino     | Yes         | All except ATTiny based                 |
+-------------+-------------+-----------------------------------------+
| Teensy      | Yes         | All versions                            |
+-------------+-------------+-----------------------------------------+
| Other       | Yes         | | Requires >700bytes RAM                |
|             |             | | Sampling rate dependent on chip speed |
|             |             | | Tested up to 2KHz on 16MHz 328p       |
+-------------+-------------+-----------------------------------------+


Settable options available in the code:

.. code-block:: C

    // -------------------- User Settable Variables --------------------
    int8_t hrpin = 0; //Whatever analog pin the sensor is hooked up to
    int8_t mode = 6; /*Speed mode. \
                     0 for 100Hz, 1 for 200Hz, 2 for 250Hz, \
                     3 for 500Hz, 4 for 1000Hz 5 for 2000Hz, \
                     and 6 for custom. Custom mode is set through Serial.\
                     See documentation for details.*/         

- **hrpin**: the pin you connected the sensor to. By default it is set to 0, meaning Analog-0 (often called A0 on the board pinout).
- **mode**: the logging mode, indicating what speed you want the logger to run at. modes 0-5 specify pre-set speeds ranging from 100Hz to 2KHz. Mode 6 indicates a user-settable mode. 

Two versions are available, a **USB-version** and a **SD-version**. The first is connected to a PC using a USB cable. The second can funcion as a stand-alone with an SD-card. **See here how to hook up an SD adapter.**

The **USB logger** starts when a serial connection is made to the device. There is an example Python file supplied that does so using :code:`PySerial`. When set to mode 6, once a seriak connection is established the logger will request a logging speed and wait for a reply.

The **SD logger** starts as soon as power is applied to it. If no SD card is present or there is an error writing to the card, the default board light (pin 13) turns on. It flashes when writing data. Custom mode 6 is not available on the SD logger.