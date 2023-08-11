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

#define ARRAY_LENGTH 32000
#define ITERATIONS 16

// constants for the random numbers
#define m UINT16_MAX
#define a 33278
#define c 1256
#define seed 55682

#define NO_CHILD 0
#define NO_PARENT UINT16_MAX

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


/**
 * Sort the given array according to
 * the heapsort algorithm.
*/
void heapsort(uint16_t array[], uint16_t array_length);


/**
 * Convert a heap to a max-heap
 * (where parents have greater value than their children)
*/
void toMaxHeap(uint16_t array[], uint16_t array_length);


/**
 * Set the children's index accordingly.
 *
 * If there's no child, it sets NO_CHILD.
*/
void getChildrenIndex(uint16_t array_length, uint16_t parentIndex, uint16_t *childIndex1, uint16_t *childIndex2);


/**
 * Set the parent's index.
 *
 * If there is no parent, it returns NO_PARENT.
*/
uint16_t getParentIndex(uint16_t childIndex);


/**
 * Correct the tree if needed from the given index
 * to be correct to the max heap.
*/
void correct(uint16_t array[], uint16_t array_length, uint16_t elementInxex);


/**
 * Swap the values of given elements
*/
void swap(uint16_t array[], uint16_t elementIndex1, uint16_t elementIndex2);


uint8_t runtime();


int main() {
    printf("%u\n", (unsigned int) runtime());
    return 0;
}


uint16_t random() {
    static uint16_t previous = seed;
    previous = (a * previous + c) % m;
    return previous;
}

void heapsort(uint16_t array[], uint16_t array_length) {
    toMaxHeap(array, array_length);
    while (array_length > 1) {
        swap(array, 0, array_length - 1);
        array_length -= 1;
        correct(array, array_length, 0);
    }
}

void toMaxHeap(uint16_t array[], uint16_t array_length) {
    for (int i = array_length - 1; i >= 1; i--) {
        correct(array, array_length, getParentIndex(i));
    }
}

void getChildrenIndex(uint16_t array_length, uint16_t parentIndex, uint16_t *childIndex1, uint16_t *childIndex2) {
    if (array_length > (parentIndex * 2) + 1) {
        *childIndex1 = (parentIndex * 2) + 1;
        if (array_length > (parentIndex * 2) + 2) {
            *childIndex2 = (parentIndex * 2) + 2;
        } else {
            *childIndex2 = NO_CHILD;
        }
    } else {
        *childIndex1 = NO_CHILD;
        *childIndex2 = NO_CHILD;
    }
}

uint16_t getParentIndex(uint16_t childIndex) {
    if (childIndex == 0) {
        return NO_PARENT;
    } else {
        return (childIndex - 1) / 2;
    }
}

void correct(uint16_t array[], uint16_t array_length, uint16_t elementInxex) {
    uint16_t childIndex1;
    uint16_t childIndex2;
    getChildrenIndex(array_length, elementInxex, &childIndex1, &childIndex2);
    if (childIndex1 == NO_CHILD && childIndex2 == NO_CHILD) {
        return;
    } else if (childIndex1 != NO_CHILD && childIndex2 == NO_CHILD) {
        if (array[elementInxex] < array[childIndex1]) {
            swap(array, elementInxex, childIndex1);
            correct(array, array_length, childIndex1);
        }
    } else if (childIndex1 == NO_CHILD && childIndex2 != NO_CHILD) {
        if (array[elementInxex] < array[childIndex2]) {
            swap(array, elementInxex, childIndex2);
            correct(array, array_length, childIndex2);
        }
    } else {
        if (array[elementInxex] < array[childIndex1] && array[childIndex1] >= array[childIndex2]) {
            swap(array, elementInxex, childIndex1);
            correct(array, array_length, childIndex1);
        } else if (array[elementInxex] < array[childIndex2] && array[childIndex2] >= array[childIndex1]) {
            swap(array, elementInxex, childIndex2);
            correct(array, array_length, childIndex2);
        } else {
            return;
        }
    }
}

void swap(uint16_t array[], uint16_t elementIndex1, uint16_t elementIndex2) {
    uint16_t temp = array[elementIndex1];
    array[elementIndex1] = array[elementIndex2];
    array[elementIndex2] = temp;
}

uint8_t runtime() {
    for (int i = 0; i < ITERATIONS; i++) {
        uint16_t array[ARRAY_LENGTH];
        for (int j = 0; j < ARRAY_LENGTH; j++) {
            array[j] = random();
        }

        heapsort(array, ARRAY_LENGTH);

        for (int j = 0; j < ARRAY_LENGTH - 1; j++) {
            if (array[j] > array[j + 1]) {
                return j;
            }
        }
    }
    return 0;
}
