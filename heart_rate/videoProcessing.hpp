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
    void skinDetectionOverFaceAndMatrixCreationFromGreenChannel();
    static void drawRectangle(cv::Mat& inputOutputImage, const Rect& rect);
    static void imageColorToGreyTransform(const cv::Mat& InputImageColor, cv::Mat& OutputGreyColor);
    static float meanFrameValue(const cv::Mat& inputFrame);
    void fillFaceSignal();
    void fillBackgroundSignal();
    void displayFrame(bool display = true);
    bool isFrameEmpty();
    void BGR2RGB();
    void clearSignals();
    void computeHeartRateEstimation(signalProcessing<float>& filtredSig, signalProcessing<float>& FFTOutput);

    /************************************** Setters *********************************/
    void setBackGroundRect(const Rect& background) { backgroundRect = background; }

    /************************************** Getters ********************************/
    const std::vector<Rect>& getFaces() const { return faces; }
    const cv::Mat& getFrame() const { return frame; }
    const cv::Mat& getFace() const { return face; }
    const double& getFs() const { return Fs; }
    signalProcessing<float>& getFaceSignal() { return faceSignal; }
    signalProcessing<float>& getBackgroundSignal() { return backgroundSignal; }
    const float& getHeartRateEstimation() const { return heartRateEstimation; }


private:
    VideoCapture capture;
    double Fs;
    unsigned int numberOfFrames;
    CascadeClassifier faceCascade; // for ROI tracking CascadeClassifier : needs to be improved for tracking ROI
    cv::Mat frame; // frame
    cv::Mat channels[3] = {};   // Channels RGB
    std::vector<Rect> faces = {}; // Rectangles containing faces
    Rect backgroundRect; // a rectangle containing background dimension
    cv::Mat face;
    cv::Mat background;
    signalProcessing<float> faceSignal; // signal samples
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
    void sendFace(const QImage& faceProcessed);
    void sendHeartRateEstimation(const float& heartRateEstimation);
    void sendSignalToPlot(const vectorFloat sigToPlot);

public slots:

    void receiveGrabFrame();
    void receiveToggleStream();

private :

    void processVideoStream();
};


#endif // VIDEOPROCESSING_HPP
