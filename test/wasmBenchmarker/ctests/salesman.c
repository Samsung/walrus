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

#include <stdint.h>
#include <stdio.h>
#include <limits.h>

#define V 10
#define LOOP 3

uint8_t next_permutation(int *p, int len)
{
    int found = 0;
    int i = len - 2;

    while (i >= 0) {
        if (p[i] < p[i + 1]) {
            found = 1;
            break;
        }
        i--;
    }

    if (!found) {
        return 0;
    }

    int tempLen = len - i;
    int tempNums[tempLen];
    int tempInt;

    for (int j = 0; j < tempLen; j++) {
        tempNums[j] = p[i + j];
    }

    for (int j = 0; j < tempLen; j++) {
        for (int k = j + 1; k < tempLen; k++) {
            if (tempNums[j] > tempNums[k]) {
                tempInt = tempNums[j];
                tempNums[j] = tempNums[k];
                tempNums[k] = tempInt;
            }
        }
    }

    int closestFound = 0;

    for (int l = 0; l < tempLen; l++) {
        if (!closestFound && tempNums[l] > p[i]) {
            p[i] = tempNums[l];
            closestFound = 1;
        } else {
            p[i + l + 1 - closestFound] = tempNums[l];
        }
    }

    return 1;
}

int min(int a, int b)
{
    return a < b ? a : b;
}

uint32_t shortest_path_sum(int edges_list[][V], int s)
{
    int nodes[V * V];
    int n = 0;

    for (int i = 0; i < V; i++) {
        if (i != s) {
            nodes[n] = i;
            n++;
        }
    }

    int shortest_path = INT_MAX;

    while (next_permutation(nodes, n)) {
        int path_weight = 0;

        int j = s;
        for (int i = 0; i < n; i++) {
            path_weight += edges_list[j][nodes[i]];
            j = nodes[i];
        }
        path_weight += edges_list[j][s];

        shortest_path = min(shortest_path, path_weight);
    }

    return shortest_path;
}

uint32_t runtime()
{
    uint32_t retVal = 0;
    for (uint8_t l = 0; l < LOOP; l++) {
        for (uint8_t i = 0; i < 7; i++) {
            int edges_list[V][V] = { { 0, 10, 15, 20, 25, 30, 35, 40, 45, 50 },
                                     { 10, 0, 35, 25, 20, 15, 20, 10, 5, 35 },
                                     { 15, 35, 0, 30, 10, 40, 50, 10, 10, 10 },
                                     { 20, 25, 30, 0, 35, 15, 10, 30, 20, 10 },
                                     { 25, 20, 10, 35, 0, 30, 15, 20, 25, 30 },
                                     { 30, 15, 40, 15, 30, 0, 20, 10, 15, 20 },
                                     { 35, 20, 50, 10, 15, 20, 0, 25, 30, 35 },
                                     { 40, 10, 10, 30, 20, 10, 25, 0, 35, 40 },
                                     { 45, 5, 10, 20, 25, 15, 30, 35, 0, 45 },
                                     { 50, 35, 10, 10, 30, 20, 35, 40, 45, 0 } };

            retVal += shortest_path_sum(edges_list, 0);
        }
    }

    return retVal;
}

int main()
{
    printf("%u\n", runtime());
}
