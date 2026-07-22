/*
 * Copyright (c) 2026-present Samsung Electronics Co., Ltd
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

#define N 650

int path[N][N];

void init()
{
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            path[i][j] = i * j % 7 + 1;
            if ((i + j) % 13 == 0 || (i + j) % 7 == 0 || (i + j) % 11 == 0) {
                path[i][j] = 999;
            }
        }
    }
}

void floydWarshall()
{
    for (int k = 0; k < N; k++) {
        for (int i = 0; i < N; i++) {
            for (int j = 0; j < N; j++) {
                path[i][j] = path[i][j] < path[i][k] + path[k][j]
                    ? path[i][j]
                    : path[i][k] + path[k][j];
            }
        }
    }
}

uint64_t runtime()
{
    init();
    floydWarshall();

    uint64_t checksum = 0;
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            checksum += (uint64_t)path[i][j];
        }
    }
    return checksum;
}

int main()
{
    printf("%llu\n", (unsigned long long)runtime());
    return 0;
}
