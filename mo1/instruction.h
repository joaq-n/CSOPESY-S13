#ifndef INSTRUCTION_H
#define INSTRUCTION_H

#include <vector>
#include <string>

enum class InstructionType {
    PRINT,
    DECLARE,
    ADD,
    SUBTRACT,
    SLEEP,
    FOR_START,
    FOR_END
};

struct Instruction {
    InstructionType type;
    std::vector<std::string> args;
    int for_repeats = 0;
    std::vector<Instruction> for_instructions;
};

#endif