#include "SpeedTracker.hpp"

namespace globed {

std::pair<double, double> VectorSpeedTracker::pushMeasurement(float time, double x, double y)
{
    double dt = (double)(time - m_lastTime);
    if (dt == 0.0) {
        // first frame, we have nothing to go off of to measure speed
        return {0.0, 0.0};
    }

    double dx = x - m_lastX;
    double dy = y - m_lastY;

    m_lastTime = time;
    m_lastX = x;
    m_lastY = y;

    Measurement mmt{
        .time = time,
        .x = dx / dt,
        .y = dy / dt,
    };

    // don't log falsey looking measurements with impossible speeds,
    // they will mess up calculations
    if (std::abs(mmt.x) < 5000.0 && std::abs(mmt.x) < 5000.0) {
        m_measurements.push_back(mmt);
    }

    while (!m_measurements.empty() && time - m_measurements.front().time > m_limit) {
        m_measurements.pop_front();
    }

    return std::make_pair(dx, dy);
}

std::pair<double, double> VectorSpeedTracker::getVector()
{
    if (m_measurements.empty()) {
        return {0.0, 0.0};
    }

    std::pair<double, double> sum{0.0, 0.0};

    for (auto &m : m_measurements) {
        sum.first += m.x;
        sum.second += m.y;
    }

    std::pair<double, double> vec{sum.first / m_measurements.size(), sum.second / m_measurements.size()};

    // use ema if we have something to go off of
    if (m_hasAverage) {
        m_avgVector = std::make_pair(qn::exponentialMovingAverage(m_avgVector.first, vec.first, 0.3),
                                     qn::exponentialMovingAverage(m_avgVector.second, vec.second, 0.3));
    } else {
        m_avgVector = vec;
    }

    m_hasAverage = true;

    return m_avgVector;
}

} // namespace globed