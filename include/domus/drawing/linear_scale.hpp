#ifndef MY_LINEAR_SCALE_H
#define MY_LINEAR_SCALE_H

#include <algorithm>

class ScaleLinear {
  private:
    double m_domainMin, m_domainMax;
    double m_rangeMin, m_rangeMax;
    double m_scaleFactor;
    bool m_clampEnabled;

  public:
    ScaleLinear(
        double domainMin, double domainMax, double rangeMin, double rangeMax, bool clamp = false)
        : m_domainMin(domainMin), m_domainMax(domainMax), m_rangeMin(rangeMin),
          m_rangeMax(rangeMax), m_clampEnabled(clamp) {
        m_scaleFactor = (rangeMax - rangeMin) / (domainMax - domainMin);
    }
    [[nodiscard]] double map(double x) const {
        double y = m_rangeMin + m_scaleFactor * (x - m_domainMin);
        if (m_clampEnabled)
            y = std::max(m_rangeMin, std::min(y, m_rangeMax));
        return y;
    }
    [[nodiscard]] double invert(double y) const {
        return m_domainMin + (y - m_rangeMin) / m_scaleFactor;
    }
};

#endif