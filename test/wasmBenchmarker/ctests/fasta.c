#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define IM 139968U
#define IA 3877U
#define IC 29573U
#define SEED 42U

#define INPUT_SIZE 1000U
#define LINELEN 60U
#define BUFLINES 100U

#define FNV_OFFSET_BASIS 1469598103934665603ULL
#define FNV_PRIME 1099511628211ULL

static uint32_t seed = SEED;

static const char *alu =
    "GGCCGGGCGCGGTGGCTCACGCCTGTAATCCCAGCACTTTGG"
    "GAGGCCGAGGCGGGCGGATCACCTGAGGTCAGGAGTTCGAGA"
    "CCAGCCTGGCCAACATGGTGAAACCCCGTCTCTACTAAAAAT"
    "ACAAAAATTAGCCGGGCGTGGTGGCGCGCGCCTGTAATCCCA"
    "GCTACTCGGGAGGCTGAGGCAGGAGAATCGCTTGAACCCGGG"
    "AGGCGGAGGTTGCAGTGAGCCGAGATCGCGCCACTGCACTCC"
    "AGCCTGGGCGACAGAGCGAGACTCCGTCTCAAAAA";

static const char *iub = "acgtBDHKMNRSVWY";

static const float iubProbability[] = {
    0.27f,
    0.12f,
    0.12f,
    0.27f,
    0.02f,
    0.02f,
    0.02f,
    0.02f,
    0.02f,
    0.02f,
    0.02f,
    0.02f,
    0.02f,
    0.02f,
    0.02f
};

static const char *homoSapiens = "acgt";

static const float homoSapiensProbability[] = {
    0.3029549426680f,
    0.1979883004921f,
    0.1975473066391f,
    0.3015094502008f
};

static const char header1[] = ">ONE Homo sapiens alu\n";
static const char header2[] = ">TWO IUB ambiguity codes\n";
static const char header3[] = ">THREE Homo sapiens frequency\n";

static uint32_t randomNumber(void)
{
    seed = (seed * IA + IC) % IM;

    return seed;
}

static uint64_t updateChecksum(uint64_t checksum, const char *buffer, uint32_t length) {
    for (uint32_t i = 0; i < length; i++) {
        checksum ^= (uint8_t)buffer[i];
        checksum *= FNV_PRIME;
    }

    return checksum;
}

static uint64_t repeatFasta(const char *sequence, uint32_t count, uint64_t checksum) {
    uint32_t sequenceLength = (uint32_t)strlen(sequence);
    uint32_t bufferLength = sequenceLength + LINELEN;

    char *buffer = (char *)malloc(bufferLength);

    if (buffer == NULL) {
        return 0;
    }

    memcpy(buffer, sequence, sequenceLength);
    memcpy(buffer + sequenceLength, sequence, LINELEN);

    uint32_t offset = 0;

    while (count >= LINELEN) {
        checksum = updateChecksum(checksum, buffer + offset, LINELEN);
        checksum = updateChecksum(checksum, "\n", 1);

        offset = (offset + LINELEN) % sequenceLength;
        count -= LINELEN;
    }

    if (count > 0) {
        checksum = updateChecksum(checksum, buffer + offset, count);
        checksum = updateChecksum(checksum, "\n", 1);
    }

    free(buffer);

    return checksum;
}

static char *buildHash(const char *symbols, const float *probability) {
    uint32_t symbolIndex = 0;
    uint32_t symbolCount = (uint32_t)strlen(symbols);

    float sum = probability[0];

    char *hash = (char *)malloc(IM);

    if (hash == NULL) {
        return NULL;
    }

    for (uint32_t i = 0; i < IM; i++) {
        float ratio = (float)i / (float)IM;

        while (symbolIndex + 1 < symbolCount && ratio >= sum) {
            symbolIndex += 1;
            sum += probability[symbolIndex];
        }

        hash[i] = symbols[symbolIndex];
    }

    return hash;
}

static char *createBuffer(void)
{
    char *buffer = (char *)malloc((LINELEN + 1) * BUFLINES);

    if (buffer == NULL) {
        return NULL;
    }

    for (uint32_t i = 0; i < BUFLINES; i++) {
        buffer[i * (LINELEN + 1) + LINELEN] = '\n';
    }

    return buffer;
}

static uint64_t randomFasta(const char *symbols, const float *probability, uint32_t count, uint64_t checksum)
{
    char *hash = buildHash(symbols, probability);
    char *buffer = createBuffer();

    if (hash == NULL || buffer == NULL) {
        free(hash);
        free(buffer);

        return 0;
    }

    uint32_t fullBuffers = count / (LINELEN * BUFLINES);

    for (uint32_t i = 0; i < fullBuffers; i++) {
        for (uint32_t line = 0; line < BUFLINES; line++) {
            for (uint32_t column = 0; column < LINELEN; column++) {
                uint32_t index = line * (LINELEN + 1) + column;

                buffer[index] = hash[randomNumber()];
            }
        }

        checksum = updateChecksum(checksum, buffer, (LINELEN + 1) * BUFLINES);
    }

    uint32_t remaining = count - fullBuffers * LINELEN * BUFLINES;
    uint32_t fullLines = remaining / LINELEN;
    uint32_t partialLength = remaining % LINELEN;

    for (uint32_t line = 0; line < fullLines; line++) {
        for (uint32_t column = 0; column < LINELEN; column++) {
            uint32_t index = line * (LINELEN + 1) + column;

            buffer[index] = hash[randomNumber()];
        }
    }

    for (uint32_t column = 0; column < partialLength; column++) {
        uint32_t index = fullLines * (LINELEN + 1) + column;

        buffer[index] = hash[randomNumber()];
    }

    checksum = updateChecksum(checksum, buffer, fullLines * (LINELEN + 1) + partialLength);

    if (partialLength > 0) {
        checksum = updateChecksum(checksum, "\n", 1);
    }

    free(buffer);
    free(hash);

    return checksum;
}

uint64_t fasta(uint32_t inputSize)
{
    uint64_t checksum = FNV_OFFSET_BASIS;

    checksum = updateChecksum(checksum, header1, sizeof(header1) - 1);
    checksum = repeatFasta(alu, inputSize * 2, checksum);

    checksum = updateChecksum(checksum, header2, sizeof(header2) - 1);
    checksum = randomFasta(iub, iubProbability, inputSize * 3, checksum);

    checksum = updateChecksum(checksum, header3, sizeof(header3) - 1);
    checksum = randomFasta(homoSapiens, homoSapiensProbability, inputSize * 5, checksum);

    return checksum;
}

uint64_t runtime(void)
{
    seed = SEED;

    return fasta(INPUT_SIZE);
}

int main()
{
    printf("%llu\n", (unsigned long long)runtime());
    return 0;
}