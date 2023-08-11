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

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

#define DISKS 24

typedef struct {
    uint64_t disks[DISKS];
    uint64_t size; // current size
} rod;

void move_disk(rod *from,
               rod *to) {
    if (from->size == 0) {
        return;
    }
    if (to->size == 0) {
        to->disks[0] = from->disks[from->size - 1];
        from->size -= 1;
        to->size += 1;
        return;
    }
    if (from->disks[from->size - 1] < to->disks[to->size - 1]) {
        return;
    }
    to->disks[to->size] = from->disks[from->size - 1];
    from->size -= 1;
    to->size += 1;
}

void move_tower(rod *from,
                rod *to,
                rod *free,
                uint64_t quantity) {
    if (quantity == 0) {
        return;
    }
    if (quantity == 1) {
        move_disk(from, to);
        return;
    }
    move_tower(from, free, to, quantity - 1);
    move_disk(from, to);
    move_tower(free, to, from, quantity - 1);
}

uint64_t hanoi() {
    // init
    rod rods[3];
    for (int i = 0; i < DISKS; i++) {
        rods[0].disks[i] = i;
        rods[1].disks[i] = 0;
        rods[2].disks[i] = 0;
    }
    rods[0].size = DISKS;
    rods[1].size = 0;
    rods[2].size = 0;

    // move tower from left side to right side
    move_tower(&rods[0], &rods[2], &rods[1], DISKS);

    // check
    if (rods[2].size < DISKS) {
        return EXIT_FAILURE;
    }
    for (int i = 0; i < DISKS; i++) {
        if (rods[2].disks[i] != i) {
            return EXIT_FAILURE;
        }
    }
    return EXIT_SUCCESS;
}

uint64_t runtime() {
    return hanoi();
}

int main() {
    printf("%llu\n", (long long unsigned int) runtime());
    return EXIT_SUCCESS;
}
