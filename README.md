# Spectogram Dataset Recorder
Simple GUI tool written in Qt to record sound from microphone input or speakers that exports it as spectogram either as Mel Scale Filter Banks or Mel Frequency Cepstral Coefficients. This program was created to create dataset for sound event detection. I relied on [this article](https://haythamfayek.com/2016/04/21/speech-processing-for-machine-learning.html) to create this tool.

## TODO
- [x] implement function to compute MFCC from given MSFB (currently only MSFB is computed even if MFCC is selected).

## Features
* Easy to use
* Allows fine tuning of conversion parameters
* Allows batch recording one after another
* Exports to four different formats: plain text (.txt), numpy array (.npy), color image (.jpg), grayscale image (.jpg)
* Saves samples under "path\to\dataset_root\class_name"

## Compiling
I have compiled it using Qt Creator 4.9.2, Qt 5.15.0 and MSVC19 64 bit. zlib 64 bit is required (I have compiled [this](https://github.com/kiyolee/zlib-win-build) and works flawlessly). To compile, change INCLUDEPATH and LIBS in .pro file to correct path to zlib. 

## Issues
For some reason, during batch recording, the recording can "hang" for some seconds. This is not really an issue as during recording only n first samples are used and the rest is discarded.