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

extern const char *input_3;

typedef struct {
    char key[50];
    uint64_t value;
} record;

record records[100];

uint8_t record_current_size = 0;

/**
 * Increment the value of the record
 * with one in the records variable
 * where the key is identical with
 * the input.
 *
 * If the there is no record with the
 * given key then a new one will be
 * created with one.
 */
void incrementRecord(const char *key);

/**
 * Return the value of the record
 * whose key is identical with the
 * parameter.
 *
 * If there is no record with the given
 * key them a new one will be created
 * with zero.
*/
uint64_t getValue(const char *key);

/**
 * Return True if the two string are indentical
 * (up to "the end of string" characters)
*/
bool isIdentical(const char *A, const char *B);

/**
 * Copy _qunatity_ characters from _from_ string
 * to to_ string.
*/
void copyStringQ(const char *from, char *to, uint8_t quantity);

/**
 * Copy string to the end of string character.
*/
void copyString(const char *from, char *to);

/**
 * Returns the length of the given string.
 * It has to end with the end of string
 * character.
*/
uint32_t stringLength(const char *string);

/**
 * Count the orrcurance of all _length_
 * length words.
 * @param input the input string
 * @param length the size of the words
*/
void frequency(const char *input, uint8_t length);

/**
 * Count the occurance of the pattern string
 * in the input string up to the end of string
 * character.
*/
void count(const char *input, const char *pattern);

/**
 * Check the result of the test.
 * @return true if successfull, else it's false
*/
bool check();

bool runtime();

int main() {
    printf("%u\n", (unsigned int) runtime());
    return 0;
}

void incrementRecord(const char *key) {
    for (uint8_t i = 0; i < record_current_size; i++) {
        if (isIdentical(key, records[i].key)) {
            records[i].value += 1;
            return;
        }
    }
    copyString(key, records[record_current_size].key);
    records[record_current_size].value = 1;
    record_current_size += 1;
}

uint64_t getValue(const char *key) {
    for (uint8_t i = 0; i < record_current_size; i++) {
        if (isIdentical(key, records[i].key)) {
            return records[i].value;
        }
    }
    copyString(key, records[record_current_size].key);
    records[record_current_size].value = 0;
    record_current_size += 1;
    return 0;
}

bool isIdentical(const char *A, const char *B) {
    uint64_t i = 0;
    while (A[i] != 0 || B[i] != 0) {
        if (A[i] != B[i]) {
            return false;
        }
        i += 1;
    }
    if (A[i] != B[i]) {
        return false;
    }
    return true;
}

void copyStringQ(const char *from, char *to, uint8_t quantity) {
    for (uint8_t i = 0; i < quantity; i++) {
        to[i] = from[i];
    }
    to[quantity] = 0;
}

void copyString(const char *from, char *to) {
    uint8_t i = 0;
    while (from[i] != 0) {
        to[i] = from[i];
        i += 1;
    }
    to[i] = 0;
}

uint32_t stringLength(const char *string) {
    uint32_t i = 0;
    while (string[i] != 0) {
        i += 1;
    }
    return i;
}

void frequency(const char *input, uint8_t length) {
    for (uint32_t i = 0; i < stringLength(input) + 1 - length; i++) {
        char temp[50];
        copyStringQ(&(input[i]), temp, length);
        incrementRecord(temp);
    }
}

void count(const char *input, const char *pattern) {
    if (stringLength(pattern) == 0) {
        return;
    }
    for (uint32_t i = 0; i < stringLength(input) + 1 - stringLength(pattern); i++) {
        char temp[50];
        copyStringQ(&(input[i]), temp, stringLength(pattern));
        if (isIdentical(pattern, temp)) {
            incrementRecord(pattern);
        }
    }
}

bool check() {
    if (
            getValue("a") == 1480 &&
            getValue("c") == 974 &&
            getValue("g") == 970 &&
            getValue("t") == 1576 &&
            getValue("aa") == 420 &&
            getValue("ac") == 272 &&
            getValue("ag") == 292 &&
            getValue("at") == 496 &&
            getValue("ca") == 273 &&
            getValue("cc") == 202 &&
            getValue("cg") == 201 &&
            getValue("ga") == 316 &&
            getValue("gc") == 185 &&
            getValue("gg") == 167 &&
            getValue("gt") == 302 &&
            getValue("ta") == 470 &&
            getValue("tc") == 315 &&
            getValue("tg") == 310 &&
            getValue("tt") == 480 &&
            getValue("ggt") == 54 &&
            getValue("ggta") == 24 &&
            getValue("ggtatt") == 4 &&
            getValue("ggtattttaatt") == 0 &&
            getValue("ggtattttaatttatagt") == 0
            ) {
        return true;
    } else {
        return false;
    }
}

bool runtime() {
    frequency(input_3, 1);
    frequency(input_3, 2);
    count(input_3, "ggt");
    count(input_3, "ggta");
    count(input_3, "ggtatt");
    count(input_3, "ggtattttaatt");
    count(input_3, "ggtattttaatttatagt");
    return check();
}

const char *input_3 =
        /* ">THREE Homo sapiens frequency" */
        "aacacttcaccaggtatcgtgaaggctcaagattacccagagaacctttgcaatataaga"
        "atatgtatgcagcattaccctaagtaattatattctttttctgactcaaagtgacaagcc"
        "ctagtgtatattaaatcggtatatttgggaaattcctcaaactatcctaatcaggtagcc"
        "atgaaagtgatcaaaaaagttcgtacttataccatacatgaattctggccaagtaaaaaa"
        "tagattgcgcaaaattcgtaccttaagtctctcgccaagatattaggatcctattactca"
        "tatcgtgtttttctttattgccgccatccccggagtatctcacccatccttctcttaaag"
        "gcctaatattacctatgcaaataaacatatattgttgaaaattgagaacctgatcgtgat"
        "tcttatgtgtaccatatgtatagtaatcacgcgactatatagtgctttagtatcgcccgt"
        "gggtgagtgaatattctgggctagcgtgagatagtttcttgtcctaatatttttcagatc"
        "gaatagcttctatttttgtgtttattgacatatgtcgaaactccttactcagtgaaagtc"
        "atgaccagatccacgaacaatcttcggaatcagtctcgttttacggcggaatcttgagtc"
        "taacttatatcccgtcgcttactttctaacaccccttatgtatttttaaaattacgttta"
        "ttcgaacgtacttggcggaagcgttattttttgaagtaagttacattgggcagactcttg"
        "acattttcgatacgactttctttcatccatcacaggactcgttcgtattgatatcagaag"
        "ctcgtgatgattagttgtcttctttaccaatactttgaggcctattctgcgaaatttttg"
        "ttgccctgcgaacttcacataccaaggaacacctcgcaacatgccttcatatccatcgtt"
        "cattgtaattcttacacaatgaatcctaagtaattacatccctgcgtaaaagatggtagg"
        "ggcactgaggatatattaccaagcatttagttatgagtaatcagcaatgtttcttgtatt"
        "aagttctctaaaatagttacatcgtaatgttatctcgggttccgcgaataaacgagatag"
        "attcattatatatggccctaagcaaaaacctcctcgtattctgttggtaattagaatcac"
        "acaatacgggttgagatattaattatttgtagtacgaagagatataaaaagatgaacaat"
        "tactcaagtcaagatgtatacgggatttataataaaaatcgggtagagatctgctttgca"
        "attcagacgtgccactaaatcgtaatatgtcgcgttacatcagaaagggtaactattatt"
        "aattaataaagggcttaatcactacatattagatcttatccgatagtcttatctattcgt"
        "tgtatttttaagcggttctaattcagtcattatatcagtgctccgagttctttattattg"
        "ttttaaggatgacaaaatgcctcttgttataacgctgggagaagcagactaagagtcgga"
        "gcagttggtagaatgaggctgcaaaagacggtctcgacgaatggacagactttactaaac"
        "caatgaaagacagaagtagagcaaagtctgaagtggtatcagcttaattatgacaaccct"
        "taatacttccctttcgccgaatactggcgtggaaaggttttaaaagtcgaagtagttaga"
        "ggcatctctcgctcataaataggtagactactcgcaatccaatgtgactatgtaatactg"
        "ggaacatcagtccgcgatgcagcgtgtttatcaaccgtccccactcgcctggggagacat"
        "gagaccacccccgtggggattattagtccgcagtaatcgactcttgacaatccttttcga"
        "ttatgtcatagcaatttacgacagttcagcgaagtgactactcggcgaaatggtattact"
        "aaagcattcgaacccacatgaatgtgattcttggcaatttctaatccactaaagcttttc"
        "cgttgaatctggttgtagatatttatataagttcactaattaagatcacggtagtatatt"
        "gatagtgatgtctttgcaagaggttggccgaggaatttacggattctctattgatacaat"
        "ttgtctggcttataactcttaaggctgaaccaggcgtttttagacgacttgatcagctgt"
        "tagaatggtttggactccctctttcatgtcagtaacatttcagccgttattgttacgata"
        "tgcttgaacaatattgatctaccacacacccatagtatattttataggtcatgctgttac"
        "ctacgagcatggtattccacttcccattcaatgagtattcaacatcactagcctcagaga"
        "tgatgacccacctctaataacgtcacgttgcggccatgtgaaacctgaacttgagtagac"
        "gatatcaagcgctttaaattgcatataacatttgagggtaaagctaagcggatgctttat"
        "ataatcaatactcaataataagatttgattgcattttagagttatgacacgacatagttc"
        "actaacgagttactattcccagatctagactgaagtactgatcgagacgatccttacgtc"
        "gatgatcgttagttatcgacttaggtcgggtctctagcggtattggtacttaaccggaca"
        "ctatactaataacccatgatcaaagcataacagaatacagacgataatttcgccaacata"
        "tatgtacagaccccaagcatgagaagctcattgaaagctatcattgaagtcccgctcaca"
        "atgtgtcttttccagacggtttaactggttcccgggagtcctggagtttcgacttacata"
        "aatggaaacaatgtattttgctaatttatctatagcgtcatttggaccaatacagaatat"
        "tatgttgcctagtaatccactataacccgcaagtgctgatagaaaatttttagacgattt"
        "ataaatgccccaagtatccctcccgtgaatcctccgttatactaattagtattcgttcat"
        "acgtataccgcgcatatatgaacatttggcgataaggcgcgtgaattgttacgtgacaga"
        "gatagcagtttcttgtgatatggttaacagacgtacatgaagggaaactttatatctata"
        "gtgatgcttccgtagaaataccgccactggtctgccaatgatgaagtatgtagctttagg"
        "tttgtactatgaggctttcgtttgtttgcagagtataacagttgcgagtgaaaaaccgac"
        "gaatttatactaatacgctttcactattggctacaaaatagggaagagtttcaatcatga"
        "gagggagtatatggatgctttgtagctaaaggtagaacgtatgtatatgctgccgttcat"
        "tcttgaaagatacataagcgataagttacgacaattataagcaacatccctaccttcgta"
        "acgatttcactgttactgcgcttgaaatacactatggggctattggcggagagaagcaga"
        "tcgcgccgagcatatacgagacctataatgttgatgatagagaaggcgtctgaattgata"
        "catcgaagtacactttctttcgtagtatctctcgtcctctttctatctccggacacaaga"
        "attaagttatatatatagagtcttaccaatcatgttgaatcctgattctcagagttcttt"
        "ggcgggccttgtgatgactgagaaacaatgcaatattgctccaaatttcctaagcaaatt"
        "ctcggttatgttatgttatcagcaaagcgttacgttatgttatttaaatctggaatgacg"
        "gagcgaagttcttatgtcggtgtgggaataattcttttgaagacagcactccttaaataa"
        "tatcgctccgtgtttgtatttatcgaatgggtctgtaaccttgcacaagcaaatcggtgg"
        "tgtatatatcggataacaattaatacgatgttcatagtgacagtatactgatcgagtcct"
        "ctaaagtcaattacctcacttaacaatctcattgatgttgtgtcattcccggtatcgccc"
        "gtagtatgtgctctgattgaccgagtgtgaaccaaggaacatctactaatgcctttgtta"
        "ggtaagatctctctgaattccttcgtgccaacttaaaacattatcaaaatttcttctact"
        "tggattaactacttttacgagcatggcaaattcccctgtggaagacggttcattattatc"
        "ggaaaccttatagaaattgcgtgttgactgaaattagatttttattgtaagagttgcatc"
        "tttgcgattcctctggtctagcttccaatgaacagtcctcccttctattcgacatcgggt"
        "ccttcgtacatgtctttgcgatgtaataattaggttcggagtgtggccttaatgggtgca"
        "actaggaatacaacgcaaatttgctgacatgatagcaaatcggtatgccggcaccaaaac"
        "gtgctccttgcttagcttgtgaatgagactcagtagttaaataaatccatatctgcaatc"
        "gattccacaggtattgtccactatctttgaactactctaagagatacaagcttagctgag"
        "accgaggtgtatatgactacgctgatatctgtaaggtaccaatgcaggcaaagtatgcga"
        "gaagctaataccggctgtttccagctttataagattaaaatttggctgtcctggcggcct"
        "cagaattgttctatcgtaatcagttggttcattaattagctaagtacgaggtacaactta"
        "tctgtcccagaacagctccacaagtttttttacagccgaaacccctgtgtgaatcttaat"
        "atccaagcgcgttatctgattagagtttacaactcagtattttatcagtacgttttgttt"
        "ccaacattacccggtatgacaaaatgacgccacgtgtcgaataatggtctgaccaatgta"
        "ggaagtgaaaagataaatat";
