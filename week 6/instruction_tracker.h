#ifndef INSTRUCTION_TRACKER_H
#define INSTRUCTION_TRACKER_H

#include <iostream>
#include "process.h"

class InstructionTracker {
public:
    static void printExecutionStatus(const Process& p) {
        std::cout << p.name << " (" << formatTime(p.creationAt) << ") "
                  << "Core:" << p.coreAssigned << " "
                  << p.executedInstructions << "/" << p.totalInstructions << std::endl;
    }

private:
    static std::string formatTime(std::time_t time) {
        char buffer[80];
        std::tm* timeinfo = std::localtime(&time);
        std::strftime(buffer, sizeof(buffer), "%m/%d/%y %I:%M:%S %p", timeinfo);
        return std::string(buffer);
    }
};

#endif
