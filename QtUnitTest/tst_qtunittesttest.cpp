#include <QString>
#include <QtTest>
#include "videoProcessing.hpp"
#include "signalProcessing.hpp"



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


class TestVideoProcessing : public QObject
{
    Q_OBJECT

public:
    TestVideoProcessing() : videoProcessingObj("73BPmin.mp4") {}
    virtual ~TestVideoProcessing() {}

private:

    videoProcessing videoProcessingObj;

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
    void testSignal1();
};



/********************************************* Testing methods *****************************************/

void TestVideoProcessing::testVideoObjCreation()
{
    videoProcessing videoObj;
    QVERIFY(videoObj.Fs == 0.0);
    QVERIFY(videoObj.numberOfFrames == 0);
    QVERIFY(videoObj.foreHeadSignal.Fs == 0.0);
    QVERIFY(videoObj.foreHeadSignal.numberOfSamples == 0);
    QVERIFY(videoObj.backgroundSignal.Fs == 0.0);
    QVERIFY(videoObj.backgroundSignal.numberOfSamples == 0);

    QVERIFY(videoProcessingObj.capture.isOpened());
    QVERIFY(videoProcessingObj.Fs == 30.0);
    QVERIFY(videoProcessingObj.numberOfFrames == 338);
    QVERIFY(videoProcessingObj.foreHeadSignal.Fs == 30.0);
    QVERIFY(videoProcessingObj.foreHeadSignal.numberOfSamples == 338);
    QVERIFY(videoProcessingObj.backgroundSignal.Fs == 30.0);
    QVERIFY(videoProcessingObj.backgroundSignal.numberOfSamples == 338);
}


void TestVideoProcessing::testFaceCascadeLoading()
{
    QVERIFY(videoProcessingObj.faceCascade.empty());
    videoProcessingObj.faceCascadeLoading("/usr/local/share/opencv4/haarcascades/haarcascade_frontalface_alt2.xml");
    QVERIFY(!videoProcessingObj.faceCascade.empty());
}


void TestVideoProcessing::testCapturingFrames()
{
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
//    // check cascadeClassifier is not empty and has been corrreclty loaded
//    QVERIFY(!videoProcessingObj.faceCascade.empty());

//    // check frames and channels are not empty
//    QVERIFY(!videoProcessingObj.frame.empty());
//    QVERIFY(!videoProcessingObj.channels[BLEUE].empty());
//    QVERIFY(!videoProcessingObj.channels[GREEN].empty());
//    QVERIFY(!videoProcessingObj.channels[RED].empty());

//    // check vector containing faces does not contain any rect for any face
//    QVERIFY(videoProcessingObj.faces.size() == 0);

    videoProcessingObj.detectFacesFromGreenChannelInVideo();

//    // check vector containing faces contains at least one face detected
//    QVERIFY(videoProcessingObj.faces.size() > 0);
}


void TestVideoProcessing::testFaceForeHeadAndBackgroundMatrixCreation()
{
//    // Check CV::Mat Faces / foreHead / background are empty
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

void TestSignalProcessing::testSignal1()
{
    QVERIFY2(true, "Failure");
}















/********************************************** Main function, running unit tests ***************************/
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
