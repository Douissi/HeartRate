#ifndef SIGNALPROCESSING_HPP
#define SIGNALPROCESSING_HPP

#include <opencv2/highgui.hpp>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/objdetect/objdetect.hpp>

#include <vector>
#include <iostream>
#include <cmath>
#include <fstream>
#include <time.h>
#include <armadillo>
#include <sigpack.h>
#include "fftw3.h"

using namespace cv;
using namespace std;
using namespace arma;
using namespace sp;

constexpr float lowFreqCutOff = 0.5f; // in Hz, corresponding to 30Bp/min
constexpr float highFreqCutOff = 4.5f; // in Hz, corresponding to 270 Bp/min
constexpr int filterOrder = 40; // Filter Order in BandPass Hanning Window Filter fixed to 30
constexpr float mu = 0.5f;
constexpr u32 windowLength = 2; // sliding window of 2 seconds

constexpr unsigned char BLEUE = 0;
constexpr unsigned char GREEN = 1;
constexpr unsigned char RED = 2;

constexpr unsigned char SIG = 1;
constexpr unsigned char FFT = 2;

constexpr bool SAVE_FFT = 1;
constexpr bool DO_NOT_SAVE_FFT = 0;


class TestVideoProcessing;
class TestSignalProcessing;


namespace functions {

inline void ASSERT(bool condition,const std::string& str)
{
    if (!condition)
    {
        std::cerr << "ASSERT FAIL WITH MSG " << str << '\n';
        abort();
    }
}


inline void plotWithGNUPLOT(const string& nameSig, const unsigned char& type)
{
    if(type == SIG)
    {
        string cmd = "gnuplot -e \""
                     "set xlabel 'Samples in frames' ;"
                     "set ylabel 'Amplitude of the signal' ;"
                     "plot '" + nameSig + "' using 1:2 with lines\" -p";
        system(cmd.c_str());
    }
    else if (type == FFT)
    {
        string cmd = "gnuplot -e \""
                     "set xlabel 'Frequencies in Hz' ;"
                     "set ylabel 'Amplitude of the FFT' ;"
                     "set xrange [0:6] ;"
                     "plot '" + nameSig + "' using 1:2 with lines\" -p";
        system(cmd.c_str());
    }
    else
    {
        ASSERT(false, "**** !!!!!! ***** plot With GNUPLOT :: type of Sig NOT KNOWN ** !!!!! ******** " );
    }
}

}


template <typename T>
class signalProcessing
{
    friend class TestVideoProcessing;
    friend class TestSignalProcessing;

public :
    signalProcessing();
    signalProcessing(const unsigned int& numOfSamples, const double& Fech);
    signalProcessing(const std::vector<T>& signalSamplesInput, const unsigned int &numOfSamples , const double &Fech);

    virtual ~signalProcessing();

    T meanVector();
    void removeMeanValue();
    void fillWithValue(const T& value);
    void fillWithVector(const std::vector<T>& inputVec);
    void reserve(const unsigned int &size);
    void resize(const unsigned int &size);
    void clearSignal();
    void saveSigToFile(const string& inputFile);
    void signalFFT(signalProcessing<T>& FFTOutput, const bool& save = DO_NOT_SAVE_FFT, const string& FFTFile = " ");
    void bandPassFilterFunction(const float& lowFreqCutOff, const float& highFreqCutOff,
                                const int& filterOrder, signalProcessing<T>& outputSig);

    static void nlmsFilter(signalProcessing<T>& inputSignal, signalProcessing<T>& originalSignal, const int& filterOrder,
                           const float& mu, signalProcessing<T>& outputSignalEstimation);
    float getFrequencyOfMaxFFT();

    //****************************************getters *********************************
    std::vector <T>& getSignalSamples() { return signalSamples; }
    const double& getFs() { return Fs; }
    const unsigned int& getNbOfSamples() { return numberOfSamples; }

    //***************************************setters ********************************
    void setFs(const double& Fech) { Fs = Fech; }
    void setNbOfSamples(const unsigned int& nbOfSamples) { numberOfSamples = nbOfSamples; }

private:

    std::vector <T> signalSamples;
    unsigned int numberOfSamples;
    double Fs;

};


#endif // SIGNALPROCESSING_HPP
