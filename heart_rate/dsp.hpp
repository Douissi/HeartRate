#ifndef DSP_HPP
#define DSP_HPP

// ---------------------------------------------------------------------------------------------
// Minimal, dependency-free replacement for the parts of the SigPack library (sp::fir1_bp and
// sp::FIR_filt<double,double,double> with set_coeffs / setup_nlms / nlms_adapt) that were used by
// this project.
//
// SigPack (and the Armadillo / LAPACK / BLAS stack it is built on) is not available through the
// Visual Studio package manager (vcpkg), so the few algorithms the project needs are implemented
// here with the standard library only. The algorithms are the same as SigPack's:
//   * fir1_bp : window-method band-pass FIR design (Hamming window), gain normalised to 1 at the
//               centre of the pass band.
//   * FIR_filt: direct-form FIR filter, y[n] = sum_k b[k] * x[n-k].
//   * NLMS    : b <- b + 2 * mu * e[n] * x / (c + x'x)
// The unit tests check the results against reference files produced with SigPack.
// ---------------------------------------------------------------------------------------------

#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <vector>

namespace dsp
{

constexpr double kPi = 3.14159265358979323846;

// Normalised sinc: sin(pi*x)/(pi*x)
inline double sinc(double x)
{
    if (x == 0.0) return 1.0;
    return std::sin(kPi * x) / (kPi * x);
}

// Hamming window of length N
inline std::vector<double> hamming(std::size_t N)
{
    std::vector<double> h(N, 1.0);
    if (N < 2) return h;
    for (std::size_t n = 0; n < N; ++n)
        h[n] = 0.54 - 0.46 * std::cos(2.0 * kPi * static_cast<double>(n) / static_cast<double>(N - 1));
    return h;
}

// Band-pass FIR filter design (window method, Hamming window).
//   M  : filter order (the filter has M+1 coefficients)
//   f0 : lower cut-off frequency, normalised to Nyquist (0 < f0 < f1 < 1)
//   f1 : upper cut-off frequency, normalised to Nyquist
// The coefficients are scaled for a unity gain at the centre frequency (f0+f1)/2.
inline std::vector<double> fir1_bp(std::size_t M, double f0, double f1)
{
    if (!(f0 > 0.0 && f0 < f1 && f1 < 1.0))
        throw std::invalid_argument("dsp::fir1_bp : cut-off frequencies must satisfy 0 < f0 < f1 < 1");

    const std::vector<double> h = hamming(M + 1);
    std::vector<double> b(M + 1);
    const double half = static_cast<double>(M) / 2.0;
    for (std::size_t m = 0; m <= M; ++m)
    {
        const double t = static_cast<double>(m) - half;
        b[m] = (f1 * sinc(f1 * t) - f0 * sinc(f0 * t)) * h[m];
    }

    // Unity gain at the centre of the pass band: |sum_m b[m] * exp(-j*pi*fc*m)|
    const double wc = kPi * (f0 + f1) / 2.0;
    double re = 0.0;
    double im = 0.0;
    for (std::size_t m = 0; m <= M; ++m)
    {
        re += b[m] * std::cos(wc * static_cast<double>(m));
        im -= b[m] * std::sin(wc * static_cast<double>(m));
    }
    const double gain = std::sqrt(re * re + im * im);
    if (gain > 0.0)
        for (double& v : b) v /= gain;
    return b;
}

// FIR filter with optional NLMS adaptation (same semantics as sp::FIR_filt<double,double,double>).
class FIR_filt
{
public:
    FIR_filt() = default;

    // Set fixed filter coefficients (resets the filter state).
    void set_coeffs(const std::vector<double>& coeffs)
    {
        b = coeffs;
        buf.assign(b.size(), 0.0);
        cur = 0;
        do_adapt = false;
    }

    // Set up an NLMS adaptive filter with N taps (coefficients start at zero).
    //   mu : step size, c : regularisation constant.
    void setup_nlms(std::size_t N, double step, double regularisation = 0.0)
    {
        b.assign(N, 0.0);
        buf.assign(N, 0.0);
        cur = 0;
        mu = step;
        c = regularisation;
        do_adapt = true;
    }

    // Filter one sample.
    double operator()(double in)
    {
        const std::size_t M = b.size();
        if (M == 0) return 0.0;

        // Circular buffer: the newest sample is at position 'cur', older ones follow.
        buf[cur] = in;
        double out = 0.0;
        std::size_t p = 0;
        for (std::size_t m = cur; m < M; ++m) out += b[p++] * buf[m];
        for (std::size_t m = 0; m < cur; ++m) out += b[p++] * buf[m];
        newest = cur;
        cur = (cur == 0) ? M - 1 : cur - 1;
        return out;
    }

    // NLMS coefficient update with the error of the last filtered sample.
    void nlms_adapt(double err)
    {
        if (!do_adapt) return;
        const std::size_t M = b.size();
        if (M == 0) return;

        // x = [x(n), x(n-1), ..., x(n-M+1)], aligned with the coefficients
        double energy = c;
        for (double v : buf) energy += v * v;
        if (energy <= 0.0) return; // nothing to adapt on (avoids a division by zero)

        const double k = 2.0 * mu * err / energy;
        std::size_t p = 0;
        for (std::size_t m = newest; m < M; ++m) b[p++] += k * buf[m];
        for (std::size_t m = 0; m < newest; ++m) b[p++] += k * buf[m];
    }

    const std::vector<double>& coeffs() const { return b; }

private:
    std::vector<double> b;   // filter coefficients
    std::vector<double> buf; // circular input buffer
    std::size_t cur = 0;     // insertion position of the next sample
    std::size_t newest = 0;  // position of the most recent sample
    double mu = 0.0;
    double c = 0.0;
    bool do_adapt = false;
};

} // namespace dsp

#endif // DSP_HPP
