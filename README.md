# README #

This project is an extension of Yuriy Chesnokov's code from the 2006 PhysioNet Computers in Cardiology challenge.  It was designed to measure QT intervals in ECG recordings, but is also able to annotate and measure other intervals such as RR, PR, etc.

The code was downloaded and merged from the following two sources:

* https://www.physionet.org/challenge/2006/sources/yuriy-chesnokov/
* http://www.codeproject.com/Articles/20995/ECG-Annotation-C-Library

It was originally very Windows-specific, and I have been working to make it more portable.  It should currently compile and run in Linux, but I will now have to go the other way and get it working in Windows again.

ISHNE file support has also been added.

### How do I get set up (TODO)? ###

* Summary of set up
* Configuration
* Dependencies
* Database configuration
* How to run tests
* Deployment instructions

### How do I run it? ###

To annotate the first lead from s0010_re.dat:
    `ecg.exe s0010_re.dat`.
Or, explicitly specifying the lead:
    `ecg.exe s0010_re.dat 1`.

The output of the ecg.cpp program is (TODO).  Note that PhysioNet .dat files must also include the .hea header file.

### Who do I talk to? ###

* Current maintainer: Alex Page, alex.page@rochester.edu
* Original author: Yuriy Chesnokov