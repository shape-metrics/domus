#pragma once

namespace domus::drawing {

class ScaleLinear {
  private:
    const double m_domain_min;
    const double m_range_min;
    const double m_range_max;
    const double m_scale_factor;
    const bool m_clamp_enabled;

  public:
    ScaleLinear(
        double domain_min, double domain_max, double range_min, double range_max, bool clamp = false
    );
    double map(double x) const;
    double invert(double y) const;
};

} // namespace domus::drawing