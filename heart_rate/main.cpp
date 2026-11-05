#include <stdlib.h>
#include <stdio.h>
#include <exception>
#include <string>
#include <vector>
#include <opencv2/core/utils/logger.hpp>
#include "videoProcessing.hpp"

using namespace cv;

// Usage : heart_rate.exe [videoFile] [haarCascadeXmlFile]
// Default files are searched in the working directory. The Visual Studio project copies them
// (content of the "data" folder) next to heart_rate.exe and uses that folder as working
// directory when debugging (F5 / Ctrl+F5).
static const char* const DEFAULT_VIDEO_FILE = "face.mp4";
static const char* const DEFAULT_FACE_CASCADE_FILE = "haarcascade_frontalface_alt2.xml";

// Display the signals and their spectrum at the end of the processing (OpenCV windows)
static const bool SHOW_PLOTS = true;


static int run(const std::string& videoFile, const std::string& faceCascadeFile)
{
  // Create a VideoCapture object and open the input file
  // If the input is the web camera, pass 0 instead of the video file name
  videoProcessing videoObj(videoFile);

  videoObj.faceCascadeLoading(faceCascadeFile);

  std::vector<Rect> lastDetectedFaces;

  /************************************************* Video Processing *************************************************/
  while(1)
  {
      videoObj.capturingFrames();
      if(videoObj.isFrameEmpty()) break;

      videoObj.detectFacesFromGreenChannelInVideo();

      // No face detected in this frame : keep the last detected face (or skip the frame if none yet)
      if(videoObj.getFaces().empty())
      {
          if(lastDetectedFaces.empty()) continue;
          videoObj.setRectFacesVector(lastDetectedFaces);
      }
      lastDetectedFaces = videoObj.getFaces();

      const std::vector<Rect>& faces = videoObj.getFaces();
      const Rect backgroundRect = Rect(faces[0].x + faces[0].width, faces[0].y + faces[0].height,
              videoObj.width - (faces[0].x + faces[0].width), videoObj.height -(faces[0].y + faces[0].height));

      videoObj.setbackGroundRect(backgroundRect);

      std::vector<Rect> foreHeads = faces;

      foreHeads[0].height = static_cast<int>(foreHeads[0].height/3.7); // only for manualforeHead detection
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
  signalProcessing<float> FFTnlmsOutputEstimation;      // one object per spectrum, so that they can all be plotted
  signalProcessing<float> FFTforeHeadMeanValueRemoved;
  signalProcessing<float> FFTfiltredForeHead;

  backgroundSignal.saveSigToFile("backgroundSignal.txt");
  foreHeadSignal.saveSigToFile("foreHeadSignal.txt");
  const signalProcessing<float> foreHeadRawSignal = foreHeadSignal; // copy for the plot (the mean value is removed below)

  cout << "************************** Signal Processing *************************" << endl;

  // Illumination Rectification
  signalProcessing<float>::nlmsFilter(backgroundSignal, foreHeadSignal, filterOrder, mu, ouputEstimationSignal);
  ouputEstimationSignal.bandPassFilterFunction(lowFreqCutOff, highFreqCutOff, filterOrder, filteredOutputEstimationSignal);
  filteredOutputEstimationSignal.saveSigToFile("nlmsOutputEstimation.txt");
  filteredOutputEstimationSignal.signalFFT(FFTnlmsOutputEstimation, SAVE_FFT, "FFTnlmsOutputEstimation.txt");

   // remove mean value "offset" of the signal
  foreHeadSignal.removeMeanValue();

  // Signal filter  ******************** TO BE OPTIMIZED **************************
  foreHeadSignal.saveSigToFile("foreHeadMeanValueRemoved.txt");
  foreHeadSignal.signalFFT(FFTforeHeadMeanValueRemoved, SAVE_FFT, "FFTforeHeadMeanValueRemoved.txt");


  foreHeadSignal.bandPassFilterFunction(lowFreqCutOff, highFreqCutOff, filterOrder, filtredSig);
  filtredSig.saveSigToFile("filteredForeHeadBandPass.txt");
  filtredSig.signalFFT(FFTfiltredForeHead, SAVE_FFT, "FFTfiltredForeHead.txt");


  // ************************************ heart rate estimation **********************
  // Highest peak of the spectrum inside the band [lowFreqCutOff, highFreqCutOff]
  auto printHeartRate = [](const std::string& name, const signalProcessing<float>& spectrum)
  {
      const double frequency = spectrum.getHeartRateFrequency(lowFreqCutOff, highFreqCutOff);
      cout << "Estimated heart rate (" << name << ") : ";
      if (frequency < 0.0) cout << "not available" << endl;
      else cout << 60.0 * frequency << " bpm (peak at " << frequency << " Hz)" << endl;
  };
  printHeartRate("forehead, band-pass filtered", FFTfiltredForeHead);
  printHeartRate("NLMS illumination rectification", FFTnlmsOutputEstimation);


  // ************************************ plotting the signals **********************
  // One OpenCV window per plot (see signalProcessing::plotSignal / plotFFT), tiled on the screen.
  // The same signals are also saved in the .txt files above (plotWithGNUPLOT can plot these files
  // if gnuplot is installed).
  if (SHOW_PLOTS)
  {
      const int plotWidth = 560;
      const int plotHeight = 200;
      int plotIndex = 0;
      auto placeWindow = [&](const std::string& windowName)
      {
          // 2 columns : signals on the left, spectra on the right
          moveWindow(windowName, 10 + (plotIndex % 2) * (plotWidth + 20), 10 + (plotIndex / 2) * (plotHeight + 45));
          plotIndex++;
      };
      auto showSignal = [&](const signalProcessing<float>& signal, const std::string& windowName)
      {
          signal.plotSignal(windowName, plotWidth, plotHeight);
          placeWindow(windowName);
      };
      auto showFFT = [&](const signalProcessing<float>& spectrum, const std::string& windowName)
      {
          spectrum.plotFFT(windowName, 6.0, lowFreqCutOff, highFreqCutOff, plotWidth, plotHeight);
          placeWindow(windowName);
      };

      showSignal(foreHeadRawSignal, "Forehead signal (raw)");
      showSignal(backgroundSignal, "Background signal");
      showSignal(filteredOutputEstimationSignal, "NLMS output, band-pass filtered");
      showFFT(FFTnlmsOutputEstimation, "FFT - NLMS output");
      showSignal(foreHeadSignal, "Forehead signal, mean value removed");
      showFFT(FFTforeHeadMeanValueRemoved, "FFT - forehead, mean value removed");
      showSignal(filtredSig, "Forehead signal, band-pass filtered");
      showFFT(FFTfiltredForeHead, "FFT - forehead, band-pass filtered");

      cout << "Plots displayed : press a key in one of the plot windows to quit." << endl;
      waitKey(0);
  }


  return 0;
}


int main(int argc, char* argv[])
{
  // The OpenCV debug libraries (Debug configuration) print INFO messages by default (search of
  // optional plugins: "... => FAILED", backends used ...), the release libraries only warnings and
  // errors. Use the same level in all configurations (set LOG_LEVEL_INFO to see these messages).
  cv::utils::logging::setLogLevel(cv::utils::logging::LOG_LEVEL_WARNING);

  const std::string videoFile = (argc > 1) ? argv[1] : DEFAULT_VIDEO_FILE;
  const std::string faceCascadeFile = (argc > 2) ? argv[2] : DEFAULT_FACE_CASCADE_FILE;

  try
  {
      return run(videoFile, faceCascadeFile);
  }
  catch (const std::exception& e)
  {
      std::cerr << "heart_rate : fatal error : " << e.what() << std::endl;
      return EXIT_FAILURE;
  }
}
