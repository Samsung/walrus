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
#include <string.h>

#include "include/memory.h"

// DEFINITIONS

typedef uint8_t byte;

byte runtime();

// TEST PARAMETER

#define ITERATIONS 7

char* message = "Lorem ipsum dolor sit amet, et wisi primis duo."
                "In quo erat tritani fuisset, no ullum vivendo "
                "torquatos quo, ne percipit convenire similique sed. "
                "At libris regione discere est, nec et dicam denique, "
                "option appellantur his cu. Sit fabulas invidunt "
                "maiestatis id. Ea nec probo oratio dictas, eu "
                "officiis sensibus has. Id sit facer fierent, eu "
                "illum iudicabit vel, ei vim tibique accumsan percipit.\n"
                "Ius illum perpetua vituperata ne, sint dolorem "
                "noluisse eu sea. Quod hendrerit conclusionemque te qui, "
                "id ius prima signiferumque necessitatibus. Et quo "
                "mollis accusam verterem. Eu quod atqui detraxit per, "
                "malis inimicus pri no, ius ad tamquam virtute vulputate. "
                "Magna graece vel eu, nam eu quando maiorum.\n"
                "Per partem detraxit senserit et, in esse constituto mea. "
                "Vis id odio dico dicat, no eam nonumy sensibus "
                "mediocritatem, malis ludus definiebas te vel. Vis ut "
                "elit virtute, ad commune voluptatibus conclusionemque has. "
                "Nam omnes nullam iudicabit at, assum errem doctus ad "
                "pro, erant persius ad pri. Ea phaedrum accusata "
                "liberavisse nec, deleniti offendit luptatum ex sit.\n"
                "In periculis forensibus nec, nisl verear vituperatoribus "
                "in eam, autem molestie dissentiet mea et. Eam an exerci "
                "ridens intellegebat, ne principes voluptatibus mea, "
                "novum sapientem ex nam. Illum admodum invidunt quo et. "
                "His noster officiis voluptatum no, vim in deserunt "
                "ullamcorper, nec cibo mundi ad.\n"
                "Pro te soleat inciderint, apeirian assentior "
                "referrentur usu eu, te brute tincidunt eam. Solet "
                "viderer eam ut. Duo eu iuvaret principes sententiae, "
                "te eum quod blandit. Vim ad legimus intellegebat "
                "disputationi, an ridens nonumes deterruisset sed. Eos "
                "dicunt assentior eu, mel ne case postulant, "
                "quando feugiat voluptaria eam ne.\n";


// HUFFMAN CODING DEFINITIONS

#define LEFT_DIGIT 0
#define RIGHT_DIGIT 1

typedef struct {
    byte letter;
    uint32_t code;
    byte codeLength; // in bit
} letterCode;

typedef struct {
    letterCode* letterCodes;
    size_t letterCodesSize; // in pieces
    byte* compressed;
    size_t compressedSize; // in bit
} zip;

#define NO_ERROR 0
#define ERROR_OCCURED 1

/**
 * @brief encode input with Huffman Coding algorithm
 * @param input the input byte string
 * @param inputLength the length of the input in bytes
 * @param output the address of the zip pointer for the output
 * @param error error stream
*/
void encodeHuffman(byte* input, size_t inputLength, zip** output, byte* error);

/**
 * @brief decode coded data with Huffman Coding algorithm
 * @param coded a pointer to a coded data
 * @param output an address for a pointer for the output
 * @param outputLength the length of the output (in bytes) will be stored here
 * @param error error stream
*/
void decodeHuffman(zip* coded, byte** output, size_t* outputLength, byte* error);

// IMPLEMENTATIONS

int main() {
    printf("%u\n", runtime());
    return 0;
}

byte runtime() {
    for (size_t i = 0; i < ITERATIONS; i++) {
        zip* encoded = NULL;
        byte error;
        encodeHuffman((byte*)message, strlen(message) + 1, &encoded, &error);
        if (error == ERROR_OCCURED) {
            return 2;
        }
        char* decoded = NULL;
        size_t decodedSize;
        decodeHuffman(encoded, (byte**)&decoded, &decodedSize, &error);
        if (error == ERROR_OCCURED) {
            return 2;
        }
        if (strcmp(message, decoded) != 0) {
            return 1;
        }
        freeMemory(encoded->letterCodes);
        freeMemory(encoded->compressed);
        freeMemory(encoded);
        freeMemory(decoded);
    }
    return 0;
}

struct element {
        struct element* left;   // less or equal
        struct element* right;  // greater
        byte letter;            // use only if left and right are NULL
        size_t occurance;       // use only if left and right are NULL
};

size_t getOccuranceSum(struct element* e) {
    if (e->left == NULL && e->right == NULL) {
        return e->occurance;
    } else {
        return getOccuranceSum(e->left) + getOccuranceSum(e->right);
    }
}

void countLetters(byte* input, size_t inputLength, struct element** output, size_t* outputLength, byte* error) {
    *error = NO_ERROR;
    // 1st character
    (*output) = (struct element*)allocateMemory(1 * sizeof(struct element));
    if ((*output) == NULL) {
        *error = ERROR_OCCURED;
        return;
    }
    (*output)[0].left      = NULL;
    (*output)[0].right     = NULL;
    (*output)[0].letter    = input[0];
    (*output)[0].occurance = 1;
    (*outputLength) = 1;
    // the rest
    for (size_t i = 1; i < inputLength; i++) {
        // If it's in the list, then increment its value by on.e
        bool found = false;
        for (size_t j = 0; j < (*outputLength); j++) {
            if ((*output)[j].letter == input[i]) {
                found = true;
                (*output)[j].occurance++;
                break;
            }
        }
        if (found == true) {
            continue;
        }
        // If it's not in the list, then add it to the list.
        (*output) = (struct element*)reallocateMemory((*output), ((*outputLength) + 1) * sizeof(struct element));
        if ((*output) == NULL) {
            *error = ERROR_OCCURED;
            return;
        }
        (*output)[(*outputLength)].left      = NULL;
        (*output)[(*outputLength)].right     = NULL;
        (*output)[(*outputLength)].letter    = input[i];
        (*output)[(*outputLength)].occurance = 1;
        (*outputLength)++;
    }
}

void orderElements(struct element* elements, size_t elementsSize) {
    bool wasSwap;
    size_t done = 0;
    do {
        wasSwap = false;
        for (size_t i = 1; i < elementsSize - done; i++) {
            if (getOccuranceSum(&elements[i - 1]) < getOccuranceSum(&elements[i])) {
                struct element temp = elements[i - 1];
                elements[i - 1] = elements[i];
                elements[i] = temp;
                wasSwap = true;
            }
        }
        done++;
    } while(wasSwap == true);
}

void unifyLastTwo(struct element** elements, size_t* elementsSize, byte* error) {
    *error = NO_ERROR;
    struct element new;
    new.left = (struct element*)allocateMemory(sizeof(struct element));
    if (new.left == NULL) {
        *error = ERROR_OCCURED;
        return;
    }
    new.right = (struct element*)allocateMemory(sizeof(struct element));
    if (new.right == NULL) {
        *error = ERROR_OCCURED;
        return;
    }
    *(new.left)  = (*elements)[(*elementsSize) - 2];
    *(new.right) = (*elements)[(*elementsSize) - 1];
    (*elements) = (struct element*)reallocateMemory((*elements), ((*elementsSize) - 1) * sizeof(struct element));
    (*elementsSize)--;
    (*elements)[(*elementsSize) - 1] = new;
}

uint32_t appendBit(uint32_t code, byte codeLength, byte newDigit, byte* error) {
    *error = NO_ERROR;
    if (codeLength >= 32) {
        *error = ERROR_OCCURED;
        return 0;
    }
    return code | (newDigit << (32 - 1 - codeLength));
}

void elementsToCodeTable(struct element* e, zip* output, uint32_t code, byte codeLength, byte* error) {
    *error = NO_ERROR;
    if (e->left == NULL && e->right == NULL) {
        (*output).letterCodes[(*output).letterCodesSize].letter = e->letter;
        (*output).letterCodes[(*output).letterCodesSize].code = code;
        (*output).letterCodes[(*output).letterCodesSize].codeLength = codeLength;
        (*output).letterCodesSize++;
    } else {
        uint32_t newCode;
        newCode = appendBit(code, codeLength, RIGHT_DIGIT, error);
        if (*error == ERROR_OCCURED) {
            return;
        }
        elementsToCodeTable(e->right, output, newCode, codeLength + 1, error);
        if (*error == ERROR_OCCURED) {
            return;
        }
        newCode = appendBit(code, codeLength, LEFT_DIGIT, error);
        if (*error == ERROR_OCCURED) {
            return;
        }
        elementsToCodeTable(e->left, output, newCode, codeLength + 1, error);
        if (*error == ERROR_OCCURED) {
            return;
        }
    }
}

void deleteElement(struct element* e) {
    if (e->left != NULL) {
        deleteElement(e->left);
    }
    if (e->right != NULL) {
        deleteElement(e->right);
    }
    freeMemory(e);
}

void createCodeTable(byte* input, size_t inputLength, zip** output, byte* error) {
    *error = NO_ERROR;
    struct element* elements = NULL;
    size_t elementsSize = 0;
    countLetters(input, inputLength, &elements, &elementsSize, error);
    if (*error == ERROR_OCCURED) {
        return;
    }
    (*output)->letterCodes = (letterCode*)CAllocateMemory(elementsSize * sizeof(letterCode));
    if ((*output)->letterCodes == NULL) {
        *error = ERROR_OCCURED;
        return;
    }
    while (elementsSize > 1) {
        orderElements(elements, elementsSize);
        unifyLastTwo(&elements, &elementsSize, error);
        if (*error == ERROR_OCCURED) {
        return;
        }
    }
    (*output)->letterCodesSize = 0;
    elementsToCodeTable(&(elements[0]), (*output), 0, 0, error);
    if (*error == ERROR_OCCURED) {
        return;
    }
    deleteElement(&(elements[0]));
}

void codeInput(byte* input, size_t inputLength, zip** output, byte* error) {
    *error = NO_ERROR;
    (*output)->compressed = NULL;
    (*output)->compressedSize = 0;
    for (size_t i = 0; i < inputLength; i++) {
        // Finding the current letter in the table
        size_t j = 0;
        while ((*output)->letterCodes[j].letter != input[i]) {
            j++;
        }
        // Print each digit individually
        for (size_t k = 0; k < (*output)->letterCodes[j].codeLength; k++) {
            if ((*output)->compressedSize % 8 == 0) {
                (*output)->compressed = (byte*)reallocateMemory((*output)->compressed, ((*output)->compressedSize / 8) + 1);
                if ((*output)->compressed == NULL) {
                    *error = ERROR_OCCURED;
                    return;
                }
                (*output)->compressed[(*output)->compressedSize / 8] = 0;
            }
            if (((*output)->letterCodes[j].code & (1 << (32 - 1 - k))) > 0) {
                (*output)->compressed[(*output)->compressedSize / 8] |= (1 << (8 - 1 - (*output)->compressedSize % 8));
            }
            (*output)->compressedSize++;
        }
    }
}

void encodeHuffman(byte* input, size_t inputLength, zip** output, byte* error) {
    *error = NO_ERROR;
    if (input == NULL || inputLength == 0) {
        (*output) = NULL;
        return;
    }
    (*output) = (zip*)CAllocateMemory(sizeof(zip));
    if ((*output) == NULL) {
        *error = ERROR_OCCURED;
        return;
    }
    createCodeTable(input, inputLength, output, error);
    if (*error == ERROR_OCCURED) {
        return;
    }
    codeInput(input, inputLength, output, error);
}

void decodeHuffman(zip* coded, byte** output, size_t* outputLength, byte* error) {
    *error = NO_ERROR;
    // Early Return
    if (coded == NULL) {
        return;
    }
    (*output) = NULL;
    (*outputLength) = 0;
    for (size_t i = 0; i < coded->compressedSize; i++) {
        static uint32_t word = 0;
        static byte wordLength = 0;

        // loading given bit
        if ((coded->compressed[i / 8] & (1 <<(8 - 1 - (i % 8)))) > 0) {
            word |= (1 << (32 - 1 - wordLength));
        }
        wordLength++;
        for (size_t j = 0; j < coded->letterCodesSize; j++) {
            if (coded->letterCodes[j].code == word && coded->letterCodes[j].codeLength == wordLength) {
                // add letter
                (*output) = reallocateMemory((*output), (*outputLength) + 1);
                if ((*output) == NULL) {
                    *error = ERROR_OCCURED;
                    return;
                }
                (*output)[(*outputLength)] = coded->letterCodes[j].letter;
                (*outputLength)++;

                word = 0;
                wordLength = 0;
                break;
            }
        }
    }
}
