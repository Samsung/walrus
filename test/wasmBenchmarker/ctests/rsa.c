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

#define max(a, b) ((a >= b) ? a : b)
#define min(a, b) ((a <= b) ? a : b)

#define square(a) (a * a)

uint64_t squareRoot(uint64_t number) {
    uint64_t root = 0;
    while (square(root + 1) <= number) {
        root++;
    }
    return root;
}

bool isPrime(uint64_t number) {
    if (number <= 1) {
        return false;
    }
    for (uint64_t i = 2; i <= squareRoot(number); i++) {
        if (number % i == 0) {
            return false;
        }
    }
    return true;
}

uint64_t NthPrimeAfterNumber(uint64_t n, uint64_t number) {
    while (n) {
        number++;
        if (isPrime(number)) {
            n--;
        }
    }
    return number;
}

static uint64_t gcd(uint64_t a, uint64_t b) {
    if (a == 0 && b == 0) { // edge case; gcd(0, 0) is commonly defined as 0
        return 0;
    }
    if (a == 0) { // then b > 0
        return b;
    }
    if (b == 0) { // then a > 0
        return a;
    }
    // now a and b are greater than 0
    uint64_t greater = max(a, b);
    uint64_t lesser = min(a, b);
    while (true) {
        uint64_t remnant = greater % lesser;
        if (remnant == 0) {
            return lesser;
        }
        greater = lesser;
        lesser = remnant;
    }
}

#define areCoPrimes(a, b) ((gcd(a, b) == 1) ? true : false)

typedef struct {
    uint64_t e; // "Exponent"
    uint64_t n;
} PublicKey;

typedef struct {
    uint64_t d; // "Decryption exponent"
    uint64_t n;
} PrivateKey;

static uint64_t generateE(uint64_t prime1, uint64_t prime2) {
    // assert(isPrime(prime1));
    // assert(isPrime(prime2));
    uint64_t phi_n = (prime1 - 1) * (prime2 - 1);
    uint64_t e = 2;
    while (areCoPrimes(e, phi_n) == false) {
        e++;
    }
    return e;
}

static uint64_t getInverse(uint64_t x, uint64_t m) {
    // assert(m > 0);
    x = x % m;
    // assert(areCoPrimes(x, m));
    uint64_t greater = m;
    uint64_t lesser = x;
    int64_t a = 0;
    int64_t b = 1;
    while (true) {
        uint64_t remnant = greater % lesser;
        uint64_t k = greater / lesser;
        int64_t newA = b;
        int64_t newB = a - (int64_t)k * b;
        a = newA;
        b = newB;
        greater = lesser;
        lesser = remnant;
        if (remnant == 0) {
            while (a < 0) {
                a += m;
            }
            return (uint64_t)a;
        }
    }
}

void generateKeys(uint64_t prime1, uint64_t prime2, PublicKey* publicKey, PrivateKey* privateKey) {
    // assert(isPrime(prime1));
    // assert(isPrime(prime2));
    // assert(prime1 > 255);
    // assert(prime2 > 255);
    uint64_t n = prime1 * prime2;
    uint64_t phi_n = (prime1 - 1) * (prime2 - 1);
    uint64_t e = generateE(prime1, prime2);
    uint64_t d = getInverse(e, phi_n);
    *publicKey = (PublicKey){e, n};
    *privateKey = (PrivateKey){d, n};
}

uint64_t power(uint64_t x, uint64_t e, uint64_t m) {
    // assert(m >= 1);
    if (e == 0) {
        return 1;
    }
    x = x % m;
    if (e == 1) {
        return x;
    }
    if (x <= 1) {
        return x;
    }
    uint64_t result = (e & 1) ? x : 1;
    uint64_t temp = x;
    e >>= 1;
    while (e > 0) {
        temp *= temp;
        temp %= m;
        if (e & 1) {
            result *= temp;
            result %= m;
        }
        e >>= 1;
    }
    return result;
}

void encryptMessage(const uint8_t* input, uint64_t messageLength, PublicKey publicKey, uint64_t* output) {
    uint64_t encryptTable[256];
    for (int i = 0; i < 256; i++) {
        encryptTable[i] = power((uint64_t)i, publicKey.e, publicKey.n);
    }
    for (uint64_t i = 0; i < messageLength; i++) {
        output[i] = encryptTable[input[i]];
    }
}

void decryptMessage(const uint64_t* input, uint64_t messageLength, PrivateKey publicKey, uint8_t* output) {
    for (uint64_t i = 0; i < messageLength; i++) {
        output[i] = power((uint64_t)(input[i]), publicKey.d, publicKey.n);
    }
}

bool areEqual(const char* str1, const char* str2) {
    unsigned int i = 0;
    while (true) {
        if (str1[i] != str2[i]) {
            return false;
        }
        if (str1[i] == '\0') {
            return true;
        }
        i++;
    }
}

int runtime() {
    const char message[] =  "Lorem ipsum dolor sit amet, et wisi primis duo."
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
    const int messageLength = sizeof(message) / sizeof(char);
    const uint64_t prime1 = NthPrimeAfterNumber(350, 0);
    const uint64_t prime2 = NthPrimeAfterNumber(1, prime1);
    PublicKey publicKey;
    PrivateKey privateKey;
    generateKeys(prime1, prime2, &publicKey, &privateKey);
    uint64_t encryptedMessage[messageLength];
    encryptMessage((uint8_t*)message, messageLength, publicKey, encryptedMessage);
    char decryptedMessage[messageLength];
    decryptMessage(encryptedMessage, messageLength, privateKey, (uint8_t*)decryptedMessage);
    if (areEqual(message, decryptedMessage)) {
        return 0;
    } else {
        return 1;
    }
}

int main() {
    printf("%d\n", runtime());
}
