#include <stdlib.h>
#include <stdio.h>
#include "mainwindow.h"
#include <QApplication>
#include <videoProcessing.hpp>
#include <signalProcessing.hpp>

using namespace cv;


int main(){

  // Create a VideoCapture object and open the input file
  // If the input is the web camera, pass 0 instead of the video file name
  videoProcessing videoObj("73BPmin.mp4");

  videoObj.faceCascadeLoading("/usr/local/share/opencv4/haarcascades/haarcascade_frontalface_alt2.xml");


  /************************************************* Video Processing *************************************************/
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

   videoObj.creatingFaceForeHeadAndBackGroundMatrixFromGreenChannel();

   videoObj.fillforeHeadSignal();  // construction of the ForeHead Signal with mean of green Channel each frame
   videoObj.fillbackgroundSignal();


   // Display results in frames
   videoProcessing::drawRectangle(frame, faces[0]);
   videoProcessing::drawRectangle(frame, foreHeads[0]);
   videoProcessing::drawRectangle(frame, backgroundRect);
   imshow( "ROI 1 : forehead", videoObj.getforeHead()); // Matrix containing only ForeHead image (in Green Channel)
   imshow( "BackGround ", videoObj.getbackground() ); // Matrix containing only background image (in Green Channel)
   imshow( "Frame", frame );

    // Press  ESC on keyboard to exit
   char c=(char)waitKey(25);
   if(c==27)
     break;
  }



  /******************************************** Signal Processing *************************************************/
  float lowFreqCutOff = 0.5; // in Hz, corresponding to 30Bp/min
  float highFreqCutOff = 4.5; // in Hz, corresponding to 270 Bp/min
  int   filterOrder = 40; // Filter Order in BandPass Hanning Window Filter fixed to 30

  const double& Fs = videoObj.getFs();

  signalProcessing<float>& foreHeadSignal = videoObj.getforeHeadSignal();
  signalProcessing<float>& backgroundSignal = videoObj.getbackgroundSignal();


  cout << "******************* Signal Processing *******************" << endl;
  backgroundSignal.saveSigToFile("backgroundSignalSamples.txt");
  foreHeadSignal.saveSigToFile("foreHeadSignalSamples.txt");


  // Illumination Rectification
  vector<float> outputEstimation(videoObj.getNumOfFrames(), 0.0);
  nlmsFilter<float>(backgroundSignal.getSignalSamples(), foreHeadSignal.getSignalSamples(), filterOrder, 0.5, outputEstimation);

  // Avoid first incorrect sample
  outputEstimation[0] = outputEstimation[1];
  vector<float> filteredOutputEstimation(outputEstimation.size(), 0.0);
  bandPassFilterFunction<float>(outputEstimation, lowFreqCutOff, highFreqCutOff, Fs, filterOrder, filteredOutputEstimation);
  signalFFT(filteredOutputEstimation, Fs, "FFTfilteredOutputEstimation.txt");
  saveSigToFile<float>(filteredOutputEstimation, "filteredOutputEstimation.txt");


   // remove mean value "offset" of the signal
  foreHeadSignal.removeMeanValue();

  // Signal filter  ******* TO BE OPTIMIZED *******
  foreHeadSignal.saveSigToFile("foreHeadSignalSamplesMeanValueRemoved.txt");
  foreHeadSignal.signalFFT("FFTResult.txt");


  signalProcessing<float> filtredSig(videoObj.getNumOfFrames(), Fs);
  foreHeadSignal.bandPassFilterFunction(lowFreqCutOff, highFreqCutOff, filterOrder, filtredSig);
  filtredSig.saveSigToFile("filteredSig.txt");
  filtredSig.signalFFT("FFTFilteredResult.txt");

  return 0;
}
