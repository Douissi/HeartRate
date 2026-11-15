#ifndef VIDEOPROCESSING_HPP
#define VIDEOPROCESSING_HPP

#include <algorithm>
#include <vector>

#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/objdetect.hpp>
#include <opencv2/video/tracking.hpp>
#include <opencv2/videoio.hpp>

#include "signalProcessing.hpp"

using namespace cv;
using namespace std;


class videoProcessing
{
    friend class heart_rate_tests::TestVideoProcessing;

public:
    videoProcessing() : capture(), Fs(0.0), numberOfFrames(0), faceCascade(), frame(), backgroundRect(),
        foreHead(), face(), background(), foreHeadSignal(), backgroundSignal(), width(0), height(0) {
    }

    videoProcessing(const String& filename) : capture(filename), Fs(0.0), numberOfFrames(0), faceCascade(), frame(),
        backgroundRect(), foreHead(), face(), background(), foreHeadSignal(), backgroundSignal(),
        width(static_cast<unsigned int>(capture.get(CAP_PROP_FRAME_WIDTH))), height(static_cast<unsigned int>(capture.get(CAP_PROP_FRAME_HEIGHT)))
    {
        // Check if camera opened successfully
        if (!capture.isOpened()) {
            ASSERT(0, "Error opening video stream or file : " + filename);
        }

        Fs = (double)(capture.get(CAP_PROP_FPS));
        numberOfFrames = static_cast<unsigned int>(capture.get(CAP_PROP_FRAME_COUNT)); // only for videoInput != webcam (filename != 0)
        foreHeadSignal = signalProcessing<float>(numberOfFrames, Fs);
        backgroundSignal = signalProcessing<float>(numberOfFrames, Fs);
        leftCheekSignal = signalProcessing<float>(numberOfFrames, Fs);
        rightCheekSignal = signalProcessing<float>(numberOfFrames, Fs);
        noseSignal = signalProcessing<float>(numberOfFrames, Fs);
        heartRateSignal = signalProcessing<float>(numberOfFrames, Fs);
    }

    virtual ~videoProcessing()
    {
        // When everything done, release the video capture object
        capture.release();
        // Closes all the frames
        destroyAllWindows();
    }

    void faceCascadeLoading(const string& path)
    {
        if (!faceCascade.load(path))
        {
            ASSERT(0, "Error while opening xml file for faceTracking : " + path);
        }
    }

    void capturingFrames()
    {
        // Capture frame-by-frame
        capture >> frame;

        // If the frame is empty, break immediately
        if (frame.empty())
        {
            cout << " Error while capturing frame :  Either frame is empty, or end of Video" << endl;
            return;
        }

        split(frame, channels); // split source channels[1] is the green one
    }

    void detectFacesFromGreenChannelInVideo()
    {
        /* first param was GreyColorImage */
        faceCascade.detectMultiScale(frame, faces, 1.1, 2, 0 | cv::CASCADE_SCALE_IMAGE, Size(100, 100));
    }

    void creatingFaceForeHeadAndBackGroundMatrixFromGreenChannel()
    {
        face = cv::Mat(channels[GREEN], faces[0]);
        foreHead = cv::Mat(channels[GREEN], foreHeads[0]);
        background = cv::Mat(channels[GREEN], backgroundRect);
    }

    // Same as creatingFaceForeHeadAndBackGroundMatrixFromGreenChannel for the cheeks and the nose :
    // views (no copy) of the green channel of the current frame inside leftCheekRect, rightCheekRect
    // and noseRect (set them first, e.g. with the *RectFromFace functions below).
    void creatingCheeksAndNoseMatrixFromGreenChannel()
    {
        leftCheek = cv::Mat(channels[GREEN], leftCheekRect);
        rightCheek = cv::Mat(channels[GREEN], rightCheekRect);
        nose = cv::Mat(channels[GREEN], noseRect);
    }

    /******************************** Regions of the face ****************************/
    // Rectangles of the regions used for the heart rate, computed from a face rectangle (e.g. faces[0]
    // found by detectFacesFromGreenChannelInVideo, or trackedFace()). The cheeks and the nose are
    // fractions of the face (they scale with it); "left" / "right" are the sides of the image (the left
    // cheek of the image is the right cheek of the person). They were chosen on test.mp4, where their
    // signals correlate with the forehead signal and give the same heart rate.

    // Forehead : same definition as main() (absolute pixel offsets, not a fraction of the face)
    static Rect foreHeadRectFromFace(const Rect& faceRect)
    {
        return Rect(faceRect.x + 50, faceRect.y, faceRect.width - 70, static_cast<int>(faceRect.height / 3.7));
    }
    static Rect leftCheekRectFromFace(const Rect& faceRect) { return fractionOfFaceRect(faceRect, 0.14, 0.54, 0.32, 0.74); }
    static Rect rightCheekRectFromFace(const Rect& faceRect) { return fractionOfFaceRect(faceRect, 0.68, 0.54, 0.86, 0.74); }
    static Rect noseRectFromFace(const Rect& faceRect) { return fractionOfFaceRect(faceRect, 0.43, 0.45, 0.57, 0.66); }

    static void drawRectangle(cv::Mat& inputOutputImage, const Rect& rect)
    {
        rectangle(inputOutputImage, Point(rect.x, rect.y), Point(rect.x + rect.width, rect.y + rect.height), Scalar(255, 255, 255), 2, 8, 0);
    }

    static void imageColorToGreyTransform(const cv::Mat& InputImageColor, cv::Mat& OutputGreyColor)
    {
        cvtColor(InputImageColor, OutputGreyColor, COLOR_BGR2GRAY);
        equalizeHist(OutputGreyColor, OutputGreyColor);
    }

    static float meanFrameValue(const cv::Mat& inputFrame)
    {
        Scalar tmpMean = mean(inputFrame);
        return static_cast<float>(tmpMean.val[0]);
    }

    void fillforeHeadSignal()
    {
        foreHeadSignal.fillWithValue(meanFrameValue(foreHead));
    }

    void fillbackgroundSignal()
    {
        backgroundSignal.fillWithValue(meanFrameValue(background));
    }

    // One sample per call (mean of the green channel of the region in the current frame), as
    // fillforeHeadSignal : call creatingCheeksAndNoseMatrixFromGreenChannel() first.
    void fillLeftCheekSignal()
    {
        leftCheekSignal.fillWithValue(meanFrameValue(leftCheek));
    }

    void fillRightCheekSignal()
    {
        rightCheekSignal.fillWithValue(meanFrameValue(rightCheek));
    }

    void fillNoseSignal()
    {
        noseSignal.fillWithValue(meanFrameValue(nose));
    }

    /******************************** Heart rate signal ******************************/
    // Builds heartRateSignal : the weighted sum of the 4 region signals (forehead, left cheek, right
    // cheek, nose), each region weighted by the quality of its pulse. The 4 signals must have the same
    // number of samples. The region signals are not modified.
    //   windowLength = 0 (default) : the whole signals are combined (e.g. at the end of the video);
    //   windowLength > 0           : only their last windowLength samples (at least 2), e.g. the current
    //                                window of the sliding window analysis; heartRateSignal then has
    //                                windowLength samples.
    //
    //   1. Each region signal (copy) : mean removed, band-pass filter [lowFreqCutOff, highFreqCutOff]
    //      (order filterOrder, as for the forehead in main()), then scaled to a standard deviation of 1,
    //      so that the regions are comparable (a region is not favoured because its green level varies
    //      more, e.g. with more pixel noise or illumination changes).
    //   2. Common reference frequency : the peak, in the heart rate band, of the spectrum of the plain
    //      average of the 4 scaled signals (consensus of the regions).
    //   3. Quality of each region : signal to noise ratio of its spectrum at that reference frequency
    //      (signalProcessing::getSNR : power within +/- w of the frequency and +/- 2w of its harmonic /
    //      rest of the band). w = 0.1 Hz, or the frequency resolution Fs / N if it is larger (short
    //      windows : e.g. 0.22 Hz for 4.5 s), so that w always covers the peak of the pulse.
    //      It is measured at the common frequency, not at the region's own peak, so that a region locked
    //      on another frequency (noise, movement) gets a low weight even if that peak is sharp.
    //   4. Weights = SNR / sum of the SNR (they add up to 1; equal weights if every SNR is 0), and
    //      heartRateSignal = sum of weight x scaled signal.
    //
    // heartRateSignal is therefore already band-pass filtered (its heart rate is read directly from its
    // spectrum : signalFFT, then getFrequencyOfMaxFFT). The regions are assumed to pulse in phase (true
    // on test.mp4 : delays between the regions of at most 2 frames, positive correlations). The weights
    // (order : forehead, left cheek, right cheek, nose) and the reference frequency are kept, see
    // getHeartRateWeights / getHeartRateReferenceFrequency.
    void buildHeartRateSignal(const unsigned int& windowLength = 0)
    {
        signalProcessing<float>* regions[NB_OF_HEART_RATE_REGIONS] = { &foreHeadSignal, &leftCheekSignal, &rightCheekSignal, &noseSignal };
        const unsigned int total = foreHeadSignal.getNbOfSamples();
        const double FsSignal = foreHeadSignal.getFs();
        ASSERT(FsSignal > 0.0, "buildHeartRateSignal fct :: the sampling frequency of the region signals is not set");
        for (signalProcessing<float>* region : regions)
            ASSERT(region->getNbOfSamples() == total, "buildHeartRateSignal fct :: the 4 region signals must have the same number of samples");
        const unsigned int N = (windowLength == 0) ? total : windowLength; // samples combined : the last N ones
        ASSERT(N >= 2 && N <= total, "buildHeartRateSignal fct :: at least 2 samples are needed, and no more than the signals contain");
        const unsigned int first = total - N;

        // 1. filtered, scaled copies
        std::vector<std::vector<float>> scaled(NB_OF_HEART_RATE_REGIONS);
        std::vector<float> average(N, 0.0f);
        for (std::size_t r = 0; r < NB_OF_HEART_RATE_REGIONS; r++)
        {
            const std::vector<float>& samples = regions[r]->getSignalSamples();
            signalProcessing<float> copy(std::vector<float>(samples.begin() + first, samples.begin() + total), N, FsSignal);
            copy.removeMeanValue();
            signalProcessing<float> filtered;
            copy.bandPassFilterFunction(lowFreqCutOff, highFreqCutOff, filterOrder, filtered);
            scaled[r] = filtered.getSignalSamples();
            scaleToUnitStandardDeviation(scaled[r]);
            for (unsigned int n = 0; n < N; n++) average[n] += scaled[r][n] / static_cast<float>(NB_OF_HEART_RATE_REGIONS);
        }

        // 2. reference frequency : heart rate peak of the average
        signalProcessing<float> averageSignal(average, N, FsSignal), averageSpectrum;
        averageSignal.signalFFT(averageSpectrum);
        heartRateReferenceFrequency = averageSpectrum.getFrequencyOfMaxFFT();

        // 3. quality of each region at the reference frequency
        const double halfWidth = std::max(0.1, FsSignal / static_cast<double>(N));
        std::vector<double> snr(NB_OF_HEART_RATE_REGIONS);
        double snrSum = 0.0;
        for (std::size_t r = 0; r < NB_OF_HEART_RATE_REGIONS; r++)
        {
            signalProcessing<float> regionSignal(scaled[r], N, FsSignal), regionSpectrum;
            regionSignal.signalFFT(regionSpectrum);
            snr[r] = regionSpectrum.getSNR(heartRateReferenceFrequency, halfWidth);
            snrSum += snr[r];
        }

        // 4. weights and weighted sum
        heartRateWeights.assign(NB_OF_HEART_RATE_REGIONS, 1.0 / NB_OF_HEART_RATE_REGIONS);
        if (snrSum > 0.0)
            for (std::size_t r = 0; r < NB_OF_HEART_RATE_REGIONS; r++) heartRateWeights[r] = snr[r] / snrSum;

        std::vector<float> combined(N, 0.0f);
        for (std::size_t r = 0; r < NB_OF_HEART_RATE_REGIONS; r++)
            for (unsigned int n = 0; n < N; n++) combined[n] += static_cast<float>(heartRateWeights[r]) * scaled[r][n];
        heartRateSignal = signalProcessing<float>(combined, N, FsSignal);
    }

    void displayFrame()
    {
        drawRectangle(frame, faces[0]);
        drawRectangle(frame, foreHeads[0]);
        drawRectangle(frame, backgroundRect);
        // cheeks and nose, when they are used (not set : empty rectangles, nothing drawn)
        for (const Rect& region : { leftCheekRect, rightCheekRect, noseRect })
            if (region.area() > 0) drawRectangle(frame, region);
        imshow("Frame", frame);
    }

    bool isFrameEmpty() { return frame.empty(); }

    /******************************** Optical flow tracking **************************/
    // A region of the face (forehead, cheek, nose ...) can be tracked without running the Haar face
    // detector on every frame : the face is detected once, a set of features inside it is followed
    // from frame to frame with pyramidal Lucas-Kanade optical flow (channels[GREEN], i.e. the same
    // green channel as the signal), and a region defined in the detection frame (e.g. a fraction of
    // the face rectangle) is moved by the same translation to find its position in the current frame.
    //
    // The displacement used is the *median* of the displacements of the tracked points (kept after a
    // forward - then backward - optical flow round trip, a deterministic way of discarding the
    // outliers, e.g. points that moved onto the glasses or the hair) : no random step (unlike e.g.
    // RANSAC) is involved, so the result is exactly reproducible from one run / platform to another.
    // This is accurate for a face that does not rotate or change scale noticeably (checked on
    // test.mp4 : the translation is a few pixels, scale and rotation are negligible), not a general
    // purpose tracker.

    // Starts the tracking : 'face' is typically the rectangle found by detectFacesFromGreenChannelInVideo
    // on the current frame. Must be called once capturingFrames() has read that frame.
    void initOpticalFlowTracking(const Rect& face)
    {
        ofReferenceFace = face;
        ofPrevGray = channels[GREEN].clone();
        seedOpticalFlowPoints(face);
        ofTranslation = cv::Point2d(0.0, 0.0);
        ofTracking = !ofPoints.empty();
        ASSERT(ofTracking, "initOpticalFlowTracking fct :: no feature found to track in the face rectangle");
    }

    bool isOpticalFlowTracking() const { return ofTracking; }

    // Tracks the points from the previously processed frame to the current one (channels[GREEN], i.e.
    // the frame just read by capturingFrames()) and updates the cumulative translation since
    // initOpticalFlowTracking(). Re-seeds new points (inside the last tracked face rectangle) when too
    // few are left. initOpticalFlowTracking() must have been called once before.
    void trackOpticalFlow()
    {
        ASSERT(ofTracking, "trackOpticalFlow fct :: initOpticalFlowTracking must be called first");
        const cv::Mat& gray = channels[GREEN];

        std::vector<cv::Point2f> next, back;
        std::vector<uchar> status, statusBack;
        std::vector<float> err, errBack;
        cv::calcOpticalFlowPyrLK(ofPrevGray, gray, ofPoints, next, status, err);
        cv::calcOpticalFlowPyrLK(gray, ofPrevGray, next, back, statusBack, errBack);

        std::vector<double> dx, dy;
        std::vector<cv::Point2f> kept;
        for (std::size_t i = 0; i < ofPoints.size(); i++)
        {
            if (!status[i] || !statusBack[i]) continue;
            if (cv::norm(back[i] - ofPoints[i]) > 0.5) continue; // forward/backward check : rejects the outliers
            dx.push_back(static_cast<double>(next[i].x - ofPoints[i].x));
            dy.push_back(static_cast<double>(next[i].y - ofPoints[i].y));
            kept.push_back(next[i]);
        }
        ASSERT(!kept.empty(), "trackOpticalFlow fct :: every tracked point was lost");

        ofTranslation.x += median(dx);
        ofTranslation.y += median(dy);
        ofPoints = kept;
        ofPrevGray = gray.clone();

        if (ofPoints.size() < 60) // too few points left : re-seed inside the last tracked face rectangle
            seedOpticalFlowPoints(trackedRect(ofReferenceFace, gray.cols, gray.rows));
    }

    // Current position of a rectangle defined in the frame given to initOpticalFlowTracking() (e.g. a
    // region defined as a fraction of the face rectangle), moved by the optical flow translation and
    // clipped to the frame (maxWidth x maxHeight).
    Rect trackedRect(const Rect& referenceRect, int maxWidth, int maxHeight) const
    {
        const int x = static_cast<int>(std::lround(referenceRect.x + ofTranslation.x));
        const int y = static_cast<int>(std::lround(referenceRect.y + ofTranslation.y));
        return Rect(x, y, referenceRect.width, referenceRect.height) & Rect(0, 0, maxWidth, maxHeight);
    }

    // Current position of the face given to initOpticalFlowTracking() (clipped to the video frame) :
    // replaces, in a tracked video, the face detected in every frame (faces[0]) from which the regions
    // are computed (foreHeadRectFromFace, leftCheekRectFromFace ...).
    Rect trackedFace() const
    {
        return trackedRect(ofReferenceFace, static_cast<int>(width), static_cast<int>(height));
    }

    // Sets all the rectangles of a frame from its face rectangle (detected, or given by trackedFace()) :
    // faces[0] = face, the background (below-right of the face, as in main() so far), and the forehead,
    // cheeks and nose (foreHeadRectFromFace, leftCheekRectFromFace ...). Then call
    // creatingFaceForeHeadAndBackGroundMatrixFromGreenChannel / creatingCheeksAndNoseMatrixFromGreenChannel
    // and the fill*Signal functions to add the samples of this frame.
    void setRegionsFromFace(const Rect& faceRect)
    {
        faces = { faceRect };
        backgroundRect = Rect(faceRect.x + faceRect.width, faceRect.y + faceRect.height,
            static_cast<int>(width) - (faceRect.x + faceRect.width), static_cast<int>(height) - (faceRect.y + faceRect.height));
        foreHeads = { foreHeadRectFromFace(faceRect) };
        leftCheekRect = leftCheekRectFromFace(faceRect);
        rightCheekRect = rightCheekRectFromFace(faceRect);
        noseRect = noseRectFromFace(faceRect);
    }

    /************************************** Setters *********************************/
    void setbackGroundRect(const Rect& background) { backgroundRect = background; }
    void setRectFacesVector(const std::vector<Rect>& rectFacesVector) { faces = rectFacesVector; }
    void setRectForeHeadsVector(const std::vector<Rect>& rectForeHeadsVector) { foreHeads = rectForeHeadsVector; }
    void setLeftCheekRect(const Rect& rect) { leftCheekRect = rect; }
    void setRightCheekRect(const Rect& rect) { rightCheekRect = rect; }
    void setNoseRect(const Rect& rect) { noseRect = rect; }

    /************************************** Getters ********************************/
    const std::vector<Rect>& getFaces() const { return faces; }
    cv::Mat& getframe() { return frame; }
    const Rect& getbackgroundRect() const { return backgroundRect; }
    const cv::Mat& getforeHead() const { return foreHead; }
    const cv::Mat& getbackground() const { return background; }
    const double& getFs() const { return Fs; }
    const unsigned int& getNumOfFrames() const { return numberOfFrames; }
    signalProcessing<float>& getforeHeadSignal() { return foreHeadSignal; }
    signalProcessing<float>& getbackgroundSignal() { return backgroundSignal; }
    const Rect& getLeftCheekRect() const { return leftCheekRect; }
    const Rect& getRightCheekRect() const { return rightCheekRect; }
    const Rect& getNoseRect() const { return noseRect; }
    const cv::Mat& getLeftCheek() const { return leftCheek; }
    const cv::Mat& getRightCheek() const { return rightCheek; }
    const cv::Mat& getNose() const { return nose; }
    signalProcessing<float>& getLeftCheekSignal() { return leftCheekSignal; }
    signalProcessing<float>& getRightCheekSignal() { return rightCheekSignal; }
    signalProcessing<float>& getNoseSignal() { return noseSignal; }
    signalProcessing<float>& getHeartRateSignal() { return heartRateSignal; }
    // set by buildHeartRateSignal : weights of forehead, left cheek, right cheek, nose (sum = 1) and the
    // reference frequency (Hz) at which the quality of the regions was measured
    const std::vector<double>& getHeartRateWeights() const { return heartRateWeights; }
    double getHeartRateReferenceFrequency() const { return heartRateReferenceFrequency; }

    // Number of regions combined by buildHeartRateSignal (forehead, left cheek, right cheek, nose)
    static constexpr std::size_t NB_OF_HEART_RATE_REGIONS = 4;


private:
    VideoCapture capture;
    double Fs;
    unsigned int numberOfFrames;
    CascadeClassifier faceCascade; // for ROI tracking CascadeClassifier : needs to be improved for tracking ROI
    cv::Mat frame; // frame
    cv::Mat channels[3] = {};   // Channels RGB
    std::vector<Rect> faces = {}; // Rectangles containing faces
    std::vector<Rect> foreHeads = {}; // Rectangles containing foreHeads
    Rect backgroundRect; // a rectangle containing background dimension
    cv::Mat foreHead;
    cv::Mat face;
    cv::Mat background;
    signalProcessing<float> foreHeadSignal; // signal samples
    signalProcessing<float> backgroundSignal; // signal samples

    // Cheeks and nose : rectangle, view of the green channel inside it, signal (one sample per frame)
    Rect leftCheekRect;
    Rect rightCheekRect;
    Rect noseRect;
    cv::Mat leftCheek;
    cv::Mat rightCheek;
    cv::Mat nose;
    signalProcessing<float> leftCheekSignal;
    signalProcessing<float> rightCheekSignal;
    signalProcessing<float> noseSignal;

    // Combination of the 4 region signals (see buildHeartRateSignal)
    signalProcessing<float> heartRateSignal;
    std::vector<double> heartRateWeights;      // forehead, left cheek, right cheek, nose
    double heartRateReferenceFrequency = 0.0;  // Hz

    // Removes the mean of a signal (the filtered signal's mean is close to 0, not exactly 0) and scales it
    // to a standard deviation of 1 (a constant signal is left unchanged : it gets a weight of 0 anyway).
    static void scaleToUnitStandardDeviation(std::vector<float>& values)
    {
        if (values.empty()) return;
        double mean = 0.0;
        for (float v : values) mean += v;
        mean /= static_cast<double>(values.size());
        double variance = 0.0;
        for (float v : values) variance += (v - mean) * (v - mean);
        const double deviation = std::sqrt(variance / static_cast<double>(values.size()));
        if (deviation <= 0.0) return;
        for (float& v : values) v = static_cast<float>((v - mean) / deviation);
    }

    // Rectangle (x0, y0) - (x1, y1) given as fractions of the face rectangle ((0, 0) = top-left corner of
    // the face, (1, 1) = bottom-right corner). Each edge is rounded to the nearest pixel and the size is
    // the difference of the edges : truncating (x1 - x0) * width instead would lose a pixel whenever the
    // product is just below an integer (e.g. (0.74 - 0.54) * 100 = 19.999999999999996 -> 19).
    static Rect fractionOfFaceRect(const Rect& faceRect, double x0, double y0, double x1, double y1)
    {
        const int left = faceRect.x + static_cast<int>(std::lround(x0 * faceRect.width));
        const int top = faceRect.y + static_cast<int>(std::lround(y0 * faceRect.height));
        const int right = faceRect.x + static_cast<int>(std::lround(x1 * faceRect.width));
        const int bottom = faceRect.y + static_cast<int>(std::lround(y1 * faceRect.height));
        return Rect(left, top, right - left, bottom - top);
    }

    // Optical flow tracking (see initOpticalFlowTracking / trackOpticalFlow above)
    cv::Mat ofPrevGray;                       // green channel of the previously processed frame
    std::vector<cv::Point2f> ofPoints;        // tracked feature points, in ofPrevGray
    Rect ofReferenceFace;                     // face rectangle given to initOpticalFlowTracking
    cv::Point2d ofTranslation = cv::Point2d(0.0, 0.0); // cumulative translation since ofReferenceFace (px)
    bool ofTracking = false;

    // Detects features to track (goodFeaturesToTrack) inside 'region', restricted to the current frame.
    void seedOpticalFlowPoints(const Rect& region)
    {
        const cv::Mat& gray = channels[GREEN];
        const Rect clipped = region & Rect(0, 0, gray.cols, gray.rows);
        cv::Mat mask = cv::Mat::zeros(gray.size(), CV_8UC1);
        mask(clipped).setTo(cv::Scalar(255));
        cv::goodFeaturesToTrack(gray, ofPoints, 200, 0.01, 7, mask);
    }

    // Median of a vector of values (not an empty one), used by trackOpticalFlow() so that the
    // displacement of the tracked region does not depend on the order of the (deterministic, but
    // otherwise arbitrary) outliers rejection : no random step is involved anywhere in the tracking.
    static double median(std::vector<double> values)
    {
        ASSERT(!values.empty(), "median fct :: empty input");
        const std::size_t mid = values.size() / 2;
        std::nth_element(values.begin(), values.begin() + mid, values.end());
        double m = values[mid];
        if (values.size() % 2 == 0)
        {
            std::nth_element(values.begin(), values.begin() + mid - 1, values.end());
            m = (m + values[mid - 1]) / 2.0;
        }
        return m;
    }

public:
    const unsigned int width;
    const unsigned int height;

};


#endif // VIDEOPROCESSING_HPP
