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

#define LOOP 3000000
#define SOLAR_MASS 39.47841760435743f
#define DAYS_PER_YEAR 365.24f
#define BODIES_COUNT 5
#define INTERACTIONS_COUNT (BODIES_COUNT * (BODIES_COUNT - 1) / 2)

#define square(x) x * x

typedef struct {
    float position[3];
    float velocity[3];
    float mass;
} body;

void move(body solarBodies[]) {
    float dPosition[INTERACTIONS_COUNT][3];
    float magnitudes[INTERACTIONS_COUNT];

    for (unsigned int i = 0, k = 0; i < BODIES_COUNT - 1; i++) {
        for (unsigned int j = i + 1; j < BODIES_COUNT; j++, k++) {
            for (unsigned int m = 0; m < 3; m++) {
                dPosition[k][m] = solarBodies[i].position[m] - solarBodies[j].position[m];
            }
        }
    }

    for (unsigned int i = 0; i < INTERACTIONS_COUNT; i++) {
        const float dx = dPosition[i][0];
        const float dy = dPosition[i][1];
        const float dz = dPosition[i][2];
        const float distanceSquared = square(dx) + square(dy) + square(dz);
        magnitudes[i] = 0.01f / (sqrtf(distanceSquared) * distanceSquared);
    }

    for (unsigned int i = 0, k = 0; i < BODIES_COUNT - 1; i++) {
        for (unsigned int j = i + 1; j < BODIES_COUNT; j++, k++) {
            const float massI = solarBodies[i].mass * magnitudes[k];
            const float massJ = solarBodies[j].mass * magnitudes[k];

            for (unsigned int m = 0; m < 3; m++) {
                solarBodies[i].velocity[m] -= dPosition[k][m] * massJ;
                solarBodies[j].velocity[m] += dPosition[k][m] * massI;
            }
        }
    }

    for (unsigned int i = 0; i < BODIES_COUNT; i++) {
        for (unsigned int m = 0; m < 3; m++) {
            solarBodies[i].position[m] += 0.01f * solarBodies[i].velocity[m];
        }
    }
}

float energy(body solarBodies[]) {
    float energy = 0.0f;

    for (unsigned int i = 0; i < BODIES_COUNT; i++) {
        energy += 0.5f * solarBodies[i].mass *
                  (solarBodies[i].velocity[0] * solarBodies[i].velocity[0] +
                   solarBodies[i].velocity[1] * solarBodies[i].velocity[1] +
                   solarBodies[i].velocity[2] * solarBodies[i].velocity[2]);

        for (unsigned int j = i + 1; j < BODIES_COUNT; j++) {
            float dx = solarBodies[i].position[0] - solarBodies[j].position[0];
            float dy = solarBodies[i].position[1] - solarBodies[j].position[1];
            float dz = solarBodies[i].position[2] - solarBodies[j].position[2];
            float distance = sqrtf(square(dx) + square(dy) + square(dz));
            energy -= (solarBodies[i].mass * solarBodies[j].mass) / distance;
        }
    }

    return energy;
}

float runtime() {
    body solarBodies[] = {
        {   // Sun
            .position = {
                0.0f,
                0.0f,
                0.0f
            },
            .velocity = {
                0.0f,
                0.0f,
                0.0f
            },
            .mass = SOLAR_MASS
        },
        {   // Jupiter
            .position = {
                +4.84143144246472090e+00f,
                -1.16032004402742839e+00f,
                -1.03622044471123109e-01f
            },
            .velocity = {
                +1.66007664274403694e-03f * DAYS_PER_YEAR,
                +7.69901118419740425e-03f * DAYS_PER_YEAR,
                -6.90460016972063023e-05f * DAYS_PER_YEAR,
            },
            .mass = +9.54791938424326609e-04f * SOLAR_MASS
        },
        {   // Saturn
            .position = {
                +8.34336671824457987e+00f,
                +4.12479856412430479e+00f,
                -4.03523417114321381e-01f
            },
            .velocity = {
                -2.76742510726862411e-03f * DAYS_PER_YEAR,
                +4.99852801234917238e-03f * DAYS_PER_YEAR,
                +2.30417297573763929e-05f * DAYS_PER_YEAR
            },
            .mass = +2.85885980666130812e-04f * SOLAR_MASS
        },
        {   // Uranus
            .position = {
                +1.28943695621391310e+01f,
                -1.51111514016986312e+01f,
                -2.23307578892655734e-01f
            },
            .velocity = {
                +2.96460137564761618e-03f * DAYS_PER_YEAR,
                +2.37847173959480950e-03f * DAYS_PER_YEAR,
                -2.96589568540237556e-05f * DAYS_PER_YEAR
            },
            .mass = +4.36624404335156298e-05f * SOLAR_MASS
        },
        {   // Neptune
            .position = {
                +1.53796971148509165e+01f,
                -2.59193146099879641e+01f,
                +1.79258772950371181e-01f
            },
            .velocity = {
                +2.68067772490389322e-03f * DAYS_PER_YEAR,
                +1.62824170038242295e-03f * DAYS_PER_YEAR,
                -9.51592254519715870e-05f * DAYS_PER_YEAR
            },
            .mass = +5.15138902046611451e-05f * SOLAR_MASS
        }
    };

    for (unsigned int i = 0; i < BODIES_COUNT; i++) {
        for (unsigned int m = 0; m < 3; m++) {
            solarBodies[0].velocity[m] -= solarBodies[i].velocity[m] * solarBodies[i].mass / SOLAR_MASS;
        }
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
