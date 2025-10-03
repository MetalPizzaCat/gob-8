#pragma once
#include <array>
#include <SDL.h>
/**
 * @brief Base class for any display implementation
 * 
 */
class Display
{
public:
    virtual void update(std::array<uint8_t, TOTAL_VIDEO_MEMORY_SIZE> const &videoData) = 0;
    virtual ~Display() = default;
};