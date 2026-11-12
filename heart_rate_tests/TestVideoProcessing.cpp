// Unit tests of the videoProcessing class (port of the former QtTest class TestVideoProcessing).
// Test data used (copied next to the test dll by the build) : test.mp4 (30 fps, 301 frames, 528x592) and
// haarcascade_frontalface_alt2.xml (both from the solution "data" folder).

#include "CppUnitTest.h"
#include "TestUtils.hpp"

#include "videoProcessing.hpp"

#include <stdexcept>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace heart_rate_tests
{

static const char* const TEST_VIDEO_FILE = "test.mp4";
static const char* const FACE_CASCADE_FILE = "haarcascade_frontalface_alt2.xml";

// Properties of TEST_VIDEO_FILE (checked with ffprobe)
static const double TEST_VIDEO_FPS = 30.0;
static const unsigned int TEST_VIDEO_FRAMES = 301;
static const int TEST_VIDEO_WIDTH = 528;
static const int TEST_VIDEO_HEIGHT = 592;

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
        UT_VERIFY(videoProcessingObj.numberOfFrames == 301);
        UT_VERIFY(videoProcessingObj.foreHeadSignal.Fs == 30.0);
        UT_VERIFY(videoProcessingObj.foreHeadSignal.numberOfSamples == 0);
        UT_VERIFY(videoProcessingObj.backgroundSignal.Fs == 30.0);
        UT_VERIFY(videoProcessingObj.backgroundSignal.numberOfSamples == 0);
        UT_VERIFY(videoProcessingObj.width == 528);
        UT_VERIFY(videoProcessingObj.height == 592);
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

    // ---------------------------------------------------------------------------------------------
    // Error cases, end of video, signal extraction, drawing, getters / setters
    // (displayFrame() is not tested : it opens a window with cv::imshow)
    // ---------------------------------------------------------------------------------------------

    TEST_METHOD(testVideoObjCreationMissingFile)
    {
        Assert::ExpectException<std::runtime_error>([] { videoProcessing videoProcessingObj("video_that_does_not_exist.mp4"); });
    }

    TEST_METHOD(testFaceCascadeLoadingMissingFile)
    {
        videoProcessing videoProcessingObj(TEST_VIDEO_FILE);
        Assert::ExpectException<std::runtime_error>([&] { videoProcessingObj.faceCascadeLoading("cascade_that_does_not_exist.xml"); });
        UT_VERIFY(videoProcessingObj.faceCascade.empty());
    }

    TEST_METHOD(testCapturingFramesUntilEndOfVideo)
    {
        videoProcessing videoProcessingObj(TEST_VIDEO_FILE);
        unsigned int nbOfFrames = 0;

        videoProcessingObj.capturingFrames();
        while (!videoProcessingObj.isFrameEmpty() && nbOfFrames < 10000)
        {
            nbOfFrames++;
            videoProcessingObj.capturingFrames();
        }

        // every frame announced by the video has been read, then the end of the video is reported
        UT_VERIFY(nbOfFrames == videoProcessingObj.getNumOfFrames());
        UT_VERIFY(videoProcessingObj.isFrameEmpty());
        // at the end of the video the channels keep the last valid frame
        UT_VERIFY(!videoProcessingObj.channels[GREEN].empty());
    }

    TEST_METHOD(testGettersAndSetters)
    {
        videoProcessing videoProcessingObj(TEST_VIDEO_FILE);
        UT_VERIFY(videoProcessingObj.getFs() == TEST_VIDEO_FPS);
        UT_VERIFY(videoProcessingObj.getNumOfFrames() == TEST_VIDEO_FRAMES);
        UT_VERIFY(videoProcessingObj.getforeHeadSignal().getFs() == TEST_VIDEO_FPS);
        UT_VERIFY(videoProcessingObj.getbackgroundSignal().getFs() == TEST_VIDEO_FPS);

        videoProcessingObj.capturingFrames();
        UT_VERIFY(videoProcessingObj.getframe().cols == TEST_VIDEO_WIDTH);
        UT_VERIFY(videoProcessingObj.getframe().rows == TEST_VIDEO_HEIGHT);

        const std::vector<Rect> faces = { Rect(10, 20, 100, 120) };
        const std::vector<Rect> foreHeads = { Rect(30, 25, 60, 30) };
        const Rect background(400, 450, 100, 100);
        videoProcessingObj.setRectFacesVector(faces);
        videoProcessingObj.setRectForeHeadsVector(foreHeads);
        videoProcessingObj.setbackGroundRect(background);
        UT_VERIFY(videoProcessingObj.getFaces() == faces);
        UT_VERIFY(videoProcessingObj.foreHeads == foreHeads); // no getter for the foreheads
        UT_VERIFY(videoProcessingObj.getbackgroundRect() == background);

        // the matrices have the size of their rectangles
        videoProcessingObj.creatingFaceForeHeadAndBackGroundMatrixFromGreenChannel();
        UT_VERIFY(videoProcessingObj.getforeHead().cols == foreHeads[0].width);
        UT_VERIFY(videoProcessingObj.getforeHead().rows == foreHeads[0].height);
        UT_VERIFY(videoProcessingObj.getbackground().cols == background.width);
        UT_VERIFY(videoProcessingObj.getbackground().rows == background.height);
    }

    TEST_METHOD(testFillSignalsFromGreenChannel)
    {
        // synthetic 100 x 100 frame : green = 50 except the bottom-right quarter (= 200),
        // blue = 0 and red = 255 everywhere, so a region taken from a wrong channel is detected
        videoProcessing videoProcessingObj;
        videoProcessingObj.channels[BLEUE] = cv::Mat(100, 100, CV_8UC1, cv::Scalar(0));
        videoProcessingObj.channels[GREEN] = cv::Mat(100, 100, CV_8UC1, cv::Scalar(50));
        videoProcessingObj.channels[GREEN](Rect(50, 50, 50, 50)).setTo(cv::Scalar(200));
        videoProcessingObj.channels[RED] = cv::Mat(100, 100, CV_8UC1, cv::Scalar(255));

        videoProcessingObj.setRectFacesVector({ Rect(0, 0, 60, 40) });
        videoProcessingObj.setRectForeHeadsVector({ Rect(10, 10, 20, 10) }); // green = 50
        videoProcessingObj.setbackGroundRect(Rect(60, 60, 30, 30));          // green = 200
        videoProcessingObj.creatingFaceForeHeadAndBackGroundMatrixFromGreenChannel();

        UT_VERIFY(videoProcessingObj.face.cols == 60 && videoProcessingObj.face.rows == 40);

        // one sample per call : the mean of the green channel inside each rectangle
        videoProcessingObj.fillforeHeadSignal();
        videoProcessingObj.fillbackgroundSignal();
        videoProcessingObj.fillforeHeadSignal();

        signalProcessing<float>& foreHeadSignal = videoProcessingObj.getforeHeadSignal();
        signalProcessing<float>& backgroundSignal = videoProcessingObj.getbackgroundSignal();
        UT_VERIFY(foreHeadSignal.getNbOfSamples() == 2);
        UT_VERIFY(foreHeadSignal.getSignalSamples()[0] == 50.0f);
        UT_VERIFY(foreHeadSignal.getSignalSamples()[1] == 50.0f);
        UT_VERIFY(backgroundSignal.getNbOfSamples() == 1);
        UT_VERIFY(backgroundSignal.getSignalSamples()[0] == 200.0f);
    }

    TEST_METHOD(testMeanFrameValue)
    {
        cv::Mat uniform(20, 30, CV_8UC1, cv::Scalar(42));
        UT_VERIFY(videoProcessing::meanFrameValue(uniform) == 42.0f);

        cv::Mat halfAndHalf(20, 30, CV_8UC1, cv::Scalar(0));
        halfAndHalf(Rect(0, 0, 15, 20)).setTo(cv::Scalar(100)); // left half = 100, right half = 0
        UT_VERIFY(videoProcessing::meanFrameValue(halfAndHalf) == 50.0f);
    }

    TEST_METHOD(testDrawRectangle)
    {
        cv::Mat image(100, 100, CV_8UC3, cv::Scalar(0, 0, 0));
        videoProcessing::drawRectangle(image, Rect(20, 30, 40, 20));

        const cv::Vec3b white(255, 255, 255), black(0, 0, 0);
        // at<>(row = y, column = x)
        UT_VERIFY(image.at<cv::Vec3b>(30, 20) == white); // top-left corner
        UT_VERIFY(image.at<cv::Vec3b>(50, 60) == white); // bottom-right corner (x + width, y + height)
        UT_VERIFY(image.at<cv::Vec3b>(40, 20) == white); // left side
        UT_VERIFY(image.at<cv::Vec3b>(40, 40) == black); // inside
        UT_VERIFY(image.at<cv::Vec3b>(5, 5) == black);   // outside
    }

    TEST_METHOD(testImageColorToGreyTransform)
    {
        // colour image with two grey levels : 10 on the left half, 100 on the right half
        cv::Mat colour(40, 40, CV_8UC3, cv::Scalar(10, 10, 10));
        colour(Rect(20, 0, 20, 40)).setTo(cv::Scalar(100, 100, 100));
        cv::Mat grey;

        videoProcessing::imageColorToGreyTransform(colour, grey);

        UT_VERIFY(grey.type() == CV_8UC1);
        UT_VERIFY(grey.cols == 40 && grey.rows == 40);
        // histogram equalisation : the two levels are stretched to 0 and 255
        UT_VERIFY(grey.at<uchar>(0, 0) == 0);
        UT_VERIFY(grey.at<uchar>(0, 39) == 255);
    }
    TEST_METHOD(testDisplayFrame)
    {
        // displayFrame draws the 3 rectangles on the frame, then shows it in the window "Frame"
        // (a window is opened : on Linux without display, run the tests with QT_QPA_PLATFORM=offscreen)
        videoProcessing videoProcessingObj;
        videoProcessingObj.frame = cv::Mat(100, 100, CV_8UC3, cv::Scalar(0, 0, 0));
        videoProcessingObj.setRectFacesVector({ Rect(10, 10, 60, 60) });
        videoProcessingObj.setRectForeHeadsVector({ Rect(20, 15, 40, 10) });
        videoProcessingObj.setbackGroundRect(Rect(75, 75, 20, 20));

        videoProcessingObj.displayFrame();

        const cv::Vec3b white(255, 255, 255), black(0, 0, 0);
        UT_VERIFY(videoProcessingObj.frame.at<cv::Vec3b>(10, 10) == white); // face, top-left corner
        UT_VERIFY(videoProcessingObj.frame.at<cv::Vec3b>(15, 20) == white); // forehead, top-left corner
        UT_VERIFY(videoProcessingObj.frame.at<cv::Vec3b>(95, 95) == white); // background, bottom-right corner
        UT_VERIFY(videoProcessingObj.frame.at<cv::Vec3b>(40, 40) == black); // inside the face, on no border
    }
};

} // namespace heart_rate_tests
