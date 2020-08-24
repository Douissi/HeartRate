#include <videoProcessing.hpp>

//*******************************************************************************************************
//********************************* Implementation of videoProcessing class *****************************
//*******************************************************************************************************

videoProcessing:: videoProcessing() : capture(), Fs(0.0), numberOfFrames (0), faceCascade(), frame(), backgroundRect(),
    face(), background(), faceSignal(), backgroundSignal(), heartRateEstimation(0.0f), width(0), height(0)
{}

videoProcessing::videoProcessing(const String& filename) : capture(filename), Fs (0.0), numberOfFrames (0), faceCascade(), frame(),
    backgroundRect(), face(), background(), faceSignal(), backgroundSignal(), heartRateEstimation(0.0f),
    width(capture.get(CAP_PROP_FRAME_WIDTH)), height(capture.get(CAP_PROP_FRAME_HEIGHT))
{
    // Check if camera opened successfully
    if(!capture.isOpened()){
        functions::ASSERT(0, "Error opening video stream or file");
    }

    Fs = (double)(capture.get(CAP_PROP_FPS));
    numberOfFrames =  capture.get(CAP_PROP_FRAME_COUNT); // only for videoInput != webcam (filename != 0)
    faceSignal = signalProcessing<float>(numberOfFrames, Fs);
    backgroundSignal = signalProcessing<float>(numberOfFrames, Fs);
}

videoProcessing::videoProcessing(const int& entry, const int windowLen) : capture(entry), Fs (0.0), numberOfFrames (0), faceCascade(), frame(),
    backgroundRect(), face(), background(), faceSignal(), backgroundSignal(), heartRateEstimation(0.0f),
    width(capture.get(CAP_PROP_FRAME_WIDTH)), height(capture.get(CAP_PROP_FRAME_HEIGHT))
{
    // Check if camera opened successfully
    if(!capture.isOpened()){
        functions::ASSERT(0, "Error opening video stream or file");
    }

    Fs = (double)(capture.get(CAP_PROP_FPS));
    numberOfFrames =  windowLen*Fs;
    faceSignal = signalProcessing<float>(numberOfFrames, Fs);
    backgroundSignal = signalProcessing<float>(numberOfFrames, Fs);
}

videoProcessing::~videoProcessing()
{
    // When everything done, release the video capture object
    capture.release();
    // Closes all the frames
    destroyAllWindows();
}

void videoProcessing::faceCascadeLoading(const string& path)
{
    if(!faceCascade.load(path))
    {
        functions::ASSERT(0, "Error while opening xml file for faceTracking");
    }
}

void videoProcessing::capturingFrames()
{
    // Capture frame-by-frame
    capture >> frame;

    // If the frame is empty, break immediately
    if (frame.empty())
    {
        cout <<" Error while capturing frame :  Either frame is empty, or end of Video" << endl;
    }

    split(frame, channels); // split source channels[1] is the green one
}

void videoProcessing::detectFacesFromGreenChannelInVideo()
{
    /* first param was GreyColorImage */
    faceCascade.detectMultiScale(frame, faces, 1.1, 2, 0|cv::CASCADE_SCALE_IMAGE, Size(100, 100) );
}

void videoProcessing::skinDetectionOverFaceAndMatrixCreationFromGreenChannel()
{
    // TODO maybe improve performance by processing skin detection only on face !!!
    // skin detection using mask over YCbCr space
    cv::Mat frameYCrCb;
    cvtColor(frame, frameYCrCb, COLOR_BGR2YCrCb);
    cv::inRange(frameYCrCb, cv::Scalar(80, 133, 77), cv::Scalar(255, 173, 127), frameYCrCb);
    cv::Mat frameSkinDetected;
    bitwise_and(frame, frame, frameSkinDetected, frameYCrCb);

    cv::Mat channelsThresholded[3] = {};
    split(frameSkinDetected, channelsThresholded); // split source channels[1] is the green one

    face =  cv::Mat(channelsThresholded[GREEN] , faces[0]);
    background = cv::Mat(channels[GREEN] , backgroundRect);
    //imshow( "Frame", face);
}

void videoProcessing::drawRectangle(cv::Mat& inputOutputImage, const Rect& rect)
{
    rectangle(inputOutputImage, Point (rect.x ,rect.y), Point (rect.x + rect.width, rect.y + rect.height), Scalar(255,255,255), 2, 8, 0);
}

void videoProcessing::imageColorToGreyTransform(const cv::Mat& InputImageColor, cv::Mat& OutputGreyColor)
{
    cvtColor(InputImageColor, OutputGreyColor, COLOR_BGR2GRAY);
    equalizeHist(OutputGreyColor, OutputGreyColor);
}

float videoProcessing::meanFrameValue(const cv::Mat& inputFrame)
{
    Scalar tmpMean = mean(inputFrame);
    return tmpMean.val[0];
}

void videoProcessing::fillFaceSignal()
{
    faceSignal.fillWithValue(meanFrameValue(face));
}

void videoProcessing::fillBackgroundSignal()
{
    backgroundSignal.fillWithValue(meanFrameValue(background));
}

void videoProcessing::displayFrame(bool display)
{
    drawRectangle(frame, faces[0]);
    drawRectangle(frame, backgroundRect);
    if(display) imshow( "Frame", frame );
}

bool videoProcessing::isFrameEmpty()
{
    return frame.empty();
}

void videoProcessing::BGR2RGB()
{
    cvtColor(frame, frame, COLOR_BGR2RGB);
}


void videoProcessing::clearSignals()
{
    faceSignal.clearSignal();
    backgroundSignal.clearSignal();
}

void videoProcessing::computeHeartRateEstimation(signalProcessing<float>& filtredSig, signalProcessing<float>& FFTOutput)
{
    faceSignal.removeMeanValue();
    faceSignal.bandPassFilterFunction(lowFreqCutOff, highFreqCutOff, filterOrder, filtredSig);
    filtredSig.signalFFT(FFTOutput);
    heartRateEstimation = FFTOutput.getFrequencyOfMaxFFT() * 60.0f;
}


//*******************************************************************************************************
//*************************** Implementation of Interface VideoProcessing class *************************
//*******************************************************************************************************

IvideoProcessing::IvideoProcessing(const string& input, QObject *parent) : QObject(parent), toggleStream(false), cptFrames(0),
  isRealTime(false), inputVideo(input), isFaceDetected(false)
{
    videoObj = new videoProcessing(input);
    videoObj->faceCascadeLoading("/usr/local/share/opencv4/haarcascades/haarcascade_frontalface_alt2.xml");
    maxNbOfFrames = windowLenProcess * static_cast<u32>(videoObj->getFs());
}

IvideoProcessing::~IvideoProcessing()
{
    delete videoObj;
}

void IvideoProcessing::receiveGrabFrame()
{
    if(!toggleStream)
    {
        return;
    }

    videoObj->capturingFrames();

    if(videoObj->isFrameEmpty())
    {
        return;
    }

    processVideoStream();

    if(!isFaceDetected)
    {
        return;
    }

    videoObj->BGR2RGB();
    QImage output = QImage((const uchar*) videoObj->getFrame().data, videoObj->getFrame().cols, videoObj->getFrame().rows, videoObj->getFrame().step, QImage::Format_RGB888);

    emit sendFrame(output);

    if(cptFrames < maxNbOfFrames)
    {
        cptFrames++;
    }
    else
    {
        cptFrames = 0;
        signalProcessing<float> filtredSig;
        signalProcessing<float> FFTOutput;
        videoObj->computeHeartRateEstimation(filtredSig, FFTOutput);
        emit sendSignalToPlot(filtredSig.getSignalSamples());
        emit sendFFTtoPlot(FFTOutput.getSignalSamples());
        videoObj->clearSignals();
        emit sendHeartRateEstimation(videoObj->getHeartRateEstimation());
    }

}


void IvideoProcessing::processVideoStream()
{
    videoObj->detectFacesFromGreenChannelInVideo();
    const std::vector<Rect>& faces = videoObj->getFaces();
    if(faces.size() == 0)
    {
        isFaceDetected = false;
        return;
    }
    else
    {
        isFaceDetected = true;
    }

    // Background detection deduced from face detection
    const Rect backgroundRect = Rect(faces[0].x + faces[0].width, faces[0].y + faces[0].height,
            videoObj->width - (faces[0].x + faces[0].width), videoObj->height -(faces[0].y + faces[0].height));

    videoObj->setBackGroundRect(backgroundRect);

    videoObj->skinDetectionOverFaceAndMatrixCreationFromGreenChannel();

    cv::Mat faceRBG;
    cvtColor(videoObj->getFace(), faceRBG, COLOR_BGR2RGB);

    QImage outputFace = QImage((const uchar*)  faceRBG.data, faceRBG.cols, faceRBG.rows, faceRBG.step, QImage::Format_RGB888);

    emit sendFace(outputFace);

    videoObj->fillFaceSignal();  // construction of the face Signal with mean of green Channel each frame
    videoObj->fillBackgroundSignal();// construction of the background Signal with mean of green Channel each frame

    videoObj->displayFrame(false); // only draw ROI
}

void IvideoProcessing::receiveToggleStream()
{
    toggleStream = !toggleStream;
}

void IvideoProcessing::receiveStopStream()
{
    toggleStream = false;
}

void IvideoProcessing::reconstructVideoObj(const string &input, const int &winLen)
{
    toggleStream = false;
    cptFrames = 0;
    delete videoObj;
    videoObj = new videoProcessing(input);
    videoObj->faceCascadeLoading("/usr/local/share/opencv4/haarcascades/haarcascade_frontalface_alt2.xml");
    maxNbOfFrames = winLen * static_cast<u32>(videoObj->getFs());
    windowLenProcess = winLen;
    inputVideo = input;
    isFaceDetected = false;
    emit sendHeartRateEstimation(0.0);
}

void IvideoProcessing::receiveReconstructVideoObj(const string input)
{
    reconstructVideoObj(input, windowLenProcess);
}

void IvideoProcessing::receiveWinLenChange(const int windowLen)
{
   reconstructVideoObj(inputVideo, windowLen);
}

void IvideoProcessing::receiveResetStream()
{
    if(!isRealTime)
    {
        reconstructVideoObj(inputVideo, windowLenProcess);
    }
    // TODO for realTime
}

