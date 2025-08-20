#pragma once
#include <deque>
#include <qunet/util/algo.hpp>

namespace globed {

class VectorSpeedTracker {
public:
    // Push new measurement, return the delta
    std::pair<double, double> pushMeasurement(float time, double x, double y);
    std::pair<double, double> getVector();

private:
    struct Measurement {
        float time;
        double x, y;
    };

    std::pair<double, double> m_avgVector{};
    std::deque<Measurement> m_measurements;
    float m_lastTime = 0.f;
    float m_limit = 0.15f;
    double m_lastX = 0.f, m_lastY = 0.f;
    bool m_hasAverage = false;
};

}
