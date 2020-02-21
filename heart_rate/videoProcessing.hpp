#ifndef VIDEOPROCESSING_HPP
#define VIDEOPROCESSING_HPP
#include "signalProcessing.hpp"
#include <QObject>
#include <QImage>


//***************************************************************************************************//
//**************************************** videoProcessing Class ************************************//
//***************************************************************************************************//

class videoProcessing
{
    friend class TestVideoProcessing;

public :
    videoProcessing();
    videoProcessing(const String& filename);
    // constructor for webcam entry == 0
    videoProcessing(const int& entry);

    virtual ~videoProcessing();

    void faceCascadeLoading(const string& path);
    void capturingFrames();
    void detectFacesFromGreenChannelInVideo();
    void creatingFaceForeHeadAndBackGroundMatrixFromGreenChannel();
    static void drawRectangle(cv::Mat& inputOutputImage, const Rect& rect);
    static void imageColorToGreyTransform(const cv::Mat& InputImageColor, cv::Mat& OutputGreyColor);
    static float meanFrameValue(const cv::Mat& inputFrame);
    void fillforeHeadSignal();
    void fillbackgroundSignal();
    void displayFrame(bool display = true);
    bool isFrameEmpty();
    void BGR2RGB();
    void clearSignals();
    void computeHeartRateEstimation();

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
    const float& getHeartRateEstimation() const { return heartRateEstimation; }


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
    float heartRateEstimation;

public:
    const unsigned int width;
    const unsigned int height;

};


//********************************************************************************************************//
//************************************* Interface for GUI with QT ****************************************//
//********************************************************************************************************//


// Interface for GUI with QT
class IvideoProcessing : public QObject
{
    Q_OBJECT

private :

    videoProcessing* videoObj;
    bool toggleStream;
    u32 cptFrames;
    u32 maxNbOfFrames;

public :

    explicit IvideoProcessing(QObject *parent = 0);
    ~IvideoProcessing();

    const double& getFs() const { return videoObj->getFs(); }
    const u32& getWidth() const { return videoObj->width; }
    const u32& getHeight() const { return videoObj->height; }

signals:

    void sendFrame(const QImage& frameProcessed);
    void sendHeartRateEstimation(const float& heartRateEstimation);

public slots:

    void receiveGrabFrame();
    void receiveToggleStream();

private :

    void processVideoStream();
};


#endif // VIDEOPROCESSING_HPP
