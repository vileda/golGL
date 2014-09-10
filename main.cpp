#include <iostream>

#include <boost/range/irange.hpp>

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "world.hpp"

using namespace std;

namespace Color {
    static SDL_Color RED = SDL_Color{255, 0, 0, 0};
    static SDL_Color GREEN = SDL_Color{0, 255, 0, 0};
    static SDL_Color BLUE = SDL_Color{0, 0, 255, 0};
    static SDL_Color BLACK = SDL_Color{0, 0, 0, 0};
    static SDL_Color WHITE = SDL_Color{255, 255, 255, 0};
    static SDL_Color TRANSPARENT = SDL_Color{0, 0, 0, 255};
};

class GameWindow {
public:
    bool paint_cell = true;
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Surface *surface;
    SDL_Event event;
    world w;
    TTF_Font *font;
    bool evolution = false;
    SDL_Texture *cells_texture;
    int scale;
    int generations = -1;
    Uint64 frames = 1;
    Uint32 last_ticks;
    string fps_text = "FPS: 0";

public:
    GameWindow(int width, int height, int scale)
            :
              scale(scale),
              w(width, height)
    {
        SDL_Init(SDL_INIT_VIDEO);
        window = SDL_CreateWindow("Game of Life", 0, 0, width*scale, height*scale, 0);
        renderer = SDL_CreateRenderer(window, 0, SDL_RENDERER_ACCELERATED);
        cells_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, width, height);
        TTF_Init();
        font = TTF_OpenFont("arial.ttf", 32);
        surface = SDL_CreateRGBSurfaceFrom(NULL, width, height, 32, 0,
                0x00FF0000,
                0x0000FF00,
                0x000000FF,
                0xFF000000);
        w.ratio_w = (width / w.width);
        w.ratio_h = (height / w.height);
        w.seed_life();
        last_ticks = SDL_GetTicks();
    }

    void loop() {
        while (paint_cell) {
            handleEvents();
            update();
        }
    }

    void handleEvents() {
        while(SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_KEYUP:
                    buttonUp();
                    break;
                case SDL_KEYDOWN:
                    buttonDown();
                    break;
                case SDL_MOUSEMOTION:
                    //handleMouse();
                    break;
            }
        }
    }

    SDL_Color const get_cell_color(const cell &c, const cell &cl) {
        if (c.alive /*&& cl.alive*/)
            return Color::GREEN;
        if (c.alive && !cl.alive)
            return Color::WHITE;
        if (!c.alive && cl.alive)
            return Color::BLUE;

        return Color::BLACK;
    }

    void render_cells() {
        SDL_LockTexture(cells_texture, NULL,
                &surface->pixels,
                &surface->pitch);
        Uint32 *p = (Uint32*)surface->pixels;
        for (auto y : boost::irange(0, w.height)) {
            for (auto x : boost::irange(0, w.width)) {
                auto &c = w.cells[x][y];
                auto &c_last = w.last_gen[x][y];
                auto cell_color = get_cell_color(c, c_last);
                *p++ = (0xFF000000|(cell_color.r<<16)|(cell_color.g<<8)|cell_color.b);
            }
        }
        SDL_UnlockTexture(cells_texture);

    }

    void toggle_cell() {
        int x = 1;//input().mouseX() / w.ratio_w;
        int y = 1;//input().mouseY() / w.ratio_h;
        w.cells[x][y].alive = !w.cells[x][y].alive;
    }

    void update() {
        if (generations > -1) {
            if (generations <= w.generation)
                exit(generations);
        }
        if (evolution) {
            w.next_generation();
        }
        render_cells();
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, cells_texture, NULL, NULL);
        Uint32 delta = SDL_GetTicks() - last_ticks;
        if(delta > 0 && frames > 300/delta) {
            fps_text = "FPS: "+ to_string(1000.f/delta) + " - Generation: " + to_string(w.generation);
            frames = 0;
        }
        SDL_Surface *text = TTF_RenderText_Shaded(font, fps_text.c_str(), Color::WHITE, Color::TRANSPARENT);
        SDL_Texture *text_texture = SDL_CreateTextureFromSurface(renderer, text);
        SDL_Rect text_pos{16,16,220,32};
        SDL_RenderCopy(renderer, text_texture, NULL, &text_pos);
        SDL_RenderPresent(renderer);
        frames++;
        last_ticks = SDL_GetTicks();
        SDL_DestroyTexture(text_texture);
        SDL_FreeSurface(text);
    }

    void buttonDown() {
        switch (event.key.keysym.scancode) {
            case SDL_SCANCODE_ESCAPE:
                exit(0);
            case SDL_SCANCODE_SPACE:
                w.seed_life();
                w.generation = 0;
                //generations = -1;
                render_cells();
                break;
            case SDL_SCANCODE_E:
                evolution = !evolution;
                break;
            case SDL_SCANCODE_C:
                evolution = false;
                w.seed_life(false);
                w.generation = 0;
                render_cells();
                break;
            case SDL_SCANCODE_S:
                evolution = false;
                w.next_generation();
                render_cells();
                break;
            case SDL_SCANCODE_D:
                w.dump_generation();
                break;
            case SDL_SCANCODE_L:
                w.load_generation("dump_" + w.last_dump_str + ".gol");
                break;
            case SDL_SCANCODE_LEFT:
                toggle_cell();
                paint_cell = true;
                break;
        }
    }

    void buttonUp() {
        //paint_cell = true;
    }
};

int main(int argc, char **argv) {

    if (argc < 4) {
        std::cout << "usage: " << argv[0] << " <width> <height> <scale> [<filename>] [<generations>]" << std::endl;
        return -1;
    }

    GameWindow window(std::stoi(argv[1]), std::stoi(argv[2]), std::stoi(argv[3]));

    if (argc >= 5) {
        window.w.load_generation(argv[4]);
    }

    if (argc >= 6) {
        window.generations = std::stoi(argv[5]);
        window.evolution = true;
    }

    window.loop();

    return 0;
}

