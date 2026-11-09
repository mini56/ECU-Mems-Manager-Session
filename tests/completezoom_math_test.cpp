#include "../completezoommath.h"
#include <cassert>
#include <cmath>
#include <iostream>

int main()
{
    const int workAreas[][2] = {
        {1024, 728}, {1200, 728}, {1280, 680}, {1366, 728},
        {1600, 860}, {1920, 1040}, {1920, 1160},
        {2560, 1400}, {3840, 2120}
    };
    for (const auto &work : workAreas) {
        const double scale = CompleteZoom::factorForWorkArea(work[0],work[1]);
        assert(scale > 0.0);
        const int usedW=static_cast<int>(std::ceil(
            CompleteZoom::kReferenceWidth * scale));
        const int usedH=static_cast<int>(std::ceil(
            CompleteZoom::kReferenceHeight * scale)) +
            CompleteZoom::kNativeCaptionAllowance;
        if (usedW > work[0] || usedH > work[1]) {
            std::cerr << "FAIL " << work[0] << 'x' << work[1]
                      << " zoom=" << scale << std::endl;
            return 1;
        }
        std::cout << "PASS " << work[0] << 'x' << work[1]
                  << " zoom=" << scale << std::endl;
    }
    return 0;
}
