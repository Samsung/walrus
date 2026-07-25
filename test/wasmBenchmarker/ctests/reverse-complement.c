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
#include <ctype.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "include/fastaInput.h"

#define EXPECTED_OUTPUT_SIZE 10245

const char* input =
    FASTA_ONE_INPUT
    FASTA_TWO_INPUT
    FASTA_THREE_INPUT;

const char *pairs = "ATCGGCTAUAMKRYWWSSYRKMVBHDDHBVNN";
char tbl[256];
char output[EXPECTED_OUTPUT_SIZE + 1];


const uint64_t EXPECTED_OUTPUT_HASH = UINT64_C(0x8cae2ea7900458e4);

char* change(const char* from, const char* to, char* output_position) {
    while (*from != '\n') {
        *output_position++ = *from++;
    }

    *output_position++ = *from++;

    if (to > from && to[-1] == '\n') {
        to--;
    }

    size_t len = 0;

    while (to > from) {
        to--;

        *output_position++ = tbl[(unsigned char)* to];
        len++;

        if (len == 60) {
            *output_position++ = '\n';
            len = 0;
        }
    }
    
    if (len != 0) {
        *output_position++ = '\n';
    }

    return output_position;
}

uint64_t hashOutput(const char *data, size_t length) {
    uint64_t hash = UINT64_C(14695981039346656037);

    for (size_t i = 0; i < length; i++) {
        hash ^= (unsigned char)data[i];
        hash *= UINT64_C(1099511628211);
    }

    return hash;
}

bool check() {
    size_t length = strlen(output);

    return length == EXPECTED_OUTPUT_SIZE && hashOutput(output, length) == EXPECTED_OUTPUT_HASH;
}

bool runtime() {

    for (const char* s = pairs; *s != '\0'; s += 2) {
        tbl[toupper((unsigned int) s[0])] = s[1];
        tbl[tolower((unsigned int) s[0])] = s[1];
    }

    const char* from = input;
    const char* end = input + strlen(input);
    char* output_position = output;

    while (from < end ) {
        const char* to = strstr(from + 1, "\n>");

        if (to == NULL) {
            output_position = change(from, end, output_position);
            break;
        }

        output_position = change(from, to + 1, output_position);
        from = to + 1;
    }
    
    *output_position = '\0';

    return check();
}

int main() {
    printf("%u\n", (unsigned int) runtime());
    return 0;
}

