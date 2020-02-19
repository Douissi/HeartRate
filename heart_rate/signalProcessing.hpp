#ifndef SIGNALPROCESSING_HPP
#define SIGNALPROCESSING_HPP

#include <QObject>
#include <QImage>

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


struct functions {

    static void ASSERT(bool condition,const std::string& str)
    {
        if (!condition)
        {
            std::cerr << "ASSERT FAIL WITH MSG " << str << '\n';
            abort();
        }
    }


    static void plotWithGNUPLOT(const string& nameSig, const unsigned char& type)
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

};



class TestVideoProcessing;
class TestSignalProcessing;

template <typename T>
class signalProcessing
{
    friend class TestVideoProcessing;
    friend class TestSignalProcessing;

public :
    signalProcessing() : numberOfSamples(0), Fs(0){}
    signalProcessing(const unsigned int& numOfSamples, const double& Fech) : numberOfSamples(0), Fs(Fech)
    {
        signalSamples.reserve(numOfSamples); // WRN = please make sure when calling size() on this vector
    }

    signalProcessing(const std::vector<T>& signalSamplesInput, const unsigned int &numOfSamples , const double &Fech) : numberOfSamples(0),
        Fs(Fech)
    {
        functions::ASSERT(signalSamplesInput.size() == numOfSamples, " Error while constructing signalProcessing Obj :: numOfSamples incorrect and not equal to size of input vector ");
        signalSamples.reserve(numOfSamples);
        fillWithVector(signalSamplesInput);
    }


    virtual ~signalProcessing()
    {
        signalSamples.clear();
    }

    T meanVector()
    {
        T tmp=0;
        for (const auto& v : signalSamples) tmp += v;
        return tmp/numberOfSamples;
    }

    void removeMeanValue()
    {
        T meanSignal = meanVector();
        std::transform(signalSamples.begin(), signalSamples.end(),signalSamples.begin(),
                       [&meanSignal](T v) -> T {return v - meanSignal;});
    }

    void fillWithValue(const T& value)
    {
        signalSamples.push_back(value);
        numberOfSamples++;
    }

    void fillWithVector(const std::vector<T>& inputVec)
    {
        for(const auto& v : inputVec) signalSamples.push_back(v);
        numberOfSamples += inputVec.size();
    }

    void reserve(const unsigned int &size)
    {
        signalSamples.reserve(size);
    }

    void resize(const unsigned int &size)
    {
        signalSamples.resize(size);
        numberOfSamples = size;
    }

    void clearSignal()
    {
        signalSamples.clear(); // MAYBE we need to reserve again
        numberOfSamples = 0;
    }

    void saveSigToFile(const string& inputFile)
    {
        fstream myfile;
        myfile.open(inputFile,fstream::out);
        if(myfile.is_open())
        {
            for(unsigned int i = 0;i < numberOfSamples; i++) myfile << i<< " " << signalSamples[i]<< std::endl;
            myfile.close();
        }
        else
        {
            functions::ASSERT(false, "Could not open the file : " + inputFile + " in saveSigToFile fct");
        }
    }

    void signalFFT(signalProcessing<T>& FFTOutput, const bool& save = DO_NOT_SAVE_FFT, const string& FFTFile = " ")
    {
        fftw_complex out[numberOfSamples/2 +1];
        double in[numberOfSamples];
        fftw_plan plan_forward;

        for (unsigned int i=0 ; i < numberOfSamples ; i++) in[i] = static_cast<double>(signalSamples[i]);

        plan_forward = fftw_plan_dft_r2c_1d ( numberOfSamples, in, out, FFTW_ESTIMATE );
        fftw_execute ( plan_forward );

        FFTOutput.resize(0);
        FFTOutput.reserve(numberOfSamples/2 +1);
        FFTOutput.setFs(Fs);

        for (unsigned int i = 0 ; i<= ((numberOfSamples/2)-1) ; i++)
        {
            FFTOutput.fillWithValue(out[i][0]*out[i][0]+ out[i][1]*out[i][1]);
        }

        if(save)
        {
            fstream myfile;
            myfile.open(FFTFile,fstream::out);

            if(myfile.is_open())
            {
                std::vector<T>& v = FFTOutput.getSignalSamples();
                for(unsigned int i = 0;i< ((numberOfSamples/2)-1); i++) myfile << Fs*i/numberOfSamples<< " " << v[i]<< std::endl; // readble mode by gnuplot
                myfile.close();
            }
            else
            {
                functions::ASSERT(false, "Could not open the file : " + FFTFile + " in signalFFT fct");
            }
        }

        fftw_destroy_plan ( plan_forward );
    }

    void bandPassFilterFunction(const float& lowFreqCutOff, const float& highFreqCutOff, const int& filterOrder,
                                signalProcessing<T>& outputSig)
    {
        vec b;
        // Create a FIR filter
        sp::FIR_filt<double, double, double> fir_filt;
        b = sp::fir1_bp(filterOrder, lowFreqCutOff/(Fs/2), highFreqCutOff/(Fs/2)); // b is filter coeffs cout << b.t()
        fir_filt.set_coeffs(b);

        outputSig.resize(0);
        outputSig.reserve(numberOfSamples);
        outputSig.setFs(Fs);

        // Filter - sample loop
        for (unsigned int n = 0; n < numberOfSamples; n++) outputSig.fillWithValue(fir_filt(signalSamples[n]));
    }


    static void nlmsFilter(signalProcessing<T>& inputSignal, signalProcessing<T>& originalSignal, const int& filterOrder,
                           const float& mu, signalProcessing<T>& outputSignalEstimation)
    {
        const std::vector<T>& inputSig = inputSignal.getSignalSamples();
        const std::vector<T>& originalSig = originalSignal.getSignalSamples();
        unsigned int N = inputSig.size();
        functions::ASSERT(N == originalSig.size(), "Failure when calling nlmsFilter:: Noise Signal and Input Signal do not have the same size");

        outputSignalEstimation.resize(N);
        outputSignalEstimation.setFs(inputSignal.getFs());
        outputSignalEstimation.setNbOfSamples(inputSignal.getNbOfSamples());
        std::vector<T>& outputEstimation = outputSignalEstimation.getSignalSamples();

        vector<T> nlmsOutput(N, 0.0);
        sp::FIR_filt<double,double,double> Ghat;
        Ghat.setup_nlms(filterOrder, mu, 0.0); // mu is 0.5, 0.0 is NLMS regulation const.
        // Filter - sample loop
        for(unsigned int n = 0 ; n < N ; n++)
        {
            // Apply adaptiv filter
            nlmsOutput[n] = Ghat(inputSig[n]);
            // Calc error
            outputEstimation[n] = originalSig[n]-nlmsOutput[n];
            Ghat.nlms_adapt(outputEstimation[n]);
        }

        // Avoid first incorrect sample
        outputEstimation[0] = outputEstimation[1];
    }

    float getFrequencyOfMaxFFT()
    {
        return static_cast<float>(distance(signalSamples.begin(), max_element(signalSamples.begin(), signalSamples.end()))) * Fs/
                (2.0f * static_cast<float>(numberOfSamples));
    }

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
