#include <QString>
#include <QtTest>
#include "videoProcessing.hpp"
#include "signalProcessing.hpp"

#define BLEUE 0
#define GREEN 1
#define RED 2

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
//    resetObj(videoProcessingObj);
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
