#include "domus/drawing/linear_scale.hpp"

namespace domus::drawing {

ScaleLinear::ScaleLinear(
    double domain_min, double domain_max, double range_min, double range_max, bool clamp
)
    : m_domain_min(domain_min), m_range_min(range_min), m_range_max(range_max),
      m_scale_factor((range_max - range_min) / (domain_max - domain_min)), m_clamp_enabled(clamp) {
    ;
}

double ScaleLinear::map(const double x) const {
    double y = m_range_min + m_scale_factor * (x - m_domain_min);
    if (m_clamp_enabled) {
        y = y < m_range_max ? y : m_range_max;
        y = y > m_range_min ? y : m_range_min;
    }
    return y;
}

double ScaleLinear::invert(const double y) const {
    return m_domain_min + (y - m_range_min) / m_scale_factor;
}

} // namespace domus::drawing