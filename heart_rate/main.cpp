#include <stdlib.h>
#include <stdio.h>
#include "mainwindow.h"
#include <QApplication>
#include <videoProcessing.hpp>
#include <signalProcessing.hpp>

using namespace cv;


int main(){

  float lowFreqCutOff = 0.5; // in Hz, corresponding to 30Bp/min
  float highFreqCutOff = 4.5; // in Hz, corresponding to 270 Bp/min
  int   filterOrder = 40; // Filter Order in BandPass Hanning Window Filter fixed to 30

  // Create a VideoCapture object and open the input file
  // If the input is the web camera, pass 0 instead of the video file name
  videoProcessing videoObj("73BPmin.mp4");

  const double& Fs = videoObj.getFs();
  const unsigned int& numberOfFrames =  videoObj.getNumOfFrames(); // only for videoInput != webcam

  videoObj.faceCascadeLoading("/usr/local/share/opencv4/haarcascades/haarcascade_frontalface_alt2.xml");

  std::vector <float> foreHeadSignalSamples;
  foreHeadSignalSamples.reserve(numberOfFrames);
  std::vector<float> backgroundSignalSamples;
  backgroundSignalSamples.reserve(numberOfFrames);

  /******************************* Video Processing ***************************/

  while(1){

   videoObj.capturingFrames();
   cv::Mat& frame = videoObj.getframe();
   if (frame.empty()) break;

   videoObj.detectFacesFromGreenChannelInVideo();
   const std::vector<Rect>& faces = videoObj.getFaces();
   videoObj.setbackGroundRect(Rect(5, 116 , 128, 200));
   const Rect& backgroundRect = videoObj.getbackgroundRect();

   std::vector<Rect> foreHeads = faces;

   foreHeads[0].y += 20; // only for manualforeHead detection
   foreHeads[0].height = foreHeads[0].height/3.7 - 20; // only for manualforeHead detection
   foreHeads[0].x += 60; // only for manual foreHead detection
   foreHeads[0].width -= 130; // only for manual foreHead detection

   videoObj.setRectForeHeadsVector(foreHeads);
   videoProcessing::drawRectangle(frame, faces[0]);
   videoProcessing::drawRectangle(frame, foreHeads[0]);
   videoProcessing::drawRectangle(frame, backgroundRect);

   videoObj.creatingFaceForeHeadAndBackGroundMatrixFromGreenChannel();

   const cv::Mat& foreHead = videoObj.getforeHead(); // new Matrix containing only ForeHead image (in Green Channel)
   const cv::Mat& background = videoObj.getbackground(); // new Matrix containing only background image (in Green Channel)



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

  cout << "******************* Signal Processing *******************" << endl;
  saveSigToFile<float>(backgroundSignalSamples, "backgroundSignalSamples.txt");
  saveSigToFile<float>(foreHeadSignalSamples, "foreHeadSignalSamples.txt");


  // Illumination Rectification
  vector<float> outputEstimation(numberOfFrames, 0.0);
  nlmsFilter<float>(backgroundSignalSamples, foreHeadSignalSamples, filterOrder, 0.5, outputEstimation);

  // Avoid first incorrect sample
  outputEstimation[0] = outputEstimation[1];
  vector<float> filteredOutputEstimation(outputEstimation.size(), 0.0);
  bandPassFilterFunction<float>(outputEstimation, lowFreqCutOff, highFreqCutOff, Fs, filterOrder, filteredOutputEstimation);
  signalFFT(filteredOutputEstimation, Fs, "FFTfilteredOutputEstimation.txt");
  saveSigToFile<float>(filteredOutputEstimation, "filteredOutputEstimation.txt");


   // remove mean value "offset" of the signal
  removeMeanValue<float>(foreHeadSignalSamples);

  // Signal filter  ******* TO BE OPTIMIZED *******
  saveSigToFile<float>(foreHeadSignalSamples, "foreHeadSignalSamplesMeanValueRemoved.txt");
  signalFFT(foreHeadSignalSamples, Fs, "FFTResult.txt");

  vector<float> filteredSig(foreHeadSignalSamples.size(), 0.0);
  bandPassFilterFunction<float>(foreHeadSignalSamples,lowFreqCutOff ,highFreqCutOff, Fs, filterOrder, filteredSig);
  saveSigToFile<float>(filteredSig, "filteredSig.txt");
  signalFFT(filteredSig, Fs, "FFTFilteredResult.txt");

  return 0;
}
