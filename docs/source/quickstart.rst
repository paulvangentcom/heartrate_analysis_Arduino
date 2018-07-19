.. _quickstart:

****************
Quickstart Guide
****************

Where to begin?
===============
If you find yourself here, chances are you want to use the developed toolkit in your research or some other open-source application. Great! On this page we will describe the options available to you. If you already know what you want please go to the :ref:`implementations` section.

Otherwise, look at the statements below and click whichever one is closest to your situation:

- **I have recorded heart rate data and want to analyse it.**
- **I have a way of recording heart rate data, and just want to analyse the recorded data.**
- **I just want to record heart rate data unintrusively, I have my own analysis tools.**
- **I want to record heart rate data unintrusively, it's ok if the analysis is done later (offline).**
- **I want to recorde heart rate data and analyse the results real-time.**




Coding Structure
================

Each Arduino implementation has a similar coding structure to help you get started as quickly as possible. Typically at the top of the file you'll find the license and reference to cite if you use the code in your scientific project. Below this you'll find the user settable variables. These are discussed on the page corresponding to the implementation. The link is provided in the file as well as here on the documentation. It will look something like this:

.. image:: images/UserSettable.jpg

Important is that you only change the variables in the  **"User Settable Variables"** section. Changing any of the other variables will likely lead to compilation errors or unexpected behaviours. Comments in the code will explain what each settable variable does, and this documentation will provide further reference.


Basic Example
=============
