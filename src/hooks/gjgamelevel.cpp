#include "gjgamelevel.hpp"

void HookedGJGameLevel::savePercentage(int p0, bool p1, int p2, int p3, bool p4) {
    if (!m_fields->shouldStopProgress) GJGameLevel::savePercentage(p0, p1, p2, p3, p4);
}