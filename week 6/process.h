// process.h
#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <ctime>
#include <iostream>
#include <iomanip>
#include <sstream> // Include for std::ostringstream

class Process {
public:
    int pid;                        // Unique process ID
    std::string name;              // Human-readable process name
    int totalInstructions;         // Total number of instructions (or lines)
    int executedInstructions;      // Instructions completed so far (current line)
    int coreAssigned;              // Assigned CPU core
    std::time_t creationAt;      // Timestamp of creation
    int priority;
    int burstTime;

    Process()
        : pid(-1), name(""), totalInstructions(0), executedInstructions(0),
          coreAssigned(-1), creationAt(std::time(nullptr)) {}

    Process(int id, const std::string& procName, int instrCount)
        : pid(id), name(procName), totalInstructions(instrCount), executedInstructions(0), coreAssigned(-1) {
        creationAt = std::time(nullptr);
    }

    bool isFinished() const {
        return executedInstructions >= totalInstructions;
    }

    int getProgressPercent() const {
        return static_cast<int>((executedInstructions * 100.0) / totalInstructions);
    }

    void printCurrentLine() const {
        std::cout << name << " is on line " << executedInstructions
                  << "/" << totalInstructions << "\n";
    }
};

class ProcessManager {
public:
    // *** CRITICAL CHANGE: Store Process pointers instead of Process objects ***
    std::vector<Process*> processList;
    std::unordered_map<std::string, Process*> processMap;

    // *** CRITICAL CHANGE: 'add' method takes a Process* ***
    void add(Process* p) {
        processList.push_back(p);
        processMap[p->name] = p; // Use -> for pointer access
    }

    bool exists(const std::string& name) const {
        return processMap.find(name) != processMap.end();
    }

    // *** CRITICAL CHANGE: operator[] returns a Process* ***
    Process* operator[](const std::string& name) {
        return processMap[name];
    }

    // *** CRITICAL CHANGE: begin() and end() for range-based for loops ***
    // These now return iterators for std::vector<Process*>
    std::vector<Process*>::iterator begin() { return processList.begin(); }
    std::vector<Process*>::iterator end() { return processList.end(); }
    // Add const versions for const ProcessManager objects
    std::vector<Process*>::const_iterator begin() const { return processList.begin(); }
    std::vector<Process*>::const_iterator end() const { return processList.end(); }

    // Add a destructor to properly clean up dynamically allocated Process objects
    ~ProcessManager() {
        for (Process* p : processList) {
            delete p; // Deallocate each Process object
        }
        processList.clear(); // Clear the vector of pointers
        processMap.clear();  // Clear the map
    }
};

// Global instance of ProcessManager
inline ProcessManager sessions;