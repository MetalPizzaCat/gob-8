#include <iostream>
#include <array>
#include <cstdint>
#include <bitset>
#include <SDL.h>

#include "DisplaySDL.hpp"
#include "Machine.hpp"

int main(int, char **)
{

    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        std::cerr << "Failed to init sdl. Error: " << SDL_GetError() << std::endl;
        return EXIT_FAILURE;
    }
    SDL_Window *window = SDL_CreateWindow("Gob-8", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 640, 320, SDL_WINDOW_SHOWN);
    if (window == nullptr)
    {
        std::cerr << "Failed to create window. Error: " << SDL_GetError() << std::endl;
        return EXIT_FAILURE;
    }
    SDL_Surface *windowSurface = SDL_GetWindowSurface(window);
    SDL_Rect windowRect;
    windowRect.w = 640;
    windowRect.h = 320;

    Machine machine;
    machine.writeSpriteToMemory(0xff, {0b10000000, 0b01000010, 0b00100100, 0b00011000});
    // mov v1, 6h
    machine.getMemory()[0] = 0x61;
    machine.getMemory()[1] = 0x6;
    // mov v2, 6h
    machine.getMemory()[2] = 0x62;
    machine.getMemory()[3] = 0x6;
    // set i, 0xff
    machine.getMemory()[4] = 0xA0;
    machine.getMemory()[5] = 0xff;
    // draw v1, v2, 4
    machine.getMemory()[6] = 0xD1;
    machine.getMemory()[7] = 0x24;
    // mov v1, 6h
    machine.getMemory()[8] = 0x61;
    machine.getMemory()[9] = 0x10;
    // mov v2, 6h
    machine.getMemory()[10] = 0x62;
    machine.getMemory()[11] = 0x10;
     // draw v1, v2, 4
    machine.getMemory()[12] = 0xD1;
    machine.getMemory()[13] = 0x24;
    //halt
    machine.getMemory()[14] = 0x00;
    machine.getMemory()[15] = 0xe1;

    DisplaySDL display;

    SDL_Event e;
    bool quit = false;
    while (!quit)
    {
        while (SDL_PollEvent(&e))
        {
            if (e.type == SDL_QUIT)
            {
                quit = true;
            }
        }
        machine.step();
        display.update(machine.getVideoMemory());
        SDL_BlitScaled(display.getSurface(), 0, windowSurface, 0);
        SDL_UpdateWindowSurface(window);
    }
    SDL_DestroyWindow(window);
    SDL_Quit();
    return EXIT_SUCCESS;
}
