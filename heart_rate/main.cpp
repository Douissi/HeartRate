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
  int   filterOrder = 40; // Filter Order in BandPass Hanning Window Filter fixed to 30

  // Create a VideoCapture object and open the input file
  // If the input is the web camera, pass 0 instead of the video file name
  VideoCapture capture("73BPmin.mp4");

  double Fs = (double)(capture.get(CAP_PROP_FPS));
  unsigned int numberOfFrames =  capture.get(CAP_PROP_FRAME_COUNT); // only for videoInput != webcam

  // Check if camera opened successfully
  if(!capture.isOpened()){
    cout << "Error opening video stream or file" << endl;
    return -1;
  }

  CascadeClassifier faceCascade;

  // Open xml file for face tracking using classification
  bool ResFaceCascadeLoading = faceCascadeLoading(faceCascade,"/usr/local/share/opencv4/haarcascades/haarcascade_frontalface_alt2.xml");

  if(!ResFaceCascadeLoading){
      cout<<"Error while opening xml file for faceTracking"<<endl;
      return -1;
  }

  std::vector <float> foreHeadSignalSamples;
  foreHeadSignalSamples.reserve(numberOfFrames);
  std::vector<float> backgroundSignalSamples;
  backgroundSignalSamples.reserve(numberOfFrames);

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
   Rect backgroundRect(5, 116 , 128, 200);

    /* first param was GreyColorImage */
   faceCascade.detectMultiScale( channels[1], faces, 1.1, 2, 0|cv::CASCADE_SCALE_IMAGE, Size(100, 100) );

   faces[0].y += 20;
   faces[0].height = faces[0].height/3.7 - 20; // only for manualforeHead detection
   faces[0].x += 60; // only for manual foreHead detection
   faces[0].width -= 130; // only for manual foreHead detection

   drawRectangle(frame,faces[0]);
   drawRectangle(frame,backgroundRect);

   cv::Mat foreHead(channels[1], faces[0]); // new Matrix containing only ForeHead image (in Green Channel)
   cv::Mat background(channels[1], backgroundRect); // new Matrix containing only background image (in Green Channel)

   Scalar tmpMean = mean(foreHead);
   float meanValue = tmpMean.val[0];
   foreHeadSignalSamples.push_back(meanValue);  // construction of the ForeHead Signal with mean of green Channel each frame

   tmpMean = mean(background);
   meanValue = tmpMean.val[0];
   backgroundSignalSamples.push_back(meanValue);


   imshow( "ROI 1 : forehead", foreHead );

   imshow( "BackGround ", background );

    // Display the resulting frame
   imshow( "Frame", frame );

    // Press  ESC on keyboard to exit
   char c=(char)waitKey(25);
   if(c==27)
     break;
  }

  /******************* Signal Processing *******************/

  // Illumination Rectification
  vector<float> outputEstimation(numberOfFrames, 0.0);

  saveSigToFile<float>(backgroundSignalSamples, "backgroundSignalSamples.txt");
  saveSigToFile<float>(foreHeadSignalSamples, "foreHeadSignalSamples.txt");

   // remove mean value "offset" of the signal
  removeMeanValue<float>(foreHeadSignalSamples);

  // Signal filter  ******* TO BE OPTIMIZED *******
  saveSigToFile<float>(foreHeadSignalSamples, "foreHeadSignalSamplesMeanValueRemoved.txt");
  SignalFFT(foreHeadSignalSamples, Fs, "FFTResult.txt");

  vector<float> filteredSig(foreHeadSignalSamples.size(), 0.0);
  bandPassFilterFunction<float>(foreHeadSignalSamples,lowFreqCutOff ,highFreqCutOff, Fs, filterOrder, filteredSig);
  saveSigToFile<float>(filteredSig, "filteredSig.txt");
  SignalFFT(filteredSig, Fs, "FFTFilteredResult.txt");


  // When everything done, release the video capture object
  capture.release();

  // Closes all the frames
  destroyAllWindows();

  return 0;
}
