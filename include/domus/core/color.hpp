#pragma once

#include <format>
#include <string>

namespace domus::color {

struct ColorRGB {
    float r, g, b;

    // Getters for [0, 255] integer range
    [[nodiscard]] constexpr int r_byte() const { return static_cast<int>(r * 255.0f + 0.5f); }
    [[nodiscard]] constexpr int g_byte() const { return static_cast<int>(g * 255.0f + 0.5f); }
    [[nodiscard]] constexpr int b_byte() const { return static_cast<int>(b * 255.0f + 0.5f); }

    // Factory method for creating from [0, 255] byte range
    [[nodiscard]] static constexpr ColorRGB from_bytes(int r, int g, int b) {
        return ColorRGB{
            static_cast<float>(r) / 255.0f,
            static_cast<float>(g) / 255.0f,
            static_cast<float>(b) / 255.0f
        };
    }
};

#define RED_RGB domus::color::ColorRGB{1.0f, 0.0f, 0.0f}
#define GREEN_RGB domus::color::ColorRGB{0.0f, 1.0f, 0.0f}
#define BLUE_RGB domus::color::ColorRGB{0.0f, 0.0f, 1.0f}
#define WHITE_RGB domus::color::ColorRGB{1.0f, 1.0f, 1.0f}
#define BLACK_RGB domus::color::ColorRGB{0.0f, 0.0f, 0.0f}
#define YELLOW_RGB domus::color::ColorRGB{1.0f, 1.0f, 0.0f}
#define CYAN_RGB domus::color::ColorRGB{0.0f, 1.0f, 1.0f}
#define MAGENTA_RGB domus::color::ColorRGB{1.0f, 0.0f, 1.0f}
#define GRAY_RGB domus::color::ColorRGB{0.5f, 0.5f, 0.5f}
#define ORANGE_RGB domus::color::ColorRGB{1.0f, 0.5f, 0.0f}
#define PURPLE_RGB domus::color::ColorRGB{0.5f, 0.0f, 0.5f}
#define TEAL_RGB domus::color::ColorRGB{0.0f, 0.5f, 0.5f}
#define LIME_RGB domus::color::ColorRGB{0.5f, 1.0f, 0.0f}
#define PINK_RGB domus::color::ColorRGB{1.0f, 0.0f, 0.5f}
#define LAVENDER_RGB domus::color::ColorRGB{0.5f, 0.0f, 1.0f}
#define BROWN_RGB domus::color::ColorRGB{0.5f, 0.25f, 0.0f}
#define OLIVE_RGB domus::color::ColorRGB{0.5f, 0.5f, 0.0f}
#define MAROON_RGB domus::color::ColorRGB{0.5f, 0.0f, 0.0f}
#define NAVY_RGB domus::color::ColorRGB{0.0f, 0.0f, 0.5f}
#define INDIGO_RGB domus::color::ColorRGB{0.0f, 0.5f, 1.0f}
#define SKY_BLUE_RGB domus::color::ColorRGB{0.0f, 1.0f, 0.5f}
#define TURQUOISE_RGB domus::color::ColorRGB{0.0f, 0.5f, 1.0f}
#define GOLD_RGB domus::color::ColorRGB{1.0f, 0.5f, 0.0f}
#define SILVER_RGB domus::color::ColorRGB{0.75f, 0.75f, 0.75f}
#define CORNERFLOWERBLUE_RGB domus::color::ColorRGB{0.392f, 0.584f, 0.929f}

inline std::string color_to_string(ColorRGB color) {
    return std::format("rgb({}, {}, {})", color.r_byte(), color.g_byte(), color.b_byte());
}

// struct ColorRGBA {
//     float r, g, b, a;
// };

} // namespace domus::color