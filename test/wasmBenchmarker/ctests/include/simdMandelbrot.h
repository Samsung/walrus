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

#ifndef SIMD_MANDELBROT_H
#define SIMD_MANDELBROT_H

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
#include <stdint.h>

#include <wasm_simd128.h>

#define WIDTH 6400
#define HIGHT 5600
#define N 20
#define REAL_AXIS_SHIFT -1.8 // ~ horizontal shift
#define IMAGINARY_AXIS_SHIFT -1.0 // ~ vertical shift
#define ZOOM 0.0015

#define byte char

#define getNthBit(b, n) ((b & (1 << (7 - n))) > 0)

#define clearNthBit(b, n) b = b & (0xFF - (1 << (7 - n)))

#define setNthBit(b, n) b = b | (1 << (7 - n))

#if FLOAT_SIZE == 4
#define const_splat wasm_f32x4_const_splat
#define gt wasm_f32x4_gt
#define add wasm_f32x4_add
#define sub wasm_f32x4_sub
#define mul wasm_f32x4_mul
#define simd_sqrt wasm_f32x4_sqrt
#define make wasm_f32x4_make
#define CHECKED_POINTS_SIZE 4
#else // FLOAT_SIZE == 8
#define const_splat wasm_f64x2_const_splat
#define gt wasm_f64x2_gt
#define add wasm_f64x2_add
#define sub wasm_f64x2_sub
#define mul wasm_f64x2_mul
#define simd_sqrt wasm_f64x2_sqrt
#define make wasm_f64x2_make
#define CHECKED_POINTS_SIZE 2
#endif

#define square(z) mul(z, z)

#define complexAbs(z_real, z_complex) simd_sqrt(add(square(z_real), square(z_imaginary)))

byte areInMandelbrotSet(v128_t c_real, v128_t c_imaginary) {
#if CHECKED_POINTS_SIZE == 4
    byte result = 0b11110000;
#else // CHECKED_POINTS_SIZE == 2
    byte result = 0b11000000;
#endif
    v128_t z_real = const_splat(0);
    v128_t z_imaginary = const_splat(0);
    for (int k = 0; k < N; k++) {
        v128_t cmp_result = gt(complexAbs(z_real, z_imaginary), const_splat(2));
        for (int i = 0; i < CHECKED_POINTS_SIZE; i++) {
            if (getNthBit(result, i) == 1 && ((TYPE*)&cmp_result)[i] != 0) {
                clearNthBit(result, i);
            }
        }
        v128_t next_z_real = add(sub(square(z_real), square(z_imaginary)), c_real); // (z_real^2 - z_imaginary^2) + c_real
        v128_t next_z_imaginary = add(mul(mul(z_real, z_imaginary), const_splat(2)), c_imaginary); // (2 * z_real * z_imaginary) + c_imaginary
        z_real = next_z_real;
        z_imaginary = next_z_imaginary;

        if (result == 0) {
            break;
        }
    }
    return result;
}

unsigned int runtime() {
    unsigned int setSize = 0;
    for (int i = 0; i < HIGHT; i++) {
        for (int j = 0; j < WIDTH; j+=CHECKED_POINTS_SIZE) {
#if CHECKED_POINTS_SIZE == 4
            v128_t real = add(mul(make(j, j+1, j+2, j+3), const_splat(ZOOM)), const_splat(REAL_AXIS_SHIFT));
            v128_t imaginary = add(mul(make(i, i, i, i), const_splat(ZOOM)), const_splat(IMAGINARY_AXIS_SHIFT));
#else // CHECKED_POINTS_SIZE == 2
            v128_t real = add(mul(make(j, j+1), const_splat(ZOOM)), const_splat(REAL_AXIS_SHIFT));
            v128_t imaginary = add(mul(make(i, i), const_splat(ZOOM)), const_splat(IMAGINARY_AXIS_SHIFT));
#endif
            byte pixels = areInMandelbrotSet(real, imaginary);
            for (int i = 0; i < CHECKED_POINTS_SIZE; i++) {
                if (getNthBit(pixels, i)) {
                    setSize++;
                }
            }
        }
    }
    return setSize;
}

int main() {
    printf("%u\n", runtime());
    return 0;
}

#endif /* SIMD_MANDELBROT_H */
