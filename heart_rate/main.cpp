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
   const Rect backgroundRect = Rect(5, 116 , 128, 200);
   videoObj.setbackGroundRect(backgroundRect);

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
  signalProcessing<float> ouputEstimationSignal(videoObj.getNumOfFrames(), Fs);
  signalProcessing<float> filteredOutputEstimationSignal(videoObj.getNumOfFrames(), Fs);
  signalProcessing<float> filtredSig(videoObj.getNumOfFrames(), Fs);

  backgroundSignal.saveSigToFile("backgroundSignal.txt");
  foreHeadSignal.saveSigToFile("foreHeadSignal.txt");

  cout << "************************** Signal Processing *************************" << endl;

  // Illumination Rectification
  signalProcessing<float>::nlmsFilter(backgroundSignal.getSignalSamples(), foreHeadSignal.getSignalSamples(),
                                      filterOrder, 0.5, ouputEstimationSignal.getSignalSamples());

  ouputEstimationSignal.bandPassFilterFunction(lowFreqCutOff, highFreqCutOff, filterOrder, filteredOutputEstimationSignal);
  filteredOutputEstimationSignal.saveSigToFile("nlmsOutputEstimation.txt");
  filteredOutputEstimationSignal.signalFFT("FFTnlmsOutputEstimation.txt");


   // remove mean value "offset" of the signal
  foreHeadSignal.removeMeanValue();

  // Signal filter  ******************** TO BE OPTIMIZED **************************
  foreHeadSignal.saveSigToFile("foreHeadMeanValueRemoved.txt");
  foreHeadSignal.signalFFT("FFTforeHeadMeanValueRemoved.txt");


  foreHeadSignal.bandPassFilterFunction(lowFreqCutOff, highFreqCutOff, filterOrder, filtredSig);
  filtredSig.saveSigToFile("filteredForeHeadBandPass.txt");
  filtredSig.signalFFT("FFTfiltredForeHead.txt");

  // ************************************ plotting the signals **********************

//  plotWithGNUPLOT("foreHeadSignal.txt", SIG);
//  plotWithGNUPLOT("backgroundSignal.txt", SIG);
//  plotWithGNUPLOT("nlmsOutputEstimation.txt", SIG);
//  plotWithGNUPLOT("FFTnlmsOutputEstimation.txt", FFT);
//  plotWithGNUPLOT("foreHeadMeanValueRemoved.txt", SIG);
//  plotWithGNUPLOT("FFTforeHeadMeanValueRemoved.txt", FFT);
//  plotWithGNUPLOT("filteredForeHeadBandPass.txt", SIG);
//  plotWithGNUPLOT("FFTfiltredForeHead.txt", FFT);


  return 0;
}
