#include "DisplaySDL.hpp"

DisplaySDL::DisplaySDL()
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        throw DisplayError(std::string("Failed to init sdl. Error: ") + SDL_GetError());
    }
    m_window = SDL_CreateWindow("Gob-8", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 640, 320, SDL_WINDOW_SHOWN);
    if (m_window == nullptr)
    {
        throw DisplayError(std::string("Failed to create window. Error:") + SDL_GetError());
    }
    m_windowSurface = SDL_GetWindowSurface(m_window);

    // m_surface = SDL_CreateRGBSurface(0, 64, 32, 32, 0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000);
    m_surface = SDL_CreateRGBSurface(0, 64, 32, 32, 0, 0, 0, 0);
    SDL_Color color;
    color.b = 255;
    color.g = 255;
    color.r = 255;
    SDL_Color backgroundColor;
    backgroundColor.b = 0;
    backgroundColor.g = 0;
    backgroundColor.r = 0;
    m_secondaryColor = backgroundColor;
    m_primaryColor = color;
}

void DisplaySDL::update(std::array<uint8_t, TOTAL_VIDEO_MEMORY_SIZE> const &videoData)
{
    SDL_LockSurface(m_surface);
    uint8_t *pixels = (uint8_t *)m_surface->pixels;
    // for (int i = 0; i < 64 * 32; i++)
    // {
    //     pixels[i * 4 + 0] = 255;
    //     pixels[i * 4 + 1] = 255;
    //     pixels[i * 4 + 2] = 255;
    //     pixels[i * 4 + 3] = 255;
    // }

    for (size_t i = 0; i < videoData.size(); i++)
    {
        SDL_Color c = videoData[i] ? m_primaryColor : m_secondaryColor;

        // SDL_Color c = m_primaryColor;
        pixels[i * 4 + 0] = c.b;
        pixels[i * 4 + 1] = c.r;
        pixels[i * 4 + 2] = c.g;
        pixels[i * 4 + 3] = 255;
    }
    SDL_UnlockSurface(m_surface);
}

void DisplaySDL::render()
{
    SDL_BlitScaled(m_surface, 0, m_windowSurface, 0);
    SDL_UpdateWindowSurface(m_window);
}

void DisplaySDL::handleInput()
{
   
}

DisplaySDL::~DisplaySDL()
{
    SDL_FreeSurface(m_surface);
    SDL_DestroyWindow(m_window);
    SDL_Quit();
}