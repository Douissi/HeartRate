#include <signalProcessing.hpp>

template <typename T>
signalProcessing<T>::signalProcessing() : numberOfSamples(0), Fs(0)
{}


template <typename T>
signalProcessing<T>::signalProcessing(const unsigned int& numOfSamples, const double& Fech) : numberOfSamples(0), Fs(Fech)
{
    signalSamples.reserve(numOfSamples); // WRN = please make sure when calling size() on this vector
}


template <typename T>
signalProcessing<T>::signalProcessing(const std::vector<T>& signalSamplesInput, const unsigned int &numOfSamples , const double &Fech) : numberOfSamples(0),
    Fs(Fech)
{
    functions::ASSERT(signalSamplesInput.size() == numOfSamples, " Error while constructing signalProcessing Obj :: numOfSamples incorrect and not equal to size of input vector ");
    signalSamples.reserve(numOfSamples);
    fillWithVector(signalSamplesInput);
}

template <typename T>
signalProcessing<T>::~signalProcessing()
{
    signalSamples.clear();
}

template <typename T>
T signalProcessing<T>::meanVector()
{
    T tmp=0;
    for (const auto& v : signalSamples) tmp += v;
    return tmp/numberOfSamples;
}

template <typename T>
void signalProcessing<T>::removeMeanValue()
{
    T meanSignal = meanVector();
    std::transform(signalSamples.begin(), signalSamples.end(),signalSamples.begin(),
                   [&meanSignal](T v) -> T {return v - meanSignal;});
}

template <typename T>
void signalProcessing<T>::fillWithValue(const T& value)
{
    signalSamples.push_back(value);
    numberOfSamples++;
}

template <typename T>
void signalProcessing<T>::fillWithVector(const std::vector<T>& inputVec)
{
    for(const auto& v : inputVec) signalSamples.push_back(v);
    numberOfSamples += inputVec.size();
}

template <typename T>
void signalProcessing<T>::reserve(const unsigned int &size)
{
    signalSamples.reserve(size);
}

template <typename T>
void signalProcessing<T>::resize(const unsigned int &size)
{
    signalSamples.resize(size);
    numberOfSamples = size;
}

template <typename T>
void signalProcessing<T>::clearSignal()
{
    signalSamples.clear(); // MAYBE we need to reserve again
    numberOfSamples = 0;
}

template <typename T>
void signalProcessing<T>::saveSigToFile(const string& inputFile)
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

template <typename T>
void signalProcessing<T>::signalFFT(signalProcessing<T>& FFTOutput, const bool& save, const string& FFTFile)
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


template <typename T>
void signalProcessing<T>::bandPassFilterFunction(const float& lowFreqCutOff, const float& highFreqCutOff, const int& filterOrder,
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


template <typename T>
void signalProcessing<T>::nlmsFilter(signalProcessing<T>& inputSignal, signalProcessing<T>& originalSignal, const int& filterOrder,
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

template <typename T>
float signalProcessing<T>::getFrequencyOfMaxFFT()
{
    return static_cast<float>(distance(signalSamples.begin(), max_element(signalSamples.begin(), signalSamples.end()))) * Fs/
            (2.0f * static_cast<float>(numberOfSamples));
}




// avoid linking issue to explicitly instantiate the template as float signals
template class signalProcessing<float>;
