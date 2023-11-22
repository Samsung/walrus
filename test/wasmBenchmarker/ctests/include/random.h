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

#ifndef RANDOM_H
#define RANDOM_H

// constants for the random number generator
#define m 65535
#define a 33278
#define c 1256
#define seed 55682

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
unsigned int random() {
    static unsigned int previous = seed;
    previous = (a * previous + c) % m;
    return previous;
}

#endif // RANDOM_H
