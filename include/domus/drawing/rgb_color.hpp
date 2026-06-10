#pragma once

struct ColorRGB {
    float r, g, b;
};

#define RED_RGB         ColorRGB{1.0f, 0.0f, 0.0f}
#define GREEN_RGB       ColorRGB{0.0f, 1.0f, 0.0f}
#define BLUE_RGB        ColorRGB{0.0f, 0.0f, 1.0f}
#define WHITE_RGB       ColorRGB{1.0f, 1.0f, 1.0f}
#define BLACK_RGB       ColorRGB{0.0f, 0.0f, 0.0f}
#define YELLOW_RGB      ColorRGB{1.0f, 1.0f, 0.0f}
#define CYAN_RGB        ColorRGB{0.0f, 1.0f, 1.0f}
#define MAGENTA_RGB     ColorRGB{1.0f, 0.0f, 1.0f}
#define GRAY_RGB        ColorRGB{0.5f, 0.5f, 0.5f}
#define ORANGE_RGB      ColorRGB{1.0f, 0.5f, 0.0f}
#define PURPLE_RGB      ColorRGB{0.5f, 0.0f, 0.5f}
#define TEAL_RGB        ColorRGB{0.0f, 0.5f, 0.5f}
#define LIME_RGB        ColorRGB{0.5f, 1.0f, 0.0f}
#define PINK_RGB        ColorRGB{1.0f, 0.0f, 0.5f}
#define LAVENDER_RGB    ColorRGB{0.5f, 0.0f, 1.0f}
#define BROWN_RGB       ColorRGB{0.5f, 0.25f, 0.0f}
#define OLIVE_RGB       ColorRGB{0.5f, 0.5f, 0.0f}
#define MAROON_RGB      ColorRGB{0.5f, 0.0f, 0.0f}
#define NAVY_RGB        ColorRGB{0.0f, 0.0f, 0.5f}
#define INDIGO_RGB      ColorRGB{0.0f, 0.5f, 1.0f}
#define SKY_BLUE_RGB    ColorRGB{0.0f, 1.0f, 0.5f}
#define TURQUOISE_RGB   ColorRGB{0.0f, 0.5f, 1.0f}
#define GOLD_RGB        ColorRGB{1.0f, 0.5f, 0.0f}
#define SILVER_RGB      ColorRGB{0.75f, 0.75f, 0.75f}

// struct ColorRGBA {
//     float r, g, b, a;
// };