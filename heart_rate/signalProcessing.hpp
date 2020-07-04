#ifndef SIGNALPROCESSING_HPP
#define SIGNALPROCESSING_HPP
#include <functions.hpp>


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
    const unsigned int& getNbOfSamples() const { return numberOfSamples; }

    //***************************************setters ********************************
    void setFs(const double& Fech) { Fs = Fech; }
    void setNbOfSamples(const unsigned int& nbOfSamples) { numberOfSamples = nbOfSamples; }

private:

    std::vector <T> signalSamples;
    unsigned int numberOfSamples;
    double Fs;

};


#endif // SIGNALPROCESSING_HPP
