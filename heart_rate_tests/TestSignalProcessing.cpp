// Unit tests of the signalProcessing class (port of the former QtTest class TestSignalProcessing).

#include "CppUnitTest.h"
#include "TestUtils.hpp"

#include "signalProcessing.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <iterator>
#include <numeric>
#include <stdexcept>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace heart_rate_tests
{

    namespace
    {
        // Sine of amplitude 1 and frequency 'frequency' (Hz), sampled at Fs (Hz), N samples
        std::vector<float> makeSine(double frequency, double Fs, unsigned int N)
        {
            std::vector<float> samples(N);
            for (unsigned int n = 0; n < N; n++)
                samples[n] = static_cast<float>(std::sin(2.0 * dsp::kPi * frequency * n / Fs));
            return samples;
        }
    } // namespace

    TEST_CLASS(TestSignalProcessing)
    {
    public:

        TEST_METHOD(testSignalConstruct)
        {
            signalProcessing<float> signal;
            UT_VERIFY(signal.Fs == 0.0);
            UT_VERIFY(signal.numberOfSamples == 0);
            UT_VERIFY(signal.signalSamples.size() == 0);
        }

        TEST_METHOD(testSignalConstruct2)
        {
            signalProcessing<float> signal(50, 10.5);
            UT_VERIFY(signal.Fs == 10.5);
            UT_VERIFY(signal.numberOfSamples == 0);
            UT_VERIFY(signal.signalSamples.size() == 0);
        }

        TEST_METHOD(testSignalConstruct3)
        {
            std::vector<float> signalVector(1000);
            std::generate(signalVector.begin(), signalVector.end(), [] { return static_cast<float>(std::rand()); });

            signalProcessing<float> signalProcessingObj(signalVector, static_cast<unsigned int>(signalVector.size()), 10.0);

            UT_VERIFY(signalProcessingObj.Fs == 10.0);
            UT_VERIFY(signalProcessingObj.numberOfSamples == signalVector.size());
            UT_VERIFY(signalProcessingObj.signalSamples.size() == signalVector.size());
            UT_VERIFY(std::equal(signalVector.begin(), signalVector.end(), signalProcessingObj.signalSamples.begin()));
        }

        TEST_METHOD(testSignalMeanValue)
        {
            std::vector<float> signalVector(1000);
            std::generate(signalVector.begin(), signalVector.end(), [] { return static_cast<float>(ut::rand()); }); // generate rand values less then 1000
            signalProcessing<float> signalProcessingObj(signalVector, static_cast<unsigned int>(signalVector.size()), 10.0);

            float meanValue = signalProcessingObj.meanVector();
            float average = static_cast<float>(std::accumulate(signalVector.begin(), signalVector.end(), 0.0) / static_cast<double>(signalVector.size()));

            UT_VERIFY(average == meanValue);
        }

        TEST_METHOD(testRemoveMeanValue)
        {
            std::vector<float> signalVector(1000);
            std::generate(signalVector.begin(), signalVector.end(), [] { return static_cast<float>(ut::rand()); }); // generate rand values less then 1000
            signalProcessing<float> signalProcessingObj(signalVector, static_cast<unsigned int>(signalVector.size()), 10.0);

            UT_VERIFY(signalProcessingObj.meanVector() != 0);

            signalProcessingObj.removeMeanValue();
            auto mean = signalProcessingObj.meanVector();
            UT_VERIFY(std::fabs(signalProcessingObj.meanVector()) < 0.00005);
        }

        TEST_METHOD(testFillWithValue)
        {
            signalProcessing<float> signal;
            UT_VERIFY(signal.Fs == 0.0);
            UT_VERIFY(signal.numberOfSamples == 0);
            UT_VERIFY(signal.signalSamples.size() == 0);
            float value = static_cast<float>(ut::rand());
            signal.fillWithValue(value);
            UT_VERIFY(signal.signalSamples.size() == 1);
            UT_VERIFY(signal.signalSamples.back() == value);
        }

        TEST_METHOD(testFillWithVector)
        {
            signalProcessing<float> signal(500, 10.5);
            UT_VERIFY(signal.Fs == 10.5);
            UT_VERIFY(signal.numberOfSamples == 0);
            UT_VERIFY(signal.signalSamples.size() == 0);

            std::vector<float> signalVector(500);
            std::generate(signalVector.begin(), signalVector.end(), [] { return static_cast<float>(ut::rand()); });

            signal.fillWithVector(signalVector);
            UT_VERIFY(signal.signalSamples.size() == 500);
            UT_VERIFY(std::equal(signalVector.begin(), signalVector.end(), signal.signalSamples.begin()));
        }

        TEST_METHOD(testSaveSigToFile)
        {
            unsigned int size = 1000;
            std::vector<float> signalVector(size);
            std::generate(signalVector.begin(), signalVector.end(), [] { return static_cast<float>(ut::rand()); });

            signalProcessing<float> signalProcessingObj(signalVector, static_cast<unsigned int>(signalVector.size()), 10.0);

            UT_VERIFY(signalProcessingObj.Fs == 10.0);
            UT_VERIFY(signalProcessingObj.numberOfSamples == signalVector.size());
            UT_VERIFY(signalProcessingObj.signalSamples.size() == signalVector.size());
            UT_VERIFY(std::equal(signalVector.begin(), signalVector.end(), signalProcessingObj.signalSamples.begin()));

            signalProcessingObj.saveSigToFile("testSaveSigToFile.txt");

            std::vector<float> signalVectorVerification;
            signalVectorVerification.reserve(size);

            ut::loadSignalFromFile<float>("testSaveSigToFile.txt", signalVectorVerification);

            UT_VERIFY(signalVectorVerification.size() == size);
            UT_VERIFY(std::equal(signalVectorVerification.begin(), signalVectorVerification.end(), signalProcessingObj.signalSamples.begin()));
        }

        TEST_METHOD(testFFTfunction)
        {
            unsigned int size = 338;
            std::vector<float> signalInput;
            signalInput.reserve(size);
            std::vector<float> FFTSignalExpectedOutput;
            FFTSignalExpectedOutput.reserve(size / 2 - 1);

            ut::loadSignalFromFile<float>("testSignalInput.txt", signalInput);
            ut::loadSignalFromFile<float>("TestFFTexpectedResult.txt", FFTSignalExpectedOutput);
            UT_VERIFY(signalInput.size() == size);

            signalProcessing<float> signalProcessingObj(signalInput, static_cast<unsigned int>(signalInput.size()), 30.0);
            signalProcessing<float> FFTSignalOutput;

            signalProcessingObj.signalFFT(FFTSignalOutput, DO_NOT_SAVE_FFT);

            // Issue with precision in float coming from *FFT result
            //UT_VERIFY(std::equal(FFTSignalExpectedOutput.begin(), FFTSignalExpectedOutput.end(), FFTSignalOutput.begin()));
            UT_VERIFY_ALL_NEAR(FFTSignalOutput.getSignalSamples(), FFTSignalExpectedOutput, size / 2 - 1, 0.05);

            FFTSignalOutput.resize(350);
            UT_VERIFY(FFTSignalOutput.numberOfSamples == 350);
        }

        TEST_METHOD(testBandPassFilterFunction)
        {
            float lowFreqCutOff = 0.5; // in Hz, corresponding to 30Bp/min
            float highFreqCutOff = 4.5; // in Hz, corresponding to 270 Bp/min
            int   filterOrder = 40; // Filter Order in BandPass Hanning Window Filter fixed to 30

            unsigned int size = 338;
            std::vector<float> signalInput;
            signalInput.reserve(size);
            std::vector<float> signalOutPutExpected;
            signalOutPutExpected.reserve(size);

            ut::loadSignalFromFile<float>("testSignalInput.txt", signalInput);
            ut::loadSignalFromFile<float>("testFiltredBandPassInputSignal.txt", signalOutPutExpected);
            UT_VERIFY(signalInput.size() == size);

            signalProcessing<float> signalProcessingObj(signalInput, static_cast<unsigned int>(signalInput.size()), 30.0);
            signalProcessing<float> signalOutputFiltred;
            signalProcessingObj.bandPassFilterFunction(lowFreqCutOff, highFreqCutOff, filterOrder, signalOutputFiltred);

            UT_VERIFY_ALL_NEAR(signalOutputFiltred.getSignalSamples(), signalOutPutExpected, size, 0.00001);
        }

        TEST_METHOD(testNLMSfilter)
        {
            unsigned int size = 338;
            std::vector<float> signalInput1;
            signalInput1.reserve(size);
            std::vector<float> signalInput2;
            signalInput2.reserve(size);
            std::vector<float> signalExpectedOutput;
            signalExpectedOutput.reserve(size);

            ut::loadSignalFromFile<float>("testInput1.txt", signalInput1);
            ut::loadSignalFromFile<float>("testInput2.txt", signalInput2);
            ut::loadSignalFromFile<float>("testExpectedNLMSoutput.txt", signalExpectedOutput);
            UT_VERIFY(signalInput1.size() == size);
            UT_VERIFY(signalInput2.size() == size);

            signalProcessing<float> signalProcessingInput1(signalInput1, static_cast<unsigned int>(signalInput1.size()), 30.0);
            signalProcessing<float> signalProcessingInput2(signalInput2, static_cast<unsigned int>(signalInput2.size()), 30.0);
            signalProcessing<float> signalProcessingOutput;

            signalProcessing<float>::nlmsFilter(signalProcessingInput1, signalProcessingInput2, 40, 0.5, signalProcessingOutput);

            UT_VERIFY_ALL_NEAR(signalProcessingOutput.getSignalSamples(), signalExpectedOutput, size, 0.001);
        }

        // ---------------------------------------------------------------------------------------------
        // Error cases, FFT file, heart-rate frequency and FFTW example
        // ---------------------------------------------------------------------------------------------

        TEST_METHOD(testSignalConstructSizeMismatch)
        {
            // the number of samples given to the constructor must be the size of the vector
            std::vector<float> signalVector(10, 1.0f);
            Assert::ExpectException<std::runtime_error>([&] { signalProcessing<float> signal(signalVector, 9, 30.0); });
        }

        TEST_METHOD(testSaveSigToFileError)
        {
            signalProcessing<float> signal(std::vector<float>(10, 1.0f), 10, 30.0);
            // the folder does not exist : the file cannot be created
            Assert::ExpectException<std::runtime_error>([&] { signal.saveSigToFile("folder_that_does_not_exist/signal.txt"); });
        }

        TEST_METHOD(testFFTOutputAndSaveToFile)
        {
            // 1.5 Hz sine, Fs = 30 Hz, N = 300 : the resolution is Fs / N = 0.1 Hz, so 1.5 Hz is exactly bin 15
            const double Fs = 30.0;
            const unsigned int N = 300;
            signalProcessing<float> sine(makeSine(1.5, Fs, N), N, Fs);
            signalProcessing<float> FFTOutput;

            sine.signalFFT(FFTOutput, SAVE_FFT, "testFFTsave.txt");

            // spectrum : N/2 bins, same sampling frequency as the signal, maximum at bin 15
            std::vector<float>& spectrum = FFTOutput.getSignalSamples();
            UT_VERIFY(FFTOutput.getNbOfSamples() == N / 2);
            UT_VERIFY(spectrum.size() == N / 2);
            UT_VERIFY(FFTOutput.getFs() == Fs);
            UT_VERIFY(std::distance(spectrum.begin(), std::max_element(spectrum.begin(), spectrum.end())) == 15);

            // file : "frequency power" on each line (N/2 - 1 lines), frequency of line i = i * Fs / N
            std::vector<double> frequencies, powers;
            ut::loadColumnsFromFile("testFFTsave.txt", frequencies, powers);
            UT_VERIFY(frequencies.size() == N / 2 - 1);
            for (std::size_t i = 0; i < frequencies.size(); i++)
            {
                UT_VERIFY(std::fabs(frequencies[i] - i * Fs / N) < 1e-4);
                UT_VERIFY(std::fabs(powers[i] - spectrum[i]) <= 1e-5 * std::max(1.0, std::fabs(static_cast<double>(spectrum[i]))));
            }
            std::remove("testFFTsave.txt");
        }

        TEST_METHOD(testFFTErrors)
        {
            signalProcessing<float> FFTOutput;

            // less than 2 samples : no spectrum
            signalProcessing<float> oneSample(std::vector<float>(1, 1.0f), 1, 30.0);
            Assert::ExpectException<std::runtime_error>([&] { oneSample.signalFFT(FFTOutput); });

            // the FFT file cannot be created
            signalProcessing<float> sine(makeSine(1.5, 30.0, 300), 300, 30.0);
            Assert::ExpectException<std::runtime_error>([&] { sine.signalFFT(FFTOutput, SAVE_FFT, "folder_that_does_not_exist/fft.txt"); });
        }

        TEST_METHOD(testFFTZeroPadding)
        {
            // 1.25 Hz sine, Fs = 30 Hz, 90 samples (3 s) : without zero padding the spectrum has a value every
            // 30 / 90 = 0.333 Hz, 1.25 Hz is between two of them (1.0 Hz and 1.333 Hz)
            const double Fs = 30.0;
            const unsigned int N = 90;
            signalProcessing<float> sine(makeSine(1.25, Fs, N), N, Fs);
            signalProcessing<float> noPadding, padding0, paddingN, padded;

            // default, NO_ZERO_PADDING and a length equal to the number of samples : no zero padding
            sine.signalFFT(noPadding);
            sine.signalFFT(padding0, DO_NOT_SAVE_FFT, " ", NO_ZERO_PADDING);
            sine.signalFFT(paddingN, DO_NOT_SAVE_FFT, " ", N);
            UT_VERIFY(noPadding.getFFTSize() == N);
            UT_VERIFY(noPadding.getNbOfSamples() == N / 2);
            UT_VERIFY(padding0.getSignalSamples() == noPadding.getSignalSamples());
            UT_VERIFY(paddingN.getSignalSamples() == noPadding.getSignalSamples());
            UT_VERIFY(std::fabs(noPadding.getFrequencyOfMaxFFT() - 4 * Fs / N) < 1e-5); // nearest value : 1.333 Hz

            // zero padding to 4 x 90 = 360 points : a value every 30 / 360 Hz. The padded spectrum at the bins 4k
            // is the spectrum without padding at the bins k (same frequencies k x 30 / 90 Hz)
            sine.signalFFT(padded, DO_NOT_SAVE_FFT, " ", 4 * N);
            UT_VERIFY(padded.getFFTSize() == 4 * N);
            UT_VERIFY(padded.getNbOfSamples() == 2 * N);
            UT_VERIFY(padded.getFs() == Fs);
            for (unsigned int k = 0; k < N / 2; k++)
            {
                const double expected = noPadding.getSignalSamples()[k];
                UT_VERIFY(std::fabs(padded.getSignalSamples()[4 * k] - expected) <= 1e-4 * std::max(1.0, expected));
            }
            UT_VERIFY(std::fabs(padded.getFrequencyOfMaxFFT() - 1.25) < 1e-5); // bin 15 = 15 x 30 / 360 = 1.25 Hz

            // 1024 points : the peak is found within half a step (30 / 1024 / 2 Hz) of 1.25 Hz
            sine.signalFFT(padded, DO_NOT_SAVE_FFT, " ", 1024);
            UT_VERIFY(padded.getNbOfSamples() == 512);
            UT_VERIFY(std::fabs(padded.getFrequencyOfMaxFFT() - 1.25) <= 0.5 * Fs / 1024);

            // saved file : "frequency power", frequency of line i = i x Fs / (zero padded length)
            sine.signalFFT(padded, SAVE_FFT, "testFFTzeroPadding.txt", 4 * N);
            std::vector<double> frequencies, powers;
            ut::loadColumnsFromFile("testFFTzeroPadding.txt", frequencies, powers);
            UT_VERIFY(frequencies.size() == 2 * N - 1);
            for (std::size_t i = 0; i < frequencies.size(); i++)
                UT_VERIFY(std::fabs(frequencies[i] - static_cast<double>(i) * Fs / (4 * N)) < 1e-4);
            std::remove("testFFTzeroPadding.txt");

            // a zero padded length smaller than the number of samples is an error
            Assert::ExpectException<std::runtime_error>([&] { sine.signalFFT(padded, DO_NOT_SAVE_FFT, " ", N - 1); });
        }

        TEST_METHOD(testGetFrequencyOfMaxFFT)
        {
            // 1.2 Hz sine (= 72 beats per minute) sampled at 30 Hz during 10 s : bin 12 of the spectrum
            const double Fs = 30.0;
            const unsigned int N = 300;
            signalProcessing<float> sine(makeSine(1.2, Fs, N), N, Fs);
            signalProcessing<float> FFTOutput;
            sine.signalFFT(FFTOutput);

            UT_VERIFY(std::fabs(FFTOutput.getFrequencyOfMaxFFT() - 1.2) < 1e-5);
        }

        TEST_METHOD(testBandPassFilterInvalidCutOff)
        {
            // Fs = 8 Hz : the high cut-off frequency (4.5 Hz) is above the Nyquist frequency (4 Hz)
            signalProcessing<float> signal(makeSine(1.2, 8.0, 80), 80, 8.0);
            signalProcessing<float> output;
            Assert::ExpectException<std::invalid_argument>([&] { signal.bandPassFilterFunction(lowFreqCutOff, highFreqCutOff, filterOrder, output); });
        }

        TEST_METHOD(testNLMSfilterErrors)
        {
            signalProcessing<float> output;
            signalProcessing<float> signal10(std::vector<float>(10, 1.0f), 10, 30.0);
            signalProcessing<float> signal20(std::vector<float>(20, 1.0f), 20, 30.0);
            signalProcessing<float> signal1(std::vector<float>(1, 1.0f), 1, 30.0);

            // the two signals must have the same size
            Assert::ExpectException<std::runtime_error>([&] { signalProcessing<float>::nlmsFilter(signal10, signal20, filterOrder, mu, output); });
            // at least 2 samples
            Assert::ExpectException<std::runtime_error>([&] { signalProcessing<float>::nlmsFilter(signal1, signal1, filterOrder, mu, output); });
        }

        TEST_METHOD(testNLMSfilterZeroReference)
        {
            // reference (background) signals = 0 : there is nothing to remove, the output is the original
            // signal (and the NLMS update must not divide by the zero energy of the reference)
            const unsigned int N = 50;
            std::vector<float> original(N);
            for (unsigned int n = 0; n < N; n++) original[n] = static_cast<float>(n);
            signalProcessing<float> reference(std::vector<float>(N, 0.0f), N, 30.0);
            signalProcessing<float> originalSignal(original, N, 30.0);
            signalProcessing<float> output;

            signalProcessing<float>::nlmsFilter(reference, originalSignal, filterOrder, mu, output);

            UT_VERIFY(output.getNbOfSamples() == N);
            UT_VERIFY(output.getFs() == 30.0);
            UT_VERIFY(output.getSignalSamples()[0] == original[1]); // first sample replaced by the second one
            for (unsigned int n = 1; n < N; n++) UT_VERIFY(output.getSignalSamples()[n] == original[n]);
        }

        TEST_METHOD(testPlotWithGNUPLOTUnknownType)
        {
            // only the error cases is tested : the SIG and FFT cases start the external program gnuplot
            Assert::ExpectException<std::runtime_error>([] { plotWithGNUPLOT("signal.txt", 0); });
        }

        TEST_METHOD(testFftExemple)
        {
            // FFTW example : spectrum (dB) of a 50 Hz sine sampled at 200 Hz (550 samples), saved in example2.txt
            UT_VERIFY(fftExemple() == 0);

            std::vector<double> frequencies, levels;
            ut::loadColumnsFromFile("example2.txt", frequencies, levels);
            UT_VERIFY(frequencies.size() == 550 / 2 - 1);
            const auto peak = std::distance(levels.begin(), std::max_element(levels.begin(), levels.end()));
            UT_VERIFY(std::fabs(frequencies[peak] - 50.0) < 0.5);
            std::remove("example2.txt");
        }
        // ---------------------------------------------------------------------------------------------
      // Filters of order 0, FIR filters used directly, gnuplot branches
      // ---------------------------------------------------------------------------------------------

        TEST_METHOD(testBandPassFilterOrder0)
        {
            // order 0 : a single coefficients normalised to a gain of 1, the output is the input
            std::vector<float> input = { 1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f };
            signalProcessing<float> signal(input, 8, 30.0), output;
            signal.bandPassFilterFunction(lowFreqCutOff, highFreqCutOff, 0, output);
            UT_VERIFY_ALL_NEAR(output.getSignalSamples(), input, input.size(), 1e-6);
        }

        TEST_METHOD(testNLMSfilterOrder0)
        {
            // order 0 : the adaptive filters has no coefficient, nothing is removed
            std::vector<float> original = { 1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f };
            signalProcessing<float> reference(std::vector<float>(8, 1.0f), 8, 30.0), originalSignal(original, 8, 30.0), output;
            signalProcessing<float>::nlmsFilter(reference, originalSignal, 0, mu, output);
            UT_VERIFY(output.getSignalSamples()[0] == original[1]); // first sample replaced by the second one
            for (unsigned int n = 1; n < 8; n++) UT_VERIFY(output.getSignalSamples()[n] == original[n]);
        }

        TEST_METHOD(testFIRFilterFixedCoefficients)
        {
            // dsp::FIR_filt (used by bandPassFilterFunction) with fixed coefficient : y[n] = 0.5 x[n] + 0.5 x[n-1]
            dsp::FIR_filt filter;
            filter.set_coeffs({ 0.5, 0.5 });
            UT_VERIFY(filter(2.0) == 1.0);
            UT_VERIFY(filter(4.0) == 3.0);
            filter.nlms_adapt(10.0); // not an adaptive filter : the coefficient do not change
            UT_VERIFY(filter.coeffs() == std::vector<double>({ 0.5, 0.5 }));
        }

        TEST_METHOD(testPlotWithGNUPLOT)
        {
            // Smoke test of the SIG and FFT branche (only checks that no exception is thrown).
            // The data file does not exist : gnuplots (if it is installed) stops with an error, no window is opened.
            plotWithGNUPLOT("file_that_does_not_exist.txt", SIG);
            plotWithGNUPLOT("file_that_does_not_exist.txt", FFT);
        }
    };

} // namespace heart_rate_tests
