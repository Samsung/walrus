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

// 4x4 square matrix
#define MATRIX_SIZE 16

void multiply_scalar(const double m1[], const double m2[], double out_m[])
{
	/* unrolled matrix multiplication */
    double a00 = m1[0];
    double a01 = m1[1];
    double a02 = m1[2];
    double a03 = m1[3];
    double a10 = m1[4];
    double a11 = m1[5];
    double a12 = m1[6];
    double a13 = m1[7];
    double a20 = m1[8];
    double a21 = m1[9];
    double a22 = m1[10];
    double a23 = m1[11];
    double a30 = m1[12];
    double a31 = m1[13];
    double a32 = m1[14];
    double a33 = m1[15];

    double b0 = m2[0];
    double b1 = m2[1];
    double b2 = m2[2];
    double b3 = m2[3];
    out_m[0] = b0 * a00 + b1 * a10 + b2 * a20 + b3 * a30;
    out_m[1] = b0 * a01 + b1 * a11 + b2 * a21 + b3 * a31;
    out_m[2] = b0 * a02 + b1 * a12 + b2 * a22 + b3 * a32;
    out_m[3] = b0 * a03 + b1 * a13 + b2 * a23 + b3 * a33;

    b0 = m2[4];
    b1 = m2[5];
    b2 = m2[6];
    b3 = m2[7];
    out_m[4] = b0 * a00 + b1 * a10 + b2 * a20 + b3 * a30;
    out_m[5] = b0 * a01 + b1 * a11 + b2 * a21 + b3 * a31;
    out_m[6] = b0 * a02 + b1 * a12 + b2 * a22 + b3 * a32;
    out_m[7] = b0 * a03 + b1 * a13 + b2 * a23 + b3 * a33;

    b0 = m2[8];
    b1 = m2[9];
    b2 = m2[10];
    b3 = m2[11];
    out_m[8] = b0 * a00 + b1 * a10 + b2 * a20 + b3 * a30;
    out_m[9] = b0 * a01 + b1 * a11 + b2 * a21 + b3 * a31;
    out_m[10] = b0 * a02 + b1 * a12 + b2 * a22 + b3 * a32;
    out_m[11] = b0 * a03 + b1 * a13 + b2 * a23 + b3 * a33;

    b0 = m2[12];
    b1 = m2[13];
    b2 = m2[14];
    b3 = m2[15];
    out_m[12] = b0 * a00 + b1 * a10 + b2 * a20 + b3 * a30;
    out_m[13] = b0 * a01 + b1 * a11 + b2 * a21 + b3 * a31;
    out_m[14] = b0 * a02 + b1 * a12 + b2 * a22 + b3 * a32;
    out_m[15] = b0 * a03 + b1 * a13 + b2 * a23 + b3 * a33;
}

double runtime()
{
    double m1[MATRIX_SIZE];
    double m2[MATRIX_SIZE];
    double out[MATRIX_SIZE];
    double sum=0;

    for (int i = 0; i < MATRIX_SIZE; i++) {
        m1[i] = (double)i;
        m2[i] = (double)i;
    }
    multiply_scalar(m1, m2, out);
    for (int i = 0; i < MATRIX_SIZE; i++) {
	    sum += out[i];
    }
    return sum;
}
