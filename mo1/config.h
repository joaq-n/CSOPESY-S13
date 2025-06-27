#ifndef CONFIG_H
#define CONFIG_H

#include <string>

// Default configuration for the scheduler
// This can be loaded from config.txt
struct Config {
    int num_cpu = 4;
    std::string scheduler = "fcfs";
    int quantum_cycles = 5;
    int batch_process_freq = 10;
    int min_ins = 100;
    int max_ins = 100;
    int delays_per_exec = 0;
};

#endif