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

#include <stdio.h>

#define N 128

#define ITERATION 400

double A[N][N];
double b[N];
double x[N];

void init(int offset)
{
    for (int i = 0; i < N; i++) {
        b[i] = 1.0 + (double)((i + offset) % N) / N;
        for (int j = 0; j < N; j++) {
            A[i][j] = (double)((i * j + offset) % N) / N;
        }
        A[i][i] += (double)N;
    }
}

double gaussianElimination()
{
    for (int k = 0; k < N - 1; k++) {
        int pivot = k;
        double max = A[k][k] < 0 ? -A[k][k] : A[k][k];
        for (int i = k + 1; i < N; i++) {
            double value = A[i][k] < 0 ? -A[i][k] : A[i][k];
            if (value > max) {
                max = value;
                pivot = i;
            }
        }
        if (pivot != k) {
            for (int j = k; j < N; j++) {
                double tmp = A[k][j];
                A[k][j] = A[pivot][j];
                A[pivot][j] = tmp;
            }
            double tmp = b[k];
            b[k] = b[pivot];
            b[pivot] = tmp;
        }
        for (int i = k + 1; i < N; i++) {
            double factor = A[i][k] / A[k][k];
            for (int j = k; j < N; j++) {
                A[i][j] -= factor * A[k][j];
            }
            b[i] -= factor * b[k];
        }
    }

    /* back substitution */
    for (int i = N - 1; i >= 0; i--) {
        double sum = b[i];
        for (int j = i + 1; j < N; j++) {
            sum -= A[i][j] * x[j];
        }
        x[i] = sum / A[i][i];
    }

    double checksum = 0.0;
    for (int i = 0; i < N; i++) {
        checksum += x[i];
    }
    return checksum;
}

double runtime()
{
    double sum = 0.0;
    for (int i = 0; i < ITERATION; i++) {
        init(i);
        sum += gaussianElimination();
    }
    return sum;
}

int main()
{
    printf("%.8lf\n", runtime());
    return 0;
}
