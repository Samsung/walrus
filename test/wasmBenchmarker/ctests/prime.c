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

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

#define PRIME_NUMBER 5000


// return the greatest x, where x^2 <= number
uint64_t whole_sqrt(uint64_t number) {
    uint64_t root = 0;
    while ((root + 1) * (root + 1) <= number) {
        root += 1;
    }
    return root;
}

uint64_t getPrime(uint64_t sequence_number) {
    // there is no 0th prime number
    if (sequence_number == 0) {
        return 0;
    }

    uint64_t prime = 2;
    for (uint64_t i = 1; i < sequence_number; i++) {
        uint64_t current_number = prime;
        while (true) {
            // check for overflow
            if (current_number == UINT64_MAX) {
                return 0;
            }
            current_number += 1;
            bool is_prime = true;
            for (uint64_t j = 2; j <= whole_sqrt(current_number); j++) {
                if (current_number % j == 0) {
                    is_prime = false;
                    break;
                }
            }
            if (is_prime) {
                break;
            }
        }
        prime = current_number;
    }

    return prime;
}

uint64_t runtime() {
    return getPrime(PRIME_NUMBER);
}

int main() {
    printf("%llu\n", (long long unsigned int) runtime());
    return EXIT_SUCCESS;
}
