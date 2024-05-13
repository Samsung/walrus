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
#include <stdbool.h>

// This test generates a solution to the N Queens problem, a variation of 8 Queens problem.
// Description of problem: https://en.wikipedia.org/wiki/Eight_queens_puzzle

#define N 25
#define LOOP 3
uint8_t board[N][N];

void print_board(uint8_t board[N][N])
{
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            printf("%d", board[i][j]);
        }
    }
}

bool is_safe(uint8_t board[N][N], unsigned row, unsigned col)
{
    for (int i = 0; i < N; ++i) {
        if (board[row][i] == 1)
            return false;
    }

    for (int i = 0; i < N; ++i) {
        if (board[i][col] == 1)
            return false;
    }

    for (int i = row, j = col; i >= 0 && j >= 0; --i, --j) {
        if (board[i][j] == 1)
            return false;
    }

    for (int i = row, j = col; j >= 0 && i < N; ++i, --j) {
        if (board[i][j] == 1)
            return false;
    }

    for (int i = row, j = col; j < N && i >= 0; --i, ++j) {
        if (board[i][j] == 1)
            return false;
    }

    for (int i = row, j = col; j < N && i < N; ++i, ++j) {
        if (board[i][j] == 1)
            return false;
    }

    return true;
}

bool nqueens(uint8_t board[N][N], int32_t col)
{
    if (col >= N) {
        return true;
    }

    for (int i = 0; i < N; ++i) {
        if (is_safe(board, i, col)) {
            board[i][col] = 1;
            if (nqueens(board, col + 1)) {
                return true;
            }
            board[i][col] = 0;
        }
    }

    return false;
}

uint8_t runtime()
{
    uint8_t retVal = 5;
    for (uint8_t l = 0; l < LOOP; l++) {
        for (uint8_t c = 0; c < 5; c++) {
            int32_t start_col = 0;
            for (int i = 0; i < N; i++) {
                for (int j = 0; j < N; j++) {
                    board[i][j] = 0;
                }
            }

            bool wasBreak = false;

            while (true) {
                if (nqueens(board, start_col)) {
                    retVal--;
                    wasBreak = true;
                    break;
                } else if (start_col < N) {
                    start_col++;
                } else {
                    retVal++;
                    wasBreak = true;
                    break;
                }
            }

            if (!wasBreak) {
                retVal++;
            }
        }
    }

    return retVal;
}

int main()
{
    printf("%u\n", runtime());
    return 0;
}
