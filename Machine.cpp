#include "Machine.hpp"
#include <iostream>
Machine::Machine()
{
    m_stackPointer = m_memory.size() - 1;
    std::fill(m_memory.begin(), m_memory.end(), 0);
    std::fill(m_videoPrimaryBuffer.begin(), m_videoPrimaryBuffer.end(), 0);
    std::fill(m_videoSecondaryBuffer.begin(), m_videoSecondaryBuffer.end(), 0);
    std::fill(m_keystates.begin(), m_keystates.end(), false);
    m_usingPrimaryVideoBuffer = true;
    m_programCounter = 0;
}
Machine::Machine(std::vector<uint8_t> const &bytes)
{
    std::fill(m_memory.begin(), m_memory.end(), 0);
    for (size_t i = 0; i < bytes.size() && i < m_memory.size(); i++)
    {
        m_memory[i] = bytes[i];
    }
    m_stackPointer = m_memory.size() - 1;

    std::fill(m_videoPrimaryBuffer.begin(), m_videoPrimaryBuffer.end(), 0);
    std::fill(m_videoSecondaryBuffer.begin(), m_videoSecondaryBuffer.end(), 0);
    std::fill(m_keystates.begin(), m_keystates.end(), false);
    m_usingPrimaryVideoBuffer = true;
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
        return;
    }
    switch ((opcode & 0xf000) >> 12)
    {
    case 0:
        opControlInstructions(opcode);
        break;
    case 1:
        m_programCounter = opcode & 0x0fff;
        return;
    case 2:
        pushToStack(m_programCounter);
        m_programCounter = opcode & 0x0fff;
        return;
    case 3:
    {
        if (m_registers[(opcode & 0x0f00) >> 8] == (opcode & 0x00ff))
        {
            m_programCounter += 2;
        }
    }
    break;
    case 4:
        if (m_registers[(opcode & 0x0f00) >> 8] != (opcode & 0x00ff))
        {
            m_programCounter += 2;
        }
        break;
    case 5:
        if (m_registers[(opcode & 0x0f00) >> 8] == m_registers[(opcode & 0x00f0) >> 4])
        {
            m_programCounter += 2;
        }
        break;
    case 6:
        m_registers[(opcode & 0x0f00) >> 8] = opcode & 0x00ff;
        break;
    case 7:
        m_registers[(opcode & 0x0f00) >> 8] += opcode & 0x00ff;
        break;
    case 8:
        opRegisterToRegister(opcode);
        break;
    case 0xA:
        m_memoryRegister = opcode & 0x0fff;
        break;
    case 0xB:
        m_programCounter = m_registers[0] + opcode & 0x0fff;
        break;
    case 0xC:
        m_registers[(opcode & 0x0f00) >> 8] = rand() & (opcode & 0x00ff);
        break;
    case 0xD:
        opDraw(opcode);
        break;
    case 0xE:
        if (handleKeyOpcodes(opcode))
        {
            return;
        }
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

void Machine::pushToStack(uint16_t value)
{
    m_memory[m_stackPointer - 1] = (value & 0xff00) >> 8;
    m_memory[m_stackPointer - 0] = (value & 0x00ff);
    m_stackPointer -= 1;
}

uint16_t Machine::popFromStack()
{
    if (m_stackPointer >= m_memory.size())
    {
        return -1;
    }
    uint16_t value = 0;
    value = m_memory[m_stackPointer + 0] << 8;
    value |= m_memory[m_stackPointer + 1];
    m_stackPointer += 1;

    return value;
}

bool Machine::hasValueOnStack()
{
    return m_stackPointer < m_memory.size();
}

void Machine::receiveInput(uint8_t key)
{
    if (m_inputAwaitDestinationRegister.has_value())
    {
        m_registers[m_inputAwaitDestinationRegister.value()] = key;
        m_inputAwaitDestinationRegister.reset();
    }
}

void Machine::setKeyState(uint8_t key, bool pressed)
{
    m_keystates[key] = pressed;
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
            getWorkVideoMemory()[pos + (8 - j)] ^= (line & (1 << j)) >> j;
        }
    }
}

void Machine::opControlInstructions(uint16_t opcode)
{
    if ((opcode & 0x0f00) != 0)
    {
        // this should call machine code stuff but don't  have any for now
        return;
    }
    switch ((opcode & 0x00f))
    {
    // clear screen
    case 0:
        std::fill(getWorkVideoMemory().begin(), getWorkVideoMemory().end(), 0);
        break;
    // swap buffer
    case 2:
        m_usingPrimaryVideoBuffer = !m_usingPrimaryVideoBuffer;
        break;
    case 0xe:
        m_programCounter = popFromStack();
        break;
    }
}

void Machine::opRegisterToRegister(uint16_t opcode)
{
    switch (opcode & 0x000f)
    {
    case 0:
        m_registers[(opcode & 0x0f00) >> 8] = m_registers[(opcode & 0x00f0) >> 4];
        break;
    case 1:
        m_registers[(opcode & 0x0f00) >> 8] |= m_registers[(opcode & 0x00f0) >> 4];
        break;
    case 2:
        m_registers[(opcode & 0x0f00) >> 8] &= m_registers[(opcode & 0x00f0) >> 4];
        break;
    case 3:
        m_registers[(opcode & 0x0f00) >> 8] ^= m_registers[(opcode & 0x00f0) >> 4];
        break;
    case 4:
    {
        const uint16_t res = (uint16_t)m_registers[(opcode & 0x0f00) >> 8] + (uint8_t)m_registers[(opcode & 0x00f0) >> 4];
        updateFlags(res);
        m_registers[(opcode & 0x0f00) >> 8] = res;
    }

    break;
    case 5:
    {
        const uint16_t res = (uint16_t)m_registers[(opcode & 0x0f00) >> 8] - (uint8_t)m_registers[(opcode & 0x00f0) >> 4];
        updateFlags(res);
        m_registers[(opcode & 0x0f00) >> 8] = res;
    }
    break;
    case 6:
    {
        m_registers[(opcode & 0x0f00) >> 8] = std::rotr(m_registers[(opcode & 0x0f00) >> 8], m_registers[(opcode & 0x00f0) >> 4]);
        break;
    }
    case 7:
    {
        const uint16_t res = (uint8_t)m_registers[(opcode & 0x00f0) >> 4] - (uint16_t)m_registers[(opcode & 0x0f00) >> 8];
        updateFlags(res);
        m_registers[(opcode & 0x0f00) >> 8] = res;
        break;
    }
    case 8:
    {
        m_registers[(opcode & 0x0f00) >> 8] = std::rotl(m_registers[(opcode & 0x0f00) >> 8], m_registers[(opcode & 0x00f0) >> 4]);
        break;
    }
    }
}

bool Machine::handleKeyOpcodes(uint16_t opcode)
{
    switch (opcode & 0xff)
    {
    case 0x9e: // skip if key is pressed
        if (m_keystates[m_registers[(opcode & 0x0f00) >> 8]])
        {
            m_programCounter += 4;
            return true;
        }
        break;
    case 0xa1: // skip if key is not pressed
        if (!m_keystates[m_registers[(opcode & 0x0f00) >> 8]])
        {
            m_programCounter += 4;
            return true;
        }
        break;
    }
    return false;
}
