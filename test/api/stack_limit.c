#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "wasm.h"

/* (module (func $f (export "f") (call $f))) */
static const unsigned char recursion_bytes[] = {
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00,
    0x01, 0x04, 0x01, 0x60, 0x00, 0x00,
    0x03, 0x02, 0x01, 0x00,
    0x07, 0x05, 0x01, 0x01, 0x66, 0x00, 0x00,
    0x0a, 0x06, 0x01, 0x04, 0x00, 0x10, 0x00, 0x0b,
};

static void check(int condition, const char* message)
{
    if (!condition) {
        fprintf(stderr, "FAIL: %s\n", message);
        exit(1);
    }
}

/* Runs on a thread whose stack is smaller than STACK_LIMIT_FROM_BASE, so the
 * limit derived from the entry stack pointer alone would lie past the end of
 * the stack and the recursion would crash instead of trapping. */
static void* run_recursion(void* arg)
{
    (void)arg;
    wasm_engine_t* engine = wasm_engine_new();
    wasm_store_t* store = wasm_store_new(engine);
    wasm_byte_vec_t binary;
    wasm_byte_vec_new(&binary, sizeof(recursion_bytes), (const char*)recursion_bytes);
    wasm_module_t* module = wasm_module_new(store, &binary);
    wasm_byte_vec_delete(&binary);
    check(module != NULL, "module creation");
    wasm_extern_vec_t imports = WASM_EMPTY_VEC;
    wasm_trap_t* trap = NULL;
    wasm_instance_t* instance = wasm_instance_new(store, module, &imports, &trap);
    check(instance != NULL && trap == NULL, "instantiation");
    wasm_extern_vec_t exports;
    wasm_instance_exports(instance, &exports);
    check(exports.size == 1, "export count");

    wasm_val_vec_t args = WASM_EMPTY_VEC;
    wasm_val_vec_t results = WASM_EMPTY_VEC;
    trap = wasm_func_call(wasm_extern_as_func(exports.data[0]), &args, &results);
    check(trap != NULL, "unbounded recursion must trap");
    wasm_message_t message;
    wasm_trap_message(trap, &message);
    const char expected[] = "call stack exhausted";
    check(message.size > sizeof(expected) - 1 && strncmp(message.data, expected, sizeof(expected) - 1) == 0,
          "trap message");
    wasm_byte_vec_delete(&message);
    wasm_trap_delete(trap);

    wasm_extern_vec_delete(&exports);
    wasm_instance_delete(instance);
    wasm_module_delete(module);
    wasm_store_delete(store);
    wasm_engine_delete(engine);
    return NULL;
}

int main(void)
{
    pthread_attr_t attr;
    check(pthread_attr_init(&attr) == 0, "pthread_attr_init");
    check(pthread_attr_setstacksize(&attr, 2 * 1024 * 1024) == 0, "pthread_attr_setstacksize");
    pthread_t thread;
    check(pthread_create(&thread, &attr, run_recursion, NULL) == 0, "pthread_create");
    check(pthread_join(thread, NULL) == 0, "pthread_join");
    pthread_attr_destroy(&attr);
    printf("stack limit test passed\n");
    return 0;
}
