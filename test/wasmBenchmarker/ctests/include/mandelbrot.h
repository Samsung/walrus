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

#ifndef MANDELBROT_H
#define MANDELBROT_H

#ifndef FLOAT_SIZE
#error define FLOAT_SIZE! (float = 4 | double = 8)
#endif

#if FLOAT_SIZE == 4
#define TYPE float
#elif FLOAT_SIZE == 8
#define TYPE double
#else
#error FLOAT_SIZE has to be 4 or 8! (float = 4 | double = 8)
#endif

#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include <stdint.h>

#define WIDTH 12800
#define HIGHT 11200
#define N 20
#define REAL_AXIS_SHIFT -1.8 // ~ horizontal shift
#define IMAGINARY_AXIS_SHIFT -1.0 // ~ vertical shift
#define ZOOM 0.0015

#define getNthBit(b, n) ((b & (1 << (7 - n))) > 0)

#define clearNthBit(b, n) b = b & (0xFF - (1 << (7 - n)))

#define setNthBit(b, n) b = b | (1 << (7 - n))

#if FLOAT_SIZE == 4
#define complexAbs(z_real, z_complex) (sqrtf(z_real * z_real + z_complex * z_complex))
#else // FLOAT_SIZE == 8
#define complexAbs(z_real, z_complex) (sqrt(z_real * z_real + z_complex * z_complex))
#endif

#define square(a) a * a

typedef uint8_t byte;

byte isInMandelbrotSet(TYPE c_real, TYPE c_imaginary) {
    byte result = 0b10000000;
    TYPE z_real = 0;
    TYPE z_imaginary = 0;
    for (int k = 0; k < N; k++) {
        TYPE abs = complexAbs(z_real, z_imaginary);
        if (getNthBit(result, 0) == 1 && abs > (TYPE)2.0) {
            clearNthBit(result, 0);
        } else {
            TYPE next_z_real = (square(z_real) - square(z_imaginary)) + c_real;
            TYPE next_z_imaginary = ((TYPE)2.0 * z_real * z_imaginary) + c_imaginary;
            z_real = next_z_real;
            z_imaginary = next_z_imaginary;
        }

        if (result == 0) {
            break;
        }
    }
    return result;
}

int runtime() {
    int setSize = 0;
    for (int i = 0; i < HIGHT; i++) {
        for (int j = 0; j < WIDTH; j++) {
            TYPE real = ((TYPE)j *(TYPE)ZOOM) + (TYPE)REAL_AXIS_SHIFT;
            TYPE imaginary = ((TYPE)i * (TYPE)ZOOM) + (TYPE)IMAGINARY_AXIS_SHIFT;
            if (getNthBit(isInMandelbrotSet(real, imaginary), 0)) {
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

#endif /* MANDELBROT_H */
