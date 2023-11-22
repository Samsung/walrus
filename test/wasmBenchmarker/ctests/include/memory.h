#ifndef MEMORY_H
#define MEMORY_H

#define nullptr ((void*)0)

#ifndef MEMORY_SIZE
#define MEMORY_SIZE  (15 * 1024) // in bytes
#endif

#ifndef RECORDS_SIZE
#define RECORDS_SIZE 1024 // in pieces
#endif

char memory[MEMORY_SIZE];

typedef struct {
    void* address;
    unsigned int size;
} record;

unsigned int recordsCurrentSize = 0;
record records[RECORDS_SIZE]; // Memory Allocation Table

/**
 * @brief Allocate memory with uninitialised values.
 * @param size the size of the memory to be allocated in bytes.
 * @return the address of the memory, or nullptr if allocation fails.
*/
void* allocateMemory(unsigned int size) {
    if (size > MEMORY_SIZE || size == 0 || recordsCurrentSize == RECORDS_SIZE) {
        return nullptr;
    }
    for (unsigned int i = 0; i <= MEMORY_SIZE - size; i++) {
        bool collision = false;
        for (unsigned int j = 0; j < recordsCurrentSize; j++) {
            if (
                    (((void*)memory + i) <= records[j].address && records[j].address <= ((void*)memory + i + size - 1)) ||
                    (((void*)memory + i) <= (records[j].address + records[j].size - 1) && (records[j].address + records[j].size - 1) <= ((void*)memory + i + size - 1)) ||
                    (records[j].address <= ((void*)memory + i) && ((void*)memory + i + size - 1) <= (records[j].address + records[j].size - 1))
                    ) {
                collision = true;
                break;
            }
        }
        if (collision == false) {
            records[recordsCurrentSize].address = memory + i;
            records[recordsCurrentSize].size = size;
            recordsCurrentSize++;
            return records[recordsCurrentSize - 1].address;
        }
    }
    return nullptr;
}

/**
 * @brief Free the memory at the given address.
 * @param ptr address of the memory to be freed.
*/
void freeMemory(void* ptr) {
    if (ptr == nullptr) return;
    for (unsigned int i = 0; i < recordsCurrentSize; i++) {
        if (records[i].address == ptr) {
            for (unsigned int j = i; j < recordsCurrentSize - 1; j++) {
                records[j].address = records[j + 1].address;
                records[j].size = records[j + 1].size;
            }
            recordsCurrentSize--;
        }
    }
}

/**
 * @brief Resize the given allocated memory.
 * Its values will be identical to the values of the
 * original memory up to the lesser size.
 * @param ptr the pointer of the original memory.
 * @param newSize the size of the new memory in bytes.
 * @return the address of the new memory, or nullptr if reallocation fails.
*/
void* reallocateMemory(void* ptr, unsigned int newSize) {
    if (newSize == 0) {
        freeMemory(ptr);
        return nullptr;
    }
    if (ptr == nullptr) {
        return allocateMemory(newSize);
    }
    unsigned int index = 0;
    while (ptr != records[index].address) {
        index++;
    }
    if (newSize <= records[index].size) {
        records[index].size = newSize;
        return records[index].address;
    }
    void* newMem = allocateMemory(newSize);
    if (newMem != nullptr) {
        for (unsigned int i = 0; i < records[index].size; i++) {
            ((char*)newMem)[i] = ((char*)ptr)[i];
        }
    }
    freeMemory(ptr);
    return newMem;
}

#endif //MEMORY_H
