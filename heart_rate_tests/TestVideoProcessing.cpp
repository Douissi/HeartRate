// Unit tests of the videoProcessing class (port of the former QtTest class TestVideoProcessing).
// Test data used (copied next to the test dll by the build) : 73BPmin.mp4 and
// haarcascade_frontalface_alt2.xml (both from the solution "data" folder).

#include "CppUnitTest.h"
#include "TestUtils.hpp"

#include "videoProcessing.hpp"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace heart_rate_tests
{

static const char* const TEST_VIDEO_FILE = "73BPmin.mp4";
static const char* const FACE_CASCADE_FILE = "haarcascade_frontalface_alt2.xml";

TEST_CLASS(TestVideoProcessing)
{
public:

    TEST_METHOD(testVideoObjCreation)
    {
        videoProcessing videoObj;
        UT_VERIFY(!videoObj.capture.isOpened());
        UT_VERIFY(videoObj.Fs == 0.0);
        UT_VERIFY(videoObj.numberOfFrames == 0);
        UT_VERIFY(videoObj.foreHeadSignal.Fs == 0.0);
        UT_VERIFY(videoObj.foreHeadSignal.numberOfSamples == 0);
        UT_VERIFY(videoObj.backgroundSignal.Fs == 0.0);
        UT_VERIFY(videoObj.backgroundSignal.numberOfSamples == 0);
        UT_VERIFY(videoObj.width == 0);
        UT_VERIFY(videoObj.height == 0);

        videoProcessing videoProcessingObj(TEST_VIDEO_FILE);
        UT_VERIFY(videoProcessingObj.capture.isOpened());
        UT_VERIFY(videoProcessingObj.Fs == 30.0);
        UT_VERIFY(videoProcessingObj.numberOfFrames == 338);
        UT_VERIFY(videoProcessingObj.foreHeadSignal.Fs == 30.0);
        UT_VERIFY(videoProcessingObj.foreHeadSignal.numberOfSamples == 0);
        UT_VERIFY(videoProcessingObj.backgroundSignal.Fs == 30.0);
        UT_VERIFY(videoProcessingObj.backgroundSignal.numberOfSamples == 0);
        UT_VERIFY(videoProcessingObj.width == 640);
        UT_VERIFY(videoProcessingObj.height == 480);
    }

    TEST_METHOD(testFaceCascadeLoading)
    {
        videoProcessing videoProcessingObj(TEST_VIDEO_FILE);
        UT_VERIFY(videoProcessingObj.faceCascade.empty());
        videoProcessingObj.faceCascadeLoading(FACE_CASCADE_FILE);
        UT_VERIFY(!videoProcessingObj.faceCascade.empty());
    }

    TEST_METHOD(testCapturingFrames)
    {
        videoProcessing videoProcessingObj(TEST_VIDEO_FILE);
        UT_VERIFY(videoProcessingObj.capture.isOpened());
        UT_VERIFY(videoProcessingObj.frame.empty());
        UT_VERIFY(videoProcessingObj.channels[BLEUE].empty());
        UT_VERIFY(videoProcessingObj.channels[GREEN].empty());
        UT_VERIFY(videoProcessingObj.channels[RED].empty());

        videoProcessingObj.capturingFrames();

        UT_VERIFY(!videoProcessingObj.frame.empty());
        UT_VERIFY(!videoProcessingObj.channels[BLEUE].empty());
        UT_VERIFY(!videoProcessingObj.channels[GREEN].empty());
        UT_VERIFY(!videoProcessingObj.channels[RED].empty());
    }

    TEST_METHOD(testDetectFacesFromGreenChannelInVideo)
    {
        videoProcessing videoProcessingObj(TEST_VIDEO_FILE);
        videoProcessingObj.faceCascadeLoading(FACE_CASCADE_FILE);
        videoProcessingObj.capturingFrames();

        // check cascadeClassifier is not empty and has been corrreclty loaded
        UT_VERIFY(!videoProcessingObj.faceCascade.empty());

        // check frames and channels are not empty
        UT_VERIFY(!videoProcessingObj.frame.empty());
        UT_VERIFY(!videoProcessingObj.channels[BLEUE].empty());
        UT_VERIFY(!videoProcessingObj.channels[GREEN].empty());
        UT_VERIFY(!videoProcessingObj.channels[RED].empty());

        // check vector containing faces does not contain any rect for any face
        UT_VERIFY(videoProcessingObj.faces.size() == 0);

        videoProcessingObj.detectFacesFromGreenChannelInVideo();

        // check vector containing faces contains at least one face detected
        UT_VERIFY(videoProcessingObj.faces.size() > 0);
    }

    TEST_METHOD(testFaceForeHeadAndBackgroundMatrixCreation)
    {
        videoProcessing videoProcessingObj(TEST_VIDEO_FILE);
        videoProcessingObj.faceCascadeLoading(FACE_CASCADE_FILE);
        videoProcessingObj.capturingFrames();
        videoProcessingObj.detectFacesFromGreenChannelInVideo();

        // Check CV::Mat Faces / foreHead / background are empty
        // (the QtTest version checked elemSize() == 0 : with a Debug build on Windows, OpenCV's
        //  Mat::elemSize() raises an assertion on an empty matrix, so empty() is checked instead)
        UT_VERIFY(videoProcessingObj.face.empty());
        UT_VERIFY(videoProcessingObj.background.empty());
        UT_VERIFY(videoProcessingObj.foreHead.empty());

        // Check channel green is not empty
        UT_VERIFY(!videoProcessingObj.channels[GREEN].empty());

        // Load rectangles for background and foreHeads
        videoProcessingObj.backgroundRect = Rect(5, 116 , 128, 200);
        videoProcessingObj.foreHeads.push_back(Rect(5, 116 , 128, 200));
        /******* Face has been set by detectFacesFromGreenChannelInVideo ******/

        UT_VERIFY(videoProcessingObj.faces.size() > 0);
        UT_VERIFY(videoProcessingObj.foreHeads.size() > 0);

        videoProcessingObj.creatingFaceForeHeadAndBackGroundMatrixFromGreenChannel();

        // Check CV::Mat Faces / foreHead / background are not empty any more
        UT_VERIFY(!videoProcessingObj.face.empty());
        UT_VERIFY(!videoProcessingObj.background.empty());
        UT_VERIFY(!videoProcessingObj.foreHead.empty());
        UT_VERIFY(videoProcessingObj.face.elemSize() != 0);
        UT_VERIFY(videoProcessingObj.background.elemSize() != 0);
        UT_VERIFY(videoProcessingObj.foreHead.elemSize() != 0);
    }
};

} // namespace heart_rate_tests
