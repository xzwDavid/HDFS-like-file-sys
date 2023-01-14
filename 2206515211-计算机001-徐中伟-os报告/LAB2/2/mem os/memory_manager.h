//default args
#ifndef _MEMORY_MANAGER_H_
#define _MEMORY_MANAGER_H_
#define MAX_PROCESS_NAME_LEN 32
#define MIN_SLICE 10
#define MAX_PROCESS_ID 1024
#define DEFAULT_MEM_SIZE 1024
#define DEFAULT_MEM_START 0

//the choice of the algorithm
#define MA_FF 1
#define MA_BF 2
#define MA_WF 3
#endif

#include <map>
#include <cstdlib>
#include <iostream>

using namespace std;

class free_block_type {
public:
    int size;
    int start_addr;
    free_block_type* next;



    static free_block_type* init_freeBlock(int mem_size, int start_addr = DEFAULT_MEM_START) {
        free_block_type* fb;
        fb = new free_block_type;
        fb->size = mem_size;
        fb->start_addr = start_addr;
        fb->next = nullptr;
        return fb;
    }

    //init the block_chain
    static free_block_type* init_freeBlock(int mem_size, free_block_type* head) {

        free_block_type *curr, *next;
        curr = head;
        while (curr) {
            next = curr->next;
            curr = next;
        }
        return free_block_type::init_freeBlock(mem_size);
    }

    // compare free blocks according to order
    bool compare(const int order, free_block_type *fb) {
        switch (order) {
        case 1: return this->start_addr < fb->start_addr;
        case 2: return this->size < fb->size;
        case 3: return this->size > fb->size;
        default: break;
        }
        return false;
    }

    void swap(free_block_type *fb) {
        int temp;
        temp = this->size;
        this->size = fb->size;
        fb->size = temp;

        temp = this->start_addr;
        this->start_addr = fb->start_addr;
        fb->start_addr = temp;
    }
};

class AllocatedBlock {
public:
    // corresponding process id
    int pid;
    // process memory size
    int size;
    // extra size from MIN SLICE
    int extraSize;
    // memory start address
    int start_addr;
    // process name(shorter than 32)
    char process_name[MAX_PROCESS_NAME_LEN];
    AllocatedBlock* next;
};


class MemoryManager {
private:
    // counter of process
    int cp;
    // map of processes
    map<int, AllocatedBlock*> pMap;

    // total memory size
    int memSize;
    // free memory size
    int freeMem;
    // current memory allocation algorithm
    int adjust_algorithm;
    // return true while memory is no more set-able
    bool isMemorySizeSet;

    // linked list of free block
    free_block_type* freeBlock_head;
    // linked list of allocated block
    AllocatedBlock* allocatedBlock_head;
public:
    MemoryManager();
    void displayMenu() const;

    void setMemorySize();

    void rearrange(const int);
    void selectMemoryAllocationAlgorithm();

    int newProcess();
    int allocateMemory(AllocatedBlock*);
    int retrenchMemory(AllocatedBlock *);

    int killProcess();
    AllocatedBlock* findProcess(const int);


    void displayMemoryUsage() const;
    int freeMemory(AllocatedBlock*);
};