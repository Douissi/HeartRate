#include <stdlib.h>
#include <stdio.h>
#include <QApplication>
#include "videoProcessing.hpp"
#include "qcvwidget.h"

using namespace cv;

void testWithNoGui()
{
    // Create a VideoCapture object and open the input file
    // If the input is the web camera, pass 0 instead of the video file name
    videoProcessing videoObj("face3.mp4");

    videoObj.faceCascadeLoading("/usr/local/share/opencv4/haarcascades/haarcascade_frontalface_alt2.xml");


    /************************************************* Video Processing *************************************************/
    while(1)
    {
        videoObj.capturingFrames();
        if(videoObj.isFrameEmpty()) break;

        videoObj.detectFacesFromGreenChannelInVideo();
        const std::vector<Rect>& faces = videoObj.getFaces();
        const Rect backgroundRect = Rect(faces[0].x + faces[0].width, faces[0].y + faces[0].height,
                videoObj.width - (faces[0].x + faces[0].width), videoObj.height -(faces[0].y + faces[0].height));

        videoObj.setBackGroundRect(backgroundRect);


        videoObj.skinDetectionOverFaceAndMatrixCreationFromGreenChannel();

        videoObj.fillFaceSignal();  // construction of the faceSignal Signal with mean of green Channel each frame
        videoObj.fillBackgroundSignal();


        // Display results in frames
        videoObj.displayFrame();

        // Press  ESC on keyboard to exit
        char c=(char)waitKey(25);
        if(c==27)
            break;
    }


    /******************************************** Signal Processing *************************************************/

    signalProcessing<float>& faceSignalSignal = videoObj.getFaceSignal();
    signalProcessing<float>& backgroundSignal = videoObj.getBackgroundSignal();
    signalProcessing<float> ouputEstimationSignal;
    signalProcessing<float> filteredOutputEstimationSignal;
    signalProcessing<float> filtredSig;
    signalProcessing<float> FFTOutput;

    backgroundSignal.saveSigToFile("backgroundSignal.txt");
    faceSignalSignal.saveSigToFile("faceSignalSignal.txt");

    cout << "************************** Signal Processing *************************" << endl;

    // Illumination Rectification
    signalProcessing<float>::nlmsFilter(backgroundSignal, faceSignalSignal, filterOrder, mu, ouputEstimationSignal);
    ouputEstimationSignal.bandPassFilterFunction(lowFreqCutOff, highFreqCutOff, filterOrder, filteredOutputEstimationSignal);
    filteredOutputEstimationSignal.saveSigToFile("nlmsOutputEstimation.txt");
    filteredOutputEstimationSignal.signalFFT(FFTOutput, SAVE_FFT, "FFTnlmsOutputEstimation.txt");

    // remove mean value "offset" of the signal
    faceSignalSignal.removeMeanValue();

    // Signal filter  ******************** TO BE OPTIMIZED **************************
    faceSignalSignal.saveSigToFile("faceSignalMeanValueRemoved.txt");
    faceSignalSignal.signalFFT(FFTOutput, SAVE_FFT, "FFTfaceSignalMeanValueRemoved.txt");


    faceSignalSignal.bandPassFilterFunction(lowFreqCutOff, highFreqCutOff, filterOrder, filtredSig);
    filtredSig.saveSigToFile("filteredfaceSignalBandPass.txt");
    filtredSig.signalFFT(FFTOutput, SAVE_FFT, "FFTfiltredfaceSignal.txt");


    // ************************************ plotting the signals **********************

//      functions::plotWithGNUPLOT("faceSignalSignal.txt", SIG);
    //  functions::plotWithGNUPLOT("backgroundSignal.txt", SIG);
    //  functions::plotWithGNUPLOT("nlmsOutputEstimation.txt", SIG);
    //  functions::plotWithGNUPLOT("FFTnlmsOutputEstimation.txt", FFT);
//      functions::plotWithGNUPLOT("faceSignalMeanValueRemoved.txt", SIG);
//      functions::plotWithGNUPLOT("FFTfaceSignalMeanValueRemoved.txt", FFT);
//      functions::plotWithGNUPLOT("filteredfaceSignalBandPass.txt", SIG);
//      functions::plotWithGNUPLOT("FFTfiltredfaceSignal.txt", FFT);

}

uchar istestWithNoGui = 0;

int main(int argc, char *argv[])
{
    if(istestWithNoGui)
    {
        testWithNoGui();
        return 0;
    }
    else
    {
        QApplication a(argc, argv);
        QcvWidget w;
        w.show();

        return a.exec();
    }
}
