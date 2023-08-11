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

#define LOOP 600
#define X_MIN -1.5
#define X_MAX 0.5
#define Y_MIN -1.0
#define Y_MAX 1.0
#define X_RES 512

uint32_t runtime() {
    const int yRes = (X_RES * (Y_MAX - Y_MIN)) / (X_MAX - X_MIN);

    double dx = (X_MAX - X_MIN) / X_RES;
    double dy = (Y_MAX - Y_MIN) / yRes;
    double x, y;
    int k;

    uint16_t retValLower = 0;
    uint16_t retValHigher = 0;

    for (int j = 0; j < yRes; j++) {
        y = Y_MAX - j * dy;

        for (int i = 0; i < X_RES; i++) {
            double u = 0;
            double v = 0;
            double u2 = 0;
            double v2 = 0;

            x = X_MIN + i * dx;

            for (k = 1; k < LOOP && (u2 + v2 < 4.0); k++) {
                v = 2 * u * v + y;
                u = u2 - v2 + x;
                u2 = u * u;
                v2 = v * v;
            }

            if (k >= LOOP) {
                retValLower++;
            } else {
                retValHigher++;
            }
        }
    }

    return retValLower + (retValHigher << 16);
}

int main() {
    printf("%d\n", runtime());
    return 0;
}