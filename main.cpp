#include <iostream>
#include <array>
#include <cstdint>
#include <bitset>
#include <SDL.h>
#include <fstream>
#include <algorithm>

#include "DisplaySDL.hpp"
#include "Machine.hpp"

static const std::vector<SDL_Scancode> Keymap = {
    // wasd
    SDL_Scancode::SDL_SCANCODE_W, // 0x1
    SDL_Scancode::SDL_SCANCODE_A, // 0x2
    SDL_Scancode::SDL_SCANCODE_S, // 0x3
    SDL_Scancode::SDL_SCANCODE_D, // 0x4
    // arrows
    SDL_Scancode::SDL_SCANCODE_UP,    // 0x5
    SDL_Scancode::SDL_SCANCODE_DOWN,  // 0x6
    SDL_Scancode::SDL_SCANCODE_LEFT,  // 0x7
    SDL_Scancode::SDL_SCANCODE_RIGHT, // 0x8
    // input right
    SDL_Scancode::SDL_SCANCODE_LSHIFT, // 0x9
    SDL_Scancode::SDL_SCANCODE_SPACE,  // 0xa
    // input left
    SDL_Scancode::SDL_SCANCODE_RSHIFT, // 0xb
    SDL_Scancode::SDL_SCANCODE_RCTRL,  // 0xc
    // pause
    SDL_Scancode::SDL_SCANCODE_ESCAPE,   // 0xd
    SDL_Scancode::SDL_SCANCODE_TAB,      // 0xe
    SDL_Scancode::SDL_SCANCODE_BACKSPACE // 0xf
};

std::optional<uint8_t> handleInput(SDL_Scancode key)
{
    std::vector<SDL_Scancode>::const_iterator it = std::find(Keymap.begin(), Keymap.end(), key);
    if (it == Keymap.end())
    {
        return {};
    }
    return (it - Keymap.begin());
}

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

    Machine machine(bytes);

    DisplaySDL display;

    SDL_Event e;
    bool quit = false;
    uint32_t timePrev = SDL_GetTicks();
    uint32_t timeNow = SDL_GetTicks();
    double delta = 0;
    std::optional<uint8_t> lastKeyPressed;
    while (!quit)
    {
        SDL_Event e;
        while (SDL_PollEvent(&e))
        {
            switch (e.type)
            {
            case SDL_QUIT:
                quit = true;
                break;
            case SDL_KEYDOWN:
                if (machine.isAwaitingInput())
                {
                    if (std::optional<uint8_t> inp = handleInput(e.key.keysym.scancode); inp.has_value())
                    {
                        lastKeyPressed = inp.value();
                    }
                }
                break;
            }
        }
        timeNow = SDL_GetTicks();
        delta = timeNow - timePrev;
        if (delta > 1000.0 / 60.0)
        {
            if (machine.isAwaitingInput())
            {
                if (lastKeyPressed.has_value())
                {
                    machine.receiveInput(lastKeyPressed.value());
                    lastKeyPressed.reset();
                }
            }
            else
            {
                machine.step();
            }
            display.update(machine.getCurrentVideoMemory());
            display.render();
            timePrev = timeNow;
            delta = 0;
        }
    }

    return EXIT_SUCCESS;
}
