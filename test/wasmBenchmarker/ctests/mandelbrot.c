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
#include <stdbool.h>
#include <math.h>
#include <stdint.h>

#define WIDTH 1600
#define HIGHT 1400
#define N 20
#define REAL_AXIS_SHIFT -1.8 // ~ horizontal shift
#define IMAGINARY_AXIS_SHIFT -1.0 // ~ vertical shift
#define ZOOM 0.0015

#define getNthBit(b, n) ((b & (1 << (7 - n))) > 0)

#define clearNthBit(b, n) b = b & (0xFF - (1 << (7 - n)))

#define setNthBit(b, n) b = b | (1 << (7 - n))

#define ABS_COMPLEX(z_real, z_complex) (sqrtf(z_real * z_real + z_complex * z_complex))

typedef uint8_t byte;

byte isInMandelbrotSet(float c_real, float c_imaginary)
{
    byte result = 0b10000000;
    float z_real = 0;
    float z_imaginary = 0;
    for (size_t k = 0; k < N; k++) {
        float complex_abs = ABS_COMPLEX(z_real, z_imaginary);
        if (getNthBit(result, 0) == 1 && complex_abs > 2) {
            clearNthBit(result, 0);
        } else {
            float next_z_real = (z_real * z_real - z_imaginary * z_imaginary) + c_real;
            float next_z_imaginary = ((float)2.0 * z_real * z_imaginary) + c_imaginary;
            z_real = next_z_real;
            z_imaginary = next_z_imaginary;
        }

        if (result == 0) {
            break;
        }
    }
    return result;
}

uint32_t runtime() {
    uint32_t setSize = 0;
    for (int i = 0; i < HIGHT; i++) {
        for (int j = 0; j < WIDTH; j++) {
            float real = ((float)j * (float)ZOOM) + (float)REAL_AXIS_SHIFT;
            float imaginary = ((float)i * (float)ZOOM) + (float)IMAGINARY_AXIS_SHIFT;
            if (getNthBit(isInMandelbrotSet(real, imaginary),0)) {
                setSize++;
            }
        }
    }
    return setSize;
}


int main() {
    printf("%u\n", runtime());
    return 0;
}
