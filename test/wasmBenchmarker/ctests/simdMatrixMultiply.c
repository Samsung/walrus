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
 *
 * Adapted from https://github.com/ngzhian/simd-benchmarks */

#include <stdio.h>
#include <stdint.h>
#include <wasm_simd128.h>

// 4x4 square matrix
#define MATRIX_SIZE 16

#define ITERATION 3500000

void multiply_simd(const double m1[], const double m2[], double out_m[])
{
    v128_t a0 = wasm_v128_load(m1 + 0);
    v128_t a1 = wasm_v128_load(m1 + 2);
    v128_t a2 = wasm_v128_load(m1 + 4);
    v128_t a3 = wasm_v128_load(m1 + 6);
    v128_t a4 = wasm_v128_load(m1 + 8);
    v128_t a5 = wasm_v128_load(m1 + 10);
    v128_t a6 = wasm_v128_load(m1 + 12);
    v128_t a7 = wasm_v128_load(m1 + 14);

    v128_t b0 = wasm_v128_load(m2 + 0);
    v128_t b1 = wasm_v128_load(m2 + 2);

    wasm_v128_store(out_m + 0,
                    wasm_f64x2_add(
                        wasm_f64x2_mul(wasm_f64x2_splat(wasm_f64x2_extract_lane(b0, 0)), a0),
                        wasm_f64x2_add(
                            wasm_f64x2_mul(wasm_f64x2_splat(wasm_f64x2_extract_lane(b0, 1)), a2),
                            wasm_f64x2_add(
                                wasm_f64x2_mul(wasm_f64x2_splat(wasm_f64x2_extract_lane(b1, 0)), a4),
                                wasm_f64x2_mul(wasm_f64x2_splat(wasm_f64x2_extract_lane(b1, 1)), a6)))));
    wasm_v128_store(out_m + 2,
                    wasm_f64x2_add(
                        wasm_f64x2_mul(wasm_f64x2_splat(wasm_f64x2_extract_lane(b0, 0)), a1),
                        wasm_f64x2_add(
                            wasm_f64x2_mul(wasm_f64x2_splat(wasm_f64x2_extract_lane(b0, 1)), a3),
                            wasm_f64x2_add(
                                wasm_f64x2_mul(wasm_f64x2_splat(wasm_f64x2_extract_lane(b1, 0)), a5),
                                wasm_f64x2_mul(wasm_f64x2_splat(wasm_f64x2_extract_lane(b1, 1)), a7)))));

    v128_t b2 = wasm_v128_load(m2 + 4);
    v128_t b3 = wasm_v128_load(m2 + 6);

    wasm_v128_store(out_m + 4,
                    wasm_f64x2_add(
                        wasm_f64x2_mul(wasm_f64x2_splat(wasm_f64x2_extract_lane(b2, 0)), a0),
                        wasm_f64x2_add(
                            wasm_f64x2_mul(wasm_f64x2_splat(wasm_f64x2_extract_lane(b2, 1)), a2),
                            wasm_f64x2_add(
                                wasm_f64x2_mul(wasm_f64x2_splat(wasm_f64x2_extract_lane(b3, 0)), a4),
                                wasm_f64x2_mul(wasm_f64x2_splat(wasm_f64x2_extract_lane(b3, 1)), a6)))));
    wasm_v128_store(out_m + 6,
                    wasm_f64x2_add(
                        wasm_f64x2_mul(wasm_f64x2_splat(wasm_f64x2_extract_lane(b2, 0)), a1),
                        wasm_f64x2_add(
                            wasm_f64x2_mul(wasm_f64x2_splat(wasm_f64x2_extract_lane(b2, 1)), a3),
                            wasm_f64x2_add(
                                wasm_f64x2_mul(wasm_f64x2_splat(wasm_f64x2_extract_lane(b3, 0)), a5),
                                wasm_f64x2_mul(wasm_f64x2_splat(wasm_f64x2_extract_lane(b3, 1)), a7)))));

    v128_t b4 = wasm_v128_load(m2 + 8);
    v128_t b5 = wasm_v128_load(m2 + 10);

    wasm_v128_store(out_m + 8,
                    wasm_f64x2_add(
                        wasm_f64x2_mul(wasm_f64x2_splat(wasm_f64x2_extract_lane(b4, 0)), a0),
                        wasm_f64x2_add(
                            wasm_f64x2_mul(wasm_f64x2_splat(wasm_f64x2_extract_lane(b4, 1)), a2),
                            wasm_f64x2_add(
                                wasm_f64x2_mul(wasm_f64x2_splat(wasm_f64x2_extract_lane(b5, 0)), a4),
                                wasm_f64x2_mul(wasm_f64x2_splat(wasm_f64x2_extract_lane(b5, 1)), a6)))));
    wasm_v128_store(out_m + 10,
                    wasm_f64x2_add(
                        wasm_f64x2_mul(wasm_f64x2_splat(wasm_f64x2_extract_lane(b4, 0)), a1),
                        wasm_f64x2_add(
                            wasm_f64x2_mul(wasm_f64x2_splat(wasm_f64x2_extract_lane(b4, 1)), a3),
                            wasm_f64x2_add(
                                wasm_f64x2_mul(wasm_f64x2_splat(wasm_f64x2_extract_lane(b5, 0)), a5),
                                wasm_f64x2_mul(wasm_f64x2_splat(wasm_f64x2_extract_lane(b5, 1)), a7)))));

    v128_t b6 = wasm_v128_load(m2 + 12);
    v128_t b7 = wasm_v128_load(m2 + 14);

    wasm_v128_store(out_m + 12,
                    wasm_f64x2_add(
                        wasm_f64x2_mul(wasm_f64x2_splat(wasm_f64x2_extract_lane(b6, 0)), a0),
                        wasm_f64x2_add(
                            wasm_f64x2_mul(wasm_f64x2_splat(wasm_f64x2_extract_lane(b6, 1)), a2),
                            wasm_f64x2_add(
                                wasm_f64x2_mul(wasm_f64x2_splat(wasm_f64x2_extract_lane(b7, 0)), a4),
                                wasm_f64x2_mul(wasm_f64x2_splat(wasm_f64x2_extract_lane(b7, 1)), a6)))));
    wasm_v128_store(out_m + 14,
                    wasm_f64x2_add(
                        wasm_f64x2_mul(wasm_f64x2_splat(wasm_f64x2_extract_lane(b6, 0)), a1),
                        wasm_f64x2_add(
                            wasm_f64x2_mul(wasm_f64x2_splat(wasm_f64x2_extract_lane(b6, 1)), a3),
                            wasm_f64x2_add(
                                wasm_f64x2_mul(wasm_f64x2_splat(wasm_f64x2_extract_lane(b7, 0)), a5),
                                wasm_f64x2_mul(wasm_f64x2_splat(wasm_f64x2_extract_lane(b7, 1)), a7)))));
}

double runtime()
{
    double m1[MATRIX_SIZE];
    double m2[MATRIX_SIZE];
    double out[MATRIX_SIZE];
    for (int i = 0; i < MATRIX_SIZE; i++) {
        m1[i] = (double)i;
        m2[i] = (double)i;
    }
    double sum;
    for (unsigned int i = 0; i < ITERATION; i++) {
        sum = 0;
        multiply_simd(m1, m2, out);
        for (int i = 0; i < MATRIX_SIZE; i++) {
            sum += out[i];
        }
    }
    return sum;
}
