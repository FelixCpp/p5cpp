#pragma once

#include <algorithm>
#include <cmath>

namespace p5
{
    inline int segmentCountForArcLength(float approxArcLength, float arcLengthPerSegment, int minSegments, int maxSegments)
    {
        return std::clamp(static_cast<int>(std::ceil(approxArcLength / arcLengthPerSegment)), minSegments, maxSegments);
    }
} // namespace p5
