#ifndef VIDEOPROCESSING_HPP
#define VIDEOPROCESSING_HPP

#include <signalProcessing.hpp>

using namespace cv;
using namespace std;


//***************************************************************************************************//
//***************************************************************************************************//
//***************************************************************************************************//
//**************************************** videoProcessing Class ************************************//
//***************************************************************************************************//
//***************************************************************************************************//
//***************************************************************************************************//
class videoProcessing
{
    friend class TestVideoProcessing;

public :
    videoProcessing() : capture(), Fs(0.0), numberOfFrames (0), faceCascade(), frame(), backgroundRect(),
        foreHead(), face(), background(), foreHeadSignal(), backgroundSignal(), width(0), height(0) {}

    videoProcessing(const String& filename) : capture(filename), Fs (0.0), numberOfFrames (0), faceCascade(), frame(),
        backgroundRect(),foreHead(), face(), background(), foreHeadSignal(), backgroundSignal(),
        width(capture.get(CAP_PROP_FRAME_WIDTH)), height(capture.get(CAP_PROP_FRAME_HEIGHT))
    {
        // Check if camera opened successfully
        if(!capture.isOpened()){
            ASSERT(0, "Error opening video stream or file");
        }

        Fs = (double)(capture.get(CAP_PROP_FPS));
        numberOfFrames =  capture.get(CAP_PROP_FRAME_COUNT); // only for videoInput != webcam (filename != 0)
        foreHeadSignal = signalProcessing<float>(numberOfFrames, Fs);
        backgroundSignal = signalProcessing<float>(numberOfFrames, Fs);
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
        if(!faceCascade.load(path))
        {
            ASSERT(0, "Error while opening xml file for faceTracking");
        }
    }

    void capturingFrames()
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

    void detectFacesFromGreenChannelInVideo()
    {
        /* first param was GreyColorImage */
        faceCascade.detectMultiScale(frame, faces, 1.1, 2, 0|cv::CASCADE_SCALE_IMAGE, Size(100, 100) );
    }

    void creatingFaceForeHeadAndBackGroundMatrixFromGreenChannel()
    {
        face =  cv::Mat(channels[GREEN] , faces[0]);
        foreHead = cv::Mat(channels[GREEN] , foreHeads[0]);
        background = cv::Mat(channels[GREEN] , backgroundRect);
    }

    static void drawRectangle(cv::Mat& inputOutputImage, const Rect& rect)
    {
        rectangle(inputOutputImage, Point (rect.x ,rect.y), Point (rect.x + rect.width, rect.y + rect.height), Scalar(255,255,255), 2, 8, 0);
    }

    static void imageColorToGreyTransform(const cv::Mat& InputImageColor, cv::Mat& OutputGreyColor)
    {
        cvtColor(InputImageColor, OutputGreyColor, COLOR_BGR2GRAY);
        equalizeHist(OutputGreyColor, OutputGreyColor);
    }

    static float meanFrameValue(const cv::Mat& inputFrame)
    {
        Scalar tmpMean = mean(inputFrame);
        return tmpMean.val[0];
    }

    void fillforeHeadSignal()
    {
        foreHeadSignal.fillWithValue(meanFrameValue(foreHead));
    }

    void fillbackgroundSignal()
    {
        backgroundSignal.fillWithValue(meanFrameValue(background));
    }

    void displayFrame()
    {
        drawRectangle(frame, faces[0]);
        drawRectangle(frame, foreHeads[0]);
        drawRectangle(frame, backgroundRect);
        imshow( "Frame", frame );
    }

    bool isFrameEmpty() { return frame.empty(); }


    /************************************** Setters *********************************/
    void setbackGroundRect(const Rect& background) { backgroundRect = background; }
    void setRectFacesVector(const std::vector<Rect>& rectFacesVector) { faces = rectFacesVector; }
    void setRectForeHeadsVector(const std::vector<Rect>& rectForeHeadsVector) { foreHeads =  rectForeHeadsVector; }

    /************************************** Getters ********************************/
    const std::vector<Rect>& getFaces() const { return faces; }
    const cv::Mat& getframe() const { return frame; }
    const Rect& getbackgroundRect() const { return backgroundRect; }
    const cv::Mat& getforeHead() const { return foreHead; }
    const cv::Mat& getbackground() const { return background; }
    const double& getFs() const { return Fs; }
    const unsigned int& getNumOfFrames() const { return numberOfFrames; }
    signalProcessing<float>& getforeHeadSignal() { return foreHeadSignal; }
    signalProcessing<float>& getbackgroundSignal() { return backgroundSignal; }


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

public:
    const unsigned int width;
    const unsigned int height;

};



//********************************************************************************************************//
//********************************************************************************************************//
//********************************************************************************************************//
//********************************************************************************************************//
//************************************* Interface for GUI with QT ****************************************//
//********************************************************************************************************//
//********************************************************************************************************//
//********************************************************************************************************//
//********************************************************************************************************//


// Interface for GUI with QT
class IvideoProcessing : public QObject
{
    Q_OBJECT


private :

    videoProcessing* videoObj;
    bool toggleStream;

public :

    explicit IvideoProcessing(QObject *parent = 0) : QObject(parent), toggleStream(false)
    {
        videoObj = new videoProcessing("face.mp4");
        videoObj->faceCascadeLoading("/usr/local/share/opencv4/haarcascades/haarcascade_frontalface_alt2.xml");
    }

    ~IvideoProcessing()
    {
        delete videoObj;
    }


signals:

    void sendFrame(QImage frameProcessed);

public slots:

    void receiveGrabFrame()
    {
        if(!toggleStream) return;
        videoObj->capturingFrames();
        if(videoObj->isFrameEmpty()) return;

        processVideoStream();

        // check colors ??
        QImage output((const unsigned char *)videoObj->getframe().data, videoObj->getframe().cols, videoObj->getframe().rows, QImage::Format_Indexed8);

        emit sendFrame(output);
    }

    void receiveToggleStream()
    {
        toggleStream = !toggleStream;
    }

private :

    void processVideoStream()
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
    }

};





#endif // VIDEOPROCESSING_HPP
