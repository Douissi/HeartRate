#ifndef SIGNALPROCESSINGFUNCTIONS_HPP
#define SIGNALPROCESSINGFUNCTIONS_HPP

#include <vector>
#include <iostream>
#include <cmath>
#include <fstream>
#include <time.h>
#include <opencv2/highgui.hpp>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/objdetect/objdetect.hpp>
#include <armadillo>
#include <sigpack.h>
#include "fftw3.h"

using namespace cv;
using namespace std;
using namespace arma;
using namespace sp;


template <typename T>
T meanVector(const vector<T>& input)
{
    T tmp=0;
    for (const auto v : input) tmp+=v;
    return tmp/input.size();
}

template <typename T>
void removeMeanValue(vector<T>& inputOutput)
{
    T meanForeHeadSignal = meanVector<T>(inputOutput);
    std::transform(inputOutput.begin(), inputOutput.end(),inputOutput.begin(),
                 [&meanForeHeadSignal](T v) -> T {return v - meanForeHeadSignal;});
}


template<typename T>
void saveSigToFile(const vector<T>& inputSig, const string& inputFile)
{
    int N = inputSig.size();
    int ff[N];
    for (int i=0; i<N ;i++) ff[i]=i;
    fstream myfile;
    myfile.open(inputFile,fstream::out);
    for(int i = 0;i< N; i++) myfile << ff[i]<< " " << inputSig[i]<< std::endl;
    myfile.close();

}


void imageColorToGreyTransform(const cv::Mat& InputImageColor, cv::Mat& OutputGreyColor)
{
    cvtColor(InputImageColor, OutputGreyColor, COLOR_BGR2GRAY);
    equalizeHist(OutputGreyColor, OutputGreyColor);

}

template<typename T>
void bandPassFilterFunction(const vector<T>& inputSig, const float& lowFreqCutOff,
                            const float& highFreqCutOff, const float& Fs, const int& filterOrder, vector<T>& outputSig)
{
    int N = inputSig.size();
    vec b;

    // Create a FIR filter
    FIR_filt<double, double, double> fir_filt;
    b = fir1_bp(filterOrder, lowFreqCutOff/(Fs/2), highFreqCutOff/(Fs/2)); // b is filter coeffs cout << b.t()
    fir_filt.set_coeffs(b);

    // Filter - sample loop
    for (int n = 0; n < N; n++) outputSig[n] = fir_filt(inputSig[n]);

}

void SignalFFT(const vector<float>& inputSignal, const double& Fs, const string& FFTFile)
{
    int N = inputSignal.size();
    fftw_complex out[N/2 +1];
    double in[N];
    double ff[N];
    fftw_plan plan_forward;

    for (int i=0; i < N ; i++) in[i] = inputSignal[i];

    for (int i=0; i<= ((N/2)-1);i++) ff[i]=Fs*i/N;

    plan_forward = fftw_plan_dft_r2c_1d ( N, in, out, FFTW_ESTIMATE );
    fftw_execute ( plan_forward );

    double v[N/2 +1];

    for (int i = 0; i<= ((N/2)-1); i++)
        v[i]=out[i][0]*out[i][0]+ out[i][1]*out[i][1];

    fstream myfile;
    myfile.open(FFTFile,fstream::out);

    for(int i = 0;i< ((N/2)-1); i++) myfile << ff[i]<< " " << v[i]<< std::endl;

     myfile.close();
    fftw_destroy_plan ( plan_forward );
//    fftw_free ( in );
//    fftw_free ( out );

}

int fftExemple()
{
    int i;
    int N=550;//Number of points acquired inside the window
    double Fs=200;//sampling frequency
//    double dF=Fs/N;
    double  T=1/Fs;//sample time
    double f=50;//frequency
    double *in;
    fftw_complex *out;
    double t[N];//time vector
    double ff[N];
    fftw_plan plan_forward;

    in = (double*) fftw_malloc(sizeof(double) * N);
    out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * N);

     for (int i=0; i<= N;i++)
     {
     t[i]=i*T;

    in[i] =0.7 *sin(2*M_PI*f*t[i]);// generate sine waveform
    //    double multiplier = 0.5 * (1 - cos(2*M_PI*i/(N-1)));//Hanning Window
    //    in[i] = multiplier * in[i];
     }

     for (int i=0; i<= ((N/2)-1);i++)
    {ff[i]=Fs*i/N;
    }
    plan_forward = fftw_plan_dft_r2c_1d ( N, in, out, FFTW_ESTIMATE );

    fftw_execute ( plan_forward );

    double v[N];

    for (int i = 0; i<= ((N/2)-1); i++)
    {

    v[i]=(20*log(sqrt(out[i][0]*out[i][0]+ out[i][1]*out[i][1])))/N;  //Here   I  have calculated the y axis of the spectrum in dB

       }

    fstream myfile;

    myfile.open("example2.txt",fstream::out);

    for(i = 0;i< ((N/2)-1); i++)

    {

    myfile << ff[i]<< " " << v[i]<< std::endl;

    }

     myfile.close();

     fftw_destroy_plan ( plan_forward );
     fftw_free ( in );
     fftw_free ( out );
     return 0;
}





#endif // SIGNALPROCESSINGFUNCTIONS_HPP
