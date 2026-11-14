#include <stdlib.h>
#include <stdio.h>
#include <cmath>
#include <exception>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>
#include <opencv2/core/utils/logger.hpp>
#include "signalPlot.hpp"
#include "videoProcessing.hpp"

using namespace cv;

// Usage : heart_rate.exe [videoFile] [haarCascadeXmlFile]
// Default files are searched in the working directory. The Visual Studio project copies them
// (content of the "data" folder) next to heart_rate.exe and uses that folder as working
// directory when debugging (F5 / Ctrl+F5).
static const char* const DEFAULT_VIDEO_FILE = "test.mp4";
static const char* const DEFAULT_FACE_CASCADE_FILE = "haarcascade_frontalface_alt2.xml";



// Sliding window analysis : result of the processing of one window (see slidingWindowLengthSeconds)
struct SlidingWindowResult
{
    double startTime = 0.0;                  // time of the first sample of the window in the video (s)
    double endTime = 0.0;                    // time after the last sample (s)
    signalProcessing<float> filtredSig;      // band-pass filtered forehead signal of the window
    signalProcessing<float> FFTfiltredSig;   // its power spectrum (zero padded)
    float heartRateBpm = 0.0f;               // estimation : frequency of the maximum of the spectrum x 60
};


// Plots of the sliding window analysis, displayed in three windows and saved as .png files :
// the filtered forehead signal of every window, the spectrum of every window, and the heart rate
// estimated by the successive windows through the video.
static void plotSlidingWindows(std::vector<SlidingWindowResult>& results, const float& wholeVideoHeartRateBpm)
{
    if (results.empty())
    {
        cout << "Sliding window : the video is shorter than one window, no plot" << endl;
        return;
    }

    std::vector<cv::Mat> signalPlots, fftPlots;
    signal_plot::PlotData heartRateCurve;
    for (std::size_t i = 0; i < results.size(); i++)
    {
        SlidingWindowResult& result = results[i];
        std::ostringstream name;
        name << "Window " << i + 1 << " : " << std::fixed << std::setprecision(1) << result.startTime
            << " s - " << result.endTime << " s";
        signalPlots.push_back(signal_plot::render(
            signal_plot::signalPlotData(name.str(), result.filtredSig.getSignalSamples(), result.filtredSig.getFs(),
                result.startTime), 460, 230));
        fftPlots.push_back(signal_plot::render(
            signal_plot::spectrumPlotData("FFT " + name.str(), result.FFTfiltredSig.getSignalSamples(),
                result.FFTfiltredSig.getFs(), result.FFTfiltredSig.getFFTSize(), 6.0,
                lowFreqCutOff, highFreqCutOff), 460, 230));
        heartRateCurve.x.push_back(result.endTime); // the estimation is available at the end of the window
        heartRateCurve.y.push_back(static_cast<double>(result.heartRateBpm));
    }

    std::ostringstream title;
    title << "Heart rate of each " << slidingWindowLengthSeconds << " s window (step " << slidingWindowStepSeconds
        << " s) - whole video : " << std::fixed << std::setprecision(0) << wholeVideoHeartRateBpm << " bpm";
    heartRateCurve.title = title.str();
    heartRateCurve.xLabel = "Time of the end of the window (s)";
    heartRateCurve.yLabel = "Heart rate (bpm)";
    heartRateCurve.showPoints = true;                  // one point per window
    heartRateCurve.showReferenceY = true;              // heart rate of the whole video
    heartRateCurve.referenceY = static_cast<double>(wholeVideoHeartRateBpm);
    heartRateCurve.referenceLabel = "whole video";

    const cv::Mat signals = signal_plot::tile(signalPlots, 3);
    const cv::Mat ffts = signal_plot::tile(fftPlots, 3);
    const cv::Mat heartRates = signal_plot::render(heartRateCurve, 900, 320);

    try
    {
        signal_plot::save("slidingWindowsSignals.png", signals);
        signal_plot::save("slidingWindowsFFT.png", ffts);
        signal_plot::save("slidingWindowsHeartRate.png", heartRates);
        cout << "Plots saved : slidingWindowsSignals.png, slidingWindowsFFT.png, slidingWindowsHeartRate.png" << endl;

        if (!signal_plot::displayAvailable())
        {
            cout << "No display : the plots are not shown in windows" << endl;
            return;
        }
        const std::string signalsWindow = "Sliding windows : filtered forehead signals";
        const std::string fftWindow = "Sliding windows : FFT of the filtered forehead signals";
        const std::string heartRateWindow = "Sliding windows : heart rate through the video";
        signal_plot::show(signalsWindow, signals, 20, 20);
        signal_plot::show(fftWindow, ffts, 20, 20 + signals.rows + 40);
        signal_plot::show(heartRateWindow, heartRates, 20 + signals.cols + 20, 20);
        cout << "Plots displayed : press a key or close the windows to quit" << endl;
        signal_plot::waitForWindows({ signalsWindow, fftWindow, heartRateWindow });
    }
    catch (const cv::Exception& e)
    {
        cout << "The plots could not be saved / displayed : " << e.what() << endl;
    }
}


static int run(const std::string& videoFile, const std::string& faceCascadeFile)
{
    // Create a VideoCapture object and open the input file
    // If the input is the web camera, pass 0 instead of the video file name
    videoProcessing videoObj(videoFile);

    videoObj.faceCascadeLoading(faceCascadeFile);

    std::vector<Rect> lastDetectedFaces;

    // Sliding window analysis : a heart rate is estimated during the acquisition, from the last
    // slidingWindowLengthSeconds of forehead signal, every slidingWindowStepSeconds (see below).
    const unsigned int slidingWindowLength = static_cast<unsigned int>(std::lround(slidingWindowLengthSeconds * videoObj.getFs()));
    const unsigned int slidingWindowStep = static_cast<unsigned int>(std::lround(slidingWindowStepSeconds * videoObj.getFs()));
    std::vector<SlidingWindowResult> slidingResults;

    /************************************************* Video Processing *************************************************/
    while (1)
    {
        videoObj.capturingFrames();
        if (videoObj.isFrameEmpty()) break;

        videoObj.detectFacesFromGreenChannelInVideo();

        // No face detected in this frame : keep the last detected face (or skip the frame if none yet)
        if (videoObj.getFaces().empty())
        {
            if (lastDetectedFaces.empty()) continue;
            videoObj.setRectFacesVector(lastDetectedFaces);
        }
        lastDetectedFaces = videoObj.getFaces();

        const std::vector<Rect>& faces = videoObj.getFaces();
        const Rect backgroundRect = Rect(faces[0].x + faces[0].width, faces[0].y + faces[0].height,
            videoObj.width - (faces[0].x + faces[0].width), videoObj.height - (faces[0].y + faces[0].height));

        videoObj.setbackGroundRect(backgroundRect);

        std::vector<Rect> foreHeads = faces;

        foreHeads[0].height = static_cast<int>(foreHeads[0].height / 3.7); // only for manualforeHead detection
        foreHeads[0].x = foreHeads[0].x + 50; // only for manual foreHead detection
        foreHeads[0].width = foreHeads[0].width - 70; // only for manual foreHead detection

        videoObj.setRectForeHeadsVector(foreHeads);

        videoObj.creatingFaceForeHeadAndBackGroundMatrixFromGreenChannel();

        videoObj.fillforeHeadSignal();  // construction of the ForeHead Signal with mean of green Channel each frame
        videoObj.fillbackgroundSignal();


        // ******************** Sliding window analysis (same processing as the whole signal below) ********************
        // As soon as the forehead signal holds slidingWindowLengthSeconds of samples, and then every
        // slidingWindowStepSeconds, its last slidingWindowLength samples are processed : mean value removed,
        // band-pass filter, zero padded FFT, and the maximum of the spectrum gives a heart rate.
        const unsigned int nbOfSamples = videoObj.getforeHeadSignal().getNbOfSamples();
        if (slidingWindowLength >= 2 && slidingWindowStep >= 1 && nbOfSamples >= slidingWindowLength &&
            (nbOfSamples - slidingWindowLength) % slidingWindowStep == 0)
        {
            const std::vector<float>& buffer = videoObj.getforeHeadSignal().getSignalSamples();
            signalProcessing<float> window(std::vector<float>(buffer.end() - slidingWindowLength, buffer.end()),
                slidingWindowLength, videoObj.getFs());
            SlidingWindowResult result;
            result.startTime = (nbOfSamples - slidingWindowLength) / videoObj.getFs();
            result.endTime = nbOfSamples / videoObj.getFs();

            window.removeMeanValue();
            window.bandPassFilterFunction(lowFreqCutOff, highFreqCutOff, filterOrder, result.filtredSig);
            result.filtredSig.signalFFT(result.FFTfiltredSig, DO_NOT_SAVE_FFT, " ", slidingWindowFFTLength);
            result.heartRateBpm = std::round(60.0f * result.FFTfiltredSig.getFrequencyOfMaxFFT());

            cout << "Sliding window " << slidingResults.size() + 1 << " : " << std::fixed << std::setprecision(1)
                << result.startTime << " s - " << result.endTime << " s -> estimated heart rate "
                << std::setprecision(0) << result.heartRateBpm << " bpm" << endl;
            slidingResults.push_back(result);
        }

        // Display results in frames
        videoObj.displayFrame();

        // Press  ESC on keyboard to exit
        char c = (char)waitKey(25);
        if (c == 27)
            break;
    }


    // the acquisition is finished : close the video window ("Frame") before the plots
    destroyAllWindows();

    /******************************************** Signal Processing *************************************************/

    signalProcessing<float>& foreHeadSignal = videoObj.getforeHeadSignal();
    signalProcessing<float>& backgroundSignal = videoObj.getbackgroundSignal();
    signalProcessing<float> ouputEstimationSignal;
    signalProcessing<float> filteredOutputEstimationSignal;
    signalProcessing<float> filtredSig;
    signalProcessing<float> FFTnlmsOutputEstimation;      // one object per spectrum, so that they can all be plotted
    signalProcessing<float> FFTforeHeadMeanValueRemoved;
    signalProcessing<float> FFTfiltredForeHead;

    backgroundSignal.saveSigToFile("backgroundSignal.txt");
    foreHeadSignal.saveSigToFile("foreHeadSignal.txt");
    const signalProcessing<float> foreHeadRawSignal = foreHeadSignal; // copy for the plot (the mean value is removed below)

    cout << "************************** Signal Processing *************************" << endl;

    // Illumination Rectification
    signalProcessing<float>::nlmsFilter(backgroundSignal, foreHeadSignal, filterOrder, mu, ouputEstimationSignal);
    ouputEstimationSignal.bandPassFilterFunction(lowFreqCutOff, highFreqCutOff, filterOrder, filteredOutputEstimationSignal);
    filteredOutputEstimationSignal.saveSigToFile("nlmsOutputEstimation.txt");
    filteredOutputEstimationSignal.signalFFT(FFTnlmsOutputEstimation, SAVE_FFT, "FFTnlmsOutputEstimation.txt");

    // remove mean value "offset" of the signal
    foreHeadSignal.removeMeanValue();

    // Signal filter  ******************** TO BE OPTIMIZED **************************
    foreHeadSignal.saveSigToFile("foreHeadMeanValueRemoved.txt");
    foreHeadSignal.signalFFT(FFTforeHeadMeanValueRemoved, SAVE_FFT, "FFTforeHeadMeanValueRemoved.txt");


    foreHeadSignal.bandPassFilterFunction(lowFreqCutOff, highFreqCutOff, filterOrder, filtredSig);
    filtredSig.saveSigToFile("filteredForeHeadBandPass.txt");
    filtredSig.signalFFT(FFTfiltredForeHead, SAVE_FFT, "FFTfiltredForeHead.txt");

    const float wholeVideoHeartRateBpm = std::round(60.0f * FFTfiltredForeHead.getFrequencyOfMaxFFT());
    cout << "Whole video (" << foreHeadSignal.getNbOfSamples() << " samples) : estimated heart rate "
        << std::fixed << std::setprecision(0) << wholeVideoHeartRateBpm << " bpm" << endl;

    // ****************************** plots of the sliding window analysis ******************************
    // One plot per window (filtered signal and spectrum) and the heart rate of the successive windows.
    plotSlidingWindows(slidingResults, wholeVideoHeartRateBpm);


    // ************************************ heart rate estimation **********************
    // Highest peak of the spectrum inside the band [lowFreqCutOff, highFreqCutOff]
    /*auto printHeartRate = [](const std::string& name, const signalProcessing<float>& spectrum)
    {
        const double frequency = spectrum.getHeartRateFrequency(lowFreqCutOff, highFreqCutOff);
        cout << "Estimated heart rate (" << name << ") : ";
        if (frequency < 0.0) cout << "not available" << endl;
        else cout << 60.0 * frequency << " bpm (peak at " << frequency << " Hz)" << endl;
    };
    printHeartRate("forehead, band-pass filtered", FFTfiltredForeHead);
    printHeartRate("NLMS illumination rectification", FFTnlmsOutputEstimation);*/


    // ************************************ plotting the signals **********************
    // One OpenCV window per plot (see signalProcessing::plotSignal / plotFFT), tiled on the screen.
    // The same signals are also saved in the .txt files above (plotWithGNUPLOT can plot these files
    // if gnuplot is installed).


    return 0;
}


int main(int argc, char* argv[])
{
    // The OpenCV debug libraries (Debug configuration) print INFO messages by default (search of
    // optional plugins: "... => FAILED", backends used ...), the release libraries only warnings and
    // errors. Use the same level in all configurations (set LOG_LEVEL_INFO to see these messages).
    cv::utils::logging::setLogLevel(cv::utils::logging::LOG_LEVEL_WARNING);

    const std::string videoFile = (argc > 1) ? argv[1] : DEFAULT_VIDEO_FILE;
    const std::string faceCascadeFile = (argc > 2) ? argv[2] : DEFAULT_FACE_CASCADE_FILE;

    try
    {
        return run(videoFile, faceCascadeFile);
    }
    catch (const std::exception& e)
    {
        std::cerr << "heart_rate : fatal error : " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}
