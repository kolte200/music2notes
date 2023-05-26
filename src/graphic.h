#pragma once

// Draw histograme

#include "extractor.h"

#include <SDL2/SDL.h>


struct Color {
    union {
        uint32_t value;
        struct {
            uint8_t r, g, b, a;
        };
    };

    Color(uint32_t hex) : value(hex) {}
    Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 0xFF) : r(r), g(g), b(b), a(a) {}
};


class Graphic {
private:
    SDL_Window* window;
    SDL_Surface* surface;
    bool drawable;

public:
    Graphic();

    int create(int32_t width, int32_t height);

    int destroy();

    void clear(Color color);

    int render_histogram(Histogram* histogram, Color color, float yscale = 1);

    int render_rect(int32_t x, int32_t y, int32_t w, int32_t h, Color color);

    bool update(); // Return if the application must exit
};
