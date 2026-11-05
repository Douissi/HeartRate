// Unit tests of the signalProcessing class (port of the former QtTest class TestSignalProcessing).

#include "CppUnitTest.h"
#include "TestUtils.hpp"

#include "signalProcessing.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <numeric>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace heart_rate_tests
{

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
        float average = static_cast<float>(std::accumulate(signalVector.begin(), signalVector.end(), 0.0)/static_cast<double>(signalVector.size()));

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
        FFTSignalExpectedOutput.reserve(size/2 -1);

        ut::loadSignalFromFile<float>("testSignalInput.txt", signalInput);
        ut::loadSignalFromFile<float>("TestFFTexpectedResult.txt", FFTSignalExpectedOutput);
        UT_VERIFY(signalInput.size() == size);

        signalProcessing<float> signalProcessingObj(signalInput, static_cast<unsigned int>(signalInput.size()), 30.0);
        signalProcessing<float> FFTSignalOutput;

        signalProcessingObj.signalFFT(FFTSignalOutput, DO_NOT_SAVE_FFT);

        // Issue with precision in float coming from *FFT result
        //UT_VERIFY(std::equal(FFTSignalExpectedOutput.begin(), FFTSignalExpectedOutput.end(), FFTSignalOutput.begin()));
        UT_VERIFY_ALL_NEAR(FFTSignalOutput.getSignalSamples(), FFTSignalExpectedOutput, size/2 -1, 0.05);

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
};

} // namespace heart_rate_tests
