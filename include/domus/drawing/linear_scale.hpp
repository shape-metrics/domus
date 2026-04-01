#pragma once

namespace domus::drawing {

class ScaleLinear {
  private:
    double m_domainMin;
    double m_rangeMin, m_rangeMax;
    double m_scaleFactor;
    bool m_clampEnabled;

  public:
    ScaleLinear(
        double domainMin, double domainMax, double rangeMin, double rangeMax, bool clamp = false
    );
    double map(double x) const;
    double invert(double y) const;
};

} // namespace domus::drawing