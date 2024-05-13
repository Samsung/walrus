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

uint32_t coins[] = {5, 10, 20, 50, 100, 200};

#define COINT_NUMBER sizeof(coins) / sizeof(uint32_t)
#define MONEY 155

/*
 * Return the smallest number of coins
 * by with the given money can be paid.
 *
 * Money shall be changeable with the given coins.
 */
uint32_t change(uint64_t money) {
    if (money == 0) {
        return 0;
    } else {
        uint32_t least_coins = UINT32_MAX;
        for (int i = 0; i < COINT_NUMBER; i++) {
            if (coins[i] > money) {
                continue;
            }
            if (1 + change(money - coins[i]) < least_coins) {
                least_coins = 1 + change(money - coins[i]);
            }
        }
        return least_coins;
    }
}

uint32_t runtime() {
    return change(MONEY);
}

int main() {
    printf("%u\n", (unsigned int) runtime());
    return 0;
}
