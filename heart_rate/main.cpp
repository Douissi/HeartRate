#include <stdlib.h>
#include <stdio.h>
#include "mainwindow.h"
#include <QApplication>
#include <facetracking.hpp>
#include <signalprocessingfunctions.hpp>


using namespace cv;


int main(){

  float lowFreqCutOff = 0.5; // in Hz, corresponding to 30Bp/min
  float highFreqCutOff = 4.5; // in Hz, corresponding to 270 Bp/min
  int   filterOrder = 30; // Filter Order in BandPass Hanning Window Filter fixed to 30

  // Create a VideoCapture object and open the input file
  // If the input is the web camera, pass 0 instead of the video file name
  VideoCapture capture("face.mp4");
  double Fs = (double)(capture.get(CAP_PROP_FPS));

  // Check if camera opened successfully
  if(!capture.isOpened()){
    cout << "Error opening video stream or file" << endl;
    return -1;
  }

  CascadeClassifier faceCascade;

  // Open xml file for face tracking using classification
  bool ResFaceCascadeLoading = faceCascadeLoading(faceCascade,"/root/opencv/opencv/data/haarcascades/haarcascade_frontalface_alt2.xml");

  if(!ResFaceCascadeLoading){
      cout<<"Error while opening xml file for faceTracking"<<endl;
      return -1;
  }

  //maybe to reserve all memory to optimize running time
  std::vector <float> foreHeadSignalSamples;

  /******************************* Video Processing ***************************/

  while(1){

   cv::Mat frame;
    // Capture frame-by-frame
   capture >> frame;

    // If the frame is empty, break immediately
   if (frame.empty())
     break;

   cv::Mat channels[3];   // Channels RGB
   split(frame, channels); // split source channels[1] is the green one

   // Rect for faces
   std::vector<Rect> faces;

    /* first param was GreyColorImage */
   faceCascade.detectMultiScale( channels[1], faces, 1.1, 2, 0|cv::CASCADE_SCALE_IMAGE, Size(100, 100) );

   faces[0].height = faces[0].height/3.7; // only for manualforeHead detection
   faces[0].x = faces[0].x + 50; // only for manual foreHead detection
   faces[0].width = faces[0].width - 70; // only for manual foreHead detection

//    faces[0].y -= 70;
//    faces[0].height = faces[0].height/3 ; // only for manualforeHead detection
//    faces[0].x = faces[0].x + 60; // only for manual foreHead detection
//    faces[0].width = faces[0].width -140; // only for manual foreHead detection

   int x = faces[0].x;
   int y = faces[0].y;
   int h = y+faces[0].height;
   int w = x+faces[0].width;
   rectangle(frame, Point (x,y), Point (w,h), Scalar(255,255,255), 2, 8, 0);

   cv::Mat foreHead(channels[1], faces[0]); // new Matrix containing only ForeHead image (in Green Channel)

   Scalar tmpMean = mean(foreHead);
   float meanValue = tmpMean.val[0];
   foreHeadSignalSamples.push_back(meanValue);  // construction of the ForeHead Signal with mean of green Channel each frame

   imshow( "ROI 1 : forehead", foreHead );

    // Display the resulting frame
   imshow( "Frame", frame );

    // Press  ESC on keyboard to exit
   char c=(char)waitKey(25);
   if(c==27)
     break;
  }

//  saveSigToFile<float>(foreHeadSignalSamples, "foreHeadSignalSamplesWithMeanValue.txt");

  /******************* Signal Processing *******************/

    // remove mean value "offset" of the signal
  float meanForeHeadSignal = meanVector<float>(foreHeadSignalSamples);
  std::transform(foreHeadSignalSamples.begin(), foreHeadSignalSamples.end(),foreHeadSignalSamples.begin(),
                 [&meanForeHeadSignal](float v) -> float {return v - meanForeHeadSignal;});

  // print samples from ForeHead signal
  for (auto v : foreHeadSignalSamples) cout << "ForeHead samples are "<< v << endl;

  // Signal filter  ******* TO BE OPTIMIZED *******
  saveSigToFile<float>(foreHeadSignalSamples, "foreHeadSignalSamples.txt");
  SignalFFT(foreHeadSignalSamples, Fs, "FFTResult.txt");

  vector<float> filteredSig(foreHeadSignalSamples.size(), 0.0);
  bandPassFilterFunction(foreHeadSignalSamples,lowFreqCutOff ,highFreqCutOff, Fs, filterOrder, filteredSig);
  saveSigToFile<float>(filteredSig, "filteredSig.txt");
  SignalFFT(filteredSig, Fs, "FFTFilteredResult.txt");



  // When everything done, release the video capture object
  capture.release();

  // Closes all the frames
  destroyAllWindows();

  return 0;
}
