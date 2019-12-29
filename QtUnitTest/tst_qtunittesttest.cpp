#include <QString>
#include <QtTest>
#include "videoProcessing.hpp"
#include "signalProcessing.hpp"
#include <numeric>

namespace  ut {

template <class T>
void resetObj(T& v)
{
    v.~T();
    new (&v) T();
}

template <class T>
void resetObj(T& v, const std::string& fileName)
{
    v.~T();
    new (&v) T(fileName);
}

int rand()
{
    return std::rand()%1000;
}

template <typename T>
void loadSignalFromFile(const string& myFileString, vector<T>& signalVector)
{
    T values;
    fstream myfile;
    myfile.open(myFileString,fstream::in);

    if(myfile.is_open())
    {
        string line;
        while(myfile)
        {
            getline(myfile, line , ' ');
            myfile >> values;
            myfile.get();
            if(!myfile) break;
            signalVector.push_back(values);
        }
        myfile.close();
    }
    else
    {
        ASSERT(false, "Enable to open in reading mode the file testSaveSigToFile.txt");
    }
}

}


class TestVideoProcessing : public QObject
{
    Q_OBJECT

public:
    TestVideoProcessing(){}
    virtual ~TestVideoProcessing() {}

private Q_SLOTS:
    void testVideoObjCreation();
    void testFaceCascadeLoading();
    void testCapturingFrames();
    void testDetectFacesFromGreenChannelInVideo();
    void testFaceForeHeadAndBackgroundMatrixCreation();

};




class TestSignalProcessing : public QObject
{
    Q_OBJECT

public:
    TestSignalProcessing(){}
    virtual ~TestSignalProcessing() {}

private Q_SLOTS:
    void testSignalConstruct();
    void testSignalConstruct2();
    void testSignalConstruct3();
    void testSignalMeanValue();
    void testRemoveMeanValue();
    void testFillWithValue();
    void testFillWithVector();
    void testSaveSigToFile();
    void testFFTfunction();
    void testBandPassFilterFunction();
    void testNLMSfilter();
};



/**********************************************************************************************************/
/**********************************************************************************************************/
/**********************************************************************************************************/
/**************************************** Test Video Processing class *************************************/
/**********************************************************************************************************/
/**********************************************************************************************************/
/**********************************************************************************************************/

void TestVideoProcessing::testVideoObjCreation()
{
    videoProcessing videoObj;
    QVERIFY(!videoObj.capture.isOpened());
    QVERIFY(videoObj.Fs == 0.0);
    QVERIFY(videoObj.numberOfFrames == 0);
    QVERIFY(videoObj.foreHeadSignal.Fs == 0.0);
    QVERIFY(videoObj.foreHeadSignal.numberOfSamples == 0);
    QVERIFY(videoObj.backgroundSignal.Fs == 0.0);
    QVERIFY(videoObj.backgroundSignal.numberOfSamples == 0);
    QVERIFY(videoObj.width == 0);
    QVERIFY(videoObj.height == 0);

    videoProcessing videoProcessingObj("73BPmin.mp4");
    QVERIFY(videoProcessingObj.capture.isOpened());
    QVERIFY(videoProcessingObj.Fs == 30.0);
    QVERIFY(videoProcessingObj.numberOfFrames == 338);
    QVERIFY(videoProcessingObj.foreHeadSignal.Fs == 30.0);
    QVERIFY(videoProcessingObj.foreHeadSignal.numberOfSamples == 0);
    QVERIFY(videoProcessingObj.backgroundSignal.Fs == 30.0);
    QVERIFY(videoProcessingObj.backgroundSignal.numberOfSamples == 0);
    QVERIFY(videoProcessingObj.width == 640);
    QVERIFY(videoProcessingObj.height == 480);
}


void TestVideoProcessing::testFaceCascadeLoading()
{
    videoProcessing videoProcessingObj("73BPmin.mp4");
    QVERIFY(videoProcessingObj.faceCascade.empty());
    videoProcessingObj.faceCascadeLoading("/usr/local/share/opencv4/haarcascades/haarcascade_frontalface_alt2.xml");
    QVERIFY(!videoProcessingObj.faceCascade.empty());
}


void TestVideoProcessing::testCapturingFrames()
{
    videoProcessing videoProcessingObj("73BPmin.mp4");
    QVERIFY(videoProcessingObj.capture.isOpened());
    QVERIFY(videoProcessingObj.frame.empty());
    QVERIFY(videoProcessingObj.channels[BLEUE].empty());
    QVERIFY(videoProcessingObj.channels[GREEN].empty());
    QVERIFY(videoProcessingObj.channels[RED].empty());

    videoProcessingObj.capturingFrames();

    QVERIFY(!videoProcessingObj.frame.empty());
    QVERIFY(!videoProcessingObj.channels[BLEUE].empty());
    QVERIFY(!videoProcessingObj.channels[GREEN].empty());
    QVERIFY(!videoProcessingObj.channels[RED].empty());
}



void TestVideoProcessing::testDetectFacesFromGreenChannelInVideo()
{
    videoProcessing videoProcessingObj("73BPmin.mp4");
    videoProcessingObj.faceCascadeLoading("/usr/local/share/opencv4/haarcascades/haarcascade_frontalface_alt2.xml");
    videoProcessingObj.capturingFrames();

    // check cascadeClassifier is not empty and has been corrreclty loaded
    QVERIFY(!videoProcessingObj.faceCascade.empty());

    // check frames and channels are not empty
    QVERIFY(!videoProcessingObj.frame.empty());
    QVERIFY(!videoProcessingObj.channels[BLEUE].empty());
    QVERIFY(!videoProcessingObj.channels[GREEN].empty());
    QVERIFY(!videoProcessingObj.channels[RED].empty());

    // check vector containing faces does not contain any rect for any face
    QVERIFY(videoProcessingObj.faces.size() == 0);

    videoProcessingObj.detectFacesFromGreenChannelInVideo();

    // check vector containing faces contains at least one face detected
    QVERIFY(videoProcessingObj.faces.size() > 0);
}


void TestVideoProcessing::testFaceForeHeadAndBackgroundMatrixCreation()
{
    // Check CV::Mat Faces / foreHead / background are empty
    //    QVERIFY(videoProcessingObj.face.elemSize() == 0);
    //    QVERIFY(videoProcessingObj.background.elemSize() == 0);
    //    QVERIFY(videoProcessingObj.foreHead.elemSize() == 0);

    //    // Check channel green is not empty
    //    QVERIFY(!videoProcessingObj.channels[GREEN].empty());

    //    // Load rectangles for background and foreHeads
    //    videoProcessingObj.backgroundRect = Rect(5, 116 , 128, 200);
    //    videoProcessingObj.foreHeads.push_back(Rect(5, 116 , 128, 200));
    //    /******* Face has been set in previous test ******/

    //    QVERIFY(videoProcessingObj.faces.size() > 0);
    //    QVERIFY(videoProcessingObj.foreHeads.size() > 0);

    //    videoProcessingObj.creatingFaceForeHeadAndBackGroundMatrixFromGreenChannel();

    //    // Check CV::Mat Faces / foreHead / background are not empty any more
    //    QVERIFY(videoProcessingObj.face.elemSize() != 0);
    //    QVERIFY(videoProcessingObj.background.elemSize() != 0);
    //    QVERIFY(videoProcessingObj.foreHead.elemSize() != 0);
}



/**********************************************************************************************************/
/**********************************************************************************************************/
/**********************************************************************************************************/
/**********************************************************************************************************/
/******************************************** Test Signal Processing class ********************************/
/**********************************************************************************************************/
/**********************************************************************************************************/
/**********************************************************************************************************/
/**********************************************************************************************************/



void TestSignalProcessing::testSignalConstruct()
{
    signalProcessing<float> signal;
    QVERIFY(signal.Fs == 0.0);
    QVERIFY(signal.numberOfSamples == 0);
    QVERIFY(signal.signalSamples.size() == 0);

}


void TestSignalProcessing::testSignalConstruct2()
{
    signalProcessing<float> signal(50, 10.5);
    QVERIFY(signal.Fs == 10.5);
    QVERIFY(signal.numberOfSamples == 0);
    QVERIFY(signal.signalSamples.size() == 0);
}

void TestSignalProcessing::testSignalConstruct3()
{
    std::vector<float> signalVector(1000);
    std::generate(signalVector.begin(), signalVector.end(), std::rand);

    signalProcessing<float> signalProcessingObj(signalVector, signalVector.size(), 10.0);

    QVERIFY(signalProcessingObj.Fs == 10.0);
    QVERIFY(signalProcessingObj.numberOfSamples == signalVector.size());
    QVERIFY(signalProcessingObj.signalSamples.size() == signalVector.size());
    QVERIFY(std::equal(signalVector.begin(), signalVector.end(), signalProcessingObj.signalSamples.begin()));
}

void TestSignalProcessing::testSignalMeanValue()
{
    std::vector<float> signalVector(1000);
    std::generate(signalVector.begin(), signalVector.end(), ut::rand); // generate rand values less then 1000
    signalProcessing<float> signalProcessingObj(signalVector, signalVector.size(), 10.0);

    float meanValue = signalProcessingObj.meanVector();
    float average = accumulate( signalVector.begin(), signalVector.end(), 0.0)/signalVector.size();

    QVERIFY(average == meanValue);
}

void TestSignalProcessing::testRemoveMeanValue()
{
    std::vector<float> signalVector(1000);
    std::generate(signalVector.begin(), signalVector.end(), ut::rand); // generate rand values less then 1000
    signalProcessing<float> signalProcessingObj(signalVector, signalVector.size(), 10.0);

    QVERIFY(signalProcessingObj.meanVector() != 0);

    signalProcessingObj.removeMeanValue();
    QVERIFY(std::fabs(signalProcessingObj.meanVector()) < 0.00005);
}


void TestSignalProcessing::testFillWithValue()
{
    signalProcessing<float> signal;
    QVERIFY(signal.Fs == 0.0);
    QVERIFY(signal.numberOfSamples == 0);
    QVERIFY(signal.signalSamples.size() == 0);
    float value = static_cast<float>(ut::rand());
    signal.fillWithValue(value);
    QVERIFY(signal.signalSamples.size() == 1);
    QVERIFY(signal.signalSamples.back() == value);
}

void TestSignalProcessing::testFillWithVector()
{
    signalProcessing<float> signal(500, 10.5);
    QVERIFY(signal.Fs == 10.5);
    QVERIFY(signal.numberOfSamples == 0);
    QVERIFY(signal.signalSamples.size() == 0);

    std::vector<float> signalVector(500);
    std::generate(signalVector.begin(), signalVector.end(), ut::rand);

    signal.fillWithVector(signalVector);
    QVERIFY(signal.signalSamples.size() == 500);
    QVERIFY(std::equal(signalVector.begin(), signalVector.end(), signal.signalSamples.begin()));
}

void TestSignalProcessing::testSaveSigToFile()
{
    unsigned int size = 1000;
    std::vector<float> signalVector(size);
    std::generate(signalVector.begin(), signalVector.end(), ut::rand);

    signalProcessing<float> signalProcessingObj(signalVector, signalVector.size(), 10.0);

    QVERIFY(signalProcessingObj.Fs == 10.0);
    QVERIFY(signalProcessingObj.numberOfSamples == signalVector.size());
    QVERIFY(signalProcessingObj.signalSamples.size() == signalVector.size());
    QVERIFY(std::equal(signalVector.begin(), signalVector.end(), signalProcessingObj.signalSamples.begin()));

    signalProcessingObj.saveSigToFile("testSaveSigToFile.txt");

    std::vector<float> signalVectorVerification;
    signalVectorVerification.reserve(size);

    ut::loadSignalFromFile<float>("testSaveSigToFile.txt", signalVectorVerification);

    QVERIFY(std::equal(signalVectorVerification.begin(), signalVectorVerification.end(), signalProcessingObj.signalSamples.begin()));
}


void TestSignalProcessing::testFFTfunction()
{
    unsigned int size = 338;
    std::vector<float> signalInput;
    signalInput.reserve(size);
    std::vector<float> FFTSignalExpectedOutput;
    FFTSignalExpectedOutput.reserve(size/2 -1);

    ut::loadSignalFromFile<float>("testSignalInput.txt", signalInput);
    ut::loadSignalFromFile<float>("TestFFTexpectedResult.txt", FFTSignalExpectedOutput);

    signalProcessing<float> signalProcessingObj(signalInput, signalInput.size(), 30.0);
    signalProcessing<float> FFTSignalOutput;

    signalProcessingObj.signalFFT(FFTSignalOutput, DO_NOT_SAVE_FFT);

    // Issue with precision in float coming from *FFT result
    //QVERIFY(std::equal(FFTSignalExpectedOutput.begin(), FFTSignalExpectedOutput.end(), FFTSignalOutput.begin()));
    for(unsigned int i = 0; i < size/2 -1 ; i++)
    {
        QVERIFY(std::fabs(FFTSignalOutput.getSignalSamples()[i] - FFTSignalExpectedOutput[i]) < 0.05);
    }
}

void TestSignalProcessing::testBandPassFilterFunction()
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

    signalProcessing<float> signalProcessingObj(signalInput, signalInput.size(), 30.0);
    signalProcessing<float> signalOutputFiltred;
    signalProcessingObj.bandPassFilterFunction(lowFreqCutOff, highFreqCutOff, filterOrder, signalOutputFiltred);

    for(unsigned int i = 0; i < size ; i++)
    {
        QVERIFY(std::fabs(signalOutputFiltred.getSignalSamples()[i] - signalOutPutExpected[i]) < 0.00001);
    }
}

void TestSignalProcessing::testNLMSfilter()
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

    signalProcessing<float> signalProcessingInput1(signalInput1, signalInput1.size(), 30.0);
    signalProcessing<float> signalProcessingInput2(signalInput2, signalInput2.size(), 30.0);
    signalProcessing<float> signalProcessingOutput;

    signalProcessing<float>::nlmsFilter(signalProcessingInput1, signalProcessingInput2, 40, 0.5, signalProcessingOutput);

    for(unsigned int i = 0; i < size ; i++)
    {
        QVERIFY(std::fabs(signalProcessingOutput.getSignalSamples()[i] - signalExpectedOutput[i]) < 0.001);
    }

}


/**********************************************************************************************************/
/**********************************************************************************************************/
/**********************************************************************************************************/
/************************************** Main function, running unit tests *********************************/
/**********************************************************************************************************/
/**********************************************************************************************************/
/**********************************************************************************************************/

//QTEST_APPLESS_MAIN(TestVideoProcessing)
int main(int argc, char *argv[])
{
    TestVideoProcessing tc;
    TestSignalProcessing tc2;
    QTEST_SET_MAIN_SOURCE_PATH
            int resTc1 = QTest::qExec(&tc, argc, argv);
    int resTc2 = QTest::qExec(&tc2, argc, argv);
    return resTc1 || resTc2;
}


#include "tst_qtunittesttest.moc"
