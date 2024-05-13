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

uint64_t factorial(uint64_t n) {
    uint64_t counter = 0;
    for (uint64_t i = 0; i < n; i++, counter++) {
        factorial(n - 1);
    }

    return counter;
}

uint64_t runtime() {
    uint64_t retVal = 0;

    for (uint64_t i = 0; i < 6000000000; i++) {
        retVal += factorial(150000000);
        retVal -= factorial(149999999);
        retVal += factorial(149999998);
    }

    return retVal;
}

int main() {
    printf("%llu\n", (long long unsigned int) runtime());
    return 0;
}
