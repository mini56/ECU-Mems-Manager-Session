#ifndef COMPLETE_ZOOM_MATH_H
#define COMPLETE_ZOOM_MATH_H
#include <algorithm>

namespace CompleteZoom {
// This is the single logical canvas shared by all 14 pages.
constexpr int kReferenceWidth = 1366;
constexpr int kReferenceHeight = 690;
constexpr int kNativeCaptionAllowance = 40;
constexpr int kScreenSafetyMargin = 8;

// Physical Windows work-area pixels -> global, uniform Qt scale factor.
// The entire page fits without changing the monitor resolution.
inline double factorForWorkArea(int width, int height)
{
    if (width <= 0 || height <= 0) return 1.0;
    const double fitWidth = std::max(1, width - kScreenSafetyMargin) /
        static_cast<double>(kReferenceWidth);
    const double fitHeight = std::max(1, height - kScreenSafetyMargin -
        kNativeCaptionAllowance) / static_cast<double>(kReferenceHeight);
    return std::max(0.20, std::min(4.0, std::min(fitWidth, fitHeight)));
}
}
#endif
