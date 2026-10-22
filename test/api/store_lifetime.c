#include <stdio.h>
#include <stdlib.h>

#include "wasm.h"

/* (module (func (export "f"))) */
static const unsigned char module_bytes[] = {
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00,
    0x01, 0x04, 0x01, 0x60, 0x00, 0x00,
    0x03, 0x02, 0x01, 0x00,
    0x07, 0x05, 0x01, 0x01, 0x66, 0x00, 0x00,
    0x0a, 0x04, 0x01, 0x02, 0x00, 0x0b,
};

struct world {
    wasm_store_t* store;
    wasm_module_t* module;
    wasm_instance_t* instance;
    wasm_extern_vec_t exports;
};

static void check(int condition, const char* message)
{
    if (!condition) {
        fprintf(stderr, "FAIL: %s\n", message);
        exit(1);
    }
}

static void create(wasm_engine_t* engine, struct world* w)
{
    w->store = wasm_store_new(engine);
    wasm_byte_vec_t binary;
    wasm_byte_vec_new(&binary, sizeof(module_bytes), (const char*)module_bytes);
    w->module = wasm_module_new(w->store, &binary);
    wasm_byte_vec_delete(&binary);
    check(w->module != NULL, "module creation");
    wasm_extern_vec_t imports = WASM_EMPTY_VEC;
    w->instance = wasm_instance_new(w->store, w->module, &imports, NULL);
    check(w->instance != NULL, "instantiation");
    wasm_instance_exports(w->instance, &w->exports);
    check(w->exports.size == 1, "export count");
}

static void destroy(struct world* w)
{
    wasm_extern_vec_delete(&w->exports);
    wasm_instance_delete(w->instance);
    wasm_module_delete(w->module);
    wasm_store_delete(w->store);
}

int main(void)
{
    wasm_engine_t* engine = wasm_engine_new();
    struct world first, second;

    /* Destroying a store while another store still owns an instance must not
     * trip the leak check for the objects of the surviving store. */
    create(engine, &first);
    create(engine, &second);
    destroy(&second);
    destroy(&first);

    create(engine, &first);
    create(engine, &second);
    destroy(&first);
    destroy(&second);

    wasm_engine_delete(engine);
    printf("store lifetime test passed\n");
    return 0;
}
