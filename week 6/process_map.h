#ifndef PROCESS_MAP_H
#define PROCESS_MAP_H

#include <unordered_map>
#include "process.h"

class ProcessMap {
public:
    std::unordered_map<int, Process*> processMap;

    void addProcess(Process* p) {
        processMap[p->pid] = p;
    }

    Process* getProcessById(int pid) {
        auto it = processMap.find(pid);
        if (it != processMap.end()) {
            return it->second;
        }
        return nullptr;
    }
};

#endif
