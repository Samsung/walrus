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
                            "quando feugiat voluptaria eam ne.\n"
                            "Curabitur vitae erat tortor. Lorem ipsum dolor "
                            "sit amet, consectetur adipiscing elit. Integer "
                            "ut ligula nec odio bibendum tempus. Suspendisse"
                            " id neque nec tortor faucibus congue. Donec vit"
                            "ae tortor at tellus feugiat dictum eget non sem"
                            ". Mauris malesuada tellus commodo, congue massa"
                            " sed, tristique nunc. Duis facilisis risus nec "
                            "eros rutrum scelerisque ac sit amet nibh. Integ"
                            "er urna ipsum, vestibulum a dui et, varius vene"
                            "natis lectus.\n\nInteger purus arcu, fermentum "
                            "sit amet ultricies in, consequat non urna. Maur"
                            "is convallis risus ut nulla porta, vitae posuer"
                            "e sapien commodo. Mauris pulvinar ligula a pulv"
                            "inar auctor. Fusce at cursus lectus, sit amet s"
                            "celerisque dolor. Quisque lacinia mauris vestib"
                            "ulum lacus rutrum vestibulum. Nullam ut ex elem"
                            "entum, suscipit leo in, egestas purus. Vestibul"
                            "um ligula velit, rutrum eget dapibus et, tempor"
                            " ut arcu. Nunc mattis ante ante, ut interdum ma"
                            "ssa congue at. Proin tortor eros, euismod sit a"
                            "met justo eu, tincidunt viverra nunc. Morbi sit"
                            " amet venenatis sapien. In sollicitudin urna en"
                            "im, eget porttitor diam pulvinar ut. Pellentesq"
                            "ue placerat euismod mattis. Cras sit amet leo e"
                            "t purus facilisis dictum eget quis libero.\n\nP"
                            "raesent ipsum dui, luctus vitae neque vitae, lo"
                            "bortis egestas nunc. Proin id leo sed urna ulla"
                            "mcorper vehicula ullamcorper quis purus. Mauris"
                            " nec cursus nibh, quis posuere ligula. Nullam r"
                            "honcus lacus non risus dictum luctus. Praesent "
                            "nulla orci, ultrices commodo tincidunt et, aliq"
                            "uam ac mauris. In laoreet convallis feugiat. Et"
                            "iam quis eros vitae enim porttitor cursus in vo"
                            "lutpat turpis. Quisque tempor turpis leo. Duis "
                            "at diam ac odio rutrum eleifend.\n\nEtiam id po"
                            "rta lacus. Sed pellentesque eleifend tellus, tr"
                            "istique mollis dui congue nec. Phasellus ante e"
                            "st, mollis semper libero suscipit, vestibulum v"
                            "ehicula diam. Phasellus pretium ex ut diam aliq"
                            "uam, quis venenatis sapien maximus. Morbi eget "
                            "interdum felis, sed aliquam magna. Pellentesque"
                            " fermentum est sapien, at congue mauris convall"
                            "is eu. Aliquam mattis aliquet libero id blandit"
                            ". Maecenas turpis augue, pretium eget finibus i"
                            "n, cursus quis ligula. Pellentesque habitant mo"
                            "rbi tristique senectus et netus et malesuada fa"
                            "mes ac turpis egestas.\n\nVestibulum pharetra t"
                            "ortor non nibh scelerisque congue. Morbi fermen"
                            "tum, enim at accumsan luctus, tellus ante moles"
                            "tie est, ut aliquet ipsum nulla quis sem. Aliqu"
                            "am eget ligula ut diam accumsan semper ultricie"
                            "s eget ante. Donec aliquet odio sed nibh euismo"
                            "d, sed efficitur nulla condimentum. Aliquam ult"
                            "rices, ipsum vitae pretium vulputate, libero ju"
                            "sto dictum velit, eu placerat purus leo quis fe"
                            "lis. Praesent ultrices massa nulla, at porta ma"
                            "ssa vehicula vel. Morbi vulputate nulla eget ur"
                            "na ornare, sed posuere est imperdiet. In non mi"
                            " vel libero pellentesque condimentum id sed mi."
                            " Praesent id nibh massa. Donec porttitor effici"
                            "tur enim, eget ultrices mi commodo at. Proin he"
                            "ndrerit justo eu fermentum ornare. Vivamus pulv"
                            "inar ex quis nisi fringilla imperdiet. Vivamus "
                            "efficitur lectus sit amet ipsum scelerisque rut"
                            "rum.\n\nDuis ac justo tellus. Aliquam erat volu"
                            "tpat. Duis sed nisl massa. Fusce posuere, magna"
                            " eget posuere tincidunt, lorem nisi tristique a"
                            "nte, in egestas enim ipsum ut sem. Phasellus si"
                            "t amet gravida elit. Nulla aliquam lectus condi"
                            "mentum ex rhoncus varius. Praesent facilisis du"
                            "i eget vestibulum placerat. Suspendisse et cons"
                            "ectetur enim, vitae imperdiet ante. Praesent ne"
                            "c tortor placerat, aliquet ex iaculis, semper f"
                            "elis. Duis gravida libero quis neque condimentu"
                            "m volutpat. Class aptent taciti sociosqu ad lit"
                            "ora torquent per conubia nostra, per inceptos h"
                            "imenaeos.\n\nInteger et purus pulvinar, finibus"
                            " quam at, sodales ipsum. Maecenas eu elementum "
                            "dui. Sed condimentum ullamcorper nisi, a lacini"
                            "a ante sagittis sit amet. Aliquam sed tempus au"
                            "gue. Donec condimentum placerat metus id posuer"
                            "e. Sed placerat blandit lorem, vitae pulvinar e"
                            "ros fermentum quis. Nunc sed mi in ipsum suscip"
                            "it iaculis in at mi. In sed lorem eu purus cong"
                            "ue maximus. Cras gravida, elit et sagittis phar"
                            "etra, elit mi volutpat augue, a volutpat leo lo"
                            "rem luctus eros. Pellentesque maximus lorem eu "
                            "bibendum suscipit. Nullam ipsum massa, eleifend"
                            " quis faucibus in, fermentum non lorem. Sed vol"
                            "utpat facilisis felis, ut laoreet est venenatis"
                            " mollis.\n\nPhasellus efficitur ante et eros ul"
                            "trices aliquet. Morbi molestie neque ante, pell"
                            "entesque vulputate mauris dictum id. Donec eu d"
                            "apibus enim. Maecenas non lacus orci. Suspendis"
                            "se potenti. Donec at arcu ac nunc laoreet eleme"
                            "ntum id et ipsum. Suspendisse sed justo purus. "
                            "Donec malesuada, nibh ac facilisis congue, mass"
                            "a risus porttitor nulla, a mattis turpis augue "
                            "non augue. In varius sem et nunc auctor sollici"
                            "tudin. Praesent dapibus faucibus vulputate. Qui"
                            "sque quis ipsum lacus. Pellentesque et sapien d"
                            "ui. Proin ut lacus dapibus, convallis ligula ul"
                            "lamcorper, finibus sem. Vestibulum sapien neque"
                            ", facilisis nec lacus ut, venenatis maximus lig"
                            "ula.\n\nMorbi sit amet accumsan ante, eu ullamc"
                            "orper libero. Donec eu cursus tellus. Nunc susc"
                            "ipit augue odio, nec tincidunt tortor efficitur"
                            " non. Vivamus eget erat odio. Maecenas tincidun"
                            "t in nunc interdum pellentesque. Maecenas nec o"
                            "dio justo. Vivamus ultricies quam nisi, et vulp";
    const int messageLength = sizeof(message) / sizeof(char);

    for(int i = 100; i < 650; i++) {
        const uint64_t prime1 = NthPrimeAfterNumber(i, 0);
        const uint64_t prime2 = NthPrimeAfterNumber(1, prime1);
        PublicKey publicKey;
        PrivateKey privateKey;
        generateKeys(prime1, prime2, &publicKey, &privateKey);
        uint64_t encryptedMessage[messageLength];
        encryptMessage((uint8_t*)message, messageLength, publicKey, encryptedMessage);
        char decryptedMessage[messageLength];
        decryptMessage(encryptedMessage, messageLength, privateKey, (uint8_t*)decryptedMessage);
        if (!areEqual(message, decryptedMessage)) {
            return 1;
        }
    }
    return 0;
}

int main() {
    printf("%d\n", runtime());
}
