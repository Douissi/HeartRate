#ifndef SIGNALPROCESSING_HPP
#define SIGNALPROCESSING_HPP

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <sstream>
#include <string>
#include <vector>

#include <fftw3.h>

#include "dsp.hpp"
#include "signalPlot.hpp"

using namespace std;

constexpr float lowFreqCutOff = 0.5f; // in Hz, corresponding to 30Bp/min
constexpr float highFreqCutOff = 4.5f; // in Hz, corresponding to 270 Bp/min
constexpr int filterOrder = 40; // Filter Order in BandPass Hanning Window Filter fixed to 30
constexpr float mu = 0.5f;

constexpr unsigned char BLEUE = 0;
constexpr unsigned char GREEN = 1;
constexpr unsigned char RED = 2;

constexpr unsigned char SIG = 1;
constexpr unsigned char FFT = 2;

constexpr bool SAVE_FFT = 1;
constexpr bool DO_NOT_SAVE_FFT = 0;


// Reports a fatal error. The message is printed and an exception is thrown, so that the
// application can report it (see main.cpp) and a unit test fails cleanly instead of
// killing the whole test host process (which is what abort() would do).
inline void ASSERT(bool condition, const std::string& str)
{
    if (!condition)
    {
        std::cerr << "ASSERT FAIL WITH MSG " << str << '\n';
        throw std::runtime_error(str);
    }
}


// Plots a signal file with gnuplot (gnuplot must be installed and available in the PATH).
// Kept for compatibility : signalProcessing::plotSignal / plotFFT display the signals directly
// (OpenCV windows), without external program.
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


// RAII helpers for FFTW resources (memory allocated with fftw_malloc and plans).
namespace fftw_raii
{
    struct FreeDeleter { void operator()(void* p) const { fftw_free(p); } };
    using RealBuffer = std::unique_ptr<double[], FreeDeleter>;
    using ComplexBuffer = std::unique_ptr<fftw_complex[], FreeDeleter>;

    struct Plan
    {
        explicit Plan(fftw_plan p) : plan(p) {}
        ~Plan() { if (plan) fftw_destroy_plan(plan); }
        Plan(const Plan&) = delete;
        Plan& operator=(const Plan&) = delete;
        fftw_plan plan;
    };
}


// Forward declarations of the unit-test classes (heart_rate_tests project) which are
// allowed to inspect the private members.
namespace heart_rate_tests
{
    class TestVideoProcessing;
    class TestSignalProcessing;
}

template <typename T>
class signalProcessing
{
    friend class heart_rate_tests::TestVideoProcessing;
    friend class heart_rate_tests::TestSignalProcessing;

public :
    signalProcessing() : numberOfSamples(0), Fs(0){}
    signalProcessing(const unsigned int& numOfSamples, const double& Fech) : numberOfSamples(0), Fs(Fech)
    {
        signalSamples.reserve(numOfSamples); // WRN = please make sure when calling size() on this vector
    }

    signalProcessing(const std::vector<T>& signalSamplesInput, const unsigned int &numOfSamples , const double &Fech) : numberOfSamples(0),
        Fs(Fech)
    {
        ASSERT(signalSamplesInput.size() == numOfSamples, " Error while constructing signalProcessing Obj :: numOfSamples incorrect and not equal to size of input vector ");
        signalSamples.reserve(numOfSamples);
        fillWithVector(signalSamplesInput);
    }


    virtual ~signalProcessing()
    {
        signalSamples.clear();
    }

    T meanVector()
    {
        // Sum accumulated in double: with a float accumulator the rounding errors of the additions
        // add up (about 1e-3 per addition once the sum reaches a few thousands), which made the
        // mean inaccurate for long signals and the result dependent on the data.
        double tmp = 0.0;
        for (const auto& v : signalSamples) tmp += static_cast<double>(v);
        return static_cast<T>(tmp / static_cast<double>(numberOfSamples));
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
        numberOfSamples += static_cast<unsigned int>(inputVec.size());
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
            ASSERT(false, "Could not open the file : " + inputFile + " in saveSigToFile fct");
        }
    }

    // Power spectrum (|X(k)|^2, k = 0 .. N/2-1) of the signal computed with FFTW.
    void signalFFT(signalProcessing<T>& FFTOutput, const bool& save = DO_NOT_SAVE_FFT, const string& FFTFile = " ")
    {
        ASSERT(numberOfSamples >= 2 && signalSamples.size() >= numberOfSamples,
               "signalFFT fct :: the signal must contain at least 2 samples");

        const unsigned int N = numberOfSamples;
        const double FsSignal = Fs;

        // Heap buffers (MSVC does not support variable length arrays), aligned for FFTW
        fftw_raii::RealBuffer in(fftw_alloc_real(N));
        fftw_raii::ComplexBuffer out(fftw_alloc_complex(N/2 + 1));
        ASSERT(in != nullptr && out != nullptr, "signalFFT fct :: FFTW memory allocation failed");

        for (unsigned int i=0 ; i < N ; i++) in[i] = static_cast<double>(signalSamples[i]);

        fftw_raii::Plan plan_forward(fftw_plan_dft_r2c_1d(static_cast<int>(N), in.get(), out.get(), FFTW_ESTIMATE));
        ASSERT(plan_forward.plan != nullptr, "signalFFT fct :: FFTW plan creation failed");
        fftw_execute(plan_forward.plan);

        FFTOutput.resize(0);
        FFTOutput.reserve(N/2 +1);
        FFTOutput.setFs(FsSignal);
        FFTOutput.fftSize = N; // needed to compute the frequency of each bin (plotFFT, getHeartRateFrequency)

        for (unsigned int i = 0 ; i<= ((N/2)-1) ; i++)
        {
            FFTOutput.fillWithValue(static_cast<T>(out[i][0]*out[i][0]+ out[i][1]*out[i][1]));
        }

        if(save)
        {
            fstream myfile;
            myfile.open(FFTFile,fstream::out);

            if(myfile.is_open())
            {
                std::vector<T>& v = FFTOutput.getSignalSamples();
                for(unsigned int i = 0;i< ((N/2)-1); i++) myfile << FsSignal*i/N<< " " << v[i]<< std::endl; // readble mode by gnuplot
                myfile.close();
            }
            else
            {
                ASSERT(false, "Could not open the file : " + FFTFile + " in signalFFT fct");
            }
        }
    }

    void bandPassFilterFunction(const float& lowFreqCutOff, const float& highFreqCutOff, const int& filterOrder,
                                signalProcessing<T>& outputSig)
    {
        // Create a band-pass FIR filter (Hamming window), b contains the filter coeffs
        dsp::FIR_filt fir_filt;
        const std::vector<double> b = dsp::fir1_bp(static_cast<std::size_t>(filterOrder), lowFreqCutOff/(Fs/2), highFreqCutOff/(Fs/2));
        fir_filt.set_coeffs(b);

        const std::vector<T> input = signalSamples; // copy: outputSig may be *this
        const unsigned int N = numberOfSamples;

        outputSig.resize(0);
        outputSig.reserve(N);
        outputSig.setFs(Fs);

        // Filter - sample loop
        for (unsigned int n = 0; n < N; n++) outputSig.fillWithValue(static_cast<T>(fir_filt(input[n])));
    }


    static void nlmsFilter(signalProcessing<T>& inputSignal, signalProcessing<T>& originalSignal, const int& filterOrder,
                           const float& mu, signalProcessing<T>& outputSignalEstimation)
    {
        const std::vector<T> inputSig = inputSignal.getSignalSamples();       // copies: the output may alias an input
        const std::vector<T> originalSig = originalSignal.getSignalSamples();
        unsigned int N = static_cast<unsigned int>(inputSig.size());
        ASSERT(N == originalSig.size(), "Failure when calling nlmsFilter:: Noise Signal and Input Signal do not have the same size");
        ASSERT(N >= 2, "Failure when calling nlmsFilter:: the signals must contain at least 2 samples");

        const double FsInput = inputSignal.getFs();
        const unsigned int nbOfSamplesInput = inputSignal.getNbOfSamples();

        outputSignalEstimation.resize(N);
        outputSignalEstimation.setFs(FsInput);
        outputSignalEstimation.setNbOfSamples(nbOfSamplesInput);
        std::vector<T>& outputEstimation = outputSignalEstimation.getSignalSamples();

        vector<T> nlmsOutput(N, T(0));
        dsp::FIR_filt Ghat;
        Ghat.setup_nlms(static_cast<std::size_t>(filterOrder), mu, 0.0); // mu is 0.5, 0.0 is NLMS regulation const.
        // Filter - sample loop
        for(unsigned int n = 0 ; n < N ; n++)
        {
            // Apply adaptiv filter
            nlmsOutput[n] = static_cast<T>(Ghat(inputSig[n]));
            // Calc error
            outputEstimation[n] = originalSig[n]-nlmsOutput[n];
            Ghat.nlms_adapt(outputEstimation[n]);
        }

        // Avoid first incorrect sample
        outputEstimation[0] = outputEstimation[1];
    }

    float getFrequencyOfMaxFFT()
    {
       return static_cast<float>(static_cast<double>(distance(signalSamples.begin(), max_element(signalSamples.begin(), signalSamples.end()))) * Fs/
               (2.0 * static_cast<double>(numberOfSamples)));
    }

    //************************************** plotting *********************************
    // The plots are drawn with OpenCV (see signalPlot.hpp). plotSignal / plotFFT display them in a
    // window; as for any OpenCV window, call cv::waitKey() afterwards to refresh / keep the windows
    // open (e.g. cv::waitKey(0) waits until a key is pressed). renderSignalPlot / renderFFTPlot
    // only return the image (to save it with cv::imwrite, or to display it elsewhere).

    // Frequency (Hz) of the bin 'index' of a spectrum computed by signalFFT()
    double getFFTBinFrequency(const unsigned int& index) const
    {
        const double length = (fftSize != 0) ? fftSize : 2.0 * numberOfSamples;
        return (length > 0.0) ? static_cast<double>(index) * Fs / length : 0.0;
    }

    // For a spectrum computed by signalFFT() : index of the highest bin whose frequency is inside
    // [lowFreq, highFreq], -1 if no bin is inside the band.
    int getHeartRateBinIndex(const double& lowFreq = lowFreqCutOff, const double& highFreq = highFreqCutOff) const
    {
        int best = -1;
        const unsigned int n = std::min(numberOfSamples, static_cast<unsigned int>(signalSamples.size()));
        for (unsigned int i = 0; i < n; i++)
        {
            const double f = getFFTBinFrequency(i);
            if (f < lowFreq || f > highFreq) continue;
            if (best < 0 || signalSamples[i] > signalSamples[static_cast<unsigned int>(best)]) best = static_cast<int>(i);
        }
        return best;
    }

    // For a spectrum computed by signalFFT() : frequency (Hz) of the highest bin inside
    // [lowFreq, highFreq] (the heart rate in beats per minute is 60 x this frequency).
    // Returns -1 if no bin is inside the band.
    double getHeartRateFrequency(const double& lowFreq = lowFreqCutOff, const double& highFreq = highFreqCutOff) const
    {
        const int index = getHeartRateBinIndex(lowFreq, highFreq);
        return (index >= 0) ? getFFTBinFrequency(static_cast<unsigned int>(index)) : -1.0;
    }

    // Image of the signal : amplitude versus time in seconds (versus sample index if Fs is unknown)
    cv::Mat renderSignalPlot(const string& title, const int& width = 600, const int& height = 230) const
    {
        signal_plot::PlotData data;
        data.title = title;
        data.yLabel = "Amplitude";
        data.xLabel = (Fs > 0.0) ? "Time (s)" : "Sample";
        const unsigned int n = std::min(numberOfSamples, static_cast<unsigned int>(signalSamples.size()));
        data.x.reserve(n);
        data.y.reserve(n);
        for (unsigned int i = 0; i < n; i++)
        {
            data.x.push_back((Fs > 0.0) ? i / Fs : static_cast<double>(i));
            data.y.push_back(static_cast<double>(signalSamples[i]));
        }
        return signal_plot::render(data, width, height);
    }

    // Image of a spectrum computed by signalFFT() : power versus frequency (Hz) in [0, maxFrequency].
    // The band [lowFreq, highFreq] is shaded and its highest peak is marked with the heart rate (bpm).
    cv::Mat renderFFTPlot(const string& title, const double& maxFrequency = 6.0,
                          const double& lowFreq = lowFreqCutOff, const double& highFreq = highFreqCutOff,
                          const int& width = 600, const int& height = 230) const
    {
        signal_plot::PlotData data;
        data.title = title;
        data.yLabel = "Power";
        data.xLabel = "Frequency (Hz)";
        const unsigned int n = std::min(numberOfSamples, static_cast<unsigned int>(signalSamples.size()));
        data.x.reserve(n);
        data.y.reserve(n);
        for (unsigned int i = 0; i < n; i++)
        {
            data.x.push_back(getFFTBinFrequency(i));
            data.y.push_back(static_cast<double>(signalSamples[i]));
        }
        data.fixedXRange = true;
        data.xMin = 0.0;
        data.xMax = (maxFrequency > 0.0) ? maxFrequency : ((Fs > 0.0) ? Fs / 2.0 : 1.0);

        data.showBand = true;
        data.bandMin = lowFreq;
        data.bandMax = highFreq;
        data.bandLabel = "heart rate band";

        const int peak = getHeartRateBinIndex(lowFreq, highFreq);
        if (peak >= 0)
        {
            const double peakFrequency = getFFTBinFrequency(static_cast<unsigned int>(peak));
            data.showMarker = true;
            data.markerX = peakFrequency;
            data.markerY = static_cast<double>(signalSamples[static_cast<unsigned int>(peak)]);
            std::ostringstream label;
            label << std::fixed << std::setprecision(2) << peakFrequency << " Hz = "
                  << std::setprecision(0) << 60.0 * peakFrequency << " bpm";
            data.markerLabel = label.str();
        }
        return signal_plot::render(data, width, height);
    }

    // Displays the signal in the window 'windowName' (see renderSignalPlot)
    void plotSignal(const string& windowName, const int& width = 600, const int& height = 230) const
    {
        signal_plot::show(windowName, renderSignalPlot(windowName, width, height));
    }

    // Displays the spectrum in the window 'windowName' (see renderFFTPlot)
    void plotFFT(const string& windowName, const double& maxFrequency = 6.0,
                 const double& lowFreq = lowFreqCutOff, const double& highFreq = highFreqCutOff,
                 const int& width = 600, const int& height = 230) const
    {
        signal_plot::show(windowName, renderFFTPlot(windowName, maxFrequency, lowFreq, highFreq, width, height));
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
    unsigned int fftSize = 0; // spectrum computed by signalFFT() : length of the FFT (0 otherwise)

};



// FFTW usage example (not used by the application): FFT of a 50 Hz sine, spectrum in dB saved in example2.txt
inline int fftExemple()
{
    constexpr int N=550;//Number of points acquired inside the window
    const double Fs=200;//sampling frequency
    //    double dF=Fs/N;
    const double  T=1/Fs;//sample time
    const double f=50;//frequency

    fftw_raii::RealBuffer in(fftw_alloc_real(N));
    fftw_raii::ComplexBuffer out(fftw_alloc_complex(N/2 + 1));
    std::vector<double> t(N);//time vector
    std::vector<double> ff(N);

    for (int i=0; i < N;i++)
    {
        t[i]=i*T;

        in[i] =0.7 *sin(2*dsp::kPi*f*t[i]);// generate sine waveform
        //    double multiplier = 0.5 * (1 - cos(2*dsp::kPi*i/(N-1)));//Hanning Window
        //    in[i] = multiplier * in[i];
    }

    for (int i=0; i<= ((N/2)-1);i++)
    {ff[i]=Fs*i/N;
    }
    fftw_raii::Plan plan_forward(fftw_plan_dft_r2c_1d ( N, in.get(), out.get(), FFTW_ESTIMATE ));

    fftw_execute ( plan_forward.plan );

    std::vector<double> v(N);

    for (int i = 0; i<= ((N/2)-1); i++)
    {

        v[i]=(20*log(sqrt(out[i][0]*out[i][0]+ out[i][1]*out[i][1])))/N;  //Here   I  have calculated the y axis of the spectrum in dB

    }

    fstream myfile;

    myfile.open("example2.txt",fstream::out);

    for(int i = 0;i< ((N/2)-1); i++)

    {

        myfile << ff[i]<< " " << v[i]<< std::endl;

    }

    myfile.close();

    return 0;
}





#endif // SIGNALPROCESSING_HPP
