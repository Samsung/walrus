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

#include "include/random.h"

#define ARRAY_LENGTH 15000
#define ITERATIONS 64

#define NO_CHILD 0
#define NO_PARENT 65535

void swap(unsigned int array[], unsigned int elementIndex1, unsigned int elementIndex2) {
    unsigned int temp = array[elementIndex1];
    array[elementIndex1] = array[elementIndex2];
    array[elementIndex2] = temp;
}

void getChildrenIndex(unsigned int array_length, unsigned int parentIndex, unsigned int* childIndex1, unsigned int* childIndex2) {
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

unsigned int getParentIndex(unsigned int childIndex) {
    if (childIndex == 0) {
        return NO_PARENT;
    } else {
        return (childIndex - 1) / 2;
    }
}

void correct(unsigned int array[], unsigned int array_length, unsigned int elementInxex) {
    unsigned int childIndex1;
    unsigned int childIndex2;
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

void toMaxHeap(unsigned int array[], unsigned int array_length) {
    for (unsigned int i = array_length - 1; i >= 1; i--) {
        correct(array, array_length, getParentIndex(i));
    }
}

void heapsort(unsigned int array[], unsigned int array_length) {
    toMaxHeap(array, array_length);
    while (array_length > 1) {
        swap(array, 0, array_length - 1);
        array_length--;
        correct(array, array_length, 0);
    }
}

int runtime() {
    for (unsigned int i = 0; i < ITERATIONS; i++) {
        unsigned int array[ARRAY_LENGTH];
        for (int j = 0; j < ARRAY_LENGTH; j++) {
            array[j] = random();
        }

        heapsort(array, ARRAY_LENGTH);

        for (unsigned int j = 0; j < ARRAY_LENGTH - 1; j++) {
            if (array[j] > array[j + 1]) {
                return j;
            }
        }
    }
    return 0;
}

int main() {
    printf("%d\n", runtime());
    return 0;
}
