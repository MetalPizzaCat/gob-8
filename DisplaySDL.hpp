#pragma once
#include "Display.hpp"

/**
 * @brief Implementation of a display for SDL using basic surface 
 * 
 */
class DisplaySDL : public Display
{
public:
    explicit DisplaySDL();
    virtual void update(std::array<uint8_t, TOTAL_VIDEO_MEMORY_SIZE> const &videoData);

    SDL_Surface *getSurface() const { return m_surface; }
    virtual ~DisplaySDL();

private:
    SDL_Surface *m_surface;
    SDL_Color m_primaryColor;
    SDL_Color m_secondaryColor;
};