# README #

This project is an extension of Yuriy Chesnokov's code from the 2006 PhysioNet Computers in Cardiology challenge.  It was designed to measure QT intervals in ECG recordings, but is also able to annotate and measure other intervals such as RR, PR, etc.

The code was downloaded and merged from the following two sources:

* https://www.physionet.org/challenge/2006/sources/yuriy-chesnokov/
* http://www.codeproject.com/Articles/20995/ECG-Annotation-C-Library

It was originally very Windows-specific, and I have been working to make it more portable.  It should currently compile and run in Linux and Mac OS, but I will now have to go the other way and get it working in Windows again.

ISHNE file support has also been added.

### How do I run it? ###

A simple Makefile is included which builds `ecg_ann`.  The `filters` directory must always accompany this binary.

There is an example PhysioNet recording in `data/`.  To annotate the first lead from s0010_re.dat: `./ecg_ann s0010_re.dat`.  Or, explicitly specifying the lead: `./ecg_ann s0010_re.dat 1`.  Note that PhysioNet .dat files must also include the .hea header file.

The output of the ecg.cpp program is the list of annotations and the mean heart rate.  Annotations and heart rate information are also saved to disk as .atr and .hrv files alongside the original recording.

You may want to edit the parameters (such as `maxQT`) in `ecgannotation.cpp`.  You can also edit them case-by-case in a text file, and point to the file with a command-line argument; see `parse_params()` in `ecg.cpp`.

### Who do I talk to? ###

* Current maintainer: Alex Page, alex.page@rochester.edu
* Original author: Yuriy Chesnokov