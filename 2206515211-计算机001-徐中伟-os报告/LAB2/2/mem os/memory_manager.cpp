#ifndef _MEMORY_MANAGER_CPP_
#define _MEMORY_MANAGER_CPP_
#endif

#include "memory_manager.h"

MemoryManager::MemoryManager()
    :
    cp(0),
    memSize(1024),
    freeMem(1024),
    adjust_algorithm(MA_FF),
    isMemorySizeSet(false),
    freeBlock_head(nullptr),
    allocatedBlock_head(nullptr) {

    char choice;
    while(1) {
        displayMenu();
        cin >> choice;
        if (choice != '1' && !isMemorySizeSet) {
            cerr << endl << "warning: memory size is set 1024 by default!";
            cerr << "(and u can not change it.)" << endl;
            isMemorySizeSet = true;
            freeBlock_head = free_block_type::init_freeBlock(memSize);
        }
        switch(choice){
        case '1': setMemorySize(); break;
        case '2': selectMemoryAllocationAlgorithm(); break;
        case '3': newProcess(); break;
        case '4': killProcess(); break;
        case '5': displayMemoryUsage(); break;
        case '0':
            // todo : release linked table
            return ;
        default: break;
        }
    }
}

// for operating in the console
void MemoryManager::displayMenu() const {
    cout << endl;
    cout << "1: set memory size (default=" << DEFAULT_MEM_SIZE << ")" << endl;
    cout << "2: select memory allocation algorithm" << endl;
    cout << "3: new process" << endl;
    cout << "4: terminate process" << endl;
    cout << "5: display memory usage" << endl;
    cout << "0: exit" << endl;
}


// input and lock memory size
void MemoryManager::setMemorySize() {
    if (isMemorySizeSet) {
        cerr << endl << "warning : you should only set once memory size." << endl;
        return ;
    }

    while (1) {
        cout << "Total memory size = ";
        cin >> memSize;
        if (memSize <= 0)
            cerr << endl << "error : please input a valid value number" << endl;
        else {
            freeMem = memSize;
            isMemorySizeSet = true;
            freeBlock_head = free_block_type::init_freeBlock(memSize);
            cout << "valid!" << endl;
            return ;
        }
    }
}

// rearrange linked block of free blocks by input args
void MemoryManager::rearrange(int order = 0) {
    // implicit announcement
    if (!order) order = adjust_algorithm;

    free_block_type *start = freeBlock_head;
    free_block_type *curr, *next, *target;
    // sub linked-block is longer than 1
    while (start->next) {
        target = curr = start;
        next = curr->next;

        // till the tail
        while (next) {
            if (next->compare(order, target)) { target =  next; }
            curr = next;
            next = curr->next;
        }

        start->swap(target);
        start = start->next;
    }


}

// load mma from console input
void MemoryManager::selectMemoryAllocationAlgorithm() {
    cout << "\t1:First Fit" << endl;
    cout << "\t2: Best Fit" << endl;
    cout << "\t3:Worst Fit" << endl;
    cerr << "Please choose 1-3" << endl;

    int maa;
    while (1) {
        cin >> maa;
        if (maa > 3 || maa < 1)
            cerr << endl << "Please input the valid value for algorithm" << endl;
        else {
            this->adjust_algorithm = maa;
            cout << "valid!" << endl;
            return ;
        }
    }
}

// create new process according to console input
// allocate memory and add an allocated block to head
// return -2 when there is no free process id
// return -1 when there is no memory at all
// return 0 when there is no enough memory
// return 1 when creation  succeed
int MemoryManager::newProcess() {
    static int succeed_times = 0;
    AllocatedBlock *ab = new AllocatedBlock;
    if (!ab) exit(-5);
    if (cp == MAX_PROCESS_ID) {
        cerr << endl << "error : no enough process id" << endl;
        return -2;
    }

    ab->pid = (1 + succeed_times) % MAX_PROCESS_ID;
    ab->next = nullptr;
    sprintf(ab->process_name,"PROCESS-%02d", ab->pid);

    int size;
    while (1) {
        cout << "Memory for " << ab->process_name << ": ";
        cin >> size;
        if (size > 0) {
            ab->size = size;
            break;
        }
        else cerr << "error : unforeseen input" << endl;
    }

    int ret = allocateMemory(ab);
    if (ret == 1) {
        ab->next = allocatedBlock_head;
        allocatedBlock_head = ab;

        cp++;
        succeed_times++;
        pMap[ab->pid] = ab;
        return 1;
    }
    else if (ret == 0)
        cerr << endl << "error : not enough memory." << endl;

    delete ab;
    return ret;
}
// allocate memory for the new process
// return -1 when there is no memory at all
// return 0 when there is no enough memory
// return 1 when allocation  succeed
int MemoryManager::allocateMemory(AllocatedBlock *ab) {
    if (freeBlock_head == nullptr) return -1;
    if (freeMem < ab->size) return 0;
    rearrange();

    free_block_type *fb_prev = nullptr;
    free_block_type *fb_curr = freeBlock_head;

    // search the first fit one
    while (fb_curr != nullptr) {
        if (fb_curr->size >= ab->size) break;
        fb_curr = fb_curr->next;
        fb_prev = fb_curr;
    }

    if (fb_curr != nullptr) {
        freeMem -= ab->size;
        ab->start_addr = fb_curr->start_addr;
        // slice is too small, make it extra
        if (fb_curr->size - ab->size < MIN_SLICE) {
            ab->size = fb_curr->size;
            ab->extraSize = fb_curr->size - ab->size;
            if (fb_prev == nullptr) freeBlock_head = fb_curr->next;
            else fb_prev->next = fb_curr->next;

            delete fb_curr;
        }
        // left memory is still big enough for a slice
        else {
            ab->extraSize = 0;
            fb_curr->start_addr += ab->size;
            fb_curr->size -= ab->size;
        }
        return 1;
    }
    else return retrenchMemory(ab);
}

// retrench memory to contain a new process
int MemoryManager::retrenchMemory(AllocatedBlock *ab){
    rearrange(1);

    int usedMem = 0;
    AllocatedBlock *curr;
    curr = allocatedBlock_head;
    while (curr) {
        curr->start_addr = usedMem;

        curr->size -= curr->extraSize;
        curr->extraSize = 0;

        usedMem += curr->size;
        curr = curr->next;
    }


    freeBlock_head = free_block_type::init_freeBlock(freeMem, usedMem);
    allocateMemory(ab);

    return 1;
}

// kill process according to pid input from console
// return 0 when there is no process running
// return 1 when termination successed
int MemoryManager::killProcess() {
    if (cp == 0) {
        cerr << "error : there is no process running" << endl;
        return 0;
    }

    int pid;
    AllocatedBlock *ab;
    while (1) {
        cout << "Kill Process, pid = ";
        cin >> pid;
        if ((ab = findProcess(pid)) != 0) { break; }
        cerr << endl << "error : no corresponding process" << endl;
        displayMemoryUsage();
    }

    // erase ab from linked-block of allocated ones
    if (ab != allocatedBlock_head) {
        AllocatedBlock *curr = allocatedBlock_head;
        while (curr->next != ab) { curr = curr->next; }
        curr->next = ab->next;
    } else allocatedBlock_head = ab->next;
   // retrenchMemory(ab);
    return freeMemory(ab);
}
// return the pointer of allocated block
AllocatedBlock* MemoryManager::findProcess(const int pid) {
    return pMap[pid];
}
// free allocated block
// create free block
int MemoryManager::freeMemory(AllocatedBlock* ab) {
    free_block_type *fb = free_block_type::init_freeBlock(ab->size, ab->start_addr);
    fb->next = freeBlock_head;
    freeBlock_head = fb;
    rearrange();

    freeMem += ab->size;
    delete ab;
    return 1;
}


void MemoryManager::displayMemoryUsage() const {
    cout << "-------------------------------------------------------------";
    cout << endl << "Free Memory: " << freeMem  << endl;
    printf("%20s %20s\n", "     start_addr", "     size");
    free_block_type *fb = freeBlock_head;
    while(fb != NULL) {
        printf("%20d %20d\n", fb->start_addr, fb->size);
        fb = fb->next;
    }

    cout << endl << "Used Memory: " << memSize - freeMem << endl;
    printf("%10s %20s %10s %10s\n","PID","ProcessName","start_addr","size");
    AllocatedBlock *ab = allocatedBlock_head;
    while(ab != NULL) {
        printf("%10d %20s %10d %10d\n",
            ab->pid, ab->process_name, ab->start_addr,ab->size);
        ab = ab->next;
    }
    cout << "-------------------------------------------------------------";
}