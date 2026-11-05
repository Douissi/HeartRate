# HeartRate

Heart rate estimation from a face video: the forehead and a background area are tracked
(OpenCV Haar cascade), the mean of the green channel gives a signal that is cleaned with an
NLMS adaptive filter (illumination rectification) and a band-pass FIR filter, and the spectrum
is computed with FFTW.

The project is a **Visual Studio C++ solution for Windows** (no Qt / Qt Creator anymore).

## Content

| Path | Description |
|------|-------------|
| `HeartRate.sln` | Visual Studio solution (open this file) |
| `heart_rate/` | Application project `heart_rate.vcxproj` (console application) |
| `heart_rate_tests/` | Unit test project `heart_rate_tests.vcxproj` (Microsoft Native C++ Unit Test Framework, runs in Test Explorer) |
| `heart_rate_tests/data/` | Reference signals used by the unit tests |
| `data/` | Videos, `haarcascade_frontalface_alt2.xml` (OpenCV face detector) |
| `vcpkg.json` | Third-party libraries (OpenCV with FFmpeg, FFTW3), installed automatically by vcpkg |
| `Directory.Build.props` / `Directory.Build.targets` | Settings shared by the projects (vcpkg, data copy) |

## Requirements

* Visual Studio 2022 (17.6 or newer) or Visual Studio 2026, with the **Desktop development with C++** workload.
  This workload includes everything needed:
  * MSVC compiler + Windows SDK,
  * the **C++ unit test framework** (Microsoft Native Unit Test Framework + Test Adapter),
  * the **vcpkg package manager** component (bundled vcpkg).
    Check it is selected in *Visual Studio Installer > Modify > Individual components > vcpkg package manager*.
* Internet access for the first build (vcpkg downloads the libraries' sources).

Alternatively a standalone vcpkg can be used: `git clone https://github.com/microsoft/vcpkg`,
run `bootstrap-vcpkg.bat`, and set the environment variable `VCPKG_ROOT` to the vcpkg folder
(then restart Visual Studio). `VCPKG_ROOT` has priority over the vcpkg bundled with Visual Studio.

## Build and run

1. Open `HeartRate.sln` in Visual Studio (*File > Open > Project/Solution*).
2. Select the configuration `Debug | x64` or `Release | x64`.
3. *Build > Build Solution*.

   **The first build takes a long time (typically 20-60 minutes):** vcpkg downloads and compiles
   OpenCV, FFmpeg and FFTW into `vcpkg_installed\` (see the *Output* window). The following builds
   reuse them (vcpkg also keeps a binary cache in `%LOCALAPPDATA%\vcpkg\archives`).
4. Run the application: set `heart_rate` as startup project and press `Ctrl+F5` / `F5`.

Build outputs are written in `bin\x64\<Configuration>\<project>\`. The build also copies there:
* the DLLs of OpenCV / FFmpeg / FFTW (vcpkg),
* the content of `data\` (videos and Haar cascade) and, for the tests, the reference signals.

The application is run from its output folder (debugger working directory = `$(OutDir)`):

```
heart_rate.exe [videoFile] [haarCascadeXmlFile]
```

Default: `face.mp4` and `haarcascade_frontalface_alt2.xml`. Press `Esc` in the video window to stop
the acquisition early. The signals and their FFT are saved as text files in the working directory
(`foreHeadSignal.txt`, `backgroundSignal.txt`, `nlmsOutputEstimation.txt`, `FFT*.txt` ...).

## Unit tests

The tests use the **Microsoft Native C++ Unit Test Framework** (`CppUnitTest.h`), installed with
Visual Studio, no extra package is needed.

* *Test > Test Explorer*, then *Run All Tests* (the solution is built first if needed).
* 16 tests: `TestSignalProcessing` (11 tests) and `TestVideoProcessing` (5 tests).

Command line alternative (Developer Command Prompt / Developer PowerShell for VS, after a build):

```
vstest.console.exe bin\x64\Debug\heart_rate_tests\heart_rate_tests.dll
```

The test data (reference signal files, `73BPmin.mp4`, Haar cascade) are copied next to
`heart_rate_tests.dll`, and the tests use that folder as working directory.

## Libraries

* **OpenCV 4** (video decoding through FFmpeg, Haar cascade face detection, display) - vcpkg port `opencv4[ffmpeg]`.
* **FFTW3** (FFT) - vcpkg port `fftw3`.
* The SigPack / Armadillo libraries used on Linux are not needed anymore: the band-pass FIR
  filter design (`fir1_bp`, Hamming window) and the NLMS adaptive filter that were taken from
  SigPack are implemented in `heart_rate/dsp.hpp` (same algorithms, validated by the unit tests
  against the reference files produced with SigPack).

The vcpkg packages versions are pinned by `builtin-baseline` in `vcpkg.json`. To move to the newest
versions, run `vcpkg x-update-baseline` in the solution folder (Developer PowerShell).

## Troubleshooting

* *"vcpkg was not found"*: install the *vcpkg package manager* component of Visual Studio, or set
  `VCPKG_ROOT` (see Requirements), then restart Visual Studio.
* vcpkg error about the baseline (e.g. the commit of `builtin-baseline` cannot be fetched): open a
  *Developer PowerShell* in the solution folder and run `vcpkg x-update-baseline` (it writes the
  current vcpkg baseline into `vcpkg.json`), then build again.
* A vcpkg package fails to build (`building opencv4:x64-windows failed with: BUILD_FAILED`):
  * the detailed logs are in `vcpkg_installed\x64-windows\vcpkg\blds\<package>\*.log` (the
    exact file is printed in the *Output* window);
  * the libraries are compiled inside the solution folder, with very deep paths: put the solution
    in a short folder such as `C:\dev\HeartRate` (not in Desktop / Documents);
  * with a recent Visual Studio (e.g. VS 2026), update the package versions: in a *Developer
    PowerShell* opened in the solution folder run `vcpkg x-update-baseline`, then rebuild.

## Notes

For printing fft in gnuplot
```
set xrange [0:6] // to set xrange
set xlabel "Frequencies in Hz"
set ylabel "Gain"
plot "FFTResult.txt" using 1:2  with lines
```
`plotWithGNUPLOT()` (see `main.cpp`, commented calls) requires gnuplot to be installed and in the `PATH`.

Test on YousFace.mp4
```
//  Rect backgroundRect(130, 50 , 500, 250);
//  faces[0].x += 100; // only for manual foreHead detection
//  faces[0].width -= 200; // only for manual foreHead detection
```

For Manual foreHead detection for face.mp4 video file
```
//  faces[0].height = faces[0].height/3.7; // only for manualforeHead detection
//  faces[0].x = faces[0].x + 50; // only for manual foreHead detection
//  faces[0].width = faces[0].width - 70; // only for manual foreHead detection
```

For Manual foreHead detection for 73BPmin.mp4
```
background is Rect(5, 116 , 128, 200);
foreHeads[0].y += 20; // only for manualforeHead detection
foreHeads[0].height = foreHeads[0].height/3.7 - 20; // only for manualforeHead detection
foreHeads[0].x += 60; // only for manual foreHead detection
foreHeads[0].width -= 130; // only for manual foreHead detection
```

For Manual foreHead detection for face2.mp4 video file
```
//    faces[0].y -= 70;
//    faces[0].height = faces[0].height/3 ; // only for manualforeHead detection
//    faces[0].x = faces[0].x + 60; // only for manual foreHead detection
//    faces[0].width = faces[0].width -140; // only for manual foreHead detection
```
# Youssef readme
This is the body of the main loop in main.cpp. It runs once per video frame and turns the video into two signals, one sample per frame. Those signals are what the signal-processing part uses afterwards to estimate the heart rate.

The idea (remote photoplethysmography): each heartbeat changes the blood volume in the skin slightly, which changes how much green light the skin absorbs. Averaging the green value over the forehead in each frame gives a signal that pulses with the heart. A background area is measured too, to capture lighting changes that have nothing to do with the pulse.

1. Read the next frame
copy
videoObj.capturingFrames();
if(videoObj.isFrameEmpty()) break;
This reads the next image of the video into frame.
It also splits the image into its blue, green and red channels (channels[0..2]).
If there is no frame left (end of the video), the loop stops.
2. Detect the face
copy
videoObj.detectFacesFromGreenChannelInVideo();
This runs OpenCV's Haar-cascade face detector (haarcascade_frontalface_alt2.xml) on the frame.
The parameters are: scale step 1.1, at least 2 neighbouring detections, and faces of at least 100×100 pixels.
The result is a list of rectangles, faces.
Despite the function's name, the detection runs on the full colour frame, not only on the green channel (OpenCV converts it internally).
3. When no face is found
copy
if(videoObj.getFaces().empty())
{
    if(lastDetectedFaces.empty()) continue;
    videoObj.setRectFacesVector(lastDetectedFaces);
}
lastDetectedFaces = videoObj.getFaces();
This is the only part I added; everything else is your original code.

The original code used faces[0] directly. If the detector missed the face in a frame, that read an empty list, which is undefined behaviour and usually a crash.
Now, if no face is found, the face rectangle from the previous frame is reused. That keeps exactly one sample per frame, which matters because the processing assumes evenly spaced samples at the video's frame rate (30 per second).
If no face has been found yet (at the very start of the video), the frame is skipped.
With face.mp4 a face is detected in all 301 frames (I checked), so this never triggers there. It only prevents crashes on other videos.
4. Background rectangle
copy
const Rect backgroundRect = Rect(faces[0].x + faces[0].width, faces[0].y + faces[0].height,
        videoObj.width - (faces[0].x + faces[0].width), videoObj.height -(faces[0].y + faces[0].height));
This is the area from the bottom-right corner of the face to the bottom-right corner of the image.
It should contain no skin, so it only sees lighting changes.
It becomes the reference for the NLMS filter later: the pulse signal is cleaned by removing the part it shares with the background.
5. Forehead rectangle
copy
foreHeads[0].height = static_cast<int>(foreHeads[0].height/3.7);
foreHeads[0].x = foreHeads[0].x + 50;
foreHeads[0].width = foreHeads[0].width - 70;
Starting from the face rectangle, this keeps:

height: the top part, about 27% of the face's height (/3.7);
left edge: moved 50 pixels to the right;
width: reduced by 70 pixels, so 20 pixels are cut on the right.
The forehead is used because it has thin, flat skin and no beard or eye movement. These numbers are tuned by hand for face.mp4; the README lists other values for 73BPmin.mp4, face2.mp4 and YousFace.mp4. They are fixed pixel counts, so they need adjusting for other videos.

6. Cut out the zones and measure one sample each
copy
videoObj.creatingFaceForeHeadAndBackGroundMatrixFromGreenChannel();
videoObj.fillforeHeadSignal();
videoObj.fillbackgroundSignal();
The first call creates three views on the green channel: face, forehead and background. These are OpenCV sub-regions, not copies.
The next two calls compute the average green value of the forehead and of the background, and append one value to each signal.
After the loop, foreHeadSignal and backgroundSignal hold one value per frame. They are saved as foreHeadSignal.txt and backgroundSignal.txt.
7. Display, and stop with Esc
copy
videoObj.displayFrame();
char c=(char)waitKey(25);
if(c==27) break;
displayFrame() draws the three rectangles in white (face, forehead, background) on the image and shows it in the "Frame" window.
waitKey(25) waits 25 ms for a key press. It is also what lets OpenCV refresh the window.
Pressing Esc (key code 27) ends the acquisition early, and the program moves on to signal processing with the frames captured so far.
The 25 ms delay only affects playback speed on screen. The signal's timing comes from the video's frame rate, not real time, so the results are the same.
After the loop
The two signals go through:

NLMS filtering, which removes the lighting changes shared with the background;
removal of the mean value;
a band-pass filter of 0.5–4.5 Hz, i.e. 30–270 beats per minute;
an FFT, where the main peak corresponds to the heart rate.



# last msg from project task: list of hcnages from QT to VS.

Here are all the changes to your own code, file by file. I checked them against the real diff with your original Qt version (the first git commit), not from memory.

The processing algorithms, constants and parameters are the same as before. Every function you had still exists with the same name and signature, so code calling them still works. The changes fall into five groups:

fixes needed to compile with Visual Studio's compiler (MSVC);
replacing the SigPack and Armadillo libraries;
bug fixes;
one precision fix;
new plotting and heart-rate functions.
1. signalProcessing.hpp
Includes and namespaces
Before (Qt)	Now	Why
Included OpenCV, <armadillo>, <sigpack.h>, "fftw3.h", <time.h>	Standard headers (<algorithm>, <string>, …), <fftw3.h>, "dsp.hpp", "signalPlot.hpp"	SigPack and Armadillo are gone (see section 3). OpenCV now comes in through signalPlot.hpp, for the plots.
using namespace cv; arma; sp; std;	only using namespace std;	arma and sp no longer exist. using namespace cv stays in videoProcessing.hpp, where it's used.
Missing includes worked by luck on Linux	<algorithm>, <iterator>, <sstream>, … added	MSVC doesn't bring headers in indirectly like GCC does (e.g. std::transform, std::max_element).
Free functions
Function	Change	Why / effect
ASSERT	Now inline, and throws std::runtime_error instead of calling abort(). The error message is still printed.	inline: a function defined in a header and included by several .cpp files gave "multiple definition" link errors. Throwing: in the test runner, abort() kills the whole test run with no information, while an exception makes just that test fail with your message. In the app, main catches it, prints it and exits with code 1, which is the same result as before.
plotWithGNUPLOT	Only made inline; the code is identical.	Same "multiple definition" reason. It still works on Windows if gnuplot is installed; main no longer uses it.
fftExemple	Rewritten without variable-length arrays: std::vector plus FFTW's own memory allocation, freed automatically. M_PI replaced by dsp::kPi.	MSVC doesn't support arrays sized at run time like double t[N], and M_PI doesn't exist there by default. Bug fixed: the loop for (i=0; i<=N; i++) wrote one element past the end of in and t. Unused by the app.
The signalProcessing<T> class
Member	Change	Why / effect
friend class TestVideoProcessing / TestSignalProcessing	Now heart_rate_tests::TestVideoProcessing / TestSignalProcessing	The test classes now live in a namespace.
meanVector()	Sums in double, then converts back to T	Precision fix (the testRemoveMeanValue failure). A float running sum loses precision on each addition once it reaches thousands. The result is the same to within about 1e-5.
signalFFT()	fftw_complex out[N/2+1]; double in[N]; replaced by heap buffers allocated with FFTW, freed automatically; the plan is also destroyed automatically. New check: at least 2 samples. It also records the FFT length in the output (fftSize).	Those arrays were run-time sized, which MSVC rejects, and large signals could overflow the stack. Bug fixed: with 0 or 1 samples, (N/2)-1 wrapped around to a huge unsigned number, giving an endless loop and a crash. The computation and results are otherwise identical.
bandPassFilterFunction()	sp::FIR_filt + sp::fir1_bp replaced by dsp::FIR_filt + dsp::fir1_bp. Works on a copy of the samples.	Same algorithm; outputs match your SigPack reference files to within 5.6e-6. The copy makes sig.bandPassFilterFunction(..., sig) (output = input) work correctly.
nlmsFilter()	sp::FIR_filt replaced by dsp::FIR_filt. Copies of the inputs. New check: at least 2 samples.	Same algorithm, matches your reference. Bug fixed: with fewer than 2 samples, outputEstimation[0] = outputEstimation[1] read out of bounds.
fillWithVector(), getFrequencyOfMaxFFT(), nlmsOutput(N, T(0))	Explicit type conversions only	Removes MSVC warnings; same results.
New private member fftSize	Length of the FFT that produced a spectrum (0 otherwise)	Needed to convert bin numbers to Hz exactly, including odd lengths.
New public methods	getFFTBinFrequency, getHeartRateBinIndex, getHeartRateFrequency, renderSignalPlot, renderFFTPlot, plotSignal, plotFFT	The features you asked for: heart rate from the spectrum, and plots (section 5).
Unchanged: constructors, removeMeanValue, fillWithValue, reserve, resize, saveSigToFile, getters and setters, and all constants (filterOrder, mu, cut-off frequencies, GREEN, SAVE_FFT, …).

2. videoProcessing.hpp
Small changes only; the logic is the same.

Change	Why / effect
Includes the OpenCV modules it uses (core, highgui, imgproc, objdetect, videoio), since signalProcessing.hpp no longer includes them	The file is self-contained.
#include <signalProcessing.hpp> changed to #include "signalProcessing.hpp"	Quotes are the correct form for your own header.
friend class heart_rate_tests::TestVideoProcessing	Test namespace.
width, height, numberOfFrames, meanFrameValue: explicit casts from double	Removes MSVC warnings about lost precision; same values.
ASSERT messages now include the file name ("Error opening video stream or file : face.mp4")	Easier diagnosis.
capturingFrames(): return; after the "frame is empty" message	It no longer calls split() on an empty frame at the end of the video. Effect: the channels keep the last valid frame instead of being emptied. main stops the loop right after anyway.
Unchanged: face detection (same parameters), rectangles and sub-images, meanFrameValue, signal filling, display, setters and getters, members.

3. New file: dsp.hpp (replaces SigPack and Armadillo)
Your code used four things from SigPack: fir1_bp, and FIR_filt<double,double,double> with set_coeffs, setup_nlms and nlms_adapt. SigPack isn't available through vcpkg, and it needs Armadillo, which in turn needs LAPACK, BLAS and a Fortran compiler on Windows. So I wrote those four pieces in about 150 lines of standard C++:

fir1_bp: band-pass filter design with a Hamming window, normalised to a gain of 1 at the centre of the band.
FIR_filt: a standard FIR filter. The NLMS adaptation uses the update b += 2·mu·e·x / (c + xᵀx).
I took the exact formulas from your reference files, which SigPack produced: the normalisation and the 2·mu factor were confirmed numerically. One addition: the NLMS update is skipped if the input energy is 0, where SigPack would divide by zero.

4. New file: signalPlot.hpp
This is the OpenCV drawing code used by the new plot methods: axes with rounded tick values, grid, curve, shaded heart-rate band, marked peak, and resizable windows. It's independent of your processing code.

5. main.cpp
Change	Why / effect
Removed #include "mainwindow.h" and <QApplication>	Qt removed. MainWindow was never used by main.
int main() now just calls a new function run(videoFile, cascadeFile) and catches exceptions	Errors (from ASSERT or OpenCV) are printed cleanly, with exit code 1.
Haar cascade path: /usr/local/share/opencv4/... changed to haarcascade_frontalface_alt2.xml, copied next to the exe. Video and cascade can be given on the command line.	Linux path removed. Default video is still face.mp4.
OpenCV log level set to WARNING	Hides the INFO messages the Debug OpenCV libraries print.
No face detected in a frame: the previous face is reused (or the frame is skipped if no face has been seen yet)	Bug fix: faces[0] on an empty list was undefined behaviour, usually a crash. No effect on face.mp4, where a face is found in all 301 frames.
foreHeads[0].height/3.7: explicit static_cast<int>	Same truncation as before, without a warning.
One FFT object per spectrum, plus a copy of the raw forehead signal	Previously FFTOutput was overwritten three times and foreHeadSignal modified in place, so they couldn't all be plotted at the end. Same .txt files written.
Estimated heart rate printed	New.
8 plot windows (SHOW_PLOTS) replace the commented-out plotWithGNUPLOT calls	New, same list of plots.
The processing chain, its order and its parameters are identical.

6. Deleted files
mainwindow.h, mainwindow.cpp, mainwindow.ui: the unused Qt window.
.pro and .pro.user files: Qt project files, replaced by .vcxproj files.
build-* folders: Linux build outputs. The videos and reference files that were in them moved to data/ and heart_rate_tests/data/.
7. Tests: tst_qtunittesttest.cpp became 4 files
Qt (QTest)	Now (Microsoft native C++ test framework)
class X : public QObject + private Q_SLOTS	TEST_CLASS(X) + TEST_METHOD(name), in namespace heart_rate_tests
QVERIFY(cond)	UT_VERIFY(cond): same meaning, and the message shows the condition, file and line
for loops of QVERIFY(fabs(a[i]-b[i]) < tol)	UT_VERIFY_ALL_NEAR(a, b, count, tol): also reports which sample failed and its values
main() + QTest::qExec	Test Explorer
Helpers ut::rand, ut::loadSignalFromFile	Moved to TestUtils.hpp; a missing file makes the test fail instead of calling abort()
Helper ut::resetObj	Removed (never used)
Running from the build folder	TestModule.cpp: sets the working folder to where the test DLL and its data files are
Changes inside the tests:

Same assertions, with these exceptions:
"Empty matrix" checks: elemSize() == 0 became empty(). In a Debug build, OpenCV raises an assertion on elemSize() of an empty matrix. The "not empty" checks keep elemSize() != 0.
Data-size checks: loaded data must have the expected number of samples. Before, a missing or short file could read out of bounds, or pass without comparing anything.
Random data: std::generate(..., std::rand) now uses small lambdas that convert to float explicitly. The values are the same.
Two new tests: testHeartRateFrequency and testRenderPlots, for 18 tests in total.
In short:

Behaviour changes:
ASSERT throws instead of aborting.
meanVector() is more precise.
Crashes are fixed: no face detected, signals with fewer than 2 samples, and the out-of-bounds write in fftExemple.
capturingFrames() skips split() on an empty frame.
Library swap: SigPack is replaced by dsp.hpp, validated against your reference files.
Additions: plots and the heart-rate estimate.
Everything else is type casts and includes needed for MSVC, with identical results.