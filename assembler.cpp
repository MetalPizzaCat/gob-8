#include <iostream>
#include <map>
#include <optional>
#include <vector>
#include <sstream>
#include <cstdint>
#include <limits>
#include <regex>
#include <exception>

enum class Instruction
{
    None,
    Move,
    Clear,
    Draw,
    Jump,
    Call,
    Return,
    Add
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

static const std::map<std::string, Instruction> Instructions = {
    {"nop", Instruction::None},
    {"mov", Instruction::Move},
    {"jmp", Instruction::Jump},
    {"goto", Instruction::Jump},
    {"clear", Instruction::None}};

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
            std::optional<Instruction> instruction = parseInstruction();
            if (!instruction.has_value())
            {
                throw AssemblingError(m_current - m_begin, m_currentLineNumber, "Excepted and instruction");
            }
            switch (instruction.value())
            {
            case Instruction::None:
                break;
            case Instruction::Move:
            {
                assembleMoveOperation();
                break;
            }
            case Instruction::Jump:
            {
                assembleJump();
            }
            break;
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
    void assembleJump()
    {
        skipWhitespace();
        if (std::optional<uint32_t> address = parseNumber(); address.has_value())
        {
            m_bytes.push_back((0x1 << 4) | ((address.value() & 0x0f00) >> 8));
            m_bytes.push_back(address.value() & 0x00ff);
        }
        else if (std::optional<std::string> label = parseLabelUsage(); label.has_value())
        {
            if (m_labelPositions.count(label.value()) > 0)
            {
                m_bytes.push_back((0x1 << 4) | ((m_labelPositions[label.value()] & 0x0f00) >> 8));
                m_bytes.push_back(m_labelPositions[label.value()] & 0x00ff);
            }
            else
            {
                addLabelReplacementPosition(label.value(), m_bytes.size());
                m_bytes.push_back(0x10);
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
        else if (std::optional<uint32_t> constVal = parseNumber(); constVal.has_value())
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

    std::optional<uint32_t> parseHex()
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
        uint32_t resNum;
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
                                      std::to_string(std::numeric_limits<uint16_t>::min()) +
                                      "< x < " +
                                      std::to_string(std::numeric_limits<uint16_t>::max()));
        }
        m_current += offset;
        return resNum;
    }

    std::optional<uint32_t> parseDecimal()
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
        uint32_t resNum;
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
                                      std::to_string(std::numeric_limits<uint16_t>::min()) +
                                      "< x < " +
                                      std::to_string(std::numeric_limits<uint16_t>::max()));
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
    std::optional<uint32_t> parseNumber()
    {
        std::optional<uint32_t> num = parseHex();

        if (num.has_value())
        {
            return num;
        }
        return parseDecimal();
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
    std::vector<uint8_t> const &getBytes() { return m_bytes; }

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
    std::string code = "jmp start \nmov a, 23; comment\n; fuck\n mov v1, 0x69 \nstart4: \na equ v0";
    std::vector<std::string> lines = prepareCode(code);
    Assembler assembler(lines);
    assembler.parse();

    return EXIT_SUCCESS;
}