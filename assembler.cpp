#include <iostream>
#include <map>
#include <optional>
#include <vector>
#include <sstream>
#include <cstdint>
#include <limits>
#include <regex>
#include <fstream>
#include <exception>

enum class Instruction
{
    None,
    Move,
    Clear,
    Render,
    Draw,
    SetMemory,
    Jump,
    Call,
    Return,
    Add,
    Sub,
    Or,
    And,
    Xor,
    RotateRight,
    RotateLeft,
    Equals,
    NotEquals,
    In,
    KeyPressed,
    KeyNotPressed,
    MemAdd,
    SetAudioTimer,
    GetTimer,
    SetTimer,
    Halt
};

enum class DataSize
{
    Byte,
    Word
};

std::string hexNumbers = "0123456789abcdef";

bool isValidHexDigit(char c)
{
    return hexNumbers.find(c) != std::string::npos;
}

using StringConstIterator = std::string::const_iterator;

struct InstructionData
{
    std::string name;
    int argumentCount;
    Instruction instruction;
};

static const std::vector<std::string> AssembleDataOperationKeywords = {
    "times", "db", "dw"};

static const std::map<std::string, DataSize> DataStoreSizeKeywords = {
    {"db", DataSize::Byte},
    {"dw", DataSize::Word}};

static const std::map<std::string, Instruction> Instructions = {
    {"nop", Instruction::None},
    {"call", Instruction::Call},
    {"mov", Instruction::Move},
    {"jmp", Instruction::Jump},
    {"goto", Instruction::Jump},
    {"hlt", Instruction::Halt},
    {"end", Instruction::Halt},
    {"ret", Instruction::Return},
    {"draw", Instruction::Draw},
    {"mem", Instruction::SetMemory},
    {"clear", Instruction::Clear},
    {"in", Instruction::In},
    {"add", Instruction::Add},
    {"sub", Instruction::Sub},
    {"or", Instruction::Or},
    {"and", Instruction::And},
    {"xor", Instruction::Xor},
    {"ror", Instruction::RotateRight},
    {"rol", Instruction::RotateLeft},
    {"eq", Instruction::Equals},
    {"neq", Instruction::NotEquals},
    {"keydown", Instruction::KeyPressed},
    {"keyup", Instruction::KeyNotPressed},
    {"memadd", Instruction::MemAdd},
    {"beep", Instruction::SetAudioTimer},
    {"settimer", Instruction::SetTimer},
    {"gettimer", Instruction::GetTimer},
    {"render", Instruction::Render},
};

class AssemblingError : public std::exception
{
public:
    const char *what() const throw() override { return m_full.c_str(); }
    AssemblingError(size_t column, size_t row, std::string const &msg) : m_row(row), m_column(column), m_message(msg)
    {
        m_full = ("Error at line " + std::to_string(m_row + 1) + " row " + std::to_string(m_column + 1) + ": " + m_message);
    }

    size_t getRow() const { return m_row; }
    size_t getColumn() const { return m_column; }

private:
    std::string m_full;
    size_t m_row;
    size_t m_column;
    std::string m_message;
};

class Assembler
{
public:
    explicit Assembler(std::vector<std::string> const &code) : m_code(code)
    {
    }

    std::vector<uint8_t> &getBytes() { return m_bytes; }

    void parse()
    {

        for (m_currentLineNumber = 0; m_currentLineNumber < m_code.size(); m_currentLineNumber++)
        {
            m_begin = m_code[m_currentLineNumber].begin();
            m_current = m_code[m_currentLineNumber].begin();
            m_end = m_code[m_currentLineNumber].end();

            skipWhitespace();
            if (m_code[m_currentLineNumber].size() == 0)
            {
                continue;
            }
            std::optional<std::string> label = parseLabel();
            if (label.has_value())
            {
                m_labelPositions[label.value()] = m_bytes.size();
            }
            skipWhitespace();

            if (m_current == m_end)
            {
                continue;
            }
            if (tryDataOperation())
            {
                assembleDataOperations();
                continue;
            }
            std::optional<Instruction> instruction = parseInstruction();
            if (!instruction.has_value())
            {
                throw AssemblingError(m_current - m_begin, m_currentLineNumber, "Excepted an instruction");
            }
            switch (instruction.value())
            {
            case Instruction::None:
                m_bytes.push_back(0);
                break;
            case Instruction::Move:
            {
                assembleMoveOperation();
                break;
            }
            case Instruction::Clear:
            {
                m_bytes.push_back(0x00);
                m_bytes.push_back(0xe0);
                expectLineEnd();
                break;
            }
            case Instruction::Render:
            {
                m_bytes.push_back(0x00);
                m_bytes.push_back(0xe2);
                expectLineEnd();
                break;
            }
            case Instruction::Draw:
            {
                assembleDraw();
                break;
            }
            case Instruction::SetMemory:
            {
                assembleAddressInstruction(0xA);
                break;
            }
            case Instruction::Jump:
            {
                assembleAddressInstruction(0x1);
                break;
            }
            case Instruction::Call:
            {
                assembleAddressInstruction(0x2);
                break;
            }

            case Instruction::Return:
            {
                m_bytes.push_back(0x00);
                m_bytes.push_back(0xee);
                expectLineEnd();
                break;
            }
            case Instruction::Add:
            {
                assembleAddOperation();

                break;
            }
            case Instruction::Sub:
            {
                assembleMathOperations(0x5);
                break;
            }
            case Instruction::Or:
            {
                assembleMathOperations(0x1);
                break;
            }
            case Instruction::And:
            {
                assembleMathOperations(0x2);
                break;
            }
            case Instruction::Xor:
            {
                assembleMathOperations(0x3);
                break;
            }
            case Instruction::RotateRight:
            {
                assembleMathOperations(0x6);
                break;
            }
            case Instruction::RotateLeft:
            {
                assembleMathOperations(0x8);
                break;
            }
            case Instruction::Equals:
            {
                assembleEqualsOperation(0x3, 0x5);
                break;
            }
            case Instruction::NotEquals:
            {
                assembleEqualsOperation(0x4, 0x9);
                break;
            }
            case Instruction::KeyPressed:
            {
                assembleCheckKeyPress(0x9e);
                break;
            }
            case Instruction::KeyNotPressed:
            {
                assembleCheckKeyPress(0xa1);
                break;
            }
            case Instruction::In:
            {
                skipWhitespace();
                if (std::optional<size_t> registerId = parseRegister(); registerId.has_value())
                {
                    m_bytes.push_back(0xF0 | registerId.value());
                    m_bytes.push_back(0x0a);
                    expectLineEnd();
                }
                else
                {
                    throw AssemblingError(m_current - m_begin, m_currentLineNumber, "Expected destination register");
                }

                break;
            }
            case Instruction::MemAdd:
            {
                assembleSingleRegisterSpecials(0x1e);
                break;
            }
            case Instruction::SetAudioTimer:
            {
                assembleSingleRegisterSpecials(0x18);
                break;
            }
            case Instruction::GetTimer:
            {
                assembleSingleRegisterSpecials(0x07);
                break;
            }
            case Instruction::SetTimer:
            {
                assembleSingleRegisterSpecials(0x15);
                break;
            }
            case Instruction::Halt:
            {
                m_bytes.push_back(0x00);
                m_bytes.push_back(0xe1);
                expectLineEnd();
                break;
            }
            default:
                throw AssemblingError(m_current - m_begin, m_currentLineNumber, "Unknown instruction");
            }
        }

        for (std::pair<const std::string, std::vector<std::size_t>> const &sub : m_labelReplacementPositions)
        {
            if (m_labelPositions.count(sub.first) == 0)
            {
                throw AssemblingError(m_current - m_begin, m_currentLineNumber, "Unknown label used");
            }
            for (size_t pos : sub.second)
            {
                m_bytes[pos] |= (m_labelPositions[sub.first] & 0x0f00) >> 8;
                m_bytes[pos + 1] = m_labelPositions[sub.first] & 0x00ff;
            }
        }
    }

    void addLabelReplacementPosition(std::string const &label, size_t pos)
    {
        if (m_labelReplacementPositions.count(label) > 0)
        {
            m_labelReplacementPositions[label].push_back(pos);
        }
        else
        {
            m_labelReplacementPositions[label] = {pos};
        }
    }

private:
    /**
     * @brief Covers several opcodes that take register as input and only differ by the second byte
     *
     * @param dataByte
     */
    void assembleSingleRegisterSpecials(uint8_t dataByte)
    {
        skipWhitespace();
        if (std::optional<size_t> reg = parseRegister(); reg.has_value())
        {
            m_bytes.push_back(0xf0 | reg.value());
            m_bytes.push_back(dataByte);
            expectLineEnd();
            return;
        }
        throw AssemblingError(m_current - m_begin, m_currentLineNumber, "Expected register");
    }
    void assembleCheckKeyPress(uint8_t dataByte)
    {
        skipWhitespace();
        if (std::optional<size_t> reg = parseRegister(); reg.has_value())
        {
            m_bytes.push_back(0xe0 | reg.value());
            m_bytes.push_back(dataByte);
            expectLineEnd();
            return;
        }
        throw AssemblingError(m_current - m_begin, m_currentLineNumber, "Expected register");
    }
    void assembleEqualsOperation(uint8_t constOperationBit, uint8_t registerOperationBit)
    {
        skipWhitespace();
        std::optional<size_t> registerId = parseRegister();
        if (!registerId.has_value())
        {
            throw AssemblingError(m_current - m_begin, m_currentLineNumber, "Expected register");
        }
        skipWhitespace();
        consumeComma();
        skipWhitespace();
        if (std::optional<size_t> register2Id = parseRegister(); register2Id.has_value())
        {
            m_bytes.push_back((registerOperationBit << 4) | registerId.value());
            m_bytes.push_back((register2Id.value() << 4));
        }
        else if (std::optional<uint8_t> val = parseNumber<uint8_t>(); val.has_value())
        {
            m_bytes.push_back((constOperationBit << 4) | registerId.value());
            m_bytes.push_back(val.value());
        }
        else
        {
            throw AssemblingError(m_current - m_begin, m_currentLineNumber, "Expected register or number");
        }
        expectLineEnd();
    }
    void assembleAddOperation()
    {
        skipWhitespace();
        std::optional<size_t> registerA = parseRegister();
        if (!registerA.has_value())
        {
            throw AssemblingError(m_current - m_begin, m_currentLineNumber, "Expected register");
        }
        skipWhitespace();
        consumeComma();
        skipWhitespace();
        if (std::optional<size_t> registerB = parseRegister(); registerB.has_value())
        {
            m_bytes.push_back(0x80 | registerA.value());
            m_bytes.push_back((registerB.value() << 4) | 4);
        }
        else if (std::optional<uint8_t> val = parseNumber<uint8_t>(); val.has_value())
        {
            m_bytes.push_back(0x70 | registerA.value());
            m_bytes.push_back(val.value());
        }
        else
        {
            throw AssemblingError(m_current - m_begin, m_currentLineNumber, "Expected register or number");
        }
        expectLineEnd();
    }
    void assembleMathOperations(uint8_t operationTypeBit)
    {
        skipWhitespace();
        std::optional<size_t> registerA = parseRegister();
        if (!registerA.has_value())
        {
            throw AssemblingError(m_current - m_begin, m_currentLineNumber, "Expected register");
        }
        skipWhitespace();
        consumeComma();
        skipWhitespace();
        if (std::optional<size_t> registerB = parseRegister(); registerB.has_value())
        {
            m_bytes.push_back(0x80 | registerA.value());
            m_bytes.push_back((registerB.value() << 4) | operationTypeBit);
        }
        else
        {
            throw AssemblingError(m_current - m_begin, m_currentLineNumber, "Expected register");
        }
        expectLineEnd();
    }
    void assembleDraw()
    {
        skipWhitespace();
        std::optional<size_t> regX = parseRegister();
        if (!regX.has_value())
        {
            throw AssemblingError(m_current - m_begin, m_currentLineNumber, "Expected register for x");
        }
        skipWhitespace();
        consumeComma();
        std::optional<size_t> regY = parseRegister();
        if (!regY.has_value())
        {
            throw AssemblingError(m_current - m_begin, m_currentLineNumber, "Expected register for y");
        }
        skipWhitespace();
        consumeComma();
        if (std::optional<uint8_t> height = parseNumber<uint8_t>(); height.has_value())
        {
            if (height > 16)
            {
                throw AssemblingError(m_current - m_begin, m_currentLineNumber, "Height of sprite for draw can not be larger than 16");
            }
            else if (height == 0)
            {
                throw AssemblingError(m_current - m_begin, m_currentLineNumber, "Height of sprite can not be 0");
            }
            m_bytes.push_back(0xd0 | regX.value());
            m_bytes.push_back(((regY.value() & 0xf) << 4) | (height.value() - 1));
            expectLineEnd();
        }
        else
        {
            throw AssemblingError(m_current - m_begin, m_currentLineNumber, "Expected value for height");
        }
    }
    void assembleDataOperations()
    {
        size_t total = 1;
        if (std::optional<size_t> repeat = parseTimes(); repeat.has_value())
        {
            total = repeat.value();
        }
        skipWhitespace();
        std::optional<DataSize> dataSize = parseDataStore();
        if (!dataSize.has_value())
        {
            throw AssemblingError(m_current - m_begin, m_currentLineNumber, "Expected data store operation");
        }
        skipWhitespace();
        std::vector<uint8_t> bytes;
        switch (dataSize.value())
        {
        case DataSize::Byte:
        {
            while (true)
            {
                if (std::optional<uint8_t> num = parseNumber<uint8_t>(); num.has_value())
                {
                    bytes.push_back(num.value());
                }
                else
                {
                    throw AssemblingError(m_current - m_begin, m_currentLineNumber, "Expected a number");
                }
                skipWhitespace();
                if (m_current == m_end || *m_current != ',')
                {
                    break;
                }
                consumeComma();
            }
            break;
        }
        case DataSize::Word:
        {
            while (true)
            {
                if (std::optional<uint16_t> num = parseNumber<uint16_t>(); num.has_value())
                {
                    bytes.push_back((num.value() & 0xff00) >> 8);
                    bytes.push_back(num.value() & 0x00ff);
                }
                else
                {
                    throw AssemblingError(m_current - m_begin, m_currentLineNumber, "Expected a number");
                }
                skipWhitespace();
                if (m_current == m_end || *m_current != ',')
                {
                    break;
                }
                consumeComma();
            }
            break;
        }
        }
        for (size_t i = 0; i < total; i++)
        {
            m_bytes.insert(m_bytes.end(), bytes.begin(), bytes.end());
        }

        expectLineEnd();
    }

    void assembleAddressInstruction(uint8_t firstByte)
    {
        skipWhitespace();
        if (std::optional<uint16_t> address = parseNumber<uint16_t>(); address.has_value())
        {
            m_bytes.push_back((firstByte << 4) | ((address.value() & 0x0f00) >> 8));
            m_bytes.push_back(address.value() & 0x00ff);
        }
        else if (std::optional<std::string> label = parseLabelUsage(); label.has_value())
        {
            if (m_labelPositions.count(label.value()) > 0)
            {
                m_bytes.push_back((firstByte << 4) | ((m_labelPositions[label.value()] & 0x0f00) >> 8));
                m_bytes.push_back(m_labelPositions[label.value()] & 0x00ff);
            }
            else
            {
                addLabelReplacementPosition(label.value(), m_bytes.size());
                m_bytes.push_back((firstByte << 4));
                m_bytes.push_back(0x00);
            }
        }
        else
        {
            throw AssemblingError(m_current - m_begin, m_currentLineNumber, "Excepted address or label");
        }
        expectLineEnd();
    }

    void assembleMoveOperation()
    {
        skipWhitespace();
        std::optional<size_t> registerId = parseRegister();
        if (!registerId.has_value())
        {
            throw AssemblingError(m_current - m_begin, m_currentLineNumber, "Excepted register name");
        }
        consumeComma();

        if (std::optional<size_t> register2Id = parseRegister(); register2Id.has_value())
        {
            m_bytes.push_back((0x8 << 4) | registerId.value());
            m_bytes.push_back(register2Id.value() << 4);
        }
        else if (std::optional<uint8_t> constVal = parseNumber<uint8_t>(); constVal.has_value())
        {
            m_bytes.push_back((0x6 << 4) | registerId.value());
            m_bytes.push_back(constVal.value());
        }
        else
        {
            throw AssemblingError(m_current - m_begin, m_currentLineNumber, "Excepted register or number");
        }
        expectLineEnd();
    }

    std::optional<uint32_t> parseTimes()
    {
        if (!tryText("times"))
        {
            return {};
        }
        m_current += std::string("times").size();
        skipWhitespace();
        if (std::optional<uint32_t> times = parseNumber<uint32_t>(); times.has_value())
        {
            return times;
        }
        throw AssemblingError(m_current - m_begin, m_currentLineNumber, "Excepted the times value");
    }

    bool tryDataOperation()
    {
        for (std::string const &keyword : AssembleDataOperationKeywords)
        {
            if (tryText(keyword))
            {
                return true;
            }
        }
        return false;
    }

    bool tryText(std::string const &text)
    {
        for (size_t i = 0; i < text.size(); i++)
        {
            if ((m_current + i) == m_end || *(m_current + i) != text[i])
            {
                return false;
            }
        }

        // make sure the word is over
        if (m_current + text.size() != m_end && !(*(m_current + text.size()) == ' ' || *(m_current + text.size()) == ','))
        {
            return false;
        }
        return true;
    }

    std::optional<Instruction> parseInstruction()
    {
        for (std::map<std::string, Instruction>::const_iterator inIt = Instructions.begin(); inIt != Instructions.end(); inIt++)
        {
            if (tryText(inIt->first))
            {
                m_current += inIt->first.size();
                return inIt->second;
            }
        }
        return {};
    }
    std::optional<DataSize> parseDataStore()
    {
        for (auto const &store : DataStoreSizeKeywords)
        {
            if (tryText(store.first))
            {
                m_current += store.first.size();
                return store.second;
            }
        }
        return {};
    }

    std::optional<std::string> parseLabel()
    {
        std::string res = "";
        for (StringConstIterator it = m_current; it != m_end; it++)
        {
            if (*it == ':')
            {
                m_current += res.size() + 1;
                return res;
            }
            if ((it == m_end && !std::isalpha(*it)) || (it != m_end && !std::isalnum(*it)))
            {
                return {};
            }
            res += *it;
        }
        return {};
    }

    std::optional<size_t> parseRegister()
    {
        if (*m_current != 'v')
        {
            return {};
        }
        char c = *(m_current + 1);
        size_t it = hexNumbers.find(*(m_current + 1));
        if (it == std::string::npos)
        {
            return {};
        }
        if (m_current + 2 == m_end || *(m_current + 2) == ' ' || *(m_current + 2) == ',')
        {
            m_current += 2;
            return it;
        }
        return {};
    }

    template <typename IntegerType>
    std::optional<IntegerType> parseHex()
    {
        if (m_current + 2 == m_end || !(*m_current == '0' && *(m_current + 1) == 'x'))
        {
            return {};
        }
        size_t offset = 2;
        if (!isValidHexDigit(*(m_current + offset)))
        {
            return {};
        }
        std::string res = "";
        for (; m_current + offset != m_end && isValidHexDigit(*(m_current + offset)); offset++)
        {
            res += *(m_current + offset);
        }
        IntegerType resNum;
        try
        {
            resNum = std::stoi(res, nullptr, 16);
        }
        catch (std::invalid_argument const &e)
        {
            return {};
        }
        catch (std::out_of_range const &e)
        {
            throw AssemblingError(m_current - m_begin, m_currentLineNumber,
                                  "Constant number is too large, valid range is " +
                                      std::to_string(std::numeric_limits<IntegerType>::min()) +
                                      "< x < " +
                                      std::to_string(std::numeric_limits<IntegerType>::max()));
        }
        m_current += offset;
        return resNum;
    }

    template <typename IntegerType>
    std::optional<IntegerType> parseBinary()
    {
        if (m_current + 2 == m_end || !(*m_current == '0' && *(m_current + 1) == 'b'))
        {
            return {};
        }
        size_t offset = 2;
        if (!(*(m_current + offset) == '0' || *(m_current + offset) == '1'))
        {
            return {};
        }
        std::string res = "";
        for (; m_current + offset != m_end && (*(m_current + offset) == '0' || *(m_current + offset) == '1'); offset++)
        {
            res += *(m_current + offset);
        }
        IntegerType resNum;
        try
        {
            resNum = std::stoi(res, nullptr, 2);
        }
        catch (std::invalid_argument const &e)
        {
            return {};
        }
        catch (std::out_of_range const &e)
        {
            throw AssemblingError(m_current - m_begin, m_currentLineNumber,
                                  "Constant number is too large, valid range is " +
                                      std::to_string(std::numeric_limits<IntegerType>::min()) +
                                      "< x < " +
                                      std::to_string(std::numeric_limits<IntegerType>::max()));
        }
        m_current += offset;
        return resNum;
    }

    template <typename IntegerType>
    std::optional<IntegerType> parseDecimal()
    {
        if (!std::isdigit(*m_current))
        {
            return {};
        }
        std::string res = "";
        size_t offset = 0;
        for (; (m_current + offset) != m_end && std::isdigit(*(m_current + offset)); offset++)
        {
            res += *(m_current + offset);
        }
        IntegerType resNum;
        try
        {
            resNum = std::stoi(res);
        }
        catch (std::invalid_argument const &e)
        {
            return {};
        }
        catch (std::out_of_range const &e)
        {
            throw AssemblingError(m_current - m_begin, m_currentLineNumber,
                                  "Constant number is too large, valid range is " +
                                      std::to_string(std::numeric_limits<IntegerType>::min()) +
                                      "< x < " +
                                      std::to_string(std::numeric_limits<IntegerType>::max()));
        }
        m_current += offset;
        return resNum;
    }

    /**
     * @brief Attempt to parse an integer number either in hex or dec representation.
     *
     * @param begin Iterator pointing to the place to start parsing from
     * @param end Iterator pointing to the end of the string
     * @return std::optional<uint32_t> Parsed value or empty value
     */
    template <typename IntegerType>
    std::optional<uint32_t> parseNumber()
    {

        if (std::optional<uint32_t> num = parseHex<IntegerType>(); num.has_value())
        {
            return num;
        }
        if (std::optional<uint32_t> num = parseBinary<IntegerType>(); num.has_value())
        {
            return num;
        }
        return parseDecimal<IntegerType>();
    }

    /**
     * @brief Try to parse a string that references a label
     *
     * @param begin
     * @param end
     * @return std::string
     */
    std::optional<std::string> parseLabelUsage()
    {
        if (!std::isalpha(*m_current))
        {
            return {};
        }
        size_t offset = 0;
        std::string res;
        for (; (m_current + offset) != m_end && std::isalnum(*(m_current + offset)); offset++)
        {
            res += *(m_current + offset);
        }
        m_current += offset;
        return res;
    }

    void skipWhitespace()
    {
        for (; m_current != m_end && *m_current == ' '; m_current++)
            ;
    }

    /**
     * @brief Try to grab a comma and throw an error if there is no comma
     *
     * @param begin Position to start from
     * @param end End of the line
     * @param lineBegin Start of the line(for displaying errors)
     * @param lineNumber Number of the line(for displaying errors)
     */
    void consumeComma()
    {
        skipWhitespace();
        if (*m_current != ',')
        {
            throw AssemblingError(m_current - m_begin, m_currentLineNumber, "Excepted comma");
        }
        m_current++;
        skipWhitespace();
    }

    /**
     * @brief Check if line ends after the given iterator
     *
     * @param begin Position to start from
     * @param end End of the line
     * @param lineBegin Start of the line(for displaying errors)
     * @param lineNumber Number of the line(for displaying errors)
     */
    void expectLineEnd()
    {
        skipWhitespace();
        if (m_current != m_end)
        {
            throw AssemblingError(m_current - m_begin, m_currentLineNumber, "Unexpected symbol");
        }
    }

private:
    std::vector<std::string> m_code;
    StringConstIterator m_end;
    StringConstIterator m_current;
    StringConstIterator m_begin;
    size_t m_currentLineNumber;
    std::vector<uint8_t> m_bytes;
    std::map<std::string, std::vector<size_t>> m_labelReplacementPositions;
    std::map<std::string, uint16_t> m_defines;
    std::map<std::string, size_t> m_labelPositions;
};

std::vector<std::string> getCodeLines(std::string const &code)
{
    std::stringstream stringStream(code);
    std::vector<std::string> result;
    std::string line;
    while (std::getline(stringStream, line, '\n'))
    {
        result.push_back(line);
    }
    return result;
}

std::vector<std::string> prepareCode(std::string const &code)
{
    std::regex commentRegex("(;(\\S|\\s)+)");
    std::regex equRegex("^([a-zA-z]\\w*) +equ +(\\w+)$");
    std::vector<std::string> result;
    std::stringstream stringStream(code);
    std::string line;
    std::map<std::string, std::string> substitutions;

    while (std::getline(stringStream, line, '\n'))
    {
        std::string current = std::regex_replace(line, commentRegex, "");
        std::smatch matches;
        if (std::regex_search(current, matches, equRegex))
        {
            substitutions[matches[1].str()] = matches[2].str();
            result.push_back("");
        }
        else
        {
            result.push_back(current);
        }
    }
    for (std::pair<const std::string, std::string> const &sub : substitutions)
    {
        std::regex subRegex(std::string("\\b(") + sub.first + ")\\b(?!:)");
        for (auto &line : result)
        {
            std::string tmp = std::regex_replace(line, subRegex, sub.second);
            line = tmp;
        }
    }
    return result;
}

int main(int argc, char **argv)
{
    std::string outputFilename = "./game.bin";
    std::string inputFilename = "./game.asm";
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
        if (arg == "-o" || arg == "--output")
        {
            if (i + 1 > argc)
            {
                std::cerr << "Missing filename for output flag" << std::endl;
                return EXIT_FAILURE;
            }
            outputFilename = std::string(argv[i + 1]);
        }
    }

    std::ifstream inputFile(inputFilename);
    if (!inputFile.is_open())
    {
        std::cerr << "Unabled to open input file" << std::endl;
        return EXIT_FAILURE;
    }
    std::stringstream codeFile;
    codeFile << inputFile.rdbuf();
    std::string code = codeFile.str(); //" mov v0, 6\n mov v1, 6 \n mem sprite\nloop: clear \n add v0, 1 \n draw v0, v1, 4\nrender\njmp loop\nhlt\n sprite: db 0b10000000, 0b01000010, 0b00100100, 0b00011000";
    std::vector<std::string> linesCleaned = prepareCode(code);
    Assembler assembler(linesCleaned);
    try
    {
        assembler.parse();
    }
    catch (AssemblingError e)
    {
        std::vector<std::string> lines = getCodeLines(code);
        std::cout << e.what() << std::endl;
        if (e.getRow() > 0)
        {
            std::cout << e.getRow() << ":  " << lines[e.getRow() - 1] << std::endl;
        }
        std::cout << e.getRow() + 1 << ":  " << lines[e.getRow()] << "\033[31m <-- error here\033[0m" << std::endl;
        if (e.getRow() + 1 < lines.size())
        {
            std::cout << e.getRow() + 2 << ":  " << lines[e.getRow() + 1] << std::endl;
        }
    }
    if (assembler.getBytes().size() > 4096)
    {
        std::cout << "\033[33mWarning! The final file exceeds the available memory in the interpreter. Final file is " << assembler.getBytes().size() << " bytes long with max being 4096 bytes\033[0m" << std::endl;
    }
    std::ofstream outfile(outputFilename, std::ios::out | std::ios::binary);
    outfile.write((const char *)assembler.getBytes().data(), assembler.getBytes().size());

    return EXIT_SUCCESS;
}