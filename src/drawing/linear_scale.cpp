#include "domus/drawing/linear_scale.hpp"

namespace domus::drawing {

ScaleLinear::ScaleLinear(
    double domainMin, double domainMax, double rangeMin, double rangeMax, bool clamp
)
    : m_domainMin(domainMin), m_rangeMin(rangeMin), m_rangeMax(rangeMax), m_clampEnabled(clamp) {
    m_scaleFactor = (rangeMax - rangeMin) / (domainMax - domainMin);
}

double ScaleLinear::map(double x) const {
    double y = m_rangeMin + m_scaleFactor * (x - m_domainMin);
    if (m_clampEnabled) {
        y = y < m_rangeMax ? y : m_rangeMax;
        y = y > m_rangeMin ? y : m_rangeMin;
    }
    return y;
}

double ScaleLinear::invert(double y) const {
    return m_domainMin + (y - m_rangeMin) / m_scaleFactor;
}

} // namespace domus::drawing