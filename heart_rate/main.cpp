#include <stdlib.h>
#include <stdio.h>
#include <algorithm>
#include <cmath>
#include <exception>
#include <functional>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <opencv2/core/utils/logger.hpp>
#include "signalPlot.hpp"
#include "videoProcessing.hpp"

using namespace cv;

// Usage : heart_rate.exe [--sliding-window | --whole-video] [videoFile] [haarCascadeXmlFile]
//
// Two modes (the first argument, optional) :
//   --sliding-window (default) : during the acquisition, a heart rate is estimated every
//       slidingWindowStepSeconds from the last slidingWindowLengthSeconds of signal (as a live
//       measurement would do), then the estimations of all the windows are plotted.
//   --whole-video : one heart rate estimated from the whole video, once it has been read.
// In both modes the heart rate comes from the same signal : the weighted combination
// (videoProcessing::buildHeartRateSignal) of 4 regions of the face (forehead, left cheek, right cheek,
// nose). The face is detected once with the Haar cascade, then tracked from frame to frame with optical
// flow (videoProcessing::initOpticalFlowTracking / trackOpticalFlow), and the regions are computed from
// the tracked face in every frame.
//
// Default files are searched in the working directory. The Visual Studio project copies them
// (content of the "data" folder) next to heart_rate.exe and uses that folder as working
// directory when debugging (F5 / Ctrl+F5). To choose the mode in Visual Studio : project properties >
// Debugging > Command Arguments (e.g. --whole-video).
static const char* const DEFAULT_VIDEO_FILE = "test2.mp4";
static const char* const DEFAULT_FACE_CASCADE_FILE = "haarcascade_frontalface_alt2.xml";

static const char* const REGION_NAMES[videoProcessing::NB_OF_HEART_RATE_REGIONS] = { "forehead", "left cheek", "right cheek", "nose" };

enum class Mode { SlidingWindow, WholeVideo };

struct Options
{
    Mode mode = Mode::SlidingWindow;
    std::string videoFile = DEFAULT_VIDEO_FILE;
    std::string faceCascadeFile = DEFAULT_FACE_CASCADE_FILE;
};

static void printUsage()
{
    cout << "Usage : heart_rate [--sliding-window | --whole-video] [videoFile] [haarCascadeXmlFile]\n"
        "  --sliding-window : heart rate of every " << slidingWindowLengthSeconds << " s window, every "
        << slidingWindowStepSeconds << " s (default)\n"
        "  --whole-video    : one heart rate for the whole video\n"
        "  videoFile          default : " << DEFAULT_VIDEO_FILE << "\n"
        "  haarCascadeXmlFile default : " << DEFAULT_FACE_CASCADE_FILE << endl;
}

// Returns false if the program must stop (help requested, or an unknown option : 'exitCode' is set).
static bool parseArguments(int argc, char* argv[], Options& options, int& exitCode)
{
    std::vector<std::string> positional;
    for (int i = 1; i < argc; i++)
    {
        const std::string arg = argv[i];
        if (arg == "--sliding-window") options.mode = Mode::SlidingWindow;
        else if (arg == "--whole-video") options.mode = Mode::WholeVideo;
        else if (arg == "--help" || arg == "-h") { printUsage(); exitCode = EXIT_SUCCESS; return false; }
        else if (arg.size() > 1 && arg[0] == '-') { std::cerr << "Unknown option : " << arg << "\n"; printUsage(); exitCode = EXIT_FAILURE; return false; }
        else positional.push_back(arg);
    }
    if (positional.size() > 2) { std::cerr << "Too many arguments\n"; printUsage(); exitCode = EXIT_FAILURE; return false; }
    if (positional.size() > 0) options.videoFile = positional[0];
    if (positional.size() > 1) options.faceCascadeFile = positional[1];
    return true;
}


/********************************************* Acquisition (both modes) *********************************************/
// Reads the video frame by frame until its end (or ESC in the video window) and adds one sample per frame
// to the forehead, left cheek, right cheek, nose and background signals of videoObj :
//   - until a face is found, the Haar cascade is run on every frame (frames without a face are skipped);
//   - the first detected face starts the optical flow tracking : from then on, the face of each frame is
//     the tracked face (no more Haar detection), and the regions are computed from it (setRegionsFromFace).
// onNewSample (optional) is called after each new sample (used by the sliding window mode).
// Returns the number of frames read.
static unsigned int acquireRegionSignals(videoProcessing& videoObj, const std::function<void()>& onNewSample = nullptr)
{
    const bool display = signal_plot::displayAvailable(); // no video window on a machine without display
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
            videoObj.initOpticalFlowTracking(videoObj.getFaces()[0]);
            cout << "Face detected in frame " << nbOfFrames << " : tracked with optical flow from now on" << endl;
        }
        else
        {
            videoObj.trackOpticalFlow();
        }

        // regions of this frame, computed from the tracked face, and one new sample in each signal
        videoObj.setRegionsFromFace(videoObj.trackedFace());
        videoObj.creatingFaceForeHeadAndBackGroundMatrixFromGreenChannel();
        videoObj.creatingCheeksAndNoseMatrixFromGreenChannel();
        videoObj.fillforeHeadSignal();
        videoObj.fillLeftCheekSignal();
        videoObj.fillRightCheekSignal();
        videoObj.fillNoseSignal();
        videoObj.fillbackgroundSignal();

        if (onNewSample) onNewSample();

        if (display)
        {
            videoObj.displayFrame(); // face, forehead, cheeks, nose and background rectangles
            // Press ESC on keyboard to exit
            if (static_cast<char>(waitKey(25)) == 27) break;
        }
    }
    // the acquisition is finished : close the video window ("Frame") before the plots
    if (display) destroyAllWindows();
    return nbOfFrames;
}

static std::string weightsText(const std::vector<double>& weights)
{
    std::ostringstream text;
    text << std::fixed << std::setprecision(2);
    for (std::size_t r = 0; r < weights.size(); r++) text << (r ? ", " : "") << REGION_NAMES[r] << " " << weights[r];
    return text.str();
}

// Saves the images (.png in the working directory), then displays them (if a display is available)
// until a key is pressed or the windows are closed.
struct PlotImage
{
    std::string windowName;
    std::string fileName;
    cv::Mat image;
    int x;
    int y;
};

static void saveAndShowPlots(const std::vector<PlotImage>& plots)
{
    try
    {
        std::string saved;
        for (const PlotImage& p : plots)
            if (signal_plot::save(p.fileName, p.image)) saved += (saved.empty() ? "" : ", ") + p.fileName;
        cout << "Plots saved : " << saved << endl;

        if (!signal_plot::displayAvailable())
        {
            cout << "No display : the plots are not shown in windows" << endl;
            return;
        }
        std::vector<std::string> windows;
        for (const PlotImage& p : plots)
        {
            signal_plot::show(p.windowName, p.image, p.x, p.y);
            windows.push_back(p.windowName);
        }
        cout << "Plots displayed : press a key or close the windows to quit" << endl;
        signal_plot::waitForWindows(windows);
    }
    catch (const cv::Exception& e)
    {
        cout << "The plots could not be saved / displayed : " << e.what() << endl;
    }
}


/*************************************************** Sliding window mode ***************************************************/
// Result of the processing of one window (see slidingWindowLengthSeconds)
struct SlidingWindowResult
{
    double startTime = 0.0;                  // time of the first sample of the window in the video (s)
    double endTime = 0.0;                    // time after the last sample (s)
    signalProcessing<float> heartRateSig;    // heartRateSignal of the window (4 regions combined, filtered)
    signalProcessing<float> FFTheartRateSig; // its power spectrum (zero padded to slidingWindowFFTLength)
    std::vector<double> weights;             // weights of the 4 regions in this window
    float heartRateBpm = 0.0f;               // estimation : frequency of the maximum of the spectrum (heart rate band) x 60
};

static void plotSlidingWindows(std::vector<SlidingWindowResult>& results)
{
    std::vector<cv::Mat> signalPlots, fftPlots;
    signal_plot::PlotData heartRateCurve;
    std::vector<double> estimations;
    for (std::size_t i = 0; i < results.size(); i++)
    {
        SlidingWindowResult& result = results[i];
        std::ostringstream name;
        name << "Window " << i + 1 << " : " << std::fixed << std::setprecision(1) << result.startTime
            << " s - " << result.endTime << " s";
        signalPlots.push_back(signal_plot::render(
            signal_plot::signalPlotData(name.str(), result.heartRateSig.getSignalSamples(), result.heartRateSig.getFs(),
                result.startTime), 460, 230));
        fftPlots.push_back(signal_plot::render(
            signal_plot::spectrumPlotData("FFT " + name.str(), result.FFTheartRateSig.getSignalSamples(),
                result.FFTheartRateSig.getFs(), result.FFTheartRateSig.getFFTSize(), 6.0,
                lowFreqCutOff, highFreqCutOff), 460, 230));
        heartRateCurve.x.push_back(result.endTime); // the estimation is available at the end of the window
        heartRateCurve.y.push_back(static_cast<double>(result.heartRateBpm));
        estimations.push_back(static_cast<double>(result.heartRateBpm));
    }

    // reference line : median of the estimations of the windows
    std::sort(estimations.begin(), estimations.end());
    const std::size_t mid = estimations.size() / 2;
    const double median = (estimations.size() % 2) ? estimations[mid] : (estimations[mid - 1] + estimations[mid]) / 2.0;

    std::ostringstream title;
    title << "Heart rate of each " << slidingWindowLengthSeconds << " s window (step " << slidingWindowStepSeconds
        << " s, 4 regions combined) - median : " << std::fixed << std::setprecision(1) << median << " bpm";
    heartRateCurve.title = title.str();
    heartRateCurve.xLabel = "Time of the end of the window (s)";
    heartRateCurve.yLabel = "Heart rate (bpm)";
    heartRateCurve.showPoints = true;                  // one point per window
    heartRateCurve.showReferenceY = true;
    heartRateCurve.referenceY = median;
    heartRateCurve.referenceLabel = "median";

    const cv::Mat signals = signal_plot::tile(signalPlots, 3);
    const cv::Mat ffts = signal_plot::tile(fftPlots, 3);
    const cv::Mat heartRates = signal_plot::render(heartRateCurve, 900, 320);
    saveAndShowPlots({
        { "Sliding windows : heartRateSignal (4 regions combined)", "slidingWindowsSignals.png", signals, 20, 20 },
        { "Sliding windows : FFT of heartRateSignal", "slidingWindowsFFT.png", ffts, 20, 20 + signals.rows + 40 },
        { "Sliding windows : heart rate through the video", "slidingWindowsHeartRate.png", heartRates, 20 + signals.cols + 20, 20 } });
}

// As soon as the region signals hold slidingWindowLengthSeconds of samples, and then every
// slidingWindowStepSeconds, their last samples (the window) are combined into heartRateSignal
// (buildHeartRateSignal : each region filtered, weighted by its quality in this window), whose zero padded
// spectrum gives the heart rate of the window.
static int runSlidingWindowMode(videoProcessing& videoObj)
{
    const double Fs = videoObj.getFs();
    const unsigned int windowLength = static_cast<unsigned int>(std::lround(slidingWindowLengthSeconds * Fs));
    const unsigned int step = static_cast<unsigned int>(std::lround(slidingWindowStepSeconds * Fs));
    if (windowLength < 2 || step < 1)
    {
        std::cerr << "Sliding window : the frame rate of the video (" << Fs << " fps) is too low" << endl;
        return EXIT_FAILURE;
    }
    cout << "Mode : sliding window (" << slidingWindowLengthSeconds << " s = " << windowLength << " samples, every "
        << slidingWindowStepSeconds << " s)" << endl;

    std::vector<SlidingWindowResult> results;
    auto processWindowIfComplete = [&]()
        {
            const unsigned int nbOfSamples = videoObj.getforeHeadSignal().getNbOfSamples();
            if (nbOfSamples < windowLength || (nbOfSamples - windowLength) % step != 0) return;

            SlidingWindowResult result;
            result.startTime = (nbOfSamples - windowLength) / Fs;
            result.endTime = nbOfSamples / Fs;
            videoObj.buildHeartRateSignal(windowLength);           // the 4 regions combined, on this window only
            result.heartRateSig = videoObj.getHeartRateSignal();   // already band-pass filtered
            result.weights = videoObj.getHeartRateWeights();
            result.heartRateSig.signalFFT(result.FFTheartRateSig, DO_NOT_SAVE_FFT, " ", std::max(slidingWindowFFTLength, windowLength));
            result.heartRateBpm = std::round(60.0f * result.FFTheartRateSig.getFrequencyOfMaxFFT());

            cout << "Window " << results.size() + 1 << " : " << std::fixed << std::setprecision(1) << result.startTime
                << " s - " << result.endTime << " s -> heart rate " << std::setprecision(0) << result.heartRateBpm
                << " bpm (weights : " << weightsText(result.weights) << ")" << endl;
            results.push_back(result);
        };

    acquireRegionSignals(videoObj, processWindowIfComplete);

    if (results.empty())
    {
        cout << "Sliding window : no complete window (the video is shorter than " << slidingWindowLengthSeconds
            << " s, or no face was found)" << endl;
        return EXIT_FAILURE;
    }
    plotSlidingWindows(results);
    return EXIT_SUCCESS;
}


/*************************************************** Whole video mode ***************************************************/
// The whole video is read first, then the 4 complete region signals are combined into heartRateSignal,
// whose spectrum gives one heart rate for the whole video.
static int runWholeVideoMode(videoProcessing& videoObj)
{
    cout << "Mode : whole video" << endl;
    acquireRegionSignals(videoObj);

    const unsigned int N = videoObj.getforeHeadSignal().getNbOfSamples();
    if (N < 2)
    {
        cout << "Whole video : not enough samples (" << N << "), was a face found ?" << endl;
        return EXIT_FAILURE;
    }
    cout << "************************** Signal Processing *************************" << endl;

    // raw signals (one sample per frame), e.g. for gnuplot (plotWithGNUPLOT)
    videoObj.getforeHeadSignal().saveSigToFile("foreHeadSignal.txt");
    videoObj.getLeftCheekSignal().saveSigToFile("leftCheekSignal.txt");
    videoObj.getRightCheekSignal().saveSigToFile("rightCheekSignal.txt");
    videoObj.getNoseSignal().saveSigToFile("noseSignal.txt");
    videoObj.getbackgroundSignal().saveSigToFile("backgroundSignal.txt");

    // Illumination rectification of the forehead with the background (NLMS), kept from the previous
    // version for analysis : saved in files, not used for the heart rate.
    {
        signalProcessing<float> background = videoObj.getbackgroundSignal(), foreHead = videoObj.getforeHeadSignal();
        signalProcessing<float> nlmsOutput, filteredNlmsOutput, FFTnlmsOutput;
        signalProcessing<float>::nlmsFilter(background, foreHead, filterOrder, mu, nlmsOutput);
        nlmsOutput.bandPassFilterFunction(lowFreqCutOff, highFreqCutOff, filterOrder, filteredNlmsOutput);
        filteredNlmsOutput.saveSigToFile("nlmsOutputEstimation.txt");
        filteredNlmsOutput.signalFFT(FFTnlmsOutput, SAVE_FFT, "FFTnlmsOutputEstimation.txt");
    }

    // the 4 regions combined (each filtered and weighted by its quality), and its spectrum
    videoObj.buildHeartRateSignal();
    signalProcessing<float>& heartRateSignal = videoObj.getHeartRateSignal();
    signalProcessing<float> FFTheartRateSignal;
    heartRateSignal.saveSigToFile("heartRateSignal.txt");
    heartRateSignal.signalFFT(FFTheartRateSignal, SAVE_FFT, "FFTheartRateSignal.txt");

    const float heartRateBpm = std::round(60.0f * FFTheartRateSignal.getFrequencyOfMaxFFT());
    cout << "Weights of the regions : " << weightsText(videoObj.getHeartRateWeights()) << endl;
    cout << "Whole video (" << N << " samples) : estimated heart rate " << std::fixed << std::setprecision(0)
        << heartRateBpm << " bpm" << endl;

    const cv::Mat signalImage = signal_plot::render(signal_plot::signalPlotData(
        "heartRateSignal : 4 regions combined (" + weightsText(videoObj.getHeartRateWeights()) + ")",
        heartRateSignal.getSignalSamples(), heartRateSignal.getFs(), 0.0), 900, 320);
    const cv::Mat fftImage = signal_plot::render(signal_plot::spectrumPlotData("FFT of heartRateSignal (whole video)",
        FFTheartRateSignal.getSignalSamples(), FFTheartRateSignal.getFs(), FFTheartRateSignal.getFFTSize(), 6.0,
        lowFreqCutOff, highFreqCutOff), 900, 320);
    saveAndShowPlots({
        { "Whole video : heartRateSignal (4 regions combined)", "wholeVideoHeartRateSignal.png", signalImage, 40, 40 },
        { "Whole video : FFT of heartRateSignal", "wholeVideoFFTHeartRateSignal.png", fftImage, 40, 40 + signalImage.rows + 50 } });
    return EXIT_SUCCESS;
}


static int run(const Options& options)
{
    // Create a VideoCapture object and open the input file
    videoProcessing videoObj(options.videoFile);
    videoObj.faceCascadeLoading(options.faceCascadeFile);

    return (options.mode == Mode::WholeVideo) ? runWholeVideoMode(videoObj) : runSlidingWindowMode(videoObj);
}


int main(int argc, char* argv[])
{
    // The OpenCV debug libraries (Debug configuration) print INFO messages by default (search of
    // optional plugins: "... => FAILED", backends used ...), the release libraries only warnings and
    // errors. Use the same level in all configurations (set LOG_LEVEL_INFO to see these messages).
    cv::utils::logging::setLogLevel(cv::utils::logging::LOG_LEVEL_WARNING);

    Options options;
    int exitCode = EXIT_SUCCESS;
    if (!parseArguments(argc, argv, options, exitCode)) return exitCode;

    try
    {
        return run(options);
    }
    catch (const std::exception& e)
    {
        std::cerr << "heart_rate : fatal error : " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}
