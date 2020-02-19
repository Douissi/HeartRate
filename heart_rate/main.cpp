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
    videoProcessing videoObj("face.mp4");

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

        videoObj.setbackGroundRect(backgroundRect);

        std::vector<Rect> foreHeads = faces;

        foreHeads[0].height = foreHeads[0].height/3.7; // only for manualforeHead detection
        foreHeads[0].x = foreHeads[0].x + 50; // only for manual foreHead detection
        foreHeads[0].width = foreHeads[0].width - 70; // only for manual foreHead detection

        videoObj.setRectForeHeadsVector(foreHeads);

        videoObj.creatingFaceForeHeadAndBackGroundMatrixFromGreenChannel();

        videoObj.fillforeHeadSignal();  // construction of the ForeHead Signal with mean of green Channel each frame
        videoObj.fillbackgroundSignal();


        // Display results in frames
        videoObj.displayFrame();

        // Press  ESC on keyboard to exit
        char c=(char)waitKey(25);
        if(c==27)
            break;
    }


    /******************************************** Signal Processing *************************************************/

    signalProcessing<float>& foreHeadSignal = videoObj.getforeHeadSignal();
    signalProcessing<float>& backgroundSignal = videoObj.getbackgroundSignal();
    signalProcessing<float> ouputEstimationSignal;
    signalProcessing<float> filteredOutputEstimationSignal;
    signalProcessing<float> filtredSig;
    signalProcessing<float> FFTOutput;

    backgroundSignal.saveSigToFile("backgroundSignal.txt");
    foreHeadSignal.saveSigToFile("foreHeadSignal.txt");

    cout << "************************** Signal Processing *************************" << endl;

    // Illumination Rectification
    signalProcessing<float>::nlmsFilter(backgroundSignal, foreHeadSignal, filterOrder, mu, ouputEstimationSignal);
    ouputEstimationSignal.bandPassFilterFunction(lowFreqCutOff, highFreqCutOff, filterOrder, filteredOutputEstimationSignal);
    filteredOutputEstimationSignal.saveSigToFile("nlmsOutputEstimation.txt");
    filteredOutputEstimationSignal.signalFFT(FFTOutput, SAVE_FFT, "FFTnlmsOutputEstimation.txt");

    // remove mean value "offset" of the signal
    foreHeadSignal.removeMeanValue();

    // Signal filter  ******************** TO BE OPTIMIZED **************************
    foreHeadSignal.saveSigToFile("foreHeadMeanValueRemoved.txt");
    foreHeadSignal.signalFFT(FFTOutput, SAVE_FFT, "FFTforeHeadMeanValueRemoved.txt");


    foreHeadSignal.bandPassFilterFunction(lowFreqCutOff, highFreqCutOff, filterOrder, filtredSig);
    filtredSig.saveSigToFile("filteredForeHeadBandPass.txt");
    filtredSig.signalFFT(FFTOutput, SAVE_FFT, "FFTfiltredForeHead.txt");


    // ************************************ plotting the signals **********************

    //  functions::plotWithGNUPLOT("foreHeadSignal.txt", SIG);
    //  functions::plotWithGNUPLOT("backgroundSignal.txt", SIG);
    //  functions::plotWithGNUPLOT("nlmsOutputEstimation.txt", SIG);
    //  functions::plotWithGNUPLOT("FFTnlmsOutputEstimation.txt", FFT);
    //  functions::plotWithGNUPLOT("foreHeadMeanValueRemoved.txt", SIG);
    //  functions::plotWithGNUPLOT("FFTforeHeadMeanValueRemoved.txt", FFT);
    //  functions::plotWithGNUPLOT("filteredForeHeadBandPass.txt", SIG);
    //  functions::plotWithGNUPLOT("FFTfiltredForeHead.txt", FFT);

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
