#include <stdio.h>
#include <stdint.h>
#include <math.h>

double eval_A(uint32_t i, uint32_t j){
    uint64_t ij = (uint64_t)i + j;
    return 1.0/ (ij * (ij + 1) / 2 + i + 1);
}

void eval_A_times_u(uint32_t N, const double u[], double Au[]){
    uint32_t i, j;

    for (i = 0; i < N; ++i){
        Au[i] = 0;
        
        for (j = 0; j < N; ++j){
            Au[i] += eval_A(i, j) * u[j];
        }
    }
}

void eval_At_times_u(uint32_t N, const double u[], double Au[]){
    uint32_t i, j;

    for (i = 0; i < N; ++i){
        Au[i] = 0;

        for (j = 0; j < N; ++j)
            Au[i] += eval_A(j, i) * u[j];
    }
}

void eval_AtA_times_u(uint32_t N, const double u[], double AtAu[]){
    double v[N];

    eval_A_times_u(N, u, v);
    eval_At_times_u(N, v, AtAu);
}

double runtime(){
    uint32_t i;
    const uint32_t N = 100;
    double u[N], v[N], vBv, vv;

    for (i = 0; i < N; ++i)
        u[i] = 1;
    for (i = 0; i < 10; ++i){
        eval_AtA_times_u(N, u, v);
        eval_AtA_times_u(N, v, u);
    }

    vBv = vv = 0;

    for (i = 0; i < N; ++i){
        vBv += u[i] * v[i];
        vv += v[i] * v[i];
    }

    return sqrt(vBv / vv);
}

int main(){
    printf("%0.9f\n", runtime());
    return 0;
}

