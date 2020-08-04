#include "audioprocessor.h"

#include <iostream>
using namespace std;

bool AudioProcessor::validateConfig() const {
    if(conf.bytesPerSample == 0 || conf.bytesPerSample > 2)
        return 0;
    if(conf.numberOfChannels == 0)
        return 0;
    if(conf.sampleRate == 0)
        return 0;
    if(conf.framingSize == 0)
        return 0;
    if(conf.framingStride == 0)
        return 0;
    if(conf.NFFT == 0)
        return 0;
    if(conf.numberOfFilterBanks == 0)
        return 0;
    return 1;
}

void AudioProcessor::transposeMatrix(vec2d & v){
    vec2d res(v[0].size(), vec(v.size(),0));

    for(unsigned int i = 0; i < res.size(); i++){
        for(unsigned int j = 0; j < res[i].size(); j++){
            res[i][j] = v[j][i];
        }
    }

    v = std::move(res);
}

void AudioProcessor::dotMatrix(vec2d & first, const vec2d & second) {
    vec2d res(first.size(), vec(second[0].size(), 0));

    // for each row in first array
    for(unsigned int i = 0; i < first.size(); i++){
        // for each column in second array
        for(unsigned int j = 0; j < second[0].size(); j++){
            long double sum = 0;
            // sum each element
            for(unsigned int k = 0; k < first[i].size(); k++){
                 sum += first[i][k] * second[k][j];
            }
            res[i][j] = sum;
        }
    }
    first.clear();
    first = std::move(res);
}

void AudioProcessor::subtractMatrix(vec2d & first, const vec & second){
    for(unsigned int i = 0; i < first.size(); i++){
        for(unsigned int j = 0; j < first[i].size(); j++){
            first[i][j] -= second[j];
        }
    }
}

void AudioProcessor::stabilizeMatrix(vec2d & v){
    for(unsigned int i = 0; i < v.size(); i++){
        for(unsigned int j = 0; j < v[i].size(); j++){
            if(v[i][j] == 0)
                v[i][j] = std::numeric_limits<long double>::epsilon();
        }
    }
}

auto AudioProcessor::meansMatrix(const vec2d & v) -> vec {
    vec result(v[0].size(), 0);
    for(unsigned int i = 0; i < v[0].size(); i++){
        long double mean = 0;
        for(unsigned int j = 0; j < v.size(); j++){
            mean += v[j][i];
        }
        mean = mean / v.size();
        result[i] = mean;
    }
    return result;
}

void AudioProcessor::fft(std::valarray<std::complex<long double>> & complexFrame) {
    // DFT
    const size_t N = complexFrame.size();
    if (N <= 1) return;

    // divide
    std::valarray<std::complex<long double>> even = complexFrame[std::slice(0, N/2, 2)];
    std::valarray<std::complex<long double>>  odd = complexFrame[std::slice(1, N/2, 2)];

    // conquer
    fft(even);
    fft(odd);

    // combine
    for (size_t k = 0; k < N/2; ++k)
    {
        std::complex<long double> t = std::polar(1.0L, -2 * 3.14159265358979323846264338328L * k / N) * odd[k];
        complexFrame[k    ] = even[k] + t;
        complexFrame[k+N/2] = even[k] - t;
    }
}

void AudioProcessor::fftVector(vec & frame) const {
    // resize frame to N
    // either append zeros or turncate samples
    if(conf.NFFT > frame.size()){
        vec zeros(conf.NFFT - frame.size(), 0);
        frame.insert(frame.end(), zeros.begin(), zeros.end());
    }
    else if(conf.NFFT < frame.size()){
        frame.erase(frame.begin() + conf.NFFT, frame.end());
    }

    // "copy" vec into complex vector
    std::valarray<std::complex<long double>> complexFrame(frame.size());
    for(unsigned int i = 0; i < frame.size(); i++)
        complexFrame[i] = frame[i];

    // fourier transform
    fft(complexFrame);

    // compute magnitude of FFT
    frame.clear();
    frame.resize(conf.NFFT/2+1); // only left half of spectrum
    for(unsigned int i = 0; i < frame.size(); i++){
        frame[i] = sqrt(complexFrame[i].real() * complexFrame[i].real() + complexFrame[i].imag() * complexFrame[i].imag());
    }
}

void AudioProcessor::fftMatrix(vec2d & frames) const {
    for(unsigned int i = 0; i < frames.size(); i++){
        fftVector(frames[i]);
    }
}

auto AudioProcessor::bytesToSamples(const byteVec & buffer) const -> vec {

    if(buffer.size() % conf.bytesPerSample * conf.numberOfChannels){
        throw AudioProcessorException("Invalid size of input audio buffer.");
    }

    vec samples(buffer.size()/conf.bytesPerSample, 0);
    for(unsigned int i = 0, j = 0; i < buffer.size(); i += conf.bytesPerSample, j++){
        uint32_t sample = 0;
        for(unsigned int k = 0; k < conf.bytesPerSample; k++){
            //sample |= static_cast<unsigned int>((static_cast<unsigned char>(buffer[i + k]) << (8 * k)));
            sample |= (static_cast<uint8_t>(buffer[i + k]) << (8 * k));
        }
        if(conf.bytesPerSample == 1){
            samples[j] = static_cast<uint8_t>(sample);
        }
        else if(conf.bytesPerSample == 2){
            samples[j] = static_cast<int16_t>(sample);
        }
    }

    return samples;
}

void AudioProcessor::channelsToMono(vec & sampleData) const {

    vec samplesMono(sampleData.size()/conf.numberOfChannels,0);

    for(unsigned int i = 0, j = 0; i < sampleData.size(); i+=conf.numberOfChannels, j++){
        long double sumSignals = 0;
        for(unsigned int k = 0; k < conf.numberOfChannels; k++){
            sumSignals += sampleData[i + k];
        }
        samplesMono[j] = sumSignals / static_cast<long double>(conf.numberOfChannels);
    }

    sampleData = std::move(samplesMono);
}

void AudioProcessor::magnitudeToPower(vec2d & magnitudes) const {
    for(unsigned int i = 0; i < magnitudes.size(); i++){
        for(unsigned int j = 0; j < magnitudes[i].size(); j++){
            unsigned int N = magnitudes[i].size();
            magnitudes[i][j] = magnitudes[i][j] * magnitudes[i][j] / N;
        }
    }
}

auto AudioProcessor::linspace(long double low, long double high, unsigned int numPoints) -> vec {
    vec result(numPoints, 0);

    long double diff = high - low;
    long double step = diff / numPoints;

    for(unsigned int i = 0; i < numPoints; i++){
        result[i] = low + i*step;

        if(i == numPoints - 1)// rounding may occur here
            result[i] = high;
    }

    return result;
}

auto AudioProcessor::frameSamples(vec & sampleData) const -> vec2d {

    const unsigned int frameLength = static_cast<unsigned int>(round(conf.framingSize / static_cast<long double>(1000) * conf.sampleRate)); // in num samples
    const unsigned int frameStep = static_cast<unsigned int>(round(conf.framingStride / static_cast<long double>(1000) * conf.sampleRate)); // in num samples
    const unsigned int numFrames = static_cast<unsigned int>(ceil((sampleData.size() - frameLength) / static_cast<long double>(frameStep)));

    // make sure there is valid number of samples to perform framing
    // if there is not then add zeros to the end to make it valid
    const int paddingToAppend = (sampleData.size() - frameLength) % frameStep;
    if(paddingToAppend){
        vec zeros(paddingToAppend, 0);
        sampleData.insert(sampleData.end(), zeros.begin(), zeros.end());
    }

    vec2d frames(numFrames, vec(frameLength, 0));
    for(unsigned int i = 0, j = 0; i < numFrames; i++, j+=frameStep){
        std::copy(sampleData.begin() + j, sampleData.begin() + j + frameLength, frames[i].begin());
    }

    return frames;
}

void AudioProcessor::preEmphasis(vec & sampleData) const {
    for(unsigned int i = 1; i < sampleData.size(); i++){
        sampleData[i] = sampleData[i] - conf.emphasisCoeff * sampleData[i - 1];
    }
}

void AudioProcessor::hammingWindow(vec2d & frames) const {
    for(unsigned int i = 0; i < frames.size(); i++){
        for(unsigned int j = 0; j < frames[i].size(); j++){
            frames[i][j] *= 0.54L - 0.46L*cos((2*3.14159265358979323846264338328L*j)/static_cast<long double>((frames[i].size()-1)));
        }
    }
}

void AudioProcessor::filterBanks(vec2d & v) const {
    const long double lowFreqMel = 0;
    const long double highFreqMel = hzToMel(conf.sampleRate / 2);
    vec points = linspace(lowFreqMel, highFreqMel, conf.numberOfFilterBanks + 2); // mel points equally spaced
    melToHz(points); // convert mel space into hz space

    for(unsigned int i = 0; i < points.size(); i++)
        points[i] = floor((conf.NFFT + 1) * points[i] / conf.sampleRate);

    vec2d fBank(conf.numberOfFilterBanks, vec(static_cast<int>(floor(conf.NFFT / 2 + 1)), 0));

    for(unsigned int i = 1; i < conf.numberOfFilterBanks + 1; i++){
        unsigned int fMinus = static_cast<int>(points[i - 1]);
        unsigned int f = static_cast<int>(points[i]);
        unsigned int fPlus = static_cast<int>(points[i + 1]);

        for(unsigned int j = fMinus; j < f; j++)
            fBank[i - 1][j] = (j - points[i - 1]) / (points[i] - points[i - 1]);
        for(unsigned int j = f; j < fPlus; j++)
            fBank[i - 1][j] = (points[i + 1] - j) / (points[i + 1] - points[i]);
    }

    transposeMatrix(fBank);
    dotMatrix(v, fBank);
    stabilizeMatrix(v);

    //convert result to dB
    for(unsigned int i = 0; i < v.size(); i++){
        for(unsigned int j = 0; j < v[i].size(); j++){
            v[i][j] = 20 * log10(v[i][j]);
        }
    }
}

auto AudioProcessor::processBuffer(const byteVec & buffer, bool mfcc) const -> vec2d {
    if(!validateConfig()){
        throw AudioProcessorException("Invalid audio configuration.");
    }

    // firstly, concatenate single bytes into audio samples
    vec vectorData = bytesToSamples(buffer);

    // now translate all channel data into mono signal by using average of samples from all channels
    channelsToMono(vectorData);

    // apply pre emphasis filter to amplify high frequencies and increase s/n ratio
    preEmphasis(vectorData);

    // split audio samples into frames as frequencies are stationary over short periods of time
    // used to get good frequency contours of the signal
    vec2d matrixData = frameSamples(vectorData);

    // apply hamming window to each frame to reduce spectral leakage
    hammingWindow(matrixData);

    // get frequency domain data from each frame
    fftMatrix(matrixData);

    // convert magnitude to power spectrum
    magnitudeToPower(matrixData);

    // apply triangular filters on Mel scale to extract frequency bands
    filterBanks(matrixData);

    if(mfcc){

    }

    // get mean of each column
    vectorData = meansMatrix(matrixData);

    // and subtract it from matrix data
    subtractMatrix(matrixData, vectorData);

    transposeMatrix(matrixData);

    return matrixData;
}
