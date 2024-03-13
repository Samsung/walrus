#ifndef MEMORY_H
#define MEMORY_H

typedef unsigned char byte;
typedef size_t size_t;

#ifndef MEMORY_SIZE
#define MEMORY_SIZE  (15 * 1024) // in bytes
#endif

#ifndef RECORDS_SIZE
#define RECORDS_SIZE 1024 // in pieces
#endif

byte memory[MEMORY_SIZE];

typedef struct {
    void* address;
    size_t size;
} record;

size_t recordsCurrentSize = 0;
record records[RECORDS_SIZE]; // Memory Allocation Table

/**
 * @brief Allocate memory with uninitialised values.
 * @param size the size of the memory to be allocated in bytes.
 * @return the address of the memory, or NULL if allocation fails.
*/
void* allocateMemory(size_t size) {
    if (size > MEMORY_SIZE || size == 0 || recordsCurrentSize == RECORDS_SIZE) {
        return NULL;
    }
    for (size_t i = 0; i <= MEMORY_SIZE - size; i++) {
        bool collision = false;
        for (size_t j = 0; j < recordsCurrentSize; j++) {
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
    return NULL;
}

/**
 * @brief allocate memory and initialise it with zeroes
 * @param size the size of the memory to be allocated in bytes.
 * @return the address of the memory, or NULL if allocation fails.
*/
void* CAllocateMemory(size_t size) {
    void* newMemory = allocateMemory(size);
    if (newMemory == NULL) {
        return NULL;
    }
    size_t iterator = 0;
    for (; iterator < size - size % sizeof(size_t); iterator += sizeof(size_t)) {
        *((size_t*)&(((byte*)newMemory)[iterator])) = (size_t)0;
    }
    for (; iterator < size; iterator++) {
        ((byte*)newMemory)[iterator] = (byte)0;
    }
    return newMemory;
}

/**
 * @brief Free the memory at the given address.
 * @param ptr address of the memory to be freed.
*/
void freeMemory(void* ptr) {
    if (ptr == NULL) return;
    for (size_t i = 0; i < recordsCurrentSize; i++) {
        if (records[i].address == ptr) {
            for (size_t j = i; j < recordsCurrentSize - 1; j++) {
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
 * @return the address of the new memory, or NULL if reallocation fails.
*/
void* reallocateMemory(void* ptr, size_t newSize) {
    if (newSize == 0) {
        freeMemory(ptr);
        return NULL;
    }
    if (ptr == NULL) {
        return allocateMemory(newSize);
    }
    size_t index = 0;
    while (ptr != records[index].address) {
        index++;
    }
    if (newSize <= records[index].size) {
        records[index].size = newSize;
        return records[index].address;
    }
    void* newMem = allocateMemory(newSize);
    if (newMem != NULL) {
        for (size_t i = 0; i < records[index].size; i++) {
            ((byte*)newMem)[i] = ((byte*)ptr)[i];
        }
    }
    freeMemory(ptr);
    return newMem;
}

#endif //MEMORY_H
