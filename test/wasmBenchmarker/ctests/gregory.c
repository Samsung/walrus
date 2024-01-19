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

double gregorySeries(uint64_t n) {
    double sum = 0;

    for (uint64_t i = 0; i < n; i++) {
        sum += (i % 2 == 0 ? 1 : -1) * (1.0 / (2 * i + 1));
    }

    return sum * 4;
}

double runtime() {
    return gregorySeries(75000000);
}

int main() {
    printf("%.8lf\n", runtime());
}

