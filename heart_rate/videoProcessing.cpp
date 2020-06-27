#include <videoProcessing.hpp>

//*******************************************************************************************************
//********************************* Implementation of videoProcessing class *****************************
//*******************************************************************************************************

videoProcessing:: videoProcessing() : capture(), Fs(0.0), numberOfFrames (0), faceCascade(), frame(), backgroundRect(),
    foreHead(), face(), background(), foreHeadSignal(), backgroundSignal(), heartRateEstimation(0.0f), width(0), height(0)
{}

videoProcessing::videoProcessing(const String& filename) : capture(filename), Fs (0.0), numberOfFrames (0), faceCascade(), frame(),
    backgroundRect(),foreHead(), face(), background(), foreHeadSignal(), backgroundSignal(), heartRateEstimation(0.0f),
    width(capture.get(CAP_PROP_FRAME_WIDTH)), height(capture.get(CAP_PROP_FRAME_HEIGHT))
{
    // Check if camera opened successfully
    if(!capture.isOpened()){
        functions::ASSERT(0, "Error opening video stream or file");
    }

    Fs = (double)(capture.get(CAP_PROP_FPS));
    numberOfFrames =  capture.get(CAP_PROP_FRAME_COUNT); // only for videoInput != webcam (filename != 0)
    foreHeadSignal = signalProcessing<float>(numberOfFrames, Fs);
    backgroundSignal = signalProcessing<float>(numberOfFrames, Fs);
}

videoProcessing::videoProcessing(const int& entry) : capture(entry), Fs (0.0), numberOfFrames (0), faceCascade(), frame(),
    backgroundRect(),foreHead(), face(), background(), foreHeadSignal(), backgroundSignal(), heartRateEstimation(0.0f),
    width(capture.get(CAP_PROP_FRAME_WIDTH)), height(capture.get(CAP_PROP_FRAME_HEIGHT))
{
    // Check if camera opened successfully
    if(!capture.isOpened()){
        functions::ASSERT(0, "Error opening video stream or file");
    }

    Fs = (double)(capture.get(CAP_PROP_FPS));
    numberOfFrames =  windowLength*Fs;
    foreHeadSignal = signalProcessing<float>(numberOfFrames, Fs);
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

void videoProcessing::creatingFaceForeHeadAndBackGroundMatrixFromGreenChannel()
{
    face =  cv::Mat(channels[GREEN] , faces[0]);
    foreHead = cv::Mat(channels[GREEN] , foreHeads[0]);
    background = cv::Mat(channels[GREEN] , backgroundRect);
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

void videoProcessing::fillforeHeadSignal()
{
    foreHeadSignal.fillWithValue(meanFrameValue(foreHead));
}

void videoProcessing::fillbackgroundSignal()
{
    backgroundSignal.fillWithValue(meanFrameValue(background));
}

void videoProcessing::displayFrame(bool display)
{
    drawRectangle(frame, faces[0]);
    drawRectangle(frame, foreHeads[0]);
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
    foreHeadSignal.clearSignal();
    backgroundSignal.clearSignal();
}

void videoProcessing::computeHeartRateEstimation()
{
    signalProcessing<float> filtredSig;
    signalProcessing<float> FFTOutput;

    foreHeadSignal.removeMeanValue();
    foreHeadSignal.bandPassFilterFunction(lowFreqCutOff, highFreqCutOff, filterOrder, filtredSig);
    filtredSig.signalFFT(FFTOutput);
    heartRateEstimation = FFTOutput.getFrequencyOfMaxFFT() * 60.0f;
}


//*******************************************************************************************************
//*************************** Implementation of Interface VideoProcessing class *************************
//*******************************************************************************************************

IvideoProcessing::IvideoProcessing(QObject *parent) : QObject(parent), toggleStream(false), cptFrames(0)
{
    videoObj = new videoProcessing("face.mp4");
    videoObj->faceCascadeLoading("/usr/local/share/opencv4/haarcascades/haarcascade_frontalface_alt2.xml");
    maxNbOfFrames = windowLength * static_cast<u32>(videoObj->getFs());
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

    if(cptFrames < maxNbOfFrames)
    {
        cptFrames++;
    }
    else
    {
        cptFrames = 0;
        videoObj->computeHeartRateEstimation();
        videoObj->clearSignals();
        emit sendHeartRateEstimation(videoObj->getHeartRateEstimation());
    }

    processVideoStream();

    videoObj->BGR2RGB();
    QImage output = QImage((const uchar*) videoObj->getframe().data, videoObj->getframe().cols, videoObj->getframe().rows, videoObj->getframe().step, QImage::Format_RGB888);

    emit sendFrame(output);

}


void IvideoProcessing::processVideoStream()
{
    videoObj->detectFacesFromGreenChannelInVideo();
    const std::vector<Rect>& faces = videoObj->getFaces();

    // Background detection deduced from face detection
    const Rect backgroundRect = Rect(faces[0].x + faces[0].width, faces[0].y + faces[0].height,
            videoObj->width - (faces[0].x + faces[0].width), videoObj->height -(faces[0].y + faces[0].height));

    videoObj->setbackGroundRect(backgroundRect);

    std::vector<Rect> foreHeads = faces;

    foreHeads[0].height = foreHeads[0].height/3.7; // only for manualforeHead detection
    foreHeads[0].x = foreHeads[0].x + 50; // only for manual foreHead detection
    foreHeads[0].width = foreHeads[0].width - 70; // only for manual foreHead detection

    videoObj->setRectForeHeadsVector(foreHeads);

    videoObj->creatingFaceForeHeadAndBackGroundMatrixFromGreenChannel();

    videoObj->fillforeHeadSignal();  // construction of the ForeHead Signal with mean of green Channel each frame
    videoObj->fillbackgroundSignal();// construction of the background Signal with mean of green Channel each frame

    videoObj->displayFrame(false); // only draw ROI
}

void IvideoProcessing::receiveToggleStream()
{
    toggleStream = !toggleStream;
    if(!toggleStream) emit sendHeartRateEstimation(0.0);
}
