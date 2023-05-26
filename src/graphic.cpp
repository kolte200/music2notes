#include "graphic.h"

#include "utils.h"


Graphic::Graphic() {
    surface = nullptr;
    window = nullptr;
    drawable = false;
}


int Graphic::create(int width, int height) {
    if (SDL_Init(SDL_INIT_EVENTS | SDL_INIT_VIDEO)) {
        fprintf(stderr, "SDL_Init error : %s", SDL_GetError());
        return 1;
    }

    window = SDL_CreateWindow("Graphic", 100, 100, width, height, SDL_WINDOW_SHOWN);
    if (window == NULL) {
        fprintf(stderr, "SDL_CreateWindow error : %s", SDL_GetError());
        return 2;
    }

    surface = SDL_GetWindowSurface(window);
    drawable = true;
    return 0;
}


int Graphic::destroy() {
    drawable = false;
    if (window != NULL)
        SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}


int Graphic::render_histogram(Histogram* histogram, Color color, float yscale) {
    if (!drawable) return 2;

    int w = surface->w;
    int h = surface->h;

    HistogramEntry* entries = histogram->entries;

    for (size_t i = 0, m = histogram->nb_entries; i < m; i++) {
        int32_t x1 =  i    * w / histogram->nb_entries;
        int32_t x2 = (i+1) * w / histogram->nb_entries;
        int32_t val = h * entries[i].value * yscale;
        render_rect(x1, h - val, x2 - x1, val, color);
    }

    return 0;
}


void Graphic::clear(Color color) {
    SDL_FillRect(surface, NULL, color.value);
}


int Graphic::render_rect(int32_t x, int32_t y, int32_t w, int32_t h, Color color) {
    int32_t x1 = max(0, x);
    int32_t y1 = max(0, y);
    int32_t x2 = min(x + w, surface->w);
    int32_t y2 = min(y + h, surface->h);

    SDL_LockSurface(surface);
    uint32_t* pixels = (uint32_t*) surface->pixels;
    for (int32_t x = x1; x < x2; x++) {
        for (int32_t y = y1; y < y2; y++) {
            pixels[y * surface->w + x] = color.value;
        }
    }
    SDL_UnlockSurface(surface);

    return 0;
}


bool Graphic::update() {
    SDL_Event event;
    while(SDL_PollEvent(&event) > 0) {
        switch(event.type) {
            case SDL_QUIT:
                destroy();
                return true;
        }
    }

    SDL_UpdateWindowSurface(window);

    return false;
}
