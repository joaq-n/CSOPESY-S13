// schedulersimulator.h
#ifndef SCHEDULERSIMULATOR_H
#define SCHEDULERSIMULATOR_H

#include <string>
#include <vector>
#include <thread>  // Needed for std::thread
#include <mutex>   // Needed for std::mutex
#include <atomic>  // Needed for std::atomic

#include "process.h"
#include "process_list.h"

class SchedulerSimulator {
public:
    void runTest();

    void startSimulation(); // Starts the simulation
    void stopSimulation();  // Stops the simulation
private:
    // Existing Config fields
    int numCPU = 1;
    std::string schedulerType = "fcfs";
    int quantumCycles = 1;
    int batchFreq = 2;
    int minInstructions = 100;
    int maxInstructions = 100;
    int delayPerExec = 0;

    std::vector<std::thread> workerThreads;     // To hold our CPU core threads
    std::mutex queueMutex;                      // Mutex to protect access to the readyQueue
    std::mutex coutMutex;                       // Mutex to protect std::cout for cleaner output
    std::atomic<bool> simulationRunning;        // Flag to signal threads to stop (thread-safe)
    std::thread simulationControlThread;

    void coreWorker(int coreId, ProcessList* readyQueue);

    void runSimulationInBackground();

    void loadConfig(const std::string& filename);
    void runFCFS();        // First-Come, First-Served scheduler logic
    void runRoundRobin();  // Round Robin scheduler logic
};

#endif