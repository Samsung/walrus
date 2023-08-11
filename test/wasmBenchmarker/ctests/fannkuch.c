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

// https://benchmarksgame-team.pages.debian.net/benchmarksgame/description/fannkuchredux.html#fannkuchredux

uint64_t fannkuch(uint64_t n) {
    uint64_t current_perm[n], temp_perm[n], count[n], max_flips = 0, perm_count = 0, checksum = 0;

    for (uint64_t i = 0; i < n; ++i) {
        current_perm[i] = i;
    }

    uint64_t rolls = n;
    while (1) {
        while (rolls != 1) {
            count[rolls - 1] = rolls;
            rolls--;
        }

        for (uint64_t i = 0; i < n; ++i) {
            temp_perm[i] = current_perm[i];
        }

        uint64_t flip_count = 0;
        uint64_t k;

        while (!((k = temp_perm[0]) == 0)) {
            uint64_t k2 = (k + 1) >> 1;
            for (uint64_t i = 0; i < k2; ++i) {
                uint64_t temp = temp_perm[i];
                temp_perm[i] = temp_perm[k - i];
                temp_perm[k - i] = temp;
            }
            flip_count++;
        }

        max_flips = flip_count > max_flips ? flip_count : max_flips;
        checksum += perm_count % 2 == 0 ? flip_count : -flip_count;

        while (1) {
            if (rolls == n) {
                return max_flips;
            }
            uint64_t perm0 = current_perm[0];
            uint64_t i = 0;
            while (i < rolls) {
                uint64_t j = i + 1;
                current_perm[i] = current_perm[j];
                i = j;
            }
            current_perm[rolls] = perm0;

            count[rolls] = count[rolls] - 1;
            if (count[rolls] > 0)
                break;
            rolls++;
        }
        perm_count++;
    }
}

uint64_t runtime() {
    uint64_t retVal = 0;

    for (uint8_t i = 0; i < 4; i++) {
        retVal += fannkuch(9);
    }

    return retVal;
}

int main() {
    printf("%llu\n", (long long unsigned int) runtime());
    return 0;
}
