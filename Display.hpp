#pragma once
#include <array>
#include <SDL.h>
#include <string>
#include <exception>
/**
 * @brief Base class for any display implementation. Display being the representation of the screen and gamepad
 *
 */
class Display
{
public:
    virtual void update(std::array<uint8_t, TOTAL_VIDEO_MEMORY_SIZE> const &videoData) = 0;
    virtual ~Display() = default;

    virtual void render() = 0;

    virtual void handleInput() = 0;

    virtual void playSound() = 0;
};

/**
 * @brief Exception class for errors related to setting up display
 * 
 */
class DisplayError : public std::exception
{
public:
    const char *what() const throw() override { return m_message.c_str(); }
    DisplayError(std::string const &msg) : m_message(msg) {}
private:
    std::string m_message;
};
