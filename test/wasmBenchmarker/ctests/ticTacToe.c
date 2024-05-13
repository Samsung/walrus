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

#define ITERATION 200

typedef enum {
    X,
    O,
    nobody
} Player;

int iteration = 0;

Player whoWillWin(const Player map[9], Player currentPlayer)  {
    iteration++;

    if (map[0] != nobody && map[0] == map[1] && map[0] == map[2]) return map[0]; // 1st row
    if (map[3] != nobody && map[3] == map[4] && map[3] == map[5]) return map[3]; // 2nd row
    if (map[6] != nobody && map[6] == map[7] && map[6] == map[8]) return map[6]; // 3rd row
    if (map[0] != nobody && map[0] == map[3] && map[0] == map[6]) return map[0]; // 1st column
    if (map[1] != nobody && map[1] == map[4] && map[1] == map[7]) return map[1]; // 2nd column
    if (map[2] != nobody && map[2] == map[5] && map[2] == map[8]) return map[2]; // 3rd column
    if (map[0] != nobody && map[0] == map[4] && map[0] == map[8]) return map[0]; // cross
    if (map[2] != nobody && map[2] == map[4] && map[2] == map[6]) return map[2]; // cross

    bool isThereFree = false;
    for (int i = 0; i < 9; i++) {
        if (map[i] == nobody) {
            isThereFree = true;
            break;
        }
    }
    if (isThereFree == false) {
        return nobody;
    }

    Player newMap[9];
    Player enemy = (currentPlayer == X) ? O : X;
    Player bestChoice = enemy;
    for (int i = 0; i < 9; i++) {
        newMap[i] = map[i];
    }
    for (int i = 0; i < 9; i++) {
        if (map[i] == nobody) {
            newMap[i] = currentPlayer;
            Player result = whoWillWin(newMap, enemy);
            if (result == nobody) {
                bestChoice = nobody;
            } else if (result == currentPlayer) {
                return currentPlayer;
            }
            newMap[i] = nobody;
        }
    }
    return bestChoice;
}

int runtime() {
    Player map[9] = {   nobody, nobody, nobody,
                        nobody, nobody, nobody,
                        nobody, nobody, nobody};
    for (int i = 0; i < ITERATION; i++) {
        if (whoWillWin(map, X) != nobody) {
            return -1;
        }
    }
    return iteration;
}

int main() {
    printf("%d\n", runtime());
    return 0;
}
