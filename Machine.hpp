#pragma once
#include <SDL.h>
#include <array>
#include <vector>
class Machine
{

public:
    /// @brief Video memory of the pseudo console. Although we can use bytes to compress the data horizontally by packing bits into bytes, we can't do that vertically
    using VideoMemoryType = std::array<uint8_t, TOTAL_VIDEO_MEMORY_SIZE>;
    using VirtualMemoryType = std::array<uint8_t, TOTAL_MEMORY_SIZE>;
    explicit Machine();
    void step();

    void render();

    /**
     * @brief Writes the data from the sprite into the ram at a given position
     *
     * @param position Point in memory to write to
     * @param sprite bytes for the sprite
     */
    void writeSpriteToMemory(size_t position, std::vector<uint8_t> sprite);

    /**
     * @brief Push the value into the virtual memory stack
     *
     * @param value Value to push onto the stack
     */
    void pushToStack(uint32_t value);

    /**
     * @brief Get the value from top of the stack or -1 if empty. To ensure that the stack actually has values
     *
     * @return uint32_t
     */
    uint32_t popFromStack();

    bool hasValueOnStack();

    /// @brief Get video memory currently used for writing data
    /// @return
    VideoMemoryType &getWorkVideoMemory() { return m_usingPrimaryVideoBuffer ? m_videoPrimaryBuffer : m_videoSecondaryBuffer; }
    /// @brief Get video memory currently ready to be displayed
    /// @return
    VideoMemoryType &getCurrentVideoMemory() { return !m_usingPrimaryVideoBuffer ? m_videoPrimaryBuffer : m_videoSecondaryBuffer; }
    VirtualMemoryType &getMemory() { return m_memory; }

private:
    void opDraw(uint16_t opcode);

    void opControlInstructions(uint16_t opcode);

    inline void opSpecialFunctions(uint16_t opcode)
    {
        if ((opcode & 0x00ff) == 0x1e)
        {
            m_memoryRegister += m_registers[(opcode & 0x0f00) >> 8];
        }
    }
    void opRegisterToRegister(uint16_t opcode);

    inline void updateFlags(uint16_t res)
    { // set carry flag
        m_registers[15] = (m_registers[15] & 0xFE) | (res & 0xFF > 0);
        // set sign flag
        m_registers[15] = (m_registers[15] & 0xFD) | ((res > 127) << 1);
        // set zero flag
        m_registers[15] = (m_registers[15] & 0xFB) | (((res & 0xff) == 0) << 2);
    }

    VirtualMemoryType m_memory;
    VideoMemoryType m_videoPrimaryBuffer;
    VideoMemoryType m_videoSecondaryBuffer;
    bool m_usingPrimaryVideoBuffer;
    size_t m_programCounter;
    size_t m_memoryRegister;
    std::array<uint8_t, 16> m_registers;
    size_t m_stackPointer;
};