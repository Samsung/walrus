/*
 * Copyright (c) 2023-present Samsung Electronics Co., Ltd
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

// DEFINITIONS

// TEST PARAMETER

#define SIZE 500 // size of array to be ordered
#define ITERATIONS 10

// MEMORY HANDLING STUFFS

#define nullptr ((void*)0)
#define MEMORY_SIZE  6000 // in bytes
#define RECORDS_SIZE 1000 // in pieces

uint8_t memory[MEMORY_SIZE];

typedef struct {
    uint8_t *address;
    uint32_t size;
} record;

uint32_t records_current_size = 0;
record records[RECORDS_SIZE]; // Memory Allocation Table

// bm = BenchMark (to avoid function name collision)

/**
 * Allocate memory with uninitialised values.
 * @param size the size of the memory to be allocated
 * @return the address of the memory, or nullptr
*/
void *malloc_bm(uint32_t size);

/**
 * Allocate memory and initialise its values with 0.
 * @param size the size of the memory to be allocated
 * @return the address of the memory, or nullptr
*/
void *calloc_bm(uint32_t size);

/**
 * Resize the given allocated memory.
 * Its values will be identical to the values of the
 * original memory up to the lesser size.
 * @param ptr the pointer of the original memory
 * @param size the size of the new memory
 * @return the address of the new memory, or nullptr
*/
void *realloc_bm(void *ptr, uint32_t size);

/**
 * Free the memory at the given address.
 * @param the address of the memory to be freed
*/
void free_bm(void *ptr);

// RANDOM NUMBER GENERATING STUFFS

// constants for the random number generator
#define m UINT16_MAX
#define a 33278
#define c 1256
#define seed 55682

/**
 * Generate random number according to
 * the linear congruential generator (LCG).
 *
 * Xˇ(n+1) = (a * Xˇ(n) + c) mod m
 *
 * where:
 * - 0 < m
 * - 0 < a < m
 * - 0 <= c < m
 * - 0 <= Xˇ(0) <= m
*/
uint16_t random();

// SORTING ALGORITHM

/**
 * Order the given array with the quicksort
 * algorithm.
 * @param array address of the array to be ordered
 * @param size the size of the array to be ordered
*/
void quick_sort(uint16_t *array, uint16_t size);

// EXTRA STUFF

/**
 * It's needed to be runned in the WasmBenchmark
 * @return 0 if test passes, 1 if fails
*/
uint8_t runtime();

// MAIN FUNCTION

int main() {
    printf("%u\n", (unsigned int) runtime());
    return 0;
}

// IMPLEMENTATIONS OF THE FUNCTIONS

// MEMORY HANDLING STUFFS

void *malloc_bm(uint32_t size) {
    if (size > MEMORY_SIZE || size == 0 || records_current_size == RECORDS_SIZE) {
        return nullptr;
    }
    for (uint32_t i = 0; i <= MEMORY_SIZE - size; i++) {
        bool collision = false;
        for (uint32_t j = 0; j < records_current_size; j++) {
            if (
                    ((memory + i) <= records[j].address && records[j].address <= (memory + i + size - 1)) ||
                    ((memory + i) <= (records[j].address + records[j].size - 1) &&
                     (records[j].address + records[j].size - 1) <= (memory + i + size - 1)) ||
                    (records[j].address <= (memory + i) &&
                     (memory + i + size - 1) <= (records[j].address + records[j].size - 1))
                    ) {
                collision = true;
                break;
            }
        }
        if (collision == false) {
            records[records_current_size].address = memory + i;
            records[records_current_size].size = size;
            records_current_size += 1;
            return records[records_current_size - 1].address;
        }
    }
    return nullptr;
}

void *calloc_bm(uint32_t size) {
    uint8_t *ret = malloc_bm(size);
    if (ret != nullptr) {
        for (uint32_t i = 0; i < size; i++) {
            ret[i] = 0;
        }
    }
    return ret;
}

void *realloc_bm(void *ptr, uint32_t size) {
    if (size == 0 || ptr == nullptr) {
        return nullptr;
    }
    uint32_t index = 0;
    while (ptr != records[index].address) index += 1;
    if (size <= records[index].size) {
        records[index].size = size;
        return records[index].address;
    }
    void *new = malloc_bm(size);
    if (new != nullptr) {
        for (uint32_t i = 0; i < records[index].size; i++) {
            ((uint8_t *) new)[i] = ((uint8_t *) ptr)[i];
        }
    }
    free_bm(ptr);
    return new;
}

void free_bm(void *address) {
    if (address == nullptr) {
        return;
    }
    for (uint32_t i = 0; i < records_current_size; i++) {
        if (records[i].address == address) {
            for (uint32_t j = i; j < records_current_size - 1; j++) {
                records[j].address = records[j + 1].address;
                records[j].size = records[j + 1].size;
            }
            records_current_size -= 1;
        }
    }
}

// RANDOM NUMBER GENERATING STUFFS

uint16_t random() {
    static uint16_t previous = seed;
    previous = (a * previous + c) % m;
    return previous;
}

// SORTING ALGORITHM

void quick_sort(uint16_t *array, uint16_t size) {
    if (size <= 1 || array == nullptr) {
        return;
    }
    uint16_t *smaller_equal = (uint16_t *) malloc_bm(size * sizeof(uint16_t));
    int smaller_equal_size = 0;
    uint16_t *greater = (uint16_t *) malloc_bm(size * sizeof(uint16_t));
    int greater_size = 0;
    uint16_t pivot = array[0];
    for (int i = 1; i < size; i++) {
        if (array[i] <= pivot) {
            smaller_equal[smaller_equal_size] = array[i];
            smaller_equal_size += 1;
        } else {
            greater[greater_size] = array[i];
            greater_size += 1;
        }
    }
    if (smaller_equal_size > 0) {
        smaller_equal = (uint16_t *) realloc_bm(smaller_equal, (smaller_equal_size * sizeof(uint16_t)));
    } else {
        free_bm((uint8_t *) smaller_equal);
    }
    if (greater_size > 0) {
        greater = (uint16_t *) realloc_bm(greater, (greater_size * sizeof(uint16_t)));
    } else {
        free_bm((uint8_t *) greater);
    }
    quick_sort(smaller_equal, smaller_equal_size);
    quick_sort(greater, greater_size);
    for (int i = 0; i < smaller_equal_size; i++) {
        array[i] = smaller_equal[i];
    }
    array[smaller_equal_size] = pivot;
    for (int i = 0; i < greater_size; i++) {
        array[smaller_equal_size + 1 + i] = greater[i];
    }
    free_bm((uint8_t *) smaller_equal);
    free_bm((uint8_t *) greater);
}

// EXTRA STUFF

uint8_t runtime() {
    for (int i = 0; i < ITERATIONS; i++) {
        uint16_t array[SIZE];
        for (int j = 0; j < SIZE; j++) {
            array[j] = random();
        }
        quick_sort(array, SIZE);
        for (int j = 0; j < SIZE - 1; j++) {
            if (array[j] > array[j + 1]) {
                return 1;
            }
        }
    }
    return 0;
}
