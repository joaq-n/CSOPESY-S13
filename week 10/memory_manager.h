#ifndef MEMORY_MANAGER_H
#define MEMORY_MANAGER_H

#include <vector>
#include <memory>
#include <string>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include "process.h"

struct MemoryBlock {
    size_t start_address;
    size_t size;
    bool is_free;
    Process* process;
    
    MemoryBlock(size_t start, size_t sz, bool free = true, Process* proc = nullptr) 
        : start_address(start), size(sz), is_free(free), process(proc) {}
};

class MemoryManager {
private:
    size_t total_memory;
    size_t memory_per_frame;
    size_t memory_per_process;
    std::vector<MemoryBlock> memory_blocks;
    int quantum_cycle_counter;
    
public:
    MemoryManager(size_t total_mem, size_t mem_per_frame, size_t mem_per_proc) 
        : total_memory(total_mem), memory_per_frame(mem_per_frame), 
          memory_per_process(mem_per_proc), quantum_cycle_counter(0) {
        
        // Initialize with one large free block
        memory_blocks.emplace_back(0, total_memory, true, nullptr);
    }
    
    // First-fit allocation
    bool allocateMemory(Process* process) {
        for (auto& block : memory_blocks) {
            if (block.is_free && block.size >= memory_per_process) {
                // Found a suitable block
                if (block.size > memory_per_process) {
                    // Split the block
                    size_t remaining_size = block.size - memory_per_process;
                    memory_blocks.emplace_back(
                        block.start_address + memory_per_process,
                        remaining_size,
                        true,
                        nullptr
                    );
                }
                
                // Allocate the block
                block.size = memory_per_process;
                block.is_free = false;
                block.process = process;
                
                return true;
            }
        }
        return false; // No suitable block found
    }
    
    // Deallocate memory when process finishes
    void deallocateMemory(Process* process) {
        for (auto& block : memory_blocks) {
            if (!block.is_free && block.process == process) {
                block.is_free = true;
                block.process = nullptr;
                break;
            }
        }
        
        // Merge adjacent free blocks
        mergeAdjacentBlocks();
    }
    
    // Check if memory is available
    bool hasAvailableMemory() const {
        for (const auto& block : memory_blocks) {
            if (block.is_free && block.size >= memory_per_process) {
                return true;
            }
        }
        return false;
    }
    
    // Get number of processes in memory
    int getProcessesInMemory() const {
        int count = 0;
        for (const auto& block : memory_blocks) {
            if (!block.is_free) {
                count++;
            }
        }
        return count;
    }
    
    // Get total external fragmentation - FIXED VERSION
    size_t getTotalExternalFragmentation() const {
        size_t total_fragmentation = 0;
        int free_block_count = 0;
        
        for (const auto& block : memory_blocks) {
            if (block.is_free) {
                free_block_count++;
                // Only count blocks that are smaller than what's needed for a process
                if (block.size < memory_per_process) {
                    total_fragmentation += block.size;
                }
            }
        }
        
        // Alternative calculation: sum of all free blocks except the largest one
        // This gives a better representation of unusable fragmented memory
        if (free_block_count > 1) {
            std::vector<size_t> free_sizes;
            for (const auto& block : memory_blocks) {
                if (block.is_free) {
                    free_sizes.push_back(block.size);
                }
            }
            
            if (!free_sizes.empty()) {
                // Sort to find the largest free block
                std::sort(free_sizes.begin(), free_sizes.end(), std::greater<size_t>());
                
                // Sum all free blocks except the largest one
                size_t fragmentation_alt = 0;
                for (size_t i = 1; i < free_sizes.size(); i++) {
                    fragmentation_alt += free_sizes[i];
                }
                
                // Return the maximum of both calculations
                return std::max(total_fragmentation, fragmentation_alt);
            }
        }
        
        return total_fragmentation;
    }

    // Generate memory snapshot file
    void generateMemorySnapshot() {
        quantum_cycle_counter++;
        
        // Get current timestamp
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        std::tm* tm_ptr = std::localtime(&time_t);
        
        std::ostringstream timestamp;
        timestamp << std::put_time(tm_ptr, "%m/%d/%Y %H:%M:%S");
        
        // Create filename
        std::string filename = "memory_stamp_" + std::to_string(quantum_cycle_counter) + ".txt";
        
        std::ofstream file(filename);
        if (!file.is_open()) {
            return;
        }
        
        // Write header information
        file << "Timestamp: (" << timestamp.str() << ")\n";
        file << "Number of processes in memory: " << getProcessesInMemory() << "\n";
        file << "Total external fragmentation in KB: " << getTotalExternalFragmentation() << "\n";
        file << "\n";
        
        // Write memory layout
        file << "----end---- = " << total_memory << "\n";
        
        // Sort blocks by start address for proper display
        std::vector<MemoryBlock> sorted_blocks = memory_blocks;
        std::sort(sorted_blocks.begin(), sorted_blocks.end(), 
                  [](const MemoryBlock& a, const MemoryBlock& b) {
                      return a.start_address < b.start_address;
                  });
        
        // Write memory blocks from top to bottom
        for (auto it = sorted_blocks.rbegin(); it != sorted_blocks.rend(); ++it) {
            const auto& block = *it;
            
            if (block.is_free) {
                file << block.start_address << "\n";
            } else {
                file << block.process->name << "\n";
                file << block.start_address + block.size << "\n";
                file << "\n";
                file << block.start_address << "\n";
            }
        }
        
        file << "----start---- = 0\n";
        file.close();
    }
    
private:
    void mergeAdjacentBlocks() {
        // Sort blocks by start address
        std::sort(memory_blocks.begin(), memory_blocks.end(), 
                  [](const MemoryBlock& a, const MemoryBlock& b) {
                      return a.start_address < b.start_address;
                  });
        
        // Merge adjacent free blocks
        for (size_t i = 0; i < memory_blocks.size() - 1; ) {
            if (memory_blocks[i].is_free && memory_blocks[i + 1].is_free &&
                memory_blocks[i].start_address + memory_blocks[i].size == 
                memory_blocks[i + 1].start_address) {
                
                // Merge the blocks
                memory_blocks[i].size += memory_blocks[i + 1].size;
                memory_blocks.erase(memory_blocks.begin() + i + 1);
            } else {
                i++;
            }
        }
    }
};

#endif