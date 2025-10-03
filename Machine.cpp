#include "Machine.hpp"
#include <iostream>
Machine::Machine()
{
    m_stackPointer = m_memory.size() - 1;
    std::fill(m_memory.begin(), m_memory.end(), 0);
    std::fill(m_video.begin(), m_video.end(), 0);
    m_programCounter = 0;
}
void Machine::step()
{
    if (m_programCounter >= m_memory.size())
    {
        return;
    }
    uint16_t opcode = m_memory[m_programCounter + 1] | (((uint16_t)m_memory[m_programCounter]) << 8);
    // halt instruction
    if (opcode == 0x00e1)
    {
        m_programCounter = m_memory.size();
    }
    switch ((opcode & 0xf000) >> 12)
    {
    case 6:
        opSetRegisterToConst(opcode);
        break;
    case 7:
        m_registers[(opcode & 0x0f00) >> 8] = opcode & 0x00ff;
        break;
    case 8:
        opRegisterToRegister(opcode);
        break;
    case 0xA:
        m_memoryRegister = opcode & 0x0fff;
        break;
    case 0xD:
        opDraw(opcode);
        break;
    case 0xE:
        break;
    case 0xF:
        opSpecialFunctions(opcode);
        break;
    default:
        break;
    }
    m_programCounter += 2;
}

void Machine::render()
{
}

void Machine::writeSpriteToMemory(size_t position, std::vector<uint8_t> sprite)
{
    for (size_t i = 0; i < sprite.size(); i++)
    {
        m_memory[position + i] = sprite[i];
    }
}

void Machine::opDraw(uint16_t opcode)
{
    const uint8_t x = m_registers[(opcode & 0x0f00) >> 8];
    const uint8_t y = m_registers[(opcode & 0x00f0) >> 4];

    const uint8_t height = opcode & 0x000f;
    for (int i = 0; i <= height; i++)
    {
        const size_t pos = x + ((y + i) * 64);
        const uint8_t line = m_memory[m_memoryRegister + i];
        for (int j = 0; j < 8; j++)
        {
            auto t1 = line & (1 << j);
            auto t2 = t1 >> j;
            m_video[pos + (8 - j)] ^= (line & (1 << j)) >> j;
        }
    }
}

void Machine::opRegisterToRegister(uint16_t opcode)
{
    switch (opcode & 0x000f)
    {
    case 0:
        m_registers[(opcode & 0x0f00) >> 16] = m_registers[(opcode & 0x00f0) >> 8];
        break;
    case 1:
        m_registers[(opcode & 0x0f00) >> 16] |= m_registers[(opcode & 0x00f0) >> 8];
        break;
    case 2:
        m_registers[(opcode & 0x0f00) >> 16] &= m_registers[(opcode & 0x00f0) >> 8];
        break;
    case 3:
        m_registers[(opcode & 0x0f00) >> 16] ^= m_registers[(opcode & 0x00f0) >> 8];
        break;
    case 4:
    {
        const uint16_t res = (uint16_t)m_registers[(opcode & 0x0f00) >> 16] + (uint8_t)m_registers[(opcode & 0x00f0) >> 8];
        updateFlags(res);
        m_registers[(opcode & 0x0f00) >> 16] = res;
    }

    break;
    case 5:
    {
        const uint16_t res = (uint16_t)m_registers[(opcode & 0x0f00) >> 16] - (uint8_t)m_registers[(opcode & 0x00f0) >> 8];
        updateFlags(res);
        m_registers[(opcode & 0x0f00) >> 16] = res;
    }
    break;
    case 6:
    {
        const uint16_t res = (uint16_t)m_registers[(opcode & 0x0f00) >> 16] >> (uint8_t)m_registers[(opcode & 0x00f0) >> 8];
        updateFlags(res);
        m_registers[(opcode & 0x0f00) >> 16] = res;
        break;
    }
    case 7:
    {
        const uint16_t res = (uint8_t)m_registers[(opcode & 0x00f0) >> 8] - (uint16_t)m_registers[(opcode & 0x0f00) >> 16];
        updateFlags(res);
        m_registers[(opcode & 0x0f00) >> 16] = res;
        break;
    }
    case 8:
    {
        const uint16_t res = (uint16_t)m_registers[(opcode & 0x0f00) >> 16] << (uint8_t)m_registers[(opcode & 0x00f0) >> 8];
        updateFlags(res);
        m_registers[(opcode & 0x0f00) >> 16] = res;
        break;
    }
    }
}
