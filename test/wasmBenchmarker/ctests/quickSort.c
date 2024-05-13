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
#include <stdbool.h>

#include "include/random.h"
#include "include/memory.h"

// TEST PARAMETERS
#define SIZE 500 // size of array to be ordered
#define ITERATIONS 17

/**
 * Order the given array with the quicksort
 * algorithm.
 * @param array address of the array to be ordered
 * @param size the size of the array to be ordered
*/
void quickSort(unsigned int* array, unsigned int size) {
    if (size <= 1 || array == NULL) {
        return;
    }
    unsigned int* smallerOrEqual = (unsigned int*)allocateMemory(size * sizeof(unsigned int));
    unsigned int smallerOrEqualSize = 0;
    unsigned int* greater = (unsigned int*)allocateMemory(size * sizeof(unsigned int));
    unsigned int greaterSize = 0;
    unsigned int pivot = array[0];
    for (unsigned int i = 1; i < size; i++) {
        if (array[i] <= pivot) {
            smallerOrEqual[smallerOrEqualSize++] = array[i];
        } else { // array[i] > pivot
            greater[greaterSize++] = array[i];
        }
    }
    smallerOrEqual = (unsigned int*)reallocateMemory(smallerOrEqual, (smallerOrEqualSize * sizeof(unsigned int)));
    greater = (unsigned int*)reallocateMemory(greater, (greaterSize * sizeof(unsigned int)));
    quickSort(smallerOrEqual, smallerOrEqualSize);
    quickSort(greater, greaterSize);
    for (unsigned int i = 0; i < smallerOrEqualSize; i++) {
        array[i] = smallerOrEqual[i];
    }
    array[smallerOrEqualSize] = pivot;
    for (unsigned int i = 0; i < greaterSize; i++) {
        array[smallerOrEqualSize + 1 + i] = greater[i];
    }
    freeMemory(smallerOrEqual);
    freeMemory(greater);
}

int runtime() {
    for (unsigned int i = 0; i < ITERATIONS; i++) {
        unsigned int array[SIZE];
        for (unsigned int j = 0; j < SIZE; j++) {
            array[j] = random();
        }
        quickSort(array, SIZE);
        for (unsigned int j = 0; j < SIZE - 1; j++) {
            if (array[j] > array[j + 1]) {
                return 1;
            }
        }
    }
    return 0;
}

int main() {
    printf("%d\n", runtime());
    return 0;
}
