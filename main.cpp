#include <iostream>
#include <array>
#include <cstdint>
#include <bitset>
#include <SDL.h>
#include <fstream>

#include "DisplaySDL.hpp"
#include "Machine.hpp"

int main(int argc, char **argv)
{
    std::string inputFilename = "./game.bin";
    for (int i = 0; i < argc; i++)
    {
        std::string arg = std::string(argv[i]);
        if (arg == "-i" || arg == "--input")
        {
            if (i + 1 > argc)
            {
                std::cerr << "Missing filename for input flag" << std::endl;
                return EXIT_FAILURE;
            }
            inputFilename = std::string(argv[i + 1]);
        }
    }
    std::ifstream file(inputFilename, std::ios::binary);
    if (!file.is_open())
    {
        std::cerr << "Unable to open the input file" << std::endl;
        return EXIT_FAILURE;
    }

    // read the data:
    std::vector<uint8_t> bytes = std::vector<uint8_t>((std::istreambuf_iterator<char>(file)),
                                                      std::istreambuf_iterator<char>());

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

    Machine machine(bytes);
    // machine.writeSpriteToMemory(0xff, {0b10000000, 0b01000010, 0b00100100, 0b00011000});
    // // mov v1, 6h
    // machine.getMemory()[0] = 0x61;
    // machine.getMemory()[1] = 0x6;
    // // mov v2, 6h
    // machine.getMemory()[2] = 0x62;
    // machine.getMemory()[3] = 0x6;
    // // set i, 0xff
    // machine.getMemory()[4] = 0xA0;
    // machine.getMemory()[5] = 0xff;
    // // clear
    // machine.getMemory()[6] = 0x00;
    // machine.getMemory()[7] = 0xe0;
    // // add v1, 1
    // machine.getMemory()[8] = 0x71;
    // machine.getMemory()[9] = 0x01;
    // // draw v1, v2, 4
    // machine.getMemory()[10] = 0xD1;
    // machine.getMemory()[11] = 0x24;

    // // if v1 == 60
    // machine.getMemory()[12] = 0x41;
    // machine.getMemory()[13] = 0x38;
    // // add v2, 1
    // machine.getMemory()[14] = 0x72;
    // machine.getMemory()[15] = 0x01;
    // // swap buffer
    // machine.getMemory()[16] = 0x00;
    // machine.getMemory()[17] = 0xe2;
    // // jump 0006
    // machine.getMemory()[18] = 0x10;
    // machine.getMemory()[19] = 0x06;
    // // halt
    // machine.getMemory()[20] = 0x00;
    // machine.getMemory()[21] = 0xe1;

    DisplaySDL display;

    SDL_Event e;
    bool quit = false;
    uint32_t timePrev = SDL_GetTicks();
    uint32_t timeNow = SDL_GetTicks();
    double delta = 0;
    while (!quit)
    {
        while (SDL_PollEvent(&e))
        {
            if (e.type == SDL_QUIT)
            {
                quit = true;
            }
        }
        timeNow = SDL_GetTicks();
        delta = timeNow - timePrev;
        if (delta > 1000.0 / 60.0)
        {
            machine.step();
            display.update(machine.getCurrentVideoMemory());
            SDL_BlitScaled(display.getSurface(), 0, windowSurface, 0);
            SDL_UpdateWindowSurface(window);
            timePrev = timeNow;
            delta = 0;
        }
    }
    SDL_DestroyWindow(window);
    SDL_Quit();
    return EXIT_SUCCESS;
}
