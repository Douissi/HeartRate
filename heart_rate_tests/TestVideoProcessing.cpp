// Unit tests of the videoProcessing class (port of the former QtTest class TestVideoProcessing).
// Test data used (copied next to the test dll by the build) : test.mp4 (30 fps, 301 frames, 528x592) and
// haarcascade_frontalface_alt2.xml (both from the solution "data" folder).

#include "CppUnitTest.h"
#include "TestUtils.hpp"

#include "videoProcessing.hpp"
#include "signalPlot.hpp"

#include <cmath>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>
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

    // Heart rate of TEST_VIDEO_FILE (beats per minute), estimated as in main() from the forehead green
    // channel : the highest peak of the power spectrum of the mean-removed, band-pass filtered
    // (0.5 - 4.5 Hz) forehead signal (301 samples at 30 fps -> bin 9 -> 0.9 Hz -> 54 bpm).
#define TEST_VIDEO_HEART_RATE_BPM 54.0

// Plots of the heart rate tests : the band-pass filtered forehead signal(s) and their spectrum (FFT).
//   1 : the plots are saved as .png next to the test dll and displayed in windows; each test
//       waits until a key is pressed or the windows are closed (or TEST_PLOTS_DISPLAY_TIME_MS)
//   0 : no plot (e.g. for automatic runs)
#define TEST_SHOW_PLOTS 1
// 0 : the windows stay open until a key is pressed / they are closed; > 0 : closed after this time (ms)
#define TEST_PLOTS_DISPLAY_TIME_MS 0

// Time window of testHeartRateEstimationFromForeHeadGreenChannelTimeWindow (seconds of TEST_VIDEO_FILE) :
// only the frames with a time (frame index / fps) in [TEST_WINDOW_START_S, TEST_WINDOW_END_S[ are used.
#define TEST_WINDOW_START_S 5.0
#define TEST_WINDOW_END_S 8.0
// Zero padding of the FFT of this test : the 90 samples of the window are completed with zeros up to
// this length (signalFFT option), the spectrum has a value every 30 / 1024 Hz = 1.76 bpm instead of
// every 30 / 90 Hz = 20 bpm.
#define TEST_WINDOW_FFT_LENGTH 1024

// Sliding window of testHeartRateEstimationFromForeHeadGreenChannelSlidingWindow : windows of
// TEST_SLIDING_WINDOW_LENGTH_S seconds, one every TEST_SLIDING_WINDOW_STEP_S seconds (0 - 4.5 s,
// 1 - 5.5 s, 2 - 6.5 s ...), FFT zero padded to TEST_SLIDING_WINDOW_FFT_LENGTH points.
// Same values as the application : the constants of signalProcessing.hpp used by main().
#define TEST_SLIDING_WINDOW_LENGTH_S slidingWindowLengthSeconds
#define TEST_SLIDING_WINDOW_STEP_S slidingWindowStepSeconds
#define TEST_SLIDING_WINDOW_FFT_LENGTH slidingWindowFFTLength

// Result of the processing of one sliding window
    struct SlidingWindowResult
    {
        double startTime = 0.0;                  // time of the first sample of the window in the video (s)
        double endTime = 0.0;                    // time after the last sample (s)
        signalProcessing<float> filtredSig;      // band-pass filtered forehead signal of the window
        signalProcessing<float> FFTfiltredSig;   // its power spectrum (zero padded)
        float heartRateBpm = 0.0f;               // estimation : frequency of the maximum of the spectrum x 60
    };

#if TEST_SHOW_PLOTS
    // One plot image : saved in 'file' (.png next to the test dll) and displayed in a window 'title' at (x, y)
    struct PlotWindow
    {
        std::string title;
        std::string file;
        cv::Mat image;
        int x;
        int y;
    };

    // Saves the images, displays them and waits until a key is pressed or the windows are closed
    // (TEST_PLOTS_DISPLAY_TIME_MS). A display problem does not make the test fail.
    static void saveAndShowPlots(const std::vector<PlotWindow>& plots)
    {
        try
        {
            std::string saved;
            for (const PlotWindow& p : plots)
                if (signal_plot::save(p.file, p.image)) saved += (saved.empty() ? "" : ", ") + p.file;
            if (!saved.empty()) Logger::WriteMessage(("Plots saved : " + saved + "\n").c_str());

            if (!signal_plot::displayAvailable())
            {
                Logger::WriteMessage("No display : the plots are not shown in windows\n");
                return;
            }
            std::vector<std::string> windows;
            for (const PlotWindow& p : plots)
            {
                signal_plot::show(p.title, p.image, p.x, p.y);
                windows.push_back(p.title);
            }
            Logger::WriteMessage("Plots displayed : press a key or close the windows to continue\n");
            signal_plot::waitForWindows(windows, TEST_PLOTS_DISPLAY_TIME_MS);
        }
        catch (const cv::Exception& e)
        {
            Logger::WriteMessage((std::string("The plots could not be saved / displayed : ") + e.what() + "\n").c_str());
        }
    }

    // Renders, saves and displays the filtered forehead signal and its spectrum.
    //   description : part of the video used (titles of the plots and of the windows)
    //   fileTag     : added to the names of the .png files ("testHeartRate<fileTag>_....png")
    //   startTime   : time of the first sample in the video (s)
    static void plotForeHeadSignalAndFFT(signalProcessing<float>& filtredSig, signalProcessing<float>& FFTfiltredForeHead,
        const std::string& description, const std::string& fileTag, double startTime)
    {
        const cv::Mat signalImage = signal_plot::render(
            signal_plot::signalPlotData("Forehead green channel after the band-pass filter (test.mp4, " + description + ")",
                filtredSig.getSignalSamples(), filtredSig.getFs(), startTime), 900, 320);
        const cv::Mat fftImage = signal_plot::render(
            signal_plot::spectrumPlotData("FFT of the filtered forehead signal (" + description + ")", FFTfiltredForeHead.getSignalSamples(),
                FFTfiltredForeHead.getFs(), FFTfiltredForeHead.getFFTSize(), 6.0, lowFreqCutOff,
                highFreqCutOff), 900, 320);

        saveAndShowPlots({
            { "Filtered forehead signal (" + description + ")", "testHeartRate" + fileTag + "_filteredForeHeadSignal.png", signalImage, 40, 40 },
            { "FFT of the filtered forehead signal (" + description + ")", "testHeartRate" + fileTag + "_FFTfilteredForeHead.png", fftImage,
              40, 40 + signalImage.rows + 50 } });
    }

    // Renders, saves and displays the filtered signals of all the sliding windows (one image, one plot
    // per window) and their spectra (one image).
    static void plotSlidingWindows(std::vector<SlidingWindowResult>& results)
    {
        std::vector<cv::Mat> signalPlots, fftPlots;
        for (std::size_t i = 0; i < results.size(); i++)
        {
            SlidingWindowResult& r = results[i];
            std::ostringstream name;
            name << "Window " << i + 1 << " : " << std::fixed << std::setprecision(1) << r.startTime << " s - " << r.endTime << " s";
            signalPlots.push_back(signal_plot::render(
                signal_plot::signalPlotData(name.str(), r.filtredSig.getSignalSamples(), r.filtredSig.getFs(), r.startTime), 460, 230));
            fftPlots.push_back(signal_plot::render(
                signal_plot::spectrumPlotData("FFT " + name.str(), r.FFTfiltredSig.getSignalSamples(), r.FFTfiltredSig.getFs(),
                    r.FFTfiltredSig.getFFTSize(), 6.0, lowFreqCutOff, highFreqCutOff), 460, 230));
        }
        const cv::Mat signals = signal_plot::tile(signalPlots, 3);
        const cv::Mat ffts = signal_plot::tile(fftPlots, 3);
        saveAndShowPlots({
            { "Sliding windows : filtered forehead signals", "testHeartRateSliding_filteredForeHeadSignals.png", signals, 20, 20 },
            { "Sliding windows : FFT of the filtered forehead signals", "testHeartRateSliding_FFTfilteredForeHead.png", ffts,
              20, 20 + signals.rows + 40 } });
    }
#endif

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
            videoProcessingObj.backgroundRect = Rect(5, 116, 128, 200);
            videoProcessingObj.foreHeads.push_back(Rect(5, 116, 128, 200));
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
        // Error cases, end of video, signal extraction, drawings, getters / setters
        // (displayFrame() is not tested : it open a window with cv::imshow)
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

            // every frame announced by the video has been reads, then the end of the video is reported
            UT_VERIFY(nbOfFrames == videoProcessingObj.getNumOfFrames());
            UT_VERIFY(videoProcessingObj.isFrameEmpty());
            // at the end of the video the channel keep the last valid frames
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

            // the matrices have the size of their rectangless
            videoProcessingObj.creatingFaceForeHeadAndBackGroundMatrixFromGreenChannel();
            UT_VERIFY(videoProcessingObj.getforeHead().cols == foreHeads[0].width);
            UT_VERIFY(videoProcessingObj.getforeHead().rows == foreHeads[0].height);
            UT_VERIFY(videoProcessingObj.getbackground().cols == background.width);
            UT_VERIFY(videoProcessingObj.getbackground().rows == background.height);
        }

        TEST_METHOD(testFillSignalsFromGreenChannel)
        {
            // synthetic 100 x 100 frames : green = 50 except the bottom-right quarter (= 200),
            // blue = 0 and red = 255 everywheres, so a region taken from a wrong channel is detected
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

            // one sample per call : the mean of the green chanel inside each rectangle
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

        TEST_METHOD(testHeartRateEstimationFromForeHeadGreenChannel)
        {
            // Same processing as main() (heart_rate/main.cpp), forehead green channel only :
            // every frame of the video -> face detection -> forehead rectangle -> mean of the green
            // channel = one sample, then mean value removed, band-pass filter, FFT, and the frequency of
            // the maximum of the spectrum is the heart rate.
            videoProcessing videoObj(TEST_VIDEO_FILE);
            videoObj.faceCascadeLoading(FACE_CASCADE_FILE);

            std::vector<Rect> lastDetectedFaces;
            unsigned int nbOfFrames = 0;

            while (true)
            {
                videoObj.capturingFrames();
                if (videoObj.isFrameEmpty()) break;
                nbOfFrames++;

                videoObj.detectFacesFromGreenChannelInVideo();

                // No face detected in this frame : keep the last detected face (or skip the frame if none yet)
                if (videoObj.getFaces().empty())
                {
                    if (lastDetectedFaces.empty()) continue;
                    videoObj.setRectFacesVector(lastDetectedFaces);
                }
                lastDetectedFaces = videoObj.getFaces();

                const std::vector<Rect>& faces = videoObj.getFaces();
                // background rectangle as in main() (needed to build the matrices, its signal is not used here)
                videoObj.setbackGroundRect(Rect(faces[0].x + faces[0].width, faces[0].y + faces[0].height,
                    videoObj.width - (faces[0].x + faces[0].width), videoObj.height - (faces[0].y + faces[0].height)));

                std::vector<Rect> foreHeads = faces;
                foreHeads[0].height = static_cast<int>(foreHeads[0].height / 3.7);
                foreHeads[0].x = foreHeads[0].x + 50;
                foreHeads[0].width = foreHeads[0].width - 70;
                videoObj.setRectForeHeadsVector(foreHeads);

                videoObj.creatingFaceForeHeadAndBackGroundMatrixFromGreenChannel();
                videoObj.fillforeHeadSignal(); // mean of the forehead green channel for this frame
            }

            // the signal contains one sample per frame of the whole video
            signalProcessing<float>& foreHeadSignal = videoObj.getforeHeadSignal();
            UT_VERIFY(nbOfFrames == TEST_VIDEO_FRAMES);
            UT_VERIFY(foreHeadSignal.getNbOfSamples() == TEST_VIDEO_FRAMES);
            UT_VERIFY(foreHeadSignal.getFs() == TEST_VIDEO_FPS);

            // remove the mean value "offset" of the signal
            foreHeadSignal.removeMeanValue();
            UT_VERIFY(std::fabs(foreHeadSignal.meanVector()) < 1e-3);

            // band-pass filter [lowFreqCutOff, highFreqCutOff] = [0.5 Hz, 4.5 Hz] = [30, 270] bpm, as in main() :
            // the filtered signal only contains the possible heart rate frequencies (the slow variations of
            // the illumination / movements below 0.5 Hz are removed)
            signalProcessing<float> filtredSig;
            foreHeadSignal.bandPassFilterFunction(lowFreqCutOff, highFreqCutOff, filterOrder, filtredSig);
            UT_VERIFY(filtredSig.getNbOfSamples() == TEST_VIDEO_FRAMES);
            UT_VERIFY(filtredSig.getFs() == TEST_VIDEO_FPS);

            // power spectrum of the filtered signal and frequency of its maximum = heart rate
            signalProcessing<float> FFTfiltredForeHead;
            filtredSig.signalFFT(FFTfiltredForeHead, DO_NOT_SAVE_FFT);
            UT_VERIFY(FFTfiltredForeHead.getNbOfSamples() == TEST_VIDEO_FRAMES / 2);

            const float heartRateBpm = std::round(60.0f * FFTfiltredForeHead.getFrequencyOfMaxFFT()); // in beats per minute
            Logger::WriteMessage((L"Estimated heart rate : " + std::to_wstring(heartRateBpm) + L" bpm\n").c_str());

#if TEST_SHOW_PLOTS
            plotForeHeadSignalAndFFT(filtredSig, FFTfiltredForeHead, "whole video", "", 0.0); // before the check : also shown when it fails
#endif
            UT_VERIFY(heartRateBpm == TEST_VIDEO_HEART_RATE_BPM);
        }

        TEST_METHOD(testHeartRateEstimationFromForeHeadGreenChannelTimeWindow)
        {
            // Same processing as testHeartRateEstimationFromForeHeadGreenChannel, but the forehead signal
            // only contains the frames of the time window [TEST_WINDOW_START_S, TEST_WINDOW_END_S[ (5 s to
            // 8 s = 3 s = 90 samples at 30 fps) : the video is read from the beginning, the frames before
            // the window are skipped, the reading stops at the end of the window. The FFT is zero padded
            // (TEST_WINDOW_FFT_LENGTH) to read the peak of this short signal more precisely.
            videoProcessing videoObj(TEST_VIDEO_FILE);
            videoObj.faceCascadeLoading(FACE_CASCADE_FILE);

            const double Fs = videoObj.getFs();
            const unsigned int firstFrame = static_cast<unsigned int>(std::lround(TEST_WINDOW_START_S * Fs)); // time = frame / Fs
            const unsigned int endFrame = static_cast<unsigned int>(std::lround(TEST_WINDOW_END_S * Fs));     // first frame after the window
            UT_VERIFY(firstFrame < endFrame && endFrame <= videoObj.getNumOfFrames());

            std::vector<Rect> lastDetectedFaces;
            unsigned int frameIndex = 0;

            for (; frameIndex < endFrame; frameIndex++)
            {
                videoObj.capturingFrames();
                if (videoObj.isFrameEmpty()) break;

                if (frameIndex < firstFrame) continue; // before the window : frame read, not used

                videoObj.detectFacesFromGreenChannelInVideo();

                // No face detected in this frame : keep the last detected face (or skip the frame if none yet)
                if (videoObj.getFaces().empty())
                {
                    if (lastDetectedFaces.empty()) continue;
                    videoObj.setRectFacesVector(lastDetectedFaces);
                }
                lastDetectedFaces = videoObj.getFaces();

                const std::vector<Rect>& faces = videoObj.getFaces();
                // background rectangle as in main() (needed to build the matrices, its signal is not used here)
                videoObj.setbackGroundRect(Rect(faces[0].x + faces[0].width, faces[0].y + faces[0].height,
                    videoObj.width - (faces[0].x + faces[0].width), videoObj.height - (faces[0].y + faces[0].height)));

                std::vector<Rect> foreHeads = faces;
                foreHeads[0].height = static_cast<int>(foreHeads[0].height / 3.7);
                foreHeads[0].x = foreHeads[0].x + 50;
                foreHeads[0].width = foreHeads[0].width - 70;
                videoObj.setRectForeHeadsVector(foreHeads);

                videoObj.creatingFaceForeHeadAndBackGroundMatrixFromGreenChannel();
                videoObj.fillforeHeadSignal(); // mean of the forehead green channel for this frame
            }

            // the video has been read until the end of the window, the signal contains one sample per
            // frame of the window
            signalProcessing<float>& foreHeadSignal = videoObj.getforeHeadSignal();
            const unsigned int N = endFrame - firstFrame;
            UT_VERIFY(frameIndex == endFrame);
            UT_VERIFY(foreHeadSignal.getNbOfSamples() == N);
            UT_VERIFY(foreHeadSignal.getFs() == TEST_VIDEO_FPS);

            // remove the mean value "offset" of the signal
            foreHeadSignal.removeMeanValue();
            UT_VERIFY(std::fabs(foreHeadSignal.meanVector()) < 1e-3);

            // band-pass filter [lowFreqCutOff, highFreqCutOff] = [0.5 Hz, 4.5 Hz] = [30, 270] bpm, as in main()
            signalProcessing<float> filtredSig;
            foreHeadSignal.bandPassFilterFunction(lowFreqCutOff, highFreqCutOff, filterOrder, filtredSig);
            UT_VERIFY(filtredSig.getNbOfSamples() == N);

            // power spectrum of the filtered signal, zero padded to TEST_WINDOW_FFT_LENGTH points, and frequency
            // of its maximum = heart rate
            signalProcessing<float> FFTfiltredForeHead;
            filtredSig.signalFFT(FFTfiltredForeHead, DO_NOT_SAVE_FFT, " ", TEST_WINDOW_FFT_LENGTH);
            UT_VERIFY(FFTfiltredForeHead.getFFTSize() == TEST_WINDOW_FFT_LENGTH);
            UT_VERIFY(FFTfiltredForeHead.getNbOfSamples() == TEST_WINDOW_FFT_LENGTH / 2);

            const float heartRateBpm = std::round(60.0f * FFTfiltredForeHead.getFrequencyOfMaxFFT()); // in beats per minute

            // With the zero padding the peak is read every Fs / TEST_WINDOW_FFT_LENGTH Hz (1.76 bpm), but a
            // 3 s signal only separates frequencies Fs / N Hz apart (resolution 20 bpm) : the estimation is
            // correct when it is less than half of this resolution (10 bpm) away from the heart rate of the video.
            const float resolutionBpm = static_cast<float>(60.0 * Fs / N);
            const float fftStepBpm = static_cast<float>(60.0 * Fs / TEST_WINDOW_FFT_LENGTH);
            std::ostringstream window;
            window << TEST_WINDOW_START_S << " s to " << TEST_WINDOW_END_S << " s";
            std::ostringstream msg;
            msg << "Estimated heart rate (" << window.str() << ") : " << heartRateBpm << " bpm (FFT of "
                << TEST_WINDOW_FFT_LENGTH << " points : one value every " << fftStepBpm << " bpm; resolution of the "
                << N << " samples : " << resolutionBpm << " bpm)\n";
            Logger::WriteMessage(msg.str().c_str());

#if TEST_SHOW_PLOTS
            plotForeHeadSignalAndFFT(filtredSig, FFTfiltredForeHead, window.str(), "Window", TEST_WINDOW_START_S); // before the check
#endif

            UT_VERIFY(std::fabs(heartRateBpm - TEST_VIDEO_HEART_RATE_BPM) < resolutionBpm / 2.0f);
        }

        TEST_METHOD(testHeartRateEstimationFromForeHeadGreenChannelSlidingWindow)
        {
            // Sliding window processing. The video is read frame by frame and the forehead signal grows by
            // one sample per frame (buffer). As soon as the buffer holds TEST_SLIDING_WINDOW_LENGTH_S (4.5 s)
            // of signal, the last 4.5 s are processed as in the other tests (mean removed, band-pass filter,
            // FFT zero padded, maximum = heart rate); then the reading goes on and the processing is
            // triggered again each time TEST_SLIDING_WINDOW_STEP_S (1 s) of new signal is available :
            // 0 - 4.5 s, 1 - 5.5 s, 2 - 6.5 s ... until the last complete window of the video
            // (10 s video : 6 windows, the last one 5 - 9.5 s).
            videoProcessing videoObj(TEST_VIDEO_FILE);
            videoObj.faceCascadeLoading(FACE_CASCADE_FILE);

            const double Fs = videoObj.getFs();
            const unsigned int windowLength = static_cast<unsigned int>(std::lround(TEST_SLIDING_WINDOW_LENGTH_S * Fs)); // 135 samples
            const unsigned int step = static_cast<unsigned int>(std::lround(TEST_SLIDING_WINDOW_STEP_S * Fs));          // 30 samples
            UT_VERIFY(windowLength >= 2 && step >= 1 && windowLength <= videoObj.getNumOfFrames());

            signalProcessing<float>& foreHeadSignal = videoObj.getforeHeadSignal(); // buffer : one sample per frame
            std::vector<SlidingWindowResult> results;
            std::vector<Rect> lastDetectedFaces;
            unsigned int nbOfFrames = 0;

            while (true)
            {
                videoObj.capturingFrames();
                if (videoObj.isFrameEmpty()) break;
                nbOfFrames++;

                videoObj.detectFacesFromGreenChannelInVideo();

                // No face detected in this frame : keep the last detected face (or skip the frame if none yet)
                if (videoObj.getFaces().empty())
                {
                    if (lastDetectedFaces.empty()) continue;
                    videoObj.setRectFacesVector(lastDetectedFaces);
                }
                lastDetectedFaces = videoObj.getFaces();

                const std::vector<Rect>& faces = videoObj.getFaces();
                // background rectangle as in main() (needed to build the matrices, its signal is not used here)
                videoObj.setbackGroundRect(Rect(faces[0].x + faces[0].width, faces[0].y + faces[0].height,
                    videoObj.width - (faces[0].x + faces[0].width), videoObj.height - (faces[0].y + faces[0].height)));

                std::vector<Rect> foreHeads = faces;
                foreHeads[0].height = static_cast<int>(foreHeads[0].height / 3.7);
                foreHeads[0].x = foreHeads[0].x + 50;
                foreHeads[0].width = foreHeads[0].width - 70;
                videoObj.setRectForeHeadsVector(foreHeads);

                videoObj.creatingFaceForeHeadAndBackGroundMatrixFromGreenChannel();
                videoObj.fillforeHeadSignal(); // new sample in the buffer

                // A window is complete : the first one when the buffer holds windowLength samples, then each
                // time step new samples have been added. The window is the last windowLength samples.
                const unsigned int nbOfSamples = foreHeadSignal.getNbOfSamples();
                if (nbOfSamples < windowLength || (nbOfSamples - windowLength) % step != 0) continue;

                const std::vector<float>& buffer = foreHeadSignal.getSignalSamples();
                signalProcessing<float> window(std::vector<float>(buffer.end() - windowLength, buffer.end()), windowLength, Fs);

                SlidingWindowResult result;
                result.startTime = (nbOfSamples - windowLength) / Fs;
                result.endTime = nbOfSamples / Fs;
                window.removeMeanValue();
                window.bandPassFilterFunction(lowFreqCutOff, highFreqCutOff, filterOrder, result.filtredSig);
                result.filtredSig.signalFFT(result.FFTfiltredSig, DO_NOT_SAVE_FFT, " ", TEST_SLIDING_WINDOW_FFT_LENGTH);
                result.heartRateBpm = std::round(60.0f * result.FFTfiltredSig.getFrequencyOfMaxFFT()); // in beats per minute

                std::ostringstream msg;
                msg << "Window " << results.size() + 1 << " : " << std::fixed << std::setprecision(1) << result.startTime
                    << " s - " << result.endTime << " s -> estimated heart rate " << std::setprecision(0) << result.heartRateBpm << " bpm\n";
                Logger::WriteMessage(msg.str().c_str());
                results.push_back(result);
            }

            // a face is found from the first frame : one sample per frame, sample n is at time n / Fs
            UT_VERIFY(nbOfFrames == TEST_VIDEO_FRAMES);
            UT_VERIFY(foreHeadSignal.getNbOfSamples() == nbOfFrames);

            // every complete window has been processed : (301 - 135) / 30 + 1 = 6 windows, one every second
            const std::size_t expectedWindows = (nbOfFrames - windowLength) / step + 1;
            UT_VERIFY(results.size() == expectedWindows);
            for (std::size_t i = 0; i < results.size(); i++)
            {
                UT_VERIFY(std::fabs(results[i].startTime - static_cast<double>(i * step) / Fs) < 1e-9);
                UT_VERIFY(results[i].filtredSig.getNbOfSamples() == windowLength);
                UT_VERIFY(results[i].FFTfiltredSig.getFFTSize() == TEST_SLIDING_WINDOW_FFT_LENGTH);
            }

#if TEST_SHOW_PLOTS
            plotSlidingWindows(results); // before the check : also shown when it fails
#endif

            // Each estimation is correct when it is less than half of the frequency resolution of a window
            // (60 / 4.5 s / 2 = 6.7 bpm) away from the heart rate of the video.
            const float toleranceBpm = static_cast<float>(0.5 * 60.0 * Fs / windowLength);
            for (const SlidingWindowResult& r : results)
                UT_VERIFY(std::fabs(r.heartRateBpm - TEST_VIDEO_HEART_RATE_BPM) < toleranceBpm);
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
            // colour image with two greys levels : 10 on the left half, 100 on the right half
            cv::Mat colour(40, 40, CV_8UC3, cv::Scalar(10, 10, 10));
            colour(Rect(20, 0, 20, 40)).setTo(cv::Scalar(100, 100, 100));
            cv::Mat grey;

            videoProcessing::imageColorToGreyTransform(colour, grey);

            UT_VERIFY(grey.type() == CV_8UC1);
            UT_VERIFY(grey.cols == 40 && grey.rows == 40);
            // histogram equalisation : the two level are stretched to 0 and 255
            UT_VERIFY(grey.at<uchar>(0, 0) == 0);
            UT_VERIFY(grey.at<uchar>(0, 39) == 255);
        }
        TEST_METHOD(testDisplayFrame)
        {
            // displayFrame draws the 3 rectangles on the frames, then show it in the window "Frame"
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
