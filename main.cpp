#include "chip8.hpp"
#include <SDL2/SDL_render.h>
#include <chrono>
#include <iostream>
#include <thread>

#include <SDL2/SDL.h>

int main(int argc, char* argv[]) {
    // graphics scale
    static constexpr int SCALE = 10;
    // graphics colors in ARGB8888 format
    static constexpr uint32_t FG_COLOR = 0xFF4AF626;
    static constexpr uint32_t BG_COLOR = 0xFF000000;
    static constexpr std::array<uint8_t, 16> keybinds = {
        SDLK_x, SDLK_1, SDLK_2, SDLK_3, SDLK_q, SDLK_w, SDLK_e, SDLK_a,
        SDLK_s, SDLK_d, SDLK_z, SDLK_c, SDLK_4, SDLK_r, SDLK_f, SDLK_v,
    };

    if (argc < 2) {
        std::cerr << "Please provide a rom file, like: chip8 file.rom" << '\n';
        return EXIT_FAILURE;
    }

    // some SDL setup boilerplate
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL initialization failed: " << SDL_GetError() << '\n';
        return EXIT_FAILURE;
    }
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    SDL_CreateWindowAndRenderer(Chip8::WIDTH * SCALE, Chip8::HEIGHT * SCALE, 0,
                                &window, &renderer);
    if (window == nullptr) {
        std::cerr << "SDL window creation failed: " << SDL_GetError() << '\n';
        EXIT_FAILURE;
    }
    if (renderer == nullptr) {
        std::cerr << "SDL renderer creation failed: " << SDL_GetError() << '\n';
        EXIT_FAILURE;
    }

    // setup the interpreter
    Chip8 chip8{};
    chip8.reset();
    chip8.load_rom(argv[1]);

    // buffer for SDL texture
    uint32_t pixels[Chip8::WIDTH * Chip8::HEIGHT];

    SDL_Texture* sdlTexture =
        SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                          SDL_TEXTUREACCESS_STREAMING, 64, 32);

    const auto& screen = chip8.get_screen();

    SDL_Event e;
    // main loop
    while (true) {
        chip8.execute();

        // handle SDL events
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                return EXIT_FAILURE;
            }

            if (e.key.keysym.sym == SDLK_ESCAPE) {
                return EXIT_FAILURE;
            }

            if (e.type == SDL_KEYDOWN) {
                for (unsigned i = 0; i < keybinds.size(); ++i) {
                    if (e.key.keysym.sym == keybinds.at(i)) {
                        chip8.key_down(i);
                    }
                }
            }

            if (e.type == SDL_KEYUP) {
                for (unsigned i = 0; i < keybinds.size(); ++i) {
                    if (e.key.keysym.sym == keybinds.at(i)) {
                        chip8.key_up(i);
                    }
                }
            }
        }

        if (chip8.is_draw_pending()) {
            for (int i = 0; i < Chip8::HEIGHT; ++i) {
                for (int j = 0; j < Chip8::WIDTH; ++j) {
                    if (screen.at(i)[j]) {
                        pixels[i * Chip8::WIDTH + j] = FG_COLOR;
                    } else {
                        pixels[i * Chip8::WIDTH + j] = BG_COLOR;
                    }
                }
            }

            SDL_UpdateTexture(sdlTexture, nullptr, pixels,
                              Chip8::WIDTH * sizeof(Uint32));
            SDL_RenderClear(renderer);
            SDL_RenderCopy(renderer, sdlTexture, nullptr, nullptr);
            SDL_RenderPresent(renderer);

            chip8.clear_draw_pending();
        }

        // 500hz
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return EXIT_SUCCESS;
}
