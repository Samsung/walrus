#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "wasm.h"

/* (module (func (export "trap") unreachable)) */
static const unsigned char call_bytes[] = {
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00,
    0x01, 0x04, 0x01, 0x60, 0x00, 0x00,
    0x03, 0x02, 0x01, 0x00,
    0x07, 0x08, 0x01, 0x04, 0x74, 0x72, 0x61, 0x70, 0x00, 0x00,
    0x0a, 0x05, 0x01, 0x03, 0x00, 0x00, 0x0b,
};

/* (module (func $start unreachable) (start $start)) */
static const unsigned char start_bytes[] = {
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00,
    0x01, 0x04, 0x01, 0x60, 0x00, 0x00,
    0x03, 0x02, 0x01, 0x00,
    0x08, 0x01, 0x00,
    0x0a, 0x05, 0x01, 0x03, 0x00, 0x00, 0x0b,
};

static void check(int condition, const char* message)
{
    if (!condition) {
        fprintf(stderr, "FAIL: %s\n", message);
        exit(1);
    }
}

static void check_message(wasm_trap_t* trap)
{
    check(trap != NULL, "expected trap");
    wasm_message_t message;
    wasm_trap_message(trap, &message);
    check(message.size > 1 && message.data != NULL, "nonempty trap message");
    check(memchr(message.data, '\0', message.size) == message.data + message.size - 1,
          "trap message includes its terminating null byte");
    wasm_byte_vec_delete(&message);
    wasm_trap_t* copy = wasm_trap_copy(trap);
    check(wasm_trap_same(trap, copy), "trap copies share identity");
    wasm_trap_delete(trap);
    wasm_trap_message(copy, &message);
    check(message.size > 1 && message.data[message.size - 1] == '\0', "copied trap owns message");
    wasm_byte_vec_delete(&message);
    wasm_trap_delete(copy);
}

int main(void)
{
    wasm_engine_t* engine = wasm_engine_new();
    wasm_store_t* store = wasm_store_new(engine);
    wasm_extern_vec_t imports = WASM_EMPTY_VEC;
    const unsigned char* bytes[] = { call_bytes, start_bytes };
    const size_t sizes[] = { sizeof(call_bytes), sizeof(start_bytes) };
    for (size_t i = 0; i < 2; i++) {
        wasm_byte_vec_t binary;
        wasm_byte_vec_new(&binary, sizes[i], (const char*)bytes[i]);
        wasm_module_t* module = wasm_module_new(store, &binary);
        check(module != NULL, "module creation");
        wasm_byte_vec_delete(&binary);
        wasm_trap_t* trap = NULL;
        wasm_instance_t* instance = wasm_instance_new(store, module, &imports, &trap);
        if (i == 0) {
            check(instance != NULL && trap == NULL, "instantiation");
            wasm_extern_vec_t exports;
            wasm_instance_exports(instance, &exports);
            check(exports.size == 1, "export count");
            wasm_val_vec_t args = WASM_EMPTY_VEC;
            wasm_val_vec_t results = WASM_EMPTY_VEC;
            trap = wasm_func_call(wasm_extern_as_func(exports.data[0]), &args, &results);
            check_message(trap);
            wasm_extern_vec_delete(&exports);
            wasm_instance_delete(instance);
        } else {
            check(instance == NULL, "start function must trap");
            check_message(trap);
        }
        wasm_module_delete(module);
    }
    wasm_store_delete(store);
    wasm_engine_delete(engine);
    return 0;
}
