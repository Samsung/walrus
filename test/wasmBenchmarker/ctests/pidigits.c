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

#include <stdint.h>
#include <stdio.h>

#define DIGITS 10000
#define ARRAY_SIZE (DIGITS * 10 / 3 + 1)

#define FNV_OFFSET_BASIS 14695981039346656037ULL
#define FNV_PRIME 1099511628211ULL

static uint32_t remainders[ARRAY_SIZE];

static void initializeRemainders(void)
{
    for (uint32_t i = 0; i < ARRAY_SIZE; ++i)
        remainders[i] = 2;
}

static void updateChecksum(uint64_t* checksum, uint32_t digit)
{
    *checksum ^= digit;
    *checksum *= FNV_PRIME;
}

static uint64_t calculatePiDigits(void)
{
    uint64_t checksum = FNV_OFFSET_BASIS;
    uint32_t predigit = 0;
    uint32_t nines = 0;
    uint32_t generated = 0;

    initializeRemainders();

    for (uint32_t digitIndex = 0; digitIndex < DIGITS; ++digitIndex) {
        uint64_t carry = 0;

        for (uint32_t i = ARRAY_SIZE; i > 0; --i) {
            uint64_t value = (uint64_t)remainders[i - 1] * 10 + carry * i;
            uint64_t divisor = (uint64_t)i * 2 - 1;

            remainders[i - 1] = (uint32_t)(value % divisor);
            carry = value / divisor;
        }

        remainders[0] = (uint32_t)(carry % 10);
        uint32_t quotient = (uint32_t)(carry / 10);

        if (quotient == 9) {
            nines += 1;
        } else if (quotient == 10) {
            if (generated < DIGITS) {
                updateChecksum(&checksum, predigit + 1);
                generated += 1;
            }

            while (nines > 0 && generated < DIGITS) {
                updateChecksum(&checksum, 0);
                generated += 1;
                nines -= 1;
            }

            predigit = 0;
        } else {
            if (digitIndex != 0 && generated < DIGITS) {
                updateChecksum(&checksum, predigit);
                generated += 1;
            }

            while (nines > 0 && generated < DIGITS) {
                updateChecksum(&checksum, 9);
                generated += 1;
                nines -= 1;
            }

            predigit = quotient;
        }
    }

    if (generated < DIGITS) {
        updateChecksum(&checksum, predigit);
        generated += 1;
    }

    while (nines > 0 && generated < DIGITS) {
        updateChecksum(&checksum, 9);
        generated += 1;
        nines -= 1;
    }

    return checksum;
}

uint64_t runtime(void)
{
    return calculatePiDigits();
}

int main(void)
{
    printf("%llu\n", (unsigned long long)runtime());
    return 0;
}