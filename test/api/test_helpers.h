#ifndef WALRUS_TEST_HELPERS_H
#define WALRUS_TEST_HELPERS_H

#include "wasm.h"

/* Test instrumentation compiled only with WALRUS_BUILD_API_TESTS.
 * These helpers are not part of the public WebAssembly C API. */
#ifdef __cplusplus
extern "C" {
#endif

size_t wasm_test_store_object_count(const wasm_store_t* store);
size_t wasm_test_store_collection_count(const wasm_store_t* store);
size_t wasm_test_store_graph_build_count(const wasm_store_t* store);
size_t wasm_test_store_table_slot_count(const wasm_store_t* store);
size_t wasm_test_store_graph_trace_count(const wasm_store_t* store);
size_t wasm_test_store_graph_edge_count(const wasm_store_t* store);
wasm_module_t* wasm_test_module_new_gc(wasm_store_t* store, const wasm_byte_vec_t* binary);

#ifdef __cplusplus
}
#endif

#endif
