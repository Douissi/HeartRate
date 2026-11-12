// Code coverage support - contains no test.
//
// MSVC only generates code for the inline / template functions that are actually called. The
// classes under test are header-only, so a function that no test calls (e.g.
// signalProcessing::getFrequencyOfMaxFFT, videoProcessing::meanFrameValue) would simply be absent
// from heart_rate_tests.dll: the coverage tool would not see it at all, instead of reporting it as
// "not covered", and the coverage percentages would be overstated.
//
// Taking the address of every function forces the compiler to generate all of them, and exporting
// the table from the DLL prevents the linker (/OPT:REF, implied by /PROFILE) from removing them.
// This gives the same function set as the gcov measurement (coverage_analysis/), where
// -fkeep-inline-functions and an explicit template instantiation play this role.
//
// When a function is added to signalProcessing / videoProcessing / dsp, add it to the table below.

#include "videoProcessing.hpp"

#include <tuple>

#ifdef _MSC_VER
#define HEART_RATE_COVERAGE_EXPORT extern "C" __declspec(dllexport)
#else
#define HEART_RATE_COVERAGE_EXPORT extern "C"
#endif

namespace
{
using SP = signalProcessing<float>;
using VP = videoProcessing;

const auto coverageAnchors = std::make_tuple(
    // free functions (signalProcessing.hpp)
    &ASSERT, &plotWithGNUPLOT, &fftExemple,

    // signalProcessing<float>
    &SP::meanVector, &SP::removeMeanValue, &SP::fillWithValue, &SP::fillWithVector, &SP::reserve,
    &SP::resize, &SP::saveSigToFile, &SP::signalFFT, &SP::bandPassFilterFunction, &SP::nlmsFilter,
    &SP::getFrequencyOfMaxFFT, &SP::getSignalSamples, &SP::getFs, &SP::getNbOfSamples, &SP::setFs,
    &SP::setNbOfSamples,

    // videoProcessing
    &VP::faceCascadeLoading, &VP::capturingFrames, &VP::detectFacesFromGreenChannelInVideo,
    &VP::creatingFaceForeHeadAndBackGroundMatrixFromGreenChannel, &VP::drawRectangle,
    &VP::imageColorToGreyTransform, &VP::meanFrameValue, &VP::fillforeHeadSignal,
    &VP::fillbackgroundSignal, &VP::displayFrame, &VP::isFrameEmpty, &VP::setbackGroundRect,
    &VP::setRectFacesVector, &VP::setRectForeHeadsVector, &VP::getFaces, &VP::getframe,
    &VP::getbackgroundRect, &VP::getforeHead, &VP::getbackground, &VP::getFs, &VP::getNumOfFrames,
    &VP::getforeHeadSignal, &VP::getbackgroundSignal,

    // dsp.hpp
    &dsp::sinc, &dsp::hamming, &dsp::fir1_bp, &dsp::FIR_filt::set_coeffs, &dsp::FIR_filt::setup_nlms,
    &dsp::FIR_filt::operator(), &dsp::FIR_filt::nlms_adapt, &dsp::FIR_filt::coeffs);

// Constructors / destructors cannot have their address taken: instantiating the objects here
// generates them (they are also called by the tests).
void constructorsAndDestructors()
{
    SP a;
    SP b(1u, 1.0);
    SP c(std::vector<float>(2, 0.0f), 2u, 1.0);
    VP v;
    VP w(String("video.mp4"));
}
} // namespace

// Never called: exists only so that the anchors above are kept in the DLL.
HEART_RATE_COVERAGE_EXPORT const void* heartRateCoverageAnchors()
{
    static void (*const ctors)() = &constructorsAndDestructors;
    static const void* const table[] = { &coverageAnchors, &ctors };
    return table;
}
