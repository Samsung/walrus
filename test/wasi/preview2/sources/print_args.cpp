#include <stdio.h>
#include <stdint.h>

int main(int argv, char *argc[])
{
    printf("Arguments:\n");
    for (uint64_t i = 0; i < (uint64_t)argv; i++) {
        printf("%s\n", argc[i]);
    }

    return 0;
}
