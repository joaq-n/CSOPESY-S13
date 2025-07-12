#ifndef CONFIG_H
#define CONFIG_H

#include <string>

// Default configuration for the scheduler
// This can be loaded from config.txt
struct Config {
    int num_cpu = 2;
    std::string scheduler = "rr";
    int quantum_cycles = 4;
    int batch_process_freq = 1;
    int min_ins = 100;
    int max_ins = 100;
    int delays_per_exec = 0;
    
    // Memory management parameters
    size_t max_overall_mem = 16384;
    size_t mem_per_frame = 16;
    size_t mem_per_proc = 4096;
};

#endif