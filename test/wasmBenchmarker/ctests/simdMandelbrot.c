#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

#include <wasm_simd128.h>

#define WIDTH 1600
#define HIGHT 1400
#define N 20
#define REAL_AXIS_SHIFT -1.8 // ~ horizontal shift
#define IMAGINARY_AXIS_SHIFT -1.0 // ~ vertical shift
#define ZOOM 0.0015

#define getNthBit(b, n) ((b & (1 << (7 - n))) > 0)

#define clearNthBit(b, n) b = b & (0xFF - (1 << (7 - n)))

#define setNthBit(b, n) b = b | (1 << (7 - n))

#define SQUARE(z) wasm_f32x4_mul(z, z)

#define ABS_COMPLEX(z_real, z_complex) wasm_f32x4_sqrt(wasm_f32x4_add(SQUARE(z_real), SQUARE(z_imaginary)))

typedef uint8_t byte;

byte areInMandelbrotSet(v128_t c_real, v128_t c_imaginary)
{
    byte result = 0b11110000;
    v128_t z_real = wasm_f32x4_const_splat(0);
    v128_t z_imaginary = wasm_f32x4_const_splat(0);
    for (size_t k = 0; k < N; k++) {
        v128_t cmp_result = wasm_f32x4_gt(ABS_COMPLEX(z_real, z_imaginary), wasm_f32x4_const_splat(2));
        for (size_t i = 0; i < 4; i++) {
            if (getNthBit(result, i) == 1 && ((float*)&cmp_result)[i] != 0) {
                clearNthBit(result, i);
            }
        }
        v128_t next_z_real = wasm_f32x4_add(wasm_f32x4_sub(SQUARE(z_real), SQUARE(z_imaginary)), c_real);
        v128_t next_z_imaginary = wasm_f32x4_add(wasm_f32x4_mul(wasm_f32x4_mul(z_real, z_imaginary), wasm_f32x4_const_splat(2)), c_imaginary);
        z_real = next_z_real;
        z_imaginary = next_z_imaginary;

        if (result == 0) {
            break;
        }
    }
    return result;
}

uint32_t runtime() {
    uint32_t setSize = 0;
    for (int i = 0; i < HIGHT; i++) {
        for (int j = 0; j < WIDTH; j+=4) {
            v128_t real = wasm_f32x4_add(wasm_f32x4_mul(wasm_f32x4_make(j, j+1, j+2, j+3), wasm_f32x4_const_splat(ZOOM)), wasm_f32x4_const_splat(REAL_AXIS_SHIFT));
            v128_t imaginary = wasm_f32x4_add(wasm_f32x4_mul(wasm_f32x4_make(i, i, i, i), wasm_f32x4_const_splat(ZOOM)), wasm_f32x4_const_splat(IMAGINARY_AXIS_SHIFT));
            byte pixels = areInMandelbrotSet(real, imaginary);
            for (int i = 0; i < 4; i++) {
                if (getNthBit(pixels, i)) {
                    setSize++;
                }
            }
        }
    }
    return setSize;
}

int main() {
    printf("%u\n", runtime());
    return 0;
}
