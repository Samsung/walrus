#ifndef WALRUS_TEST_WAT_H
#define WALRUS_TEST_WAT_H

#include "wasm.h"

#ifdef __cplusplus
extern "C" {
#endif
void wasm_test_wat(wasm_byte_vec_t* bytes, const char* name);
#ifdef __cplusplus
}
#endif

#endif
