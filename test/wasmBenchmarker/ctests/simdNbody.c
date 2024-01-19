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
#include <math.h>

#include <wasm_simd128.h>

#define LOOP 450000
#define SOLAR_MASS 39.47841760435743f
#define DAYS_PER_YEAR 365.24f
#define BODIES_COUNT 5
#define INTERACTIONS_COUNT (BODIES_COUNT * (BODIES_COUNT - 1) / 2)

typedef struct {
    v128_t position; // last element is not used
    v128_t velocity; // last element is not used
    float mass;
} body;

void move(body solarBodies[]) {
    v128_t dPosition[INTERACTIONS_COUNT]; // last element of each one is unused
    float magnitudes[INTERACTIONS_COUNT];

    for (unsigned int i = 0, k = 0; i < BODIES_COUNT - 1; i++) {
        for (unsigned int j = i + 1; j < BODIES_COUNT; j++, k++) {
            dPosition[k] = wasm_f32x4_sub(solarBodies[i].position, solarBodies[j].position);
        }
    }

    for (unsigned int i = 0; i < INTERACTIONS_COUNT; i++) {
        v128_t dPosition_squared = wasm_f32x4_mul(dPosition[i], dPosition[i]);
        const float distanceSquared =   wasm_f32x4_extract_lane(dPosition_squared, 0) +
                                        wasm_f32x4_extract_lane(dPosition_squared, 1) +
                                        wasm_f32x4_extract_lane(dPosition_squared, 2);
        magnitudes[i] = 0.01f / (sqrtf(distanceSquared) * distanceSquared);
    }

    for (unsigned int i = 0, k = 0; i < BODIES_COUNT - 1; i++) {
        for (unsigned int j = i + 1; j < BODIES_COUNT; j++, k++) {
            const float massI = solarBodies[i].mass * magnitudes[k];
            const float massJ = solarBodies[j].mass * magnitudes[k];

            solarBodies[i].velocity = wasm_f32x4_sub(solarBodies[i].velocity, wasm_f32x4_mul(dPosition[k], wasm_f32x4_splat(massJ)));
            solarBodies[j].velocity = wasm_f32x4_add(solarBodies[j].velocity, wasm_f32x4_mul(dPosition[k], wasm_f32x4_splat(massI)));
        }
    }

    for (unsigned int i = 0; i < BODIES_COUNT; i++) {
        solarBodies[i].position = wasm_f32x4_add(solarBodies[i].position, wasm_f32x4_mul(wasm_f32x4_splat(0.01f), solarBodies[i].velocity));
    }
}

float energy(body solarBodies[]) {
    float energy = 0.0f;

    for (unsigned int i = 0; i < BODIES_COUNT; i++) {
        v128_t solarBodyVelocitySquare = wasm_f32x4_mul(solarBodies[i].velocity, solarBodies[i].velocity);
        float sum = wasm_f32x4_extract_lane(solarBodyVelocitySquare, 0) +
                    wasm_f32x4_extract_lane(solarBodyVelocitySquare, 1) +
                    wasm_f32x4_extract_lane(solarBodyVelocitySquare, 2);
        energy += 0.5f * solarBodies[i].mass * sum;

        for (unsigned int j = i + 1; j < BODIES_COUNT; j++) {
            v128_t d = wasm_f32x4_sub(solarBodies[i].position, solarBodies[j].position);
            v128_t dSquare = wasm_f32x4_mul(d, d);
            float distance = sqrtf( wasm_f32x4_extract_lane(dSquare, 0) +
                                    wasm_f32x4_extract_lane(dSquare, 1) +
                                    wasm_f32x4_extract_lane(dSquare, 2)
                                );
            energy -= (solarBodies[i].mass * solarBodies[j].mass) / distance;
        }
    }

    return energy;
}

float runtime() {
    body solarBodies[] = {
        {   // Sun
            .position = wasm_f32x4_make(
                0.0f,
                0.0f,
                0.0f,
                0.0f // unused
            ),
            .velocity = wasm_f32x4_make(
                0.0f,
                0.0f,
                0.0f,
                0.0f // unused
            ),
            .mass = SOLAR_MASS
        },
        {   // Jupiter
            .position = wasm_f32x4_make(
                +4.84143144246472090e+00f,
                -1.16032004402742839e+00f,
                -1.03622044471123109e-01f,
                +0.0f // unused
            ),
            .velocity = wasm_f32x4_mul( wasm_f32x4_make(
                +1.66007664274403694e-03f,
                +7.69901118419740425e-03f,
                -6.90460016972063023e-05f,
                +0.0f // unused
            ), wasm_f32x4_splat(DAYS_PER_YEAR)),
            .mass = +9.54791938424326609e-04f * SOLAR_MASS
        },
        {   // Saturn
            .position = wasm_f32x4_make(
                +8.34336671824457987e+00f,
                +4.12479856412430479e+00f,
                -4.03523417114321381e-01f,
                +0.0f // unused
            ),
            .velocity = wasm_f32x4_mul( wasm_f32x4_make(
                -2.76742510726862411e-03f,
                +4.99852801234917238e-03f,
                +2.30417297573763929e-05f,
                +0.0f // unused
            ), wasm_f32x4_splat(DAYS_PER_YEAR)),
            .mass = +2.85885980666130812e-04f * SOLAR_MASS
        },
        {   // Uranus
            .position = wasm_f32x4_make(
                +1.28943695621391310e+01f,
                -1.51111514016986312e+01f,
                -2.23307578892655734e-01f,
                +0.0f // unused
            ),
            .velocity = wasm_f32x4_mul( wasm_f32x4_make(
                +2.96460137564761618e-03f,
                +2.37847173959480950e-03f,
                -2.96589568540237556e-05f,
                +0.0f // unused
            ), wasm_f32x4_splat(DAYS_PER_YEAR)),
            .mass = +4.36624404335156298e-05f * SOLAR_MASS
        },
        {   // Neptune
            .position = wasm_f32x4_make(
                +1.53796971148509165e+01f,
                -2.59193146099879641e+01f,
                +1.79258772950371181e-01f,
                +0.0f // unused
            ),
            .velocity = wasm_f32x4_mul( wasm_f32x4_make(
                +2.68067772490389322e-03f,
                +1.62824170038242295e-03f,
                -9.51592254519715870e-05f,
                +0.0f // unused
            ), wasm_f32x4_splat(DAYS_PER_YEAR)),
            .mass = +5.15138902046611451e-05f * SOLAR_MASS
        }
    };

    for (unsigned int i = 0; i < BODIES_COUNT; i++) {
        solarBodies[0].velocity = wasm_f32x4_sub(solarBodies[0].velocity, wasm_f32x4_div(wasm_f32x4_mul(solarBodies[i].velocity, wasm_f32x4_splat(solarBodies[i].mass)),wasm_f32x4_splat(SOLAR_MASS)));
    }

    for (unsigned int i = 0; i < LOOP; i++) {
        move(solarBodies);
    }

    return energy(solarBodies);
}

int main() {
    printf("%.8f\n", runtime());
    return 0;
}
