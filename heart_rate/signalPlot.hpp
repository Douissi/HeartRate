#ifndef SIGNALPLOT_HPP
#define SIGNALPLOT_HPP

// ---------------------------------------------------------------------------------------------
// Rendering of 2D curves (signals, spectra) into an image with OpenCV : axes, grid, tick labels,
// title, optional shaded band and marked point. Used by signalProcessing::renderSignalPlot /
// renderFFTPlot / plotSignal / plotFFT (replacement of the gnuplot based plotWithGNUPLOT).
// Only OpenCV core / imgproc / highgui are needed (no external program).
// ---------------------------------------------------------------------------------------------

#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <iomanip>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

namespace signal_plot
{

// Description of a plot
struct PlotData
{
    std::string title;
    std::string xLabel;
    std::string yLabel;
    std::vector<double> x;          // abscissa of the points
    std::vector<double> y;          // ordinate of the points (same size as x)

    bool fixedXRange = false;       // false : x range = range of the data
    double xMin = 0.0;
    double xMax = 1.0;

    bool showBand = false;          // shaded vertical band [bandMin, bandMax] (e.g. heart rate band)
    double bandMin = 0.0;
    double bandMax = 0.0;
    std::string bandLabel;

    bool showMarker = false;        // marked point (e.g. peak of the spectrum) with a label
    double markerX = 0.0;
    double markerY = 0.0;
    std::string markerLabel;
};

// Colors (B, G, R)
inline cv::Scalar backgroundColor() { return cv::Scalar(255, 255, 255); }
inline cv::Scalar textColor()       { return cv::Scalar(30, 30, 30); }
inline cv::Scalar axisColor()       { return cv::Scalar(90, 90, 90); }
inline cv::Scalar gridColor()       { return cv::Scalar(225, 225, 225); }
inline cv::Scalar zeroLineColor()   { return cv::Scalar(150, 150, 150); }
inline cv::Scalar curveColor()      { return cv::Scalar(180, 90, 20); }
inline cv::Scalar bandColor()       { return cv::Scalar(225, 245, 225); }
inline cv::Scalar bandTextColor()   { return cv::Scalar(60, 130, 60); }
inline cv::Scalar markerColor()     { return cv::Scalar(30, 30, 220); }

constexpr int kFont = cv::FONT_HERSHEY_SIMPLEX;
constexpr double kTickFontScale = 0.4;
constexpr double kTitleFontScale = 0.5;

inline int roundToInt(double v) { return static_cast<int>(std::lround(v)); }

// "Nice" tick step (1, 2 or 5 x 10^n) giving about targetTicks intervals on range
inline double niceStep(double range, int targetTicks)
{
    if (!(range > 0.0) || !std::isfinite(range)) return 1.0;
    const double raw = range / std::max(1, targetTicks);
    const double magnitude = std::pow(10.0, std::floor(std::log10(raw)));
    const double normalized = raw / magnitude;
    double nice = 10.0;
    if (normalized < 1.5) nice = 1.0;
    else if (normalized < 3.0) nice = 2.0;
    else if (normalized < 7.0) nice = 5.0;
    return nice * magnitude;
}

// Tick values multiple of step inside [lo, hi]
inline std::vector<double> ticks(double lo, double hi, double step)
{
    std::vector<double> values;
    const double first = std::ceil(lo / step - 1e-9) * step;
    for (int i = 0; i < 100; ++i)
    {
        const double v = first + i * step;
        if (v > hi + step * 1e-9) break;
        values.push_back(std::fabs(v) < step * 1e-9 ? 0.0 : v); // avoid "-0"
    }
    return values;
}

// Tick labels of an axis : fixed notation with enough decimals for the step, or %g style for
// the whole axis when the values are very large or the step very small
inline std::vector<std::string> formatTicks(const std::vector<double>& values, double step)
{
    double maxAbs = 0.0;
    for (double v : values) maxAbs = std::max(maxAbs, std::fabs(v));
    const bool general = maxAbs >= 1e6 || step < 1e-3;
    const int decimals = step >= 1.0 ? 0 : static_cast<int>(std::ceil(-std::log10(step) - 1e-9));

    std::vector<std::string> labels;
    for (double v : values)
    {
        std::ostringstream out;
        if (general) out << std::setprecision(3) << v;
        else out << std::fixed << std::setprecision(decimals) << v;
        labels.push_back(out.str());
    }
    return labels;
}

inline cv::Size textSize(const std::string& text, double fontScale, int* baseLine = nullptr)
{
    int base = 0;
    const cv::Size size = cv::getTextSize(text, kFont, fontScale, 1, &base);
    if (baseLine) *baseLine = base;
    return size;
}

inline bool overlaps(const cv::Rect& a, const cv::Rect& b)
{
    return a.x < b.x + b.width && b.x < a.x + a.width && a.y < b.y + b.height && b.y < a.y + a.height;
}

// Box covered by a text drawn at 'origin' (bottom-left corner of the text, as for cv::putText)
inline cv::Rect textBox(const cv::Point& origin, const cv::Size& size)
{
    return cv::Rect(origin.x - 2, origin.y - size.height - 2, size.width + 4, size.height + 5);
}

inline void drawText(cv::Mat& img, const std::string& text, cv::Point origin, double fontScale, const cv::Scalar& color)
{
    cv::putText(img, text, origin, kFont, fontScale, color, 1, cv::LINE_AA);
}

// Renders the plot in a new width x height BGR image
inline cv::Mat render(const PlotData& data, int width = 600, int height = 230)
{
    width = std::max(width, 200);
    height = std::max(height, 120);
    cv::Mat img(height, width, CV_8UC3, backgroundColor());

    // ---------------------------------------------------------------- data ranges
    const std::size_t n = std::min(data.x.size(), data.y.size());
    double xLo = data.xMin;
    double xHi = data.xMax;
    if (!data.fixedXRange)
    {
        xLo = std::numeric_limits<double>::max();
        xHi = std::numeric_limits<double>::lowest();
        for (std::size_t i = 0; i < n; ++i)
        {
            xLo = std::min(xLo, data.x[i]);
            xHi = std::max(xHi, data.x[i]);
        }
        if (n == 0) { xLo = 0.0; xHi = 1.0; }
    }
    if (!(xHi > xLo)) xHi = xLo + 1.0;

    double yMin = std::numeric_limits<double>::max();
    double yMax = std::numeric_limits<double>::lowest();
    std::size_t visible = 0;
    for (std::size_t i = 0; i < n; ++i)
    {
        if (data.x[i] < xLo || data.x[i] > xHi || !std::isfinite(data.y[i])) continue;
        yMin = std::min(yMin, data.y[i]);
        yMax = std::max(yMax, data.y[i]);
        ++visible;
    }
    if (visible == 0) { yMin = 0.0; yMax = 1.0; }
    if (!(yMax > yMin))
    {
        const double d = (std::fabs(yMin) > 0.0) ? std::fabs(yMin) * 0.1 : 1.0;
        yMin -= d;
        yMax += d;
    }
    const double yStep = niceStep(yMax - yMin, 5);
    // bounds rounded to the tick step (the tolerance keeps a value lying on a tick, e.g. a power
    // spectrum minimum of 1e-12, from adding a whole extra step)
    const double yLo = std::floor(yMin / yStep + 1e-9) * yStep;
    const double yHi = std::ceil(yMax / yStep - 1e-9) * yStep;
    const double xStep = niceStep(xHi - xLo, 8);

    const std::vector<double> yTicks = ticks(yLo, yHi, yStep);
    const std::vector<double> xTicks = ticks(xLo, xHi, xStep);

    // ---------------------------------------------------------------- layout
    const std::vector<std::string> xLabels = formatTicks(xTicks, xStep);
    const std::vector<std::string> yLabels = formatTicks(yTicks, yStep);
    int maxYLabelWidth = 0;
    for (const std::string& label : yLabels) maxYLabelWidth = std::max(maxYLabelWidth, textSize(label, kTickFontScale).width);
    const int tickTextHeight = textSize("0", kTickFontScale).height;
    const int titleHeight = textSize("Ag", kTitleFontScale).height;

    const int left = maxYLabelWidth + 14;
    const int right = 18;
    const int top = titleHeight + 18;
    const int bottom = 2 * tickTextHeight + 26;
    const cv::Rect area(left, top, std::max(10, width - left - right), std::max(10, height - top - bottom));

    auto px = [&](double x) { return area.x + roundToInt((x - xLo) / (xHi - xLo) * (area.width - 1)); };
    auto py = [&](double y) { return area.y + roundToInt((yHi - y) / (yHi - yLo) * (area.height - 1)); };

    // ---------------------------------------------------------------- band (behind everything)
    if (data.showBand && data.bandMax > data.bandMin)
    {
        const double b0 = std::max(data.bandMin, xLo);
        const double b1 = std::min(data.bandMax, xHi);
        if (b1 > b0)
        {
            cv::rectangle(img, cv::Point(px(b0), area.y), cv::Point(px(b1), area.y + area.height - 1), bandColor(), cv::FILLED);
        }
    }

    // ---------------------------------------------------------------- grid and tick labels
    for (std::size_t i = 0; i < xTicks.size(); ++i)
    {
        const int x = px(xTicks[i]);
        cv::line(img, cv::Point(x, area.y), cv::Point(x, area.y + area.height - 1), gridColor(), 1);
        const std::string& label = xLabels[i];
        const cv::Size s = textSize(label, kTickFontScale);
        drawText(img, label, cv::Point(x - s.width / 2, area.y + area.height + tickTextHeight + 6), kTickFontScale, textColor());
    }
    for (std::size_t i = 0; i < yTicks.size(); ++i)
    {
        const int y = py(yTicks[i]);
        cv::line(img, cv::Point(area.x, y), cv::Point(area.x + area.width - 1, y), gridColor(), 1);
        const std::string& label = yLabels[i];
        const cv::Size s = textSize(label, kTickFontScale);
        drawText(img, label, cv::Point(area.x - s.width - 6, y + s.height / 2), kTickFontScale, textColor());
    }
    if (yLo < 0.0 && yHi > 0.0)
        cv::line(img, cv::Point(area.x, py(0.0)), cv::Point(area.x + area.width - 1, py(0.0)), zeroLineColor(), 1);

    // ---------------------------------------------------------------- curve
    std::vector<std::vector<cv::Point>> curve(1);
    curve[0].reserve(n);
    for (std::size_t i = 0; i < n; ++i)
    {
        if (data.x[i] < xLo || data.x[i] > xHi || !std::isfinite(data.y[i])) continue;
        curve[0].emplace_back(px(data.x[i]), py(data.y[i]));
    }
    if (curve[0].size() >= 2)
        cv::polylines(img, curve, false, curveColor(), 1, cv::LINE_AA);
    else if (curve[0].size() == 1)
        cv::circle(img, curve[0][0], 2, curveColor(), cv::FILLED, cv::LINE_AA);
    else
        drawText(img, "no data", cv::Point(area.x + area.width / 2 - 25, area.y + area.height / 2), kTitleFontScale, axisColor());

    // ---------------------------------------------------------------- marker
    bool hasMarkerLabel = false;
    cv::Point markerLabelOrigin;
    cv::Rect markerLabelBox;
    if (data.showMarker && data.markerX >= xLo && data.markerX <= xHi)
    {
        const cv::Point m(px(data.markerX), py(data.markerY));
        cv::line(img, cv::Point(m.x, area.y + area.height - 1), m, markerColor(), 1, cv::LINE_AA);
        cv::circle(img, m, 4, markerColor(), cv::FILLED, cv::LINE_AA);
        if (!data.markerLabel.empty())
        {
            const cv::Size s = textSize(data.markerLabel, kTickFontScale);
            int tx = m.x + 8;
            if (tx + s.width > area.x + area.width - 2) tx = m.x - 8 - s.width; // no room on the right
            const int ty = std::max(area.y + s.height + 4, std::min(m.y + s.height / 2, area.y + area.height - 4));
            hasMarkerLabel = true;
            markerLabelOrigin = cv::Point(tx, ty);
            markerLabelBox = textBox(markerLabelOrigin, s);
        }
    }

    // ---------------------------------------------------------------- labels (drawn over the curves)
    if (data.showBand && !data.bandLabel.empty() && data.bandMax > data.bandMin)
    {
        const int bx0 = px(std::max(data.bandMin, xLo));
        const int bx1 = px(std::min(data.bandMax, xHi));
        if (bx1 > bx0)
        {
            const cv::Size s = textSize(data.bandLabel, kTickFontScale);
            const int topY = area.y + s.height + 4;
            const int bottomY = area.y + area.height - 5;
            // top left of the band, else top right, else bottom left : the first place free of the marker label
            const cv::Point candidates[3] = { cv::Point(bx0 + 4, topY), cv::Point(bx1 - 4 - s.width, topY), cv::Point(bx0 + 4, bottomY) };
            cv::Point origin = candidates[0];
            for (const cv::Point& c : candidates)
            {
                if (!hasMarkerLabel || !overlaps(textBox(c, s), markerLabelBox)) { origin = c; break; }
            }
            drawText(img, data.bandLabel, origin, kTickFontScale, bandTextColor());
        }
    }
    if (hasMarkerLabel)
        drawText(img, data.markerLabel, markerLabelOrigin, kTickFontScale, markerColor());

    // ---------------------------------------------------------------- frame, title, axis labels
    cv::rectangle(img, cv::Point(area.x, area.y), cv::Point(area.x + area.width - 1, area.y + area.height - 1), axisColor(), 1);
    drawText(img, data.title, cv::Point(area.x, titleHeight + 6), kTitleFontScale, textColor());
    if (!data.yLabel.empty())
    {
        const cv::Size s = textSize(data.yLabel, kTickFontScale);
        drawText(img, data.yLabel, cv::Point(area.x + area.width - s.width, titleHeight + 6), kTickFontScale, axisColor());
    }
    if (!data.xLabel.empty())
    {
        const cv::Size s = textSize(data.xLabel, kTickFontScale);
        drawText(img, data.xLabel, cv::Point(area.x + (area.width - s.width) / 2, height - 6), kTickFontScale, axisColor());
    }
    return img;
}

// Displays an image in a resizable window (the window keeps the image size at creation).
// As for any OpenCV window, the window is refreshed / kept open by cv::waitKey().
inline void show(const std::string& windowName, const cv::Mat& image)
{
    cv::namedWindow(windowName, cv::WINDOW_NORMAL | cv::WINDOW_KEEPRATIO);
    cv::resizeWindow(windowName, image.cols, image.rows);
    cv::imshow(windowName, image);
}

} // namespace signal_plot

#endif // SIGNALPLOT_HPP
