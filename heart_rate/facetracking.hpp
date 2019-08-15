#ifndef FACETRACKING_HPP
#define FACETRACKING_HPP

#include <opencv2/highgui.hpp>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/objdetect/objdetect.hpp>
#include <iostream>

using namespace cv;
using namespace std;


bool faceCascadeLoading(CascadeClassifier& faceCascade,const string& path)
{
    // Open xml file for face tracking using classification
    bool faceCascadeLoading =  faceCascade.load(path);

    if(!faceCascadeLoading){
        cout<<"Error while opening xml file for faceTracking"<<endl;
        return false;
    }

    return true;
}

void drawRectangle(cv::Mat& inputOutputImage, const Rect& rect)
{
    int x = rect.x;
    int y = rect.y;
    int h = y + rect.height;
    int w = x + rect.width;
    rectangle(inputOutputImage, Point (x,y), Point (w,h), Scalar(255,255,255), 2, 8, 0);
}

#endif // FACETRACKING_HPP
