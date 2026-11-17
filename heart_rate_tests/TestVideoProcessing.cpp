// Unit tests of the videoProcessing class (port of the former QtTest class TestVideoProcessing).
// Test data used (copied next to the test dll by the build) : test.mp4 (30 fps, 301 frames, 528x592),
// test2.mp4 (30 fps, 300 frames, 570x718) and haarcascade_frontalface_alt2.xml (all from the solution
// "data" folder).

#include "CppUnitTest.h"
#include "TestUtils.hpp"

#include "videoProcessing.hpp"
#include "signalPlot.hpp"

#include <algorithm>
#include <cmath>
#include <functional>
#include <iomanip>
#include <limits>
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

    // Second reference video (the tests ending with Test2); properties checked with ffprobe
    static const char* const TEST2_VIDEO_FILE = "test2.mp4";
    static const double TEST2_VIDEO_FPS = 30.0;
    static const unsigned int TEST2_VIDEO_FRAMES = 300;

    // Heart rate of TEST2_VIDEO_FILE over the whole video (bpm), estimated as for TEST_VIDEO_FILE with the forehead
    // rectangle test2ForeHeadRectFromFace (300 samples : one bin every 6 bpm -> bin 9 -> 0.9 Hz -> 54 bpm; about 55 bpm
    // with a finer, zero padded FFT). One clear peak (the next highest value is about 1/3 of it), and every skin region
    // of the video (forehead, cheeks) gives 53 - 55 bpm when it is measured at a fixed position.
    // With the forehead rectangle of test.mp4 this video gives 90 bpm, which is not its heart rate : when the face is
    // detected in every frame, its Haar rectangle jitters at about 1.5 Hz (90 bpm) and that forehead rectangle, moved
    // with it, crosses the eyebrows and the background : the jitter becomes a 90 bpm brightness variation, stronger
    // than the pulse. (The optical flow tracking of main() does not jitter : 54 bpm with both rectangles.)
#define TEST2_VIDEO_HEART_RATE_BPM 54.0

    // Forehead rectangle of TEST2_VIDEO_FILE, as fractions of the detected face rectangle (it scales with the face; the
    // pixel offsets of test.mp4 do not fit this wider face, whose Haar rectangle starts in the middle of the forehead).
    // Limits measured on the 300 frames (in face widths / heights from the top-left corner of the face rectangle) :
    // eyebrows from y = +0.205, hairline at y = -0.14 at the lowest, specular highlight (shiny skin, no pulse) at
    // x = 0.15 - 0.25 and y = -0.20 - -0.08, temples beyond x = 0.78. The rectangle is the evenly lit skin inside these
    // limits, with a margin for the jitter of the face rectangle (up to 0.036 x height) : about 240 x 140 px, 70 % of it
    // above the face rectangle. It is clipped to the frame.
    static const double TEST2_FOREHEAD_LEFT = 0.27;    // fractions of the face width
    static const double TEST2_FOREHEAD_RIGHT = 0.75;
    static const double TEST2_FOREHEAD_TOP = -0.12;    // fractions of the face height (< 0 : above the face rectangle)
    static const double TEST2_FOREHEAD_BOTTOM = 0.16;

    // Forehead rectangle of a video, from the face rectangle of a frame of frameWidth x frameHeight pixels
    using ForeHeadRectFunction = Rect(*)(const Rect& face, int frameWidth, int frameHeight);

    // test.mp4 : the forehead of main() (videoProcessing::foreHeadRectFromFace : x + 50 px, width - 70 px, height / 3.7)
    static Rect testForeHeadRectFromFace(const Rect& face, int, int)
    {
        return videoProcessing::foreHeadRectFromFace(face);
    }

    static Rect test2ForeHeadRectFromFace(const Rect& face, int frameWidth, int frameHeight)
    {
        const int left = face.x + static_cast<int>(std::lround(TEST2_FOREHEAD_LEFT * face.width));
        const int right = face.x + static_cast<int>(std::lround(TEST2_FOREHEAD_RIGHT * face.width));
        const int top = face.y + static_cast<int>(std::lround(TEST2_FOREHEAD_TOP * face.height));
        const int bottom = face.y + static_cast<int>(std::lround(TEST2_FOREHEAD_BOTTOM * face.height));
        return Rect(left, top, right - left, bottom - top) & Rect(0, 0, frameWidth, frameHeight);
    }

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

// Sliding windows (testHeartRateEstimationFrom...SlidingWindow) : windows of TEST_SLIDING_WINDOW_LENGTH_S seconds,
// one every TEST_SLIDING_WINDOW_STEP_S seconds (0 - 4.5 s, 1 - 5.5 s, 2 - 6.5 s ...), FFT zero padded to
// TEST_SLIDING_WINDOW_FFT_LENGTH points. Same values as the application : the constants of signalProcessing.hpp.
#define TEST_SLIDING_WINDOW_LENGTH_S slidingWindowLengthSeconds
#define TEST_SLIDING_WINDOW_STEP_S slidingWindowStepSeconds
#define TEST_SLIDING_WINDOW_FFT_LENGTH slidingWindowFFTLength

// test2.mp4 (testHeartRateEstimationFromForeHeadGreenChannelSlidingWindowTest2) : 7 s windows instead of 4.5 s, same
// step and FFT length. Its pulse is much weaker than the one of test.mp4 (signal to noise ratio about 10 times lower) :
// with 4.5 s or 6 s windows some windows find a wrong peak (32 - 33 bpm, slow variations at the bottom of the heart
// rate band); from 7 s on, every window finds the pulse (7 s : 4 windows in the 10 s video; 8 s : 3, also correct).
#define TEST2_SLIDING_WINDOW_LENGTH_S 7.0
// The heart rate of test2.mp4 goes from about 58 bpm (first seconds) down to about 51 bpm (last seconds), the same in
// every region measured separately (forehead, each cheek, the 4 regions combined, with and without optical flow
// tracking) : a window measures the heart rate of its own seconds and can differ from TEST2_VIDEO_HEART_RATE_BPM
// (whole video) by up to about this value (bpm), whatever the precision of the processing.
#define TEST2_HEART_RATE_VARIATION_BPM 4.0

    // Heart rate estimated from a signal (estimateHeartRate) : the whole video, a time window or one sliding window
    struct HeartRateEstimate
    {
        double startTime = 0.0;                  // time of the first sample of the signal in the video (s)
        double endTime = 0.0;                    // time after the last sample (s)
        signalProcessing<float> filtredSig;      // band-pass filtered signal
        signalProcessing<float> FFTfiltredSig;   // its power spectrum (zero padded or not)
        float heartRateBpm = 0.0f;               // estimation : frequency of the maximum of the spectrum x 60
    };

    // "Window <index + 1> : <start> s - <end> s" (logs and plot titles of the sliding windows)
    static std::string windowName(std::size_t index, const HeartRateEstimate& window)
    {
        std::ostringstream name;
        name << "Window " << index + 1 << " : " << std::fixed << std::setprecision(1) << window.startTime << " s - "
            << window.endTime << " s";
        return name.str();
    }

    // Half of the frequency resolution (Fs / number of samples) of the signal of an estimation, in bpm : an estimation
    // is correct when it is less than this away from the heart rate (e.g. 60 x 30 / 301 / 2 = 3 bpm for the whole
    // video, 60 / 4.5 s / 2 = 6.7 bpm for a 4.5 s window).
    static float halfResolutionBpm(HeartRateEstimate& estimate)
    {
        return static_cast<float>(0.5 * 60.0 * estimate.filtredSig.getFs() / estimate.filtredSig.getNbOfSamples());
    }

    // Median of the heart rates of sliding windows (as the reference line of the sliding window plot of main())
    static float medianHeartRateBpm(const std::vector<HeartRateEstimate>& windows)
    {
        std::vector<float> estimations;
        for (const HeartRateEstimate& w : windows) estimations.push_back(w.heartRateBpm);
        std::sort(estimations.begin(), estimations.end());
        const std::size_t mid = estimations.size() / 2;
        return (estimations.size() % 2) ? estimations[mid] : (estimations[mid - 1] + estimations[mid]) / 2.0f;
    }

    // A window is complete when the signal holds windowLength samples, then each time step new samples are added
    static bool isWindowComplete(unsigned int nbOfSamples, unsigned int windowLength, unsigned int step)
    {
        return nbOfSamples >= windowLength && (nbOfSamples - windowLength) % step == 0;
    }

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

    // Renders, saves and displays the filtered signal of a region and its spectrum.
    //   regionName  : region the signal comes from (e.g. "ForeHead", "LeftCheek") : used in the plot
    //                 titles, the window titles and the names of the .png files
    //   description : part of the video used (titles of the plots and of the windows)
    //   fileTag     : added to the names of the .png files ("testHeartRate<fileTag>_....png")
    //   startTime   : time of the first sample in the video (s)
    //   videoName   : video the signal comes from (title of the signal plot)
    static void plotForeHeadSignalAndFFT(signalProcessing<float>& filtredSig, signalProcessing<float>& FFTfiltredForeHead,
        const std::string& description, const std::string& fileTag, double startTime, const std::string& regionName = "ForeHead",
        const std::string& videoName = TEST_VIDEO_FILE)
    {
        const cv::Mat signalImage = signal_plot::render(
            signal_plot::signalPlotData(regionName + " green channel after the band-pass filter (" + videoName + ", " + description + ")",
                filtredSig.getSignalSamples(), filtredSig.getFs(), startTime), 900, 320);
        const cv::Mat fftImage = signal_plot::render(
            signal_plot::spectrumPlotData("FFT of the filtered " + regionName + " signal (" + description + ")", FFTfiltredForeHead.getSignalSamples(),
                FFTfiltredForeHead.getFs(), FFTfiltredForeHead.getFFTSize(), 6.0, lowFreqCutOff,
                highFreqCutOff), 900, 320);

        saveAndShowPlots({
            { "Filtered " + regionName + " signal (" + description + ")", "testHeartRate" + fileTag + "_filtered" + regionName + "Signal.png", signalImage, 40, 40 },
            { "FFT of the filtered " + regionName + " signal (" + description + ")", "testHeartRate" + fileTag + "_FFTfiltered" + regionName + ".png", fftImage,
              40, 40 + signalImage.rows + 50 } });
    }

    // Renders, saves and displays the filtered signals of all the sliding windows (one image, one plot
    // per window) and their spectra (one image).
    //   signalName : what the windows contain (titles); fileTag : names of the .png files
    //   ("testHeartRate<fileTag>_filtered...Signals.png"). The defaults are the forehead sliding window test.
    static void plotSlidingWindows(std::vector<HeartRateEstimate>& results, const std::string& signalName = "filtered forehead signals",
        const std::string& fileTag = "Sliding")
    {
        std::vector<cv::Mat> signalPlots, fftPlots;
        for (std::size_t i = 0; i < results.size(); i++)
        {
            HeartRateEstimate& r = results[i];
            const std::string name = windowName(i, r);
            signalPlots.push_back(signal_plot::render(
                signal_plot::signalPlotData(name, r.filtredSig.getSignalSamples(), r.filtredSig.getFs(), r.startTime), 460, 230));
            fftPlots.push_back(signal_plot::render(
                signal_plot::spectrumPlotData("FFT " + name, r.FFTfiltredSig.getSignalSamples(), r.FFTfiltredSig.getFs(),
                    r.FFTfiltredSig.getFFTSize(), 6.0, lowFreqCutOff, highFreqCutOff), 460, 230));
        }
        const cv::Mat signals = signal_plot::tile(signalPlots, 3);
        const cv::Mat ffts = signal_plot::tile(fftPlots, 3);
        saveAndShowPlots({
            { "Sliding windows : " + signalName, "testHeartRate" + fileTag + "_filteredForeHeadSignals.png", signals, 20, 20 },
            { "Sliding windows : FFT of the " + signalName, "testHeartRate" + fileTag + "_FFTfilteredForeHead.png", ffts,
              20, 20 + signals.rows + 40 } });
    }
#endif

    // ---------------------------------------------------------------------------------------------
    // Acquisition : one sample per frame (mean of the green channel of a region), the face being
    //   - detected in every frame with the Haar cascade (fillDetectedForeHeadSignal : the forehead only), or
    //   - "method E" (fillTrackedRegionSignals) : detected only once, then given in every following frame by
    //     optical flow tracking (videoProcessing::initOpticalFlowTracking / trackOpticalFlow / trackedFace); the
    //     regions (videoProcessing::foreHeadRectFromFace, leftCheekRectFromFace, rightCheekRectFromFace,
    //     noseRectFromFace) are computed from that tracked face, as in main().
    // ---------------------------------------------------------------------------------------------

    // Reads the video and fills the forehead signal of videoObj, as main() for every frame : face detection ->
    // forehead rectangle (foreHeadRect, e.g. testForeHeadRectFromFace) -> mean of its green channel. When no face is
    // detected in a frame, the last detected face is kept (the frames before the first detected face are skipped).
    // Only the frames [firstFrame, endFrame[ are used : the frames before firstFrame are read but not used, the
    // reading stops after endFrame (default : the whole video). onNewSample (optional) is called after each new
    // sample (sliding windows). Returns the number of frames read.
    static unsigned int fillDetectedForeHeadSignal(videoProcessing& videoObj, ForeHeadRectFunction foreHeadRect,
        const std::function<void()>& onNewSample = nullptr, unsigned int firstFrame = 0,
        unsigned int endFrame = std::numeric_limits<unsigned int>::max())
    {
        std::vector<Rect> lastDetectedFaces;
        unsigned int nbOfFrames = 0;
        while (nbOfFrames < endFrame)
        {
            videoObj.capturingFrames();
            if (videoObj.isFrameEmpty()) break;
            if (nbOfFrames++ < firstFrame) continue; // before the time window : frame read, not used

            videoObj.detectFacesFromGreenChannelInVideo();

            // No face detected in this frame : keep the last detected face (or skip the frame if none yet)
            if (videoObj.getFaces().empty())
            {
                if (lastDetectedFaces.empty()) continue;
                videoObj.setRectFacesVector(lastDetectedFaces);
            }
            lastDetectedFaces = videoObj.getFaces();

            // rectangles of main() from the face (the background is needed to build the matrices, its signal is not
            // used here), then the forehead of this video
            const Rect face = lastDetectedFaces[0];
            videoObj.setRegionsFromFace(face);
            const Rect foreHead = foreHeadRect(face, videoObj.getframe().cols, videoObj.getframe().rows);
            UT_VERIFY(foreHead.area() > 0);
            videoObj.setRectForeHeadsVector({ foreHead });

            videoObj.creatingFaceForeHeadAndBackGroundMatrixFromGreenChannel();
            videoObj.fillforeHeadSignal(); // mean of the forehead green channel for this frame

            if (onNewSample) onNewSample();
        }
        return nbOfFrames;
    }

    // Reads the whole video and fills the forehead, left cheek, right cheek and nose signals of videoObj
    // (one sample per frame each). Same steps as main() for every frame, except that the face rectangle
    // comes from the optical flow tracking (the Haar cascade is only used until a face is found : the
    // frames before that, if any, are skipped, as in the other tests). Returns the number of frames read.
    // onNewSample (optional) is called after each new sample (sliding window test), as in main().
    static unsigned int fillTrackedRegionSignals(videoProcessing& videoObj, const std::function<void()>& onNewSample = nullptr)
    {
        unsigned int nbOfFrames = 0;
        while (true)
        {
            videoObj.capturingFrames();
            if (videoObj.isFrameEmpty()) break;
            nbOfFrames++;

            if (!videoObj.isOpticalFlowTracking())
            {
                videoObj.detectFacesFromGreenChannelInVideo();
                if (videoObj.getFaces().empty()) continue; // no face yet : frame skipped
                videoObj.initOpticalFlowTracking(videoObj.getFaces()[0]); // Haar cascade used once
            }
            else
            {
                videoObj.trackOpticalFlow(); // every following frame : optical flow only
            }

            // face of this frame (tracked), and the regions computed from it, as main() does from faces[0]
            // (face, background, forehead, cheeks and nose rectangles; the background is needed to build the
            // matrices, its signal is not used here)
            videoObj.setRegionsFromFace(videoObj.trackedFace());

            videoObj.creatingFaceForeHeadAndBackGroundMatrixFromGreenChannel();
            videoObj.creatingCheeksAndNoseMatrixFromGreenChannel();

            // one new sample (mean of the green channel of the region) in each signal
            videoObj.fillforeHeadSignal();
            videoObj.fillLeftCheekSignal();
            videoObj.fillRightCheekSignal();
            videoObj.fillNoseSignal();

            if (onNewSample) onNewSample();
        }
        return nbOfFrames;
    }

    // ---------------------------------------------------------------------------------------------
    // Processing and checks shared by the heart rate tests
    // ---------------------------------------------------------------------------------------------

    // Same processing as main() : mean value removed (in place), band-pass filter [lowFreqCutOff, highFreqCutOff] =
    // [0.5 Hz, 4.5 Hz] = [30, 270] bpm (the slow variations of the illumination / movements are removed), power
    // spectrum (zero padded to fftLength points, if given) and the frequency of its maximum = heart rate.
    // Checks every step : the signal holds expectedSamples samples at expectedFs Hz.
    static HeartRateEstimate estimateHeartRate(signalProcessing<float>& signal, unsigned int expectedSamples, double expectedFs,
        unsigned int fftLength = NO_ZERO_PADDING)
    {
        UT_VERIFY(signal.getNbOfSamples() == expectedSamples);
        UT_VERIFY(signal.getFs() == expectedFs);

        signal.removeMeanValue(); // remove the mean value "offset" of the signal
        UT_VERIFY(std::fabs(signal.meanVector()) < 1e-3);

        HeartRateEstimate estimate;
        signal.bandPassFilterFunction(lowFreqCutOff, highFreqCutOff, filterOrder, estimate.filtredSig);
        UT_VERIFY(estimate.filtredSig.getNbOfSamples() == expectedSamples);
        UT_VERIFY(estimate.filtredSig.getFs() == expectedFs);

        estimate.filtredSig.signalFFT(estimate.FFTfiltredSig, DO_NOT_SAVE_FFT, " ", fftLength);
        const unsigned int N = (fftLength == NO_ZERO_PADDING) ? expectedSamples : fftLength; // length of the FFT
        UT_VERIFY(estimate.FFTfiltredSig.getFFTSize() == N);
        UT_VERIFY(estimate.FFTfiltredSig.getNbOfSamples() == N / 2);

        estimate.heartRateBpm = std::round(60.0f * estimate.FFTfiltredSig.getFrequencyOfMaxFFT()); // in beats per minute
        estimate.endTime = expectedSamples / expectedFs;
        return estimate;
    }

    // Every complete window of a video of nbOfFrames frames has been processed, one every step samples (e.g.
    // (301 - 135) / 30 + 1 = 6 windows, the last one 5 - 9.5 s), each of windowLength samples, FFT zero padded.
    static void verifySlidingWindows(std::vector<HeartRateEstimate>& windows, unsigned int nbOfFrames, unsigned int windowLength,
        unsigned int step, double Fs)
    {
        UT_VERIFY(windows.size() == (nbOfFrames - windowLength) / step + 1);
        for (std::size_t i = 0; i < windows.size(); i++)
        {
            UT_VERIFY(std::fabs(windows[i].startTime - static_cast<double>(i * step) / Fs) < 1e-9);
            UT_VERIFY(windows[i].filtredSig.getNbOfSamples() == windowLength);
            UT_VERIFY(windows[i].FFTfiltredSig.getFFTSize() == TEST_SLIDING_WINDOW_FFT_LENGTH);
        }
    }

    // Weights of the 4 regions in heartRateSignal (buildHeartRateSignal) : one per region, in [0, 1], sum = 1
    static void verifyHeartRateWeights(const std::vector<double>& weights)
    {
        UT_VERIFY(weights.size() == videoProcessing::NB_OF_HEART_RATE_REGIONS);
        double weightSum = 0.0;
        for (double w : weights)
        {
            UT_VERIFY(w >= 0.0 && w <= 1.0);
            weightSum += w;
        }
        UT_VERIFY(std::fabs(weightSum - 1.0) < 1e-9);
    }

    // Whole video, face detected in every frame (testHeartRateEstimationFromForeHeadGreenChannel, ...Test2) : heart rate
    // of the forehead signal of the video (one sample per frame, a face is found from the first frame), logged and
    // plotted (.png : testHeartRate<fileTag>_...). The check of the heart rate is left to the test.
    static HeartRateEstimate foreHeadHeartRateOfWholeVideo(const char* videoFile, ForeHeadRectFunction foreHeadRect,
        unsigned int expectedFrames, double expectedFs, const std::string& fileTag)
    {
        videoProcessing videoObj(videoFile);
        videoObj.faceCascadeLoading(FACE_CASCADE_FILE);
        UT_VERIFY(fillDetectedForeHeadSignal(videoObj, foreHeadRect) == expectedFrames);

        HeartRateEstimate estimate = estimateHeartRate(videoObj.getforeHeadSignal(), expectedFrames, expectedFs);
        Logger::WriteMessage(("Estimated heart rate (" + std::string(videoFile) + ") : " + std::to_string(estimate.heartRateBpm) + " bpm\n").c_str());
#if TEST_SHOW_PLOTS
        plotForeHeadSignalAndFFT(estimate.filtredSig, estimate.FFTfiltredSig, "whole video", fileTag, 0.0, "ForeHead", videoFile);
#endif
        return estimate;
    }

    // Sliding windows of the forehead signal, face detected in every frame
    // (testHeartRateEstimationFromForeHeadGreenChannelSlidingWindow, ...Test2). The video is read frame by frame and the
    // forehead signal grows by one sample per frame (buffer). As soon as the buffer holds windowSeconds of signal, and
    // then each time TEST_SLIDING_WINDOW_STEP_S of new signal is available, its last windowSeconds are processed as
    // the whole video (estimateHeartRate, FFT zero padded to TEST_SLIDING_WINDOW_FFT_LENGTH points) : 0 - w s,
    // 1 - (w + 1) s ... until the last complete window. Each estimation is logged; the check is left to the test.
    static std::vector<HeartRateEstimate> foreHeadSlidingWindows(const char* videoFile, ForeHeadRectFunction foreHeadRect,
        double windowSeconds, unsigned int expectedFrames)
    {
        videoProcessing videoObj(videoFile);
        videoObj.faceCascadeLoading(FACE_CASCADE_FILE);

        const double Fs = videoObj.getFs();
        const unsigned int windowLength = static_cast<unsigned int>(std::lround(windowSeconds * Fs));          // e.g. 4.5 s : 135 samples
        const unsigned int step = static_cast<unsigned int>(std::lround(TEST_SLIDING_WINDOW_STEP_S * Fs)); // 30 samples
        UT_VERIFY(windowLength >= 2 && step >= 1 && windowLength <= videoObj.getNumOfFrames());
        UT_VERIFY(TEST_SLIDING_WINDOW_FFT_LENGTH >= windowLength); // zero padding : the FFT is not shorter than a window

        signalProcessing<float>& buffer = videoObj.getforeHeadSignal(); // one sample per frame
        std::vector<HeartRateEstimate> windows;
        const unsigned int nbOfFrames = fillDetectedForeHeadSignal(videoObj, foreHeadRect, [&]()
            {
                const unsigned int nbOfSamples = buffer.getNbOfSamples();
                if (!isWindowComplete(nbOfSamples, windowLength, step)) return;

                // the window : the last windowLength samples of the buffer
                const std::vector<float>& samples = buffer.getSignalSamples();
                signalProcessing<float> window(std::vector<float>(samples.end() - windowLength, samples.end()), windowLength, Fs);
                HeartRateEstimate estimate = estimateHeartRate(window, windowLength, Fs, TEST_SLIDING_WINDOW_FFT_LENGTH);
                estimate.startTime = (nbOfSamples - windowLength) / Fs;
                estimate.endTime = nbOfSamples / Fs;

                std::ostringstream msg;
                msg << windowName(windows.size(), estimate) << " -> estimated heart rate " << std::fixed << std::setprecision(0)
                    << estimate.heartRateBpm << " bpm\n";
                Logger::WriteMessage(msg.str().c_str());
                windows.push_back(estimate);
            });

        // a face is found from the first frame : one sample per frame, sample n is at time n / Fs
        UT_VERIFY(nbOfFrames == expectedFrames);
        UT_VERIFY(buffer.getNbOfSamples() == nbOfFrames);
        verifySlidingWindows(windows, nbOfFrames, windowLength, step, Fs);
        return windows;
    }

    // The 4 tests testHeartRateEstimationFrom<Region>OpticalFlow : the regions of test.mp4 tracked with optical flow
    // (fillTrackedRegionSignals), then the same processing and check as the forehead (estimateHeartRate).
    //   regionSignal : getter of the signal of the region (e.g. &videoProcessing::getLeftCheekSignal)
    //   regionName   : log message, plot / window titles and names of the .png files (plotForeHeadSignalAndFFT)
    static void checkHeartRateOfTrackedRegion(signalProcessing<float>& (videoProcessing::* regionSignal)(), const std::string& regionName)
    {
        videoProcessing videoObj(TEST_VIDEO_FILE);
        videoObj.faceCascadeLoading(FACE_CASCADE_FILE);
        UT_VERIFY(fillTrackedRegionSignals(videoObj) == TEST_VIDEO_FRAMES);

        HeartRateEstimate estimate = estimateHeartRate((videoObj.*regionSignal)(), TEST_VIDEO_FRAMES, TEST_VIDEO_FPS);
        Logger::WriteMessage((std::wstring(regionName.begin(), regionName.end()) + L" : estimated heart rate "
            + std::to_wstring(estimate.heartRateBpm) + L" bpm\n").c_str());
#if TEST_SHOW_PLOTS
        plotForeHeadSignalAndFFT(estimate.filtredSig, estimate.FFTfiltredSig, "whole video, optical flow tracking",
            regionName + "OpticalFlow", 0.0, regionName);
#endif

        // Tracking a region with optical flow (vs. detecting the face in every frame) involves different
        // floating point computations (goodFeaturesToTrack, Lucas-Kanade) that can differ very slightly
        // between platforms / OpenCV builds ; a small tolerance (half of the frequency resolution of the
        // whole video, as in testHeartRateEstimationFromForeHeadGreenChannelTimeWindow) is used instead of
        // the exact equality of testHeartRateEstimationFromForeHeadGreenChannel, so that such a negligible
        // difference (not enough to change which FFT bin is the highest) does not make the test fail.
        UT_VERIFY(std::fabs(estimate.heartRateBpm - TEST_VIDEO_HEART_RATE_BPM) < halfResolutionBpm(estimate));
    }

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
            UT_VERIFY(videoObj.leftCheekSignal.Fs == 0.0 && videoObj.leftCheekSignal.numberOfSamples == 0);
            UT_VERIFY(videoObj.rightCheekSignal.Fs == 0.0 && videoObj.rightCheekSignal.numberOfSamples == 0);
            UT_VERIFY(videoObj.noseSignal.Fs == 0.0 && videoObj.noseSignal.numberOfSamples == 0);
            UT_VERIFY(videoObj.heartRateSignal.Fs == 0.0 && videoObj.heartRateSignal.numberOfSamples == 0);
            UT_VERIFY(!videoObj.isOpticalFlowTracking());
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
            UT_VERIFY(videoProcessingObj.leftCheekSignal.Fs == 30.0 && videoProcessingObj.leftCheekSignal.numberOfSamples == 0);
            UT_VERIFY(videoProcessingObj.rightCheekSignal.Fs == 30.0 && videoProcessingObj.rightCheekSignal.numberOfSamples == 0);
            UT_VERIFY(videoProcessingObj.noseSignal.Fs == 30.0 && videoProcessingObj.noseSignal.numberOfSamples == 0);
            UT_VERIFY(videoProcessingObj.heartRateSignal.Fs == 30.0 && videoProcessingObj.heartRateSignal.numberOfSamples == 0);
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

        TEST_METHOD(testRegionsFromFace)
        {
            // 100 x 100 face at (10, 20) : the fractions give whole pixels (each edge is rounded, so the
            // 20 % high cheeks are 20 pixels high, not 19 as a truncated (0.74 - 0.54) * 100 would give)
            const Rect faceRect(10, 20, 100, 100);
            UT_VERIFY(videoProcessing::leftCheekRectFromFace(faceRect) == Rect(10 + 14, 20 + 54, 18, 20));
            UT_VERIFY(videoProcessing::rightCheekRectFromFace(faceRect) == Rect(10 + 68, 20 + 54, 18, 20));
            UT_VERIFY(videoProcessing::noseRectFromFace(faceRect) == Rect(10 + 43, 20 + 45, 14, 21));
            // forehead : main() definition (x + 50, y, width - 70, height / 3.7)
            UT_VERIFY(videoProcessing::foreHeadRectFromFace(faceRect) == Rect(60, 20, 30, 27));

            // the regions scale with the face : same fractions of a 200 x 200 face
            const Rect bigFace(0, 0, 200, 200);
            UT_VERIFY(videoProcessing::leftCheekRectFromFace(bigFace) == Rect(28, 108, 36, 40));
            UT_VERIFY(videoProcessing::noseRectFromFace(bigFace) == Rect(86, 90, 28, 42));
        }

        TEST_METHOD(testSetRegionsFromFace)
        {
            // all the rectangles of a frame from one face rectangle (test.mp4 : 528 x 592 frames)
            videoProcessing videoObj(TEST_VIDEO_FILE);
            const Rect faceRect(36, 77, 399, 399);
            videoObj.setRegionsFromFace(faceRect);

            UT_VERIFY(videoObj.getFaces().size() == 1 && videoObj.getFaces()[0] == faceRect);
            UT_VERIFY(videoObj.foreHeads.size() == 1 && videoObj.foreHeads[0] == videoProcessing::foreHeadRectFromFace(faceRect));
            UT_VERIFY(videoObj.foreHeads[0] == Rect(86, 77, 329, 107));
            UT_VERIFY(videoObj.getLeftCheekRect() == Rect(92, 292, 72, 80));
            UT_VERIFY(videoObj.getRightCheekRect() == Rect(307, 292, 72, 80));
            UT_VERIFY(videoObj.getNoseRect() == Rect(208, 257, 55, 83));
            // background : from the bottom-right corner of the face to the bottom-right corner of the frame
            UT_VERIFY(videoObj.getbackgroundRect() == Rect(435, 476, TEST_VIDEO_WIDTH - 435, TEST_VIDEO_HEIGHT - 476));
        }

        TEST_METHOD(testFillCheeksAndNoseSignalsFromGreenChannel)
        {
            // synthetic 100 x 100 frame : green = 10 except three blocks (left cheek = 60, right cheek = 120,
            // nose = 180); blue = 0 and red = 255, so a region read from a wrong channel is detected
            videoProcessing videoProcessingObj;
            videoProcessingObj.channels[BLEUE] = cv::Mat(100, 100, CV_8UC1, cv::Scalar(0));
            videoProcessingObj.channels[GREEN] = cv::Mat(100, 100, CV_8UC1, cv::Scalar(10));
            videoProcessingObj.channels[GREEN](Rect(5, 50, 20, 20)).setTo(cv::Scalar(60));
            videoProcessingObj.channels[GREEN](Rect(75, 50, 20, 20)).setTo(cv::Scalar(120));
            videoProcessingObj.channels[GREEN](Rect(40, 40, 20, 30)).setTo(cv::Scalar(180));
            videoProcessingObj.channels[RED] = cv::Mat(100, 100, CV_8UC1, cv::Scalar(255));

            videoProcessingObj.setLeftCheekRect(Rect(5, 50, 20, 20));
            videoProcessingObj.setRightCheekRect(Rect(75, 50, 20, 20));
            videoProcessingObj.setNoseRect(Rect(40, 40, 20, 30));
            UT_VERIFY(videoProcessingObj.getLeftCheekRect() == Rect(5, 50, 20, 20));
            UT_VERIFY(videoProcessingObj.getRightCheekRect() == Rect(75, 50, 20, 20));
            UT_VERIFY(videoProcessingObj.getNoseRect() == Rect(40, 40, 20, 30));

            videoProcessingObj.creatingCheeksAndNoseMatrixFromGreenChannel();
            // the matrices have the size of their rectangles
            UT_VERIFY(videoProcessingObj.getLeftCheek().cols == 20 && videoProcessingObj.getLeftCheek().rows == 20);
            UT_VERIFY(videoProcessingObj.getRightCheek().cols == 20 && videoProcessingObj.getRightCheek().rows == 20);
            UT_VERIFY(videoProcessingObj.getNose().cols == 20 && videoProcessingObj.getNose().rows == 30);

            // one sample per call : the mean of the green channel inside each rectangle
            videoProcessingObj.fillLeftCheekSignal();
            videoProcessingObj.fillRightCheekSignal();
            videoProcessingObj.fillNoseSignal();
            videoProcessingObj.fillNoseSignal();

            UT_VERIFY(videoProcessingObj.getLeftCheekSignal().getNbOfSamples() == 1);
            UT_VERIFY(videoProcessingObj.getLeftCheekSignal().getSignalSamples()[0] == 60.0f);
            UT_VERIFY(videoProcessingObj.getRightCheekSignal().getNbOfSamples() == 1);
            UT_VERIFY(videoProcessingObj.getRightCheekSignal().getSignalSamples()[0] == 120.0f);
            UT_VERIFY(videoProcessingObj.getNoseSignal().getNbOfSamples() == 2);
            UT_VERIFY(videoProcessingObj.getNoseSignal().getSignalSamples()[1] == 180.0f);
            // the forehead / background signals are not affected
            UT_VERIFY(videoProcessingObj.getforeHeadSignal().getNbOfSamples() == 0);
            UT_VERIFY(videoProcessingObj.getbackgroundSignal().getNbOfSamples() == 0);
        }

        TEST_METHOD(testBuildHeartRateSignal)
        {
            // synthetic region signals, 10 s at 30 Hz, pulse = 1.2 Hz sine (72 bpm), green level ~ 100 :
            //   forehead, cheeks : pulse + a small sine at another frequency (each its own)
            //   nose             : weak pulse, dominated by a 0.8 Hz sine (e.g. a movement)
            // the nose must get an almost zero weight, and heartRateSignal must pulse at 1.2 Hz
            const double Fs = 30.0;
            const unsigned int N = 300;
            auto sine = [&](double f, unsigned int n) { return std::sin(2.0 * 3.14159265358979323846 * f * n / Fs); };
            std::vector<float> foreHead(N), leftCheek(N), rightCheek(N), nose(N);
            for (unsigned int n = 0; n < N; n++)
            {
                foreHead[n] = static_cast<float>(100.0 + sine(1.2, n) + 0.3 * sine(2.9, n));
                leftCheek[n] = static_cast<float>(90.0 + sine(1.2, n) + 0.3 * sine(3.7, n));
                rightCheek[n] = static_cast<float>(95.0 + sine(1.2, n) + 0.3 * sine(3.3, n));
                nose[n] = static_cast<float>(110.0 + 0.2 * sine(1.2, n) + sine(0.8, n));
            }
            videoProcessing videoObj;
            videoObj.foreHeadSignal = signalProcessing<float>(foreHead, N, Fs);
            videoObj.leftCheekSignal = signalProcessing<float>(leftCheek, N, Fs);
            videoObj.rightCheekSignal = signalProcessing<float>(rightCheek, N, Fs);
            videoObj.noseSignal = signalProcessing<float>(nose, N, Fs);

            videoObj.buildHeartRateSignal();

            UT_VERIFY(std::fabs(videoObj.getHeartRateReferenceFrequency() - 1.2) < 1e-6);
            const std::vector<double>& w = videoObj.getHeartRateWeights();
            UT_VERIFY(w.size() == 4);
            UT_VERIFY(std::fabs(w[0] + w[1] + w[2] + w[3] - 1.0) < 1e-9);
            UT_VERIFY(w[3] < 0.01);                               // nose : almost ignored
            UT_VERIFY(w[0] > 0.3 && w[1] > 0.3 && w[2] > 0.3);    // the 3 clean regions share the rest

            signalProcessing<float>& heartRateSignal = videoObj.getHeartRateSignal();
            UT_VERIFY(heartRateSignal.getNbOfSamples() == N);
            UT_VERIFY(heartRateSignal.getFs() == Fs);
            signalProcessing<float> spectrum;
            heartRateSignal.signalFFT(spectrum);
            UT_VERIFY(std::fabs(spectrum.getFrequencyOfMaxFFT() - 1.2) < 1e-5);

            // the region signals are not modified
            UT_VERIFY(videoObj.getNoseSignal().getSignalSamples() == nose);
            UT_VERIFY(videoObj.getforeHeadSignal().getSignalSamples() == foreHead);
        }

        TEST_METHOD(testBuildHeartRateSignalWindow)
        {
            // 20 s at 30 Hz : during the first 10 s every region pulses at 2.5 Hz, during the last 10 s at
            // 1.2 Hz (and the nose is dominated by a 0.8 Hz sine). buildHeartRateSignal(300) only combines the
            // last 300 samples (as the sliding window analysis of main() does) : 1.2 Hz, 300 samples.
            const double Fs = 30.0;
            const unsigned int N = 600, window = 300;
            auto sine = [&](double f, unsigned int n) { return std::sin(2.0 * 3.14159265358979323846 * f * n / Fs); };
            std::vector<float> foreHead(N), leftCheek(N), rightCheek(N), nose(N);
            for (unsigned int n = 0; n < N; n++)
            {
                const bool last = n >= N - window;
                const double pulse = last ? sine(1.2, n) : sine(2.5, n);
                foreHead[n] = static_cast<float>(100.0 + pulse);
                leftCheek[n] = static_cast<float>(90.0 + pulse);
                rightCheek[n] = static_cast<float>(95.0 + pulse);
                nose[n] = static_cast<float>(110.0 + (last ? 0.2 * pulse + sine(0.8, n) : pulse));
            }
            videoProcessing videoObj;
            videoObj.foreHeadSignal = signalProcessing<float>(foreHead, N, Fs);
            videoObj.leftCheekSignal = signalProcessing<float>(leftCheek, N, Fs);
            videoObj.rightCheekSignal = signalProcessing<float>(rightCheek, N, Fs);
            videoObj.noseSignal = signalProcessing<float>(nose, N, Fs);

            videoObj.buildHeartRateSignal(window);
            UT_VERIFY(videoObj.getHeartRateSignal().getNbOfSamples() == window);
            UT_VERIFY(std::fabs(videoObj.getHeartRateReferenceFrequency() - 1.2) < 1e-6);
            UT_VERIFY(videoObj.getHeartRateWeights()[3] < 0.01); // nose, in this window
            signalProcessing<float> spectrum;
            videoObj.getHeartRateSignal().signalFFT(spectrum);
            UT_VERIFY(std::fabs(spectrum.getFrequencyOfMaxFFT() - 1.2) < 1e-5);
            UT_VERIFY(videoObj.getforeHeadSignal().getNbOfSamples() == N); // the region signals are not modified

            // the window must hold at least 2 samples, and not more than the signals
            Assert::ExpectException<std::runtime_error>([&] { videoObj.buildHeartRateSignal(N + 1); });
            Assert::ExpectException<std::runtime_error>([&] { videoObj.buildHeartRateSignal(1); });
        }

        TEST_METHOD(testBuildHeartRateSignalEqualRegionsAndErrors)
        {
            const double Fs = 30.0;
            const unsigned int N = 300;
            std::vector<float> pulse(N);
            for (unsigned int n = 0; n < N; n++) pulse[n] = static_cast<float>(100.0 + std::sin(2.0 * 3.14159265358979323846 * 1.2 * n / Fs));

            // 4 identical regions : same quality, equal weights
            videoProcessing videoObj;
            videoObj.foreHeadSignal = videoObj.leftCheekSignal = videoObj.rightCheekSignal = videoObj.noseSignal =
                signalProcessing<float>(pulse, N, Fs);
            videoObj.buildHeartRateSignal();
            for (double w : videoObj.getHeartRateWeights()) UT_VERIFY(std::fabs(w - 0.25) < 1e-9);

            // the 4 signals must have the same number of samples (at least 2)
            videoProcessing different;
            different.foreHeadSignal = different.leftCheekSignal = different.rightCheekSignal = signalProcessing<float>(pulse, N, Fs);
            different.noseSignal = signalProcessing<float>(std::vector<float>(pulse.begin(), pulse.begin() + 10), 10, Fs);
            Assert::ExpectException<std::runtime_error>([&] { different.buildHeartRateSignal(); });

            videoProcessing empty; // nothing filled
            Assert::ExpectException<std::runtime_error>([&] { empty.buildHeartRateSignal(); });
        }

        TEST_METHOD(testHeartRateEstimationFromForeHeadGreenChannel)
        {
            // Same processing as main() (heart_rate/main.cpp), forehead green channel only :
            // every frame of the video -> face detection -> forehead rectangle -> mean of the green
            // channel = one sample, then mean value removed, band-pass filter, FFT, and the frequency of
            // the maximum of the spectrum is the heart rate.
            HeartRateEstimate estimate = foreHeadHeartRateOfWholeVideo(TEST_VIDEO_FILE, testForeHeadRectFromFace,
                TEST_VIDEO_FRAMES, TEST_VIDEO_FPS, "");
            UT_VERIFY(estimate.heartRateBpm == TEST_VIDEO_HEART_RATE_BPM);
        }

        TEST_METHOD(testHeartRateEstimationFromForeHeadGreenChannelTest2)
        {
            // Same processing and same check as testHeartRateEstimationFromForeHeadGreenChannel (the whole video
            // at once), on test2.mp4, with its own forehead rectangle (test2ForeHeadRectFromFace, see
            // TEST2_VIDEO_HEART_RATE_BPM).
            HeartRateEstimate estimate = foreHeadHeartRateOfWholeVideo(TEST2_VIDEO_FILE, test2ForeHeadRectFromFace,
                TEST2_VIDEO_FRAMES, TEST2_VIDEO_FPS, "Test2");
            UT_VERIFY(estimate.heartRateBpm == TEST2_VIDEO_HEART_RATE_BPM);
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

            // the video has been read until the end of the window, the signal contains one sample per frame of the window
            UT_VERIFY(fillDetectedForeHeadSignal(videoObj, testForeHeadRectFromFace, nullptr, firstFrame, endFrame) == endFrame);
            const unsigned int N = endFrame - firstFrame;
            HeartRateEstimate estimate = estimateHeartRate(videoObj.getforeHeadSignal(), N, TEST_VIDEO_FPS, TEST_WINDOW_FFT_LENGTH);
            estimate.startTime = TEST_WINDOW_START_S;
            estimate.endTime = TEST_WINDOW_END_S;

            // With the zero padding the peak is read every Fs / TEST_WINDOW_FFT_LENGTH Hz (1.76 bpm), but a
            // 3 s signal only separates frequencies Fs / N Hz apart (resolution 20 bpm) : the estimation is
            // correct when it is less than half of this resolution (10 bpm) away from the heart rate of the video.
            const float resolutionBpm = static_cast<float>(60.0 * Fs / N);
            const float fftStepBpm = static_cast<float>(60.0 * Fs / TEST_WINDOW_FFT_LENGTH);
            std::ostringstream window;
            window << TEST_WINDOW_START_S << " s to " << TEST_WINDOW_END_S << " s";
            std::ostringstream msg;
            msg << "Estimated heart rate (" << window.str() << ") : " << estimate.heartRateBpm << " bpm (FFT of "
                << TEST_WINDOW_FFT_LENGTH << " points : one value every " << fftStepBpm << " bpm; resolution of the "
                << N << " samples : " << resolutionBpm << " bpm)\n";
            Logger::WriteMessage(msg.str().c_str());

#if TEST_SHOW_PLOTS
            plotForeHeadSignalAndFFT(estimate.filtredSig, estimate.FFTfiltredSig, window.str(), "Window", TEST_WINDOW_START_S); // before the check
#endif

            UT_VERIFY(std::fabs(estimate.heartRateBpm - TEST_VIDEO_HEART_RATE_BPM) < resolutionBpm / 2.0f);
        }

        TEST_METHOD(testHeartRateEstimationFromForeHeadGreenChannelSlidingWindow)
        {
            // Sliding window processing (foreHeadSlidingWindows) : windows of TEST_SLIDING_WINDOW_LENGTH_S (4.5 s),
            // one every TEST_SLIDING_WINDOW_STEP_S (1 s) : 0 - 4.5 s, 1 - 5.5 s, 2 - 6.5 s ... until the last
            // complete window of the video (10 s video : 6 windows, the last one 5 - 9.5 s).
            std::vector<HeartRateEstimate> windows = foreHeadSlidingWindows(TEST_VIDEO_FILE, testForeHeadRectFromFace,
                TEST_SLIDING_WINDOW_LENGTH_S, TEST_VIDEO_FRAMES);

#if TEST_SHOW_PLOTS
            plotSlidingWindows(windows); // before the check : also shown when it fails
#endif

            // Each estimation is correct when it is less than half of the frequency resolution of a window
            // (60 / 4.5 s / 2 = 6.7 bpm) away from the heart rate of the video.
            for (HeartRateEstimate& w : windows)
                UT_VERIFY(std::fabs(w.heartRateBpm - TEST_VIDEO_HEART_RATE_BPM) < halfResolutionBpm(w));
        }

        TEST_METHOD(testHeartRateEstimationFromForeHeadGreenChannelSlidingWindowTest2)
        {
            // Same sliding window processing as testHeartRateEstimationFromForeHeadGreenChannelSlidingWindow, on
            // test2.mp4, with longer windows (TEST2_SLIDING_WINDOW_LENGTH_S = 7 s, step 1 s : 0 - 7 s, 1 - 8 s,
            // 2 - 9 s, 3 - 10 s) and the forehead rectangle of test2.mp4 (test2ForeHeadRectFromFace).
            std::vector<HeartRateEstimate> windows = foreHeadSlidingWindows(TEST2_VIDEO_FILE, test2ForeHeadRectFromFace,
                TEST2_SLIDING_WINDOW_LENGTH_S, TEST2_VIDEO_FRAMES);

            const float medianBpm = medianHeartRateBpm(windows);
            const float halfResolution = halfResolutionBpm(windows.front()); // 60 / 7 s / 2 = 4.3 bpm
            std::ostringstream msg;
            msg << "Median of the " << windows.size() << " windows : " << std::fixed << std::setprecision(1) << medianBpm
                << " bpm (heart rate of the whole video : " << TEST2_VIDEO_HEART_RATE_BPM << " bpm, half resolution of a window : "
                << std::setprecision(2) << halfResolution << " bpm)\n";
            Logger::WriteMessage(msg.str().c_str());

#if TEST_SHOW_PLOTS
            plotSlidingWindows(windows, "filtered forehead signals (test2.mp4, 7 s windows)", "SlidingTest2"); // before the check
#endif

            // 1. Every window finds the pulse : less than half of the resolution of a window away from the heart rate of
            //    the video, plus the variation of the heart rate during the video (TEST2_HEART_RATE_VARIATION_BPM : a
            //    window measures the heart rate of its own seconds, 58 - 51 bpm). A wrong peak (e.g. 32 bpm, found by
            //    4.5 s windows) is far outside.
            for (const HeartRateEstimate& w : windows)
                UT_VERIFY(std::fabs(w.heartRateBpm - TEST2_VIDEO_HEART_RATE_BPM) < halfResolution + static_cast<float>(TEST2_HEART_RATE_VARIATION_BPM));

            // 2. Together, the windows give the heart rate of the video : their median is less than half of the
            //    resolution of a window away from it (the same criterion as each window of test.mp4, whose heart rate
            //    is constant).
            UT_VERIFY(std::fabs(medianBpm - TEST2_VIDEO_HEART_RATE_BPM) < halfResolution);
        }

        // -----------------------------------------------------------------------------------------
        // Heart rate estimation from regions other than the forehead, tracked with optical flow
        // (method E, "Haar once + KLT tracking") instead of running the face detector on every frame.
        // Same whole-video processing as testHeartRateEstimationFromForeHeadGreenChannel (see
        // fillTrackedRegionSignals / checkHeartRateOfTrackedRegion above) : the signal attribute of the
        // region (one sample per frame : mean of its green channel), then mean removal, band-pass filter,
        // FFT, and the frequency of the maximum of the spectrum is the heart rate.
        // -----------------------------------------------------------------------------------------

        TEST_METHOD(testHeartRateEstimationFromForeHeadOpticalFlow)
        {
            // Same region as testHeartRateEstimationFromForeHeadGreenChannel (foreHeadRectFromFace), but
            // computed in every frame from the tracked face instead of a new Haar detection.
            checkHeartRateOfTrackedRegion(&videoProcessing::getforeHeadSignal, "ForeHead");
        }

        TEST_METHOD(testHeartRateEstimationFromLeftCheekOpticalFlow)
        {
            // Left cheek of the image (right cheek of the person) : videoProcessing::leftCheekRectFromFace
            checkHeartRateOfTrackedRegion(&videoProcessing::getLeftCheekSignal, "LeftCheek");
        }

        TEST_METHOD(testHeartRateEstimationFromRightCheekOpticalFlow)
        {
            // Right cheek of the image (left cheek of the person) : videoProcessing::rightCheekRectFromFace
            checkHeartRateOfTrackedRegion(&videoProcessing::getRightCheekSignal, "RightCheek");
        }

        TEST_METHOD(testHeartRateEstimationFromNoseOpticalFlow)
        {
            // Nose : videoProcessing::noseRectFromFace
            checkHeartRateOfTrackedRegion(&videoProcessing::getNoseSignal, "Nose");
        }

        TEST_METHOD(testHeartRateEstimationFromHeartRateSignalOpticalFlow)
        {
            // Full processing : the heart rate comes from heartRateSignal, the weighted combination of the
            // 4 regions, and not from any region alone (the region signals are not processed here).
            videoProcessing videoObj(TEST_VIDEO_FILE);
            videoObj.faceCascadeLoading(FACE_CASCADE_FILE);

            // 1. extraction of the 4 regions, tracked with optical flow : one sample per frame each
            UT_VERIFY(fillTrackedRegionSignals(videoObj) == TEST_VIDEO_FRAMES);
            for (auto regionSignal : { &videoProcessing::getforeHeadSignal, &videoProcessing::getLeftCheekSignal,
                                       &videoProcessing::getRightCheekSignal, &videoProcessing::getNoseSignal })
                UT_VERIFY((videoObj.*regionSignal)().getNbOfSamples() == TEST_VIDEO_FRAMES);

            // 2. construction of heartRateSignal (each region filtered, scaled, weighted by its quality)
            videoObj.buildHeartRateSignal();
            HeartRateEstimate estimate;
            estimate.filtredSig = videoObj.getHeartRateSignal(); // already band-pass filtered
            UT_VERIFY(estimate.filtredSig.getNbOfSamples() == TEST_VIDEO_FRAMES);
            UT_VERIFY(estimate.filtredSig.getFs() == TEST_VIDEO_FPS);

            const std::vector<double>& weights = videoObj.getHeartRateWeights();
            verifyHeartRateWeights(weights);
            std::ostringstream msg;
            msg << std::fixed << std::setprecision(3) << "heartRateSignal : reference frequency "
                << videoObj.getHeartRateReferenceFrequency() << " Hz, weights : forehead " << weights[0] << ", left cheek "
                << weights[1] << ", right cheek " << weights[2] << ", nose " << weights[3] << "\n";
            Logger::WriteMessage(msg.str().c_str());

            // 3. heart rate of heartRateSignal (already band-pass filtered : directly its spectrum)
            estimate.filtredSig.signalFFT(estimate.FFTfiltredSig, DO_NOT_SAVE_FFT);
            UT_VERIFY(estimate.FFTfiltredSig.getNbOfSamples() == TEST_VIDEO_FRAMES / 2);
            estimate.heartRateBpm = std::round(60.0f * estimate.FFTfiltredSig.getFrequencyOfMaxFFT());
            Logger::WriteMessage((L"heartRateSignal : estimated heart rate " + std::to_wstring(estimate.heartRateBpm) + L" bpm\n").c_str());

#if TEST_SHOW_PLOTS
            plotForeHeadSignalAndFFT(estimate.filtredSig, estimate.FFTfiltredSig, "whole video, weighted sum of the 4 regions",
                "CombinedOpticalFlow", 0.0, "HeartRate");
#endif
            // same criterion as the region tests : within half a frequency step of the heart rate of the video
            UT_VERIFY(std::fabs(estimate.heartRateBpm - TEST_VIDEO_HEART_RATE_BPM) < halfResolutionBpm(estimate));
        }

        TEST_METHOD(testHeartRateEstimationFromHeartRateSignalSlidingWindow)
        {
            // Sliding window mode of main() : the 4 regions are tracked and filled frame by frame; as soon as
            // they hold TEST_SLIDING_WINDOW_LENGTH_S of signal, and then every TEST_SLIDING_WINDOW_STEP_S, their
            // last samples (the window) are combined into heartRateSignal (buildHeartRateSignal(windowLength) :
            // the weights are computed for this window only), whose zero padded spectrum gives the heart rate.
            videoProcessing videoObj(TEST_VIDEO_FILE);
            videoObj.faceCascadeLoading(FACE_CASCADE_FILE);

            const double Fs = videoObj.getFs();
            const unsigned int windowLength = static_cast<unsigned int>(std::lround(TEST_SLIDING_WINDOW_LENGTH_S * Fs)); // 135 samples
            const unsigned int step = static_cast<unsigned int>(std::lround(TEST_SLIDING_WINDOW_STEP_S * Fs));          // 30 samples

            std::vector<HeartRateEstimate> windows;
            auto processWindowIfComplete = [&]()
                {
                    const unsigned int nbOfSamples = videoObj.getforeHeadSignal().getNbOfSamples();
                    if (!isWindowComplete(nbOfSamples, windowLength, step)) return;

                    HeartRateEstimate w;
                    w.startTime = (nbOfSamples - windowLength) / Fs;
                    w.endTime = nbOfSamples / Fs;
                    videoObj.buildHeartRateSignal(windowLength);
                    w.filtredSig = videoObj.getHeartRateSignal(); // already band-pass filtered
                    w.filtredSig.signalFFT(w.FFTfiltredSig, DO_NOT_SAVE_FFT, " ", TEST_SLIDING_WINDOW_FFT_LENGTH);
                    w.heartRateBpm = std::round(60.0f * w.FFTfiltredSig.getFrequencyOfMaxFFT());

                    const std::vector<double>& weights = videoObj.getHeartRateWeights();
                    verifyHeartRateWeights(weights);

                    std::ostringstream msg;
                    msg << windowName(windows.size(), w) << " -> heart rate " << std::fixed << std::setprecision(0) << w.heartRateBpm
                        << " bpm (weights " << std::setprecision(2) << weights[0] << " " << weights[1] << " " << weights[2] << " "
                        << weights[3] << ")\n";
                    Logger::WriteMessage(msg.str().c_str());
                    windows.push_back(w);
                };

            UT_VERIFY(fillTrackedRegionSignals(videoObj, processWindowIfComplete) == TEST_VIDEO_FRAMES);
            verifySlidingWindows(windows, TEST_VIDEO_FRAMES, windowLength, step, Fs); // (301 - 135) / 30 + 1 = 6 windows

#if TEST_SHOW_PLOTS
            plotSlidingWindows(windows, "heartRateSignal (4 regions combined)", "SlidingHeartRateSignal"); // before the check
#endif
            // same criterion as testHeartRateEstimationFromForeHeadGreenChannelSlidingWindow : half of the
            // frequency resolution of a window (60 / 4.5 s / 2 = 6.7 bpm)
            for (HeartRateEstimate& w : windows)
                UT_VERIFY(std::fabs(w.heartRateBpm - TEST_VIDEO_HEART_RATE_BPM) < halfResolutionBpm(w));
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
            // displayFrame draws the rectangles (face, forehead, background, and the cheeks / nose when they
            // are set) on the frame, then show it in the window "Frame"
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
            UT_VERIFY(videoProcessingObj.frame.at<cv::Vec3b>(0, 0) == black);   // cheeks / nose not set : not drawn

            // with the cheeks and the nose set, they are drawn too
            videoProcessingObj.setLeftCheekRect(Rect(15, 40, 10, 10));
            videoProcessingObj.setRightCheekRect(Rect(50, 40, 10, 10));
            videoProcessingObj.setNoseRect(Rect(30, 20, 8, 8));
            videoProcessingObj.displayFrame();
            UT_VERIFY(videoProcessingObj.frame.at<cv::Vec3b>(40, 15) == white); // left cheek, top-left corner
            UT_VERIFY(videoProcessingObj.frame.at<cv::Vec3b>(40, 50) == white); // right cheek, top-left corner
            UT_VERIFY(videoProcessingObj.frame.at<cv::Vec3b>(20, 30) == white); // nose, top-left corner
            UT_VERIFY(videoProcessingObj.frame.at<cv::Vec3b>(40, 40) == black); // still on no border
        }
    };

} // namespace heart_rate_tests
