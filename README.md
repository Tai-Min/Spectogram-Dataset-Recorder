# Spectogram Dataset Recorder
Simple GUI tool written in Qt to record sound from microphone input or speakers and export it as spectogram either as Mel Scale Filter Banks or Mel Frequency Cepstral Coefficients. This program was written to create dataset for sound event detection. I relied on [this blog post](https://haythamfayek.com/2016/04/21/speech-processing-for-machine-learning.html) to create conversion routine.

![alt text](https://github.com/Tai-Min/Spectogram-Dataset-Recorder/blob/master/recorder.jpg "Recorder image")

## Features
* Easy to use
* Lightweigh
* Allows fine tuning of conversion parameters
* Allows batch recording one after another
* Exports to four different formats: plain text (.txt), numpy array (.npy), color image (.jpg), grayscale image (.jpg)
* Saves samples under "path\to\dataset_root\class_name"
* Does not overwrite previous recordings
* audioprocessor.h written in pure STL so it can be reused in any C++ project to convert audio/pcm data to spectogram

## Compiling
I have compiled it using Qt Creator 4.9.2, Qt 5.15.0 and MSVC19 64 bit. zlib 64 bit is required (I have compiled [this](https://github.com/kiyolee/zlib-win-build) and works flawlessly). To compile, change INCLUDEPATH and LIBS in .pro file to correct path to zlib. 
