#ifndef SIGNALPLOT_HPP
#define SIGNALPLOT_HPP

// ---------------------------------------------------------------------------------------------
// Plots drawn with OpenCV (no external program such as gnuplot) :
//   * render()  : draws a curve in an image (axes with rounded tick values, grid, title, labels,
//                 optional shaded frequency band and marked point),
//   * tile()    : assembles several images in a grid (e.g. one plot per analysis window),
//   * show()    : displays an image in a resizable window,
//   * save()    : writes an image file (.png ...),
//   * waitForWindows() : keeps the windows open until a key is pressed, the windows are closed or
//                 a timeout,
//   * signalPlotData() / spectrumPlotData() : plot data of a signal (amplitude versus time) and of
//                 a power spectrum computed by signalProcessing::signalFFT (power versus
//                 frequency, heart rate band and highest peak of the band marked).
// Independent of the processing classes (signalProcessing / videoProcessing).
// ---------------------------------------------------------------------------------------------

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

namespace signal_plot
{

    struct PlotData
    {
        std::string title;
        std::string xLabel;
        std::string yLabel;
        std::vector<double> x;
        std::vector<double> y;

        // x range of the plot : [xMin, xMax] if fixedXRange, range of the data otherwise
        bool fixedXRange = false;
        double xMin = 0.0;
        double xMax = 0.0;

        // shaded band [bandMin, bandMax] on the x axis (e.g. the heart rate frequencies)
        bool showBand = false;
        double bandMin = 0.0;
        double bandMax = 0.0;
        std::string bandLabel;

        // dots on the points of the curve (useful when the curve has few points)
        bool showPoints = false;

        // horizontal reference line (e.g. the heart rate estimated on the whole video)
        bool showReferenceY = false;
        double referenceY = 0.0;
        std::string referenceLabel;

        // marked point (e.g. the peak of a spectrum) with its label
        bool showMarker = false;
        double markerX = 0.0;
        double markerY = 0.0;
        std::string markerLabel;
    };

    namespace detail
    {
        inline const cv::Scalar kBackground(255, 255, 255);
        inline const cv::Scalar kGrid(225, 225, 225);
        inline const cv::Scalar kZeroLine(170, 170, 170);
        inline const cv::Scalar kFrame(90, 90, 90);
        inline const cv::Scalar kText(40, 40, 40);
        inline const cv::Scalar kCurve(180, 100, 20);  // blue (BGR)
        inline const cv::Scalar kBand(220, 245, 220);  // light green
        inline const cv::Scalar kBandText(60, 130, 60);
        inline const cv::Scalar kMarker(40, 40, 220);  // red
        inline const cv::Scalar kReference(120, 120, 120); // grey
        constexpr int kFont = cv::FONT_HERSHEY_SIMPLEX;
        constexpr double kTickFont = 0.42;
        constexpr double kLabelFont = 0.48;
        constexpr double kTitleFont = 0.58;

        // Tick step close to range / targetTicks, rounded to 1, 2 or 5 x 10^n
        inline double niceStep(double range, int targetTicks)
        {
            if (!(range > 0.0) || !std::isfinite(range)) return 1.0;
            const double raw = range / targetTicks;
            const double magnitude = std::pow(10.0, std::floor(std::log10(raw)));
            const double normalised = raw / magnitude;
            const double nice = (normalised < 1.5) ? 1.0 : (normalised < 3.0) ? 2.0 : (normalised < 7.0) ? 5.0 : 10.0;
            return nice * magnitude;
        }

        // Tick label : enough decimals for the step, scientific notation for very large / small values
        inline std::string formatTick(double value, double step, bool scientific)
        {
            if (std::fabs(value) < step * 1e-6) value = 0.0; // no "-0"
            std::ostringstream s;
            if (scientific && value != 0.0)
                s << std::scientific << std::setprecision(1) << value;
            else
            {
                const int decimals = (step >= 1.0) ? 0 : static_cast<int>(std::ceil(-std::log10(step) - 1e-9));
                s << std::fixed << std::setprecision(decimals) << value;
            }
            return s.str();
        }

        inline cv::Size textSize(const std::string& text, double scale, int* baseLine = nullptr)
        {
            int base = 0;
            const cv::Size size = cv::getTextSize(text, kFont, scale, 1, &base);
            if (baseLine) *baseLine = base;
            return size;
        }

        inline void text(cv::Mat& image, const std::string& str, cv::Point bottomLeft, double scale, const cv::Scalar& color)
        {
            cv::putText(image, str, bottomLeft, kFont, scale, color, 1, cv::LINE_AA);
        }

        struct Ticks
        {
            double first = 0.0;
            double step = 1.0;
            double last = 0.0;
            bool scientific = false;
        };

        inline Ticks makeTicks(double lo, double hi, int targetTicks)
        {
            Ticks t;
            t.step = niceStep(hi - lo, targetTicks);
            t.first = std::ceil(lo / t.step - 1e-9) * t.step;
            t.last = hi + t.step * 1e-9;
            const double largest = std::max(std::fabs(lo), std::fabs(hi));
            t.scientific = (largest >= 1e5) || (largest > 0.0 && largest < 1e-3);
            return t;
        }
    } // namespace detail

    // Draws the plot in a width x height image (BGR)
    inline cv::Mat render(const PlotData& data, int width = 600, int height = 230)
    {
        using namespace detail;
        width = std::max(width, 240);
        height = std::max(height, 150);
        cv::Mat image(height, width, CV_8UC3, kBackground);

        const std::size_t n = std::min(data.x.size(), data.y.size());

        // ---- x range
        double x0 = data.xMin, x1 = data.xMax;
        if (!data.fixedXRange)
        {
            x0 = 0.0;
            x1 = 1.0;
            if (n > 0)
            {
                x0 = *std::min_element(data.x.begin(), data.x.begin() + static_cast<std::ptrdiff_t>(n));
                x1 = *std::max_element(data.x.begin(), data.x.begin() + static_cast<std::ptrdiff_t>(n));
            }
        }
        if (!(x1 > x0)) { x0 -= 0.5; x1 += 0.5; }

        // ---- y range : points inside the x range (and the marker), rounded to the ticks
        double y0 = 0.0, y1 = 0.0;
        bool any = false;
        for (std::size_t i = 0; i < n; i++)
        {
            if (data.x[i] < x0 || data.x[i] > x1 || !std::isfinite(data.y[i])) continue;
            if (!any) { y0 = y1 = data.y[i]; any = true; }
            y0 = std::min(y0, data.y[i]);
            y1 = std::max(y1, data.y[i]);
        }
        if (data.showReferenceY && std::isfinite(data.referenceY))
        {
            if (!any) { y0 = y1 = data.referenceY; any = true; }
            y0 = std::min(y0, data.referenceY);
            y1 = std::max(y1, data.referenceY);
        }
        if (!any) { y0 = 0.0; y1 = 1.0; }
        if (!(y1 > y0)) { const double d = (y0 == 0.0) ? 1.0 : std::fabs(y0) * 0.1; y0 -= d; y1 += d; }
        if (data.showMarker) y1 += 0.15 * (y1 - y0); // room above the marked point for its label
        if (data.showReferenceY)                     // the reference line must not be on the border
        {
            const double margin = 0.08 * (y1 - y0);
            y0 -= margin;
            y1 += margin;
        }
        const int top = 46;
        const int bottom = 48;
        const int right = 22;
        const int py0 = top, py1 = height - bottom;

        // ---- y ticks (about one every 45 pixels), y range rounded to the ticks
        const int yTickCount = std::max(2, (py1 - py0) / 45);
        Ticks yTicks = makeTicks(y0, y1, yTickCount);
        y0 = std::floor(y0 / yTicks.step + 1e-9) * yTicks.step;
        y1 = std::ceil(y1 / yTicks.step - 1e-9) * yTicks.step;
        yTicks = makeTicks(y0, y1, yTickCount);

        // ---- left margin : width of the y tick labels
        int maxTickWidth = 0;
        for (double v = yTicks.first; v <= yTicks.last; v += yTicks.step)
            maxTickWidth = std::max(maxTickWidth, textSize(formatTick(v, yTicks.step, yTicks.scientific), kTickFont).width);
        const int px0 = 14 + maxTickWidth, px1 = width - right;

        // ---- x ticks (about one every 90 pixels)
        const Ticks xTicks = makeTicks(x0, x1, std::max(2, (px1 - px0) / 90));

        auto X = [&](double x) { return static_cast<int>(std::lround(px0 + (x - x0) / (x1 - x0) * (px1 - px0))); };
        auto Y = [&](double y) { return static_cast<int>(std::lround(py1 - (y - y0) / (y1 - y0) * (py1 - py0))); };

        // ---- marked point and its label (drawn at the end, placed here so that the band label avoids it)
        const bool marker = data.showMarker && data.markerX >= x0 && data.markerX <= x1;
        cv::Point markerPoint;
        cv::Rect markerLabelBox;
        if (marker)
        {
            markerPoint = cv::Point(X(data.markerX), Y(data.markerY));
            if (!data.markerLabel.empty())
            {
                // above-right of the point (above-left near the right border), inside the plot area
                const cv::Size s = textSize(data.markerLabel, kLabelFont);
                int tx = markerPoint.x + 8;
                if (tx + s.width > px1 - 4) tx = markerPoint.x - 8 - s.width;
                const int ty = std::max(markerPoint.y - 9, py0 + s.height + 4);
                markerLabelBox = cv::Rect(tx, ty - s.height, s.width, s.height + 4);
            }
        }

        // ---- shaded band
        if (data.showBand)
        {
            const double b0 = std::max(data.bandMin, x0), b1 = std::min(data.bandMax, x1);
            if (b1 > b0)
            {
                cv::rectangle(image, cv::Rect(cv::Point(X(b0), py0), cv::Point(X(b1), py1)), kBand, cv::FILLED);
                if (!data.bandLabel.empty())
                {
                    // top-right corner of the band, else top-left : the first place that does not hide the
                    // marked point and its label (not drawn if both do; the bottom of a spectrum is where
                    // the curve usually is)
                    const cv::Size s = textSize(data.bandLabel, kTickFont);
                    const int rightX = std::max(X(b0) + 5, X(b1) - 5 - s.width), leftX = X(b0) + 5;
                    const int topY = py0 + 5 + s.height;
                    const cv::Point candidates[2] = { { rightX, topY },{ leftX, topY } };
                    for (const cv::Point& c : candidates)
                    {
                        const cv::Rect box(c.x - 2, c.y - s.height - 2, s.width + 4, s.height + 6);
                        const bool hidesMarker = marker && ((box & markerLabelBox).area() > 0 ||
                            box.contains(markerPoint) || (box & cv::Rect(markerPoint.x - 7, markerPoint.y - 7, 15, 15)).area() > 0);
                        if (!hidesMarker)
                        {
                            text(image, data.bandLabel, c, kTickFont, kBandText);
                            break;
                        }
                    }
                }
            }
        }

        // ---- grid and tick labels
        for (double v = yTicks.first; v <= yTicks.last; v += yTicks.step)
        {
            const int y = Y(v);
            cv::line(image, cv::Point(px0, y), cv::Point(px1, y), kGrid, 1);
            const std::string label = formatTick(v, yTicks.step, yTicks.scientific);
            const cv::Size s = textSize(label, kTickFont);
            text(image, label, cv::Point(px0 - 6 - s.width, y + s.height / 2), kTickFont, kText);
        }
        for (double v = xTicks.first; v <= xTicks.last; v += xTicks.step)
        {
            const int x = X(v);
            cv::line(image, cv::Point(x, py0), cv::Point(x, py1), kGrid, 1);
            const std::string label = formatTick(v, xTicks.step, xTicks.scientific);
            const cv::Size s = textSize(label, kTickFont);
            text(image, label, cv::Point(x - s.width / 2, py1 + 8 + s.height), kTickFont, kText);
        }
        if (y0 < 0.0 && y1 > 0.0) cv::line(image, cv::Point(px0, Y(0.0)), cv::Point(px1, Y(0.0)), kZeroLine, 1);

        // ---- horizontal reference line (dashed) and its label
        if (data.showReferenceY && data.referenceY >= y0 && data.referenceY <= y1)
        {
            const int y = Y(data.referenceY);
            for (int x = px0; x < px1; x += 12) cv::line(image, cv::Point(x, y), cv::Point(std::min(x + 6, px1), y), kReference, 1);
            if (!data.referenceLabel.empty())
            {
                const cv::Size s = textSize(data.referenceLabel, kTickFont);
                text(image, data.referenceLabel, cv::Point(px1 - 4 - s.width, y - 4), kTickFont, kReference);
            }
        }

        // ---- curve (segments inside the x range)
        for (std::size_t i = 1; i < n; i++)
        {
            if (data.x[i - 1] < x0 || data.x[i] > x1 || !std::isfinite(data.y[i - 1]) || !std::isfinite(data.y[i])) continue;
            cv::line(image, cv::Point(X(data.x[i - 1]), Y(data.y[i - 1])), cv::Point(X(data.x[i]), Y(data.y[i])), kCurve, 2, cv::LINE_AA);
        }

        // ---- dots on the points
        if (data.showPoints)
            for (std::size_t i = 0; i < n; i++)
            {
                if (data.x[i] < x0 || data.x[i] > x1 || !std::isfinite(data.y[i])) continue;
                cv::circle(image, cv::Point(X(data.x[i]), Y(data.y[i])), 3, kCurve, cv::FILLED, cv::LINE_AA);
            }

        // ---- frame
        cv::rectangle(image, cv::Rect(cv::Point(px0, py0), cv::Point(px1, py1)), kFrame, 1);

        // ---- marked point
        if (marker)
        {
            cv::line(image, markerPoint, cv::Point(markerPoint.x, py1), kMarker, 1);
            cv::circle(image, markerPoint, 5, kMarker, 2, cv::LINE_AA);
            if (!data.markerLabel.empty())
                text(image, data.markerLabel, cv::Point(markerLabelBox.x, markerLabelBox.y + textSize(data.markerLabel, kLabelFont).height),
                    kLabelFont, kMarker);
        }

        // ---- title and axis labels
        if (!data.title.empty())
        {
            const int titleWidth = textSize(data.title, kTitleFont).width;
            const double scale = (titleWidth > width - 10) ? kTitleFont * (width - 10) / titleWidth : kTitleFont; // fits in the image
            const cv::Size s = textSize(data.title, scale);
            text(image, data.title, cv::Point((width - s.width) / 2, 20), scale, kText);
        }
        if (!data.yLabel.empty()) text(image, data.yLabel, cv::Point(8, py0 - 9), kLabelFont, kText);
        if (!data.xLabel.empty())
        {
            const cv::Size s = textSize(data.xLabel, kLabelFont);
            text(image, data.xLabel, cv::Point(px0 + (px1 - px0 - s.width) / 2, height - 9), kLabelFont, kText);
        }
        return image;
    }

    // Assembles images (BGR, as returned by render) in a grid of 'columns' columns, row by row, on a white
    // background. Every cell has the size of the largest image. Empty images leave their cell blank.
    inline cv::Mat tile(const std::vector<cv::Mat>& images, int columns)
    {
        if (images.empty()) return cv::Mat();
        const int count = static_cast<int>(images.size());
        columns = std::max(1, std::min(columns, count));
        const int rows = (count + columns - 1) / columns;
        int cellWidth = 1, cellHeight = 1;
        for (const cv::Mat& image : images)
        {
            cellWidth = std::max(cellWidth, image.cols);
            cellHeight = std::max(cellHeight, image.rows);
        }
        cv::Mat grid(rows * cellHeight, columns * cellWidth, CV_8UC3, detail::kBackground);
        for (int i = 0; i < count; i++)
        {
            if (images[static_cast<std::size_t>(i)].empty()) continue;
            const cv::Mat& image = images[static_cast<std::size_t>(i)];
            image.copyTo(grid(cv::Rect((i % columns) * cellWidth, (i / columns) * cellHeight, image.cols, image.rows)));
        }
        return grid;
    }

    // Plot data of a signal : amplitude versus time in seconds (versus sample index if Fs <= 0).
    // startTime : time of the first sample (s), e.g. for a signal cut out of a longer recording.
    template <typename T>
    PlotData signalPlotData(const std::string& title, const std::vector<T>& samples, double Fs, double startTime = 0.0)
    {
        PlotData data;
        data.title = title;
        data.yLabel = "Amplitude";
        data.xLabel = (Fs > 0.0) ? "Time (s)" : "Sample";
        data.x.reserve(samples.size());
        data.y.reserve(samples.size());
        for (std::size_t i = 0; i < samples.size(); i++)
        {
            data.x.push_back((Fs > 0.0) ? startTime + static_cast<double>(i) / Fs : static_cast<double>(i));
            data.y.push_back(static_cast<double>(samples[i]));
        }
        return data;
    }

    // Plot data of a power spectrum computed by signalProcessing::signalFFT : bin i is at the
    // frequency i * Fs / fftLength (fftLength = number of samples of the signal transformed).
    // Plotted in [0, maxFrequency] Hz (maxFrequency <= 0 : up to Fs / 2); the band [lowFreq, highFreq]
    // is shaded and its highest bin is marked with its frequency and the heart rate (x 60 = bpm).
    template <typename T>
    PlotData spectrumPlotData(const std::string& title, const std::vector<T>& power, double Fs, std::size_t fftLength,
        double maxFrequency, double lowFreq, double highFreq)
    {
        PlotData data;
        data.title = title;
        data.yLabel = "Power";
        data.xLabel = "Frequency (Hz)";
        const double resolution = (fftLength > 0) ? Fs / static_cast<double>(fftLength) : 0.0;
        data.x.reserve(power.size());
        data.y.reserve(power.size());
        for (std::size_t i = 0; i < power.size(); i++)
        {
            data.x.push_back(static_cast<double>(i) * resolution);
            data.y.push_back(static_cast<double>(power[i]));
        }
        data.fixedXRange = true;
        data.xMin = 0.0;
        data.xMax = (maxFrequency > 0.0) ? maxFrequency : Fs / 2.0;

        data.showBand = true;
        data.bandMin = lowFreq;
        data.bandMax = highFreq;
        data.bandLabel = "heart rate band";

        std::size_t peak = power.size();
        for (std::size_t i = 0; i < power.size(); i++)
        {
            if (data.x[i] < lowFreq || data.x[i] > highFreq) continue;
            if (peak == power.size() || data.y[i] > data.y[peak]) peak = i;
        }
        if (peak < power.size())
        {
            data.showMarker = true;
            data.markerX = data.x[peak];
            data.markerY = data.y[peak];
            std::ostringstream label;
            label << std::fixed << std::setprecision(2) << data.markerX << " Hz = " << std::setprecision(0)
                << 60.0 * data.markerX << " bpm";
            data.markerLabel = label.str();
        }
        return data;
    }

    // true when windows can be opened (Linux : an X11 / Wayland display is needed)
    inline bool displayAvailable()
    {
#if defined(_WIN32) || defined(__APPLE__)
        return true;
#else
        const char* x11 = std::getenv("DISPLAY");
        const char* wayland = std::getenv("WAYLAND_DISPLAY");
        return (x11 && *x11) || (wayland && *wayland);
#endif
    }

    // Displays the image in a resizable window; x, y >= 0 : position of the window on the screen.
    // As for any OpenCV window, cv::waitKey (or waitForWindows) must be called to draw / keep it.
    inline void show(const std::string& windowName, const cv::Mat& image, int x = -1, int y = -1)
    {
        cv::namedWindow(windowName, cv::WINDOW_NORMAL);
        cv::resizeWindow(windowName, image.cols, image.rows);
        if (x >= 0 && y >= 0) cv::moveWindow(windowName, x, y);
        cv::imshow(windowName, image);
    }

    // Writes the image (format given by the extension, e.g. ".png"). Returns false on error.
    inline bool save(const std::string& fileName, const cv::Mat& image)
    {
        return cv::imwrite(fileName, image);
    }

    namespace detail
    {
        // cv::WND_PROP_VISIBLE of a window : >= 1 visible, < 1 closed or unknown (-1 if OpenCV reports an error)
        inline double windowVisibility(const std::string& windowName)
        {
            try { return cv::getWindowProperty(windowName, cv::WND_PROP_VISIBLE); }
            catch (const cv::Exception&) { return -1.0; }
        }
    } // namespace detail

    // Keeps the windows open until a key is pressed, all the windows are closed, or timeoutMs
    // milliseconds (timeoutMs <= 0 : no timeout), then destroys the windows that are still open.
    // Closing a window is detected with cv::WND_PROP_VISIBLE (Win32 / Qt backends, recent GTK); when
    // the backend does not report it, the wait ends with a key press (or the timeout) only.
    // A window closed by the user (X button) has already been destroyed by OpenCV : it is not destroyed
    // again (cv::destroyWindow would raise a "NULL window" error, printed in the console by the Debug
    // OpenCV libraries even though it is caught).
    inline void waitForWindows(const std::vector<std::string>& windowNames, int timeoutMs = 0)
    {
        const auto start = std::chrono::steady_clock::now();
        std::vector<bool> seenVisible(windowNames.size(), false); // the backend reports the visibility of this window
        std::vector<bool> closed(windowNames.size(), false);      // closed by the user
        for (;;)
        {
            if (cv::waitKey(50) >= 0) break;

            bool anyOpen = false;
            for (std::size_t i = 0; i < windowNames.size(); i++)
            {
                if (closed[i]) continue;
                if (detail::windowVisibility(windowNames[i]) >= 1.0) seenVisible[i] = true;
                else if (seenVisible[i]) closed[i] = true; // was visible, is not any more
                if (!closed[i]) anyOpen = true;            // never seen visible : state unknown, still open
            }
            if (!anyOpen) break;

            const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count();
            if (timeoutMs > 0 && elapsed >= timeoutMs) break;
        }
        for (std::size_t i = 0; i < windowNames.size(); i++)
        {
            // skip the windows closed by the user, also if that happened just before the key press
            if (closed[i] || (seenVisible[i] && detail::windowVisibility(windowNames[i]) < 1.0)) continue;
            try { cv::destroyWindow(windowNames[i]); }
            catch (const cv::Exception&) {}
        }
        cv::waitKey(1); // lets the windows disappear
    }

} // namespace signal_plot

#endif // SIGNALPLOT_HPP
