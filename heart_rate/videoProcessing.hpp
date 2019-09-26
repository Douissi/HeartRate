#ifndef VIDEOPROCESSING_HPP
#define VIDEOPROCESSING_HPP

#include <opencv2/highgui.hpp>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/objdetect/objdetect.hpp>
#include <iostream>

using namespace cv;
using namespace std;


class videoProcessing
{
public :
    videoProcessing() : capture(), Fs(0.0), numberOfFrames (0), faceCascade(), frame(), backgroundRect(),
        foreHead(), face(), background() {}

    videoProcessing(const String& filename) : capture(filename), numberOfFrames (0), faceCascade(), frame(), backgroundRect(),
        foreHead(), face(), background()
    {
        // Check if camera opened successfully
        if(!capture.isOpened()){
            cout << "Error opening video stream or file" << endl;
            assert(0);
        }

        Fs = (double)(capture.get(CAP_PROP_FPS));
        numberOfFrames =  capture.get(CAP_PROP_FRAME_COUNT); // only for videoInput != webcam (filename != 0)
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
            cout<<"Error while opening xml file for faceTracking"<<endl;
            assert(0);
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
        faceCascade.detectMultiScale( channels[1], faces, 1.1, 2, 0|cv::CASCADE_SCALE_IMAGE, Size(100, 100) );
    }

    void creatingFaceForeHeadAndBackGroundMatrixFromGreenChannel()
    {
        face =  cv::Mat(channels[1] , faces[0]);
        foreHead = cv::Mat(channels[1] , foreHeads[0]);
        background = cv::Mat(channels[1] , backgroundRect);
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



    /************************************** Setters *********************************/
    void setbackGroundRect(const Rect& background) { backgroundRect = background; }
    void setRectFacesVector(const std::vector<Rect>& rectFacesVector) { faces = rectFacesVector; }
    void setRectForeHeadsVector(const std::vector<Rect>& rectForeHeadsVector) { foreHeads =  rectForeHeadsVector; }

    /************************************** Getters ********************************/
    const std::vector<Rect>& getFaces() const { return faces; }
    cv::Mat& getframe() { return frame; }
    const Rect& getbackgroundRect() const { return backgroundRect; }
    const cv::Mat& getforeHead() const { return foreHead; }
    const cv::Mat& getbackground() const { return background; }
    const double& getFs() const { return Fs; }
    const unsigned int& getNumOfFrames() const { return numberOfFrames; }


private:
    VideoCapture capture;
    double Fs;
    unsigned int numberOfFrames;
    CascadeClassifier faceCascade; // for ROI tracking
    cv::Mat frame; // frame
    cv::Mat channels[3] = {};   // Channels RGB
    std::vector<Rect> faces = {}; // Rectangles containing faces
    std::vector<Rect> foreHeads = {}; // Rectangles containing foreHeads
    Rect backgroundRect; // a rectangle containing background dimension
    cv::Mat foreHead;
    cv::Mat face;
    cv::Mat background;

};


#endif // VIDEOPROCESSING_HPP
