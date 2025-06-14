#ifndef PROCESS_LIST_H
#define PROCESS_LIST_H

#include <vector>
#include "process.h"
#include "instruction_tracker.h"

class ProcessList {
public:
    std::vector<Process*> processes;

    void addProcess(Process* p) {
        processes.push_back(p);
    }

    void printRunningProcesses() const {
        for (const auto& p : processes) {
            if (!p->isFinished()) {
                InstructionTracker::printExecutionStatus(*p);
            }
        }
    }

    void printFinishedProcesses() const {
        for (const auto& p : processes) {
            if (p->isFinished()) {
                InstructionTracker::printExecutionStatus(*p);
            }
        }
    }
};

#endif
