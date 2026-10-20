#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "wasm.h"

/*
(module
  (memory i64 1)
  (type $t (struct (field i8)))

  (func (export "toStruct") (param i32) (result (ref $t))
    local.get 0
    struct.new $t
  )

  (func (export "toI31") (param i32) (result (ref i31))
    local.get 0
    ref.i31
  )
)
*/

static const unsigned char module_bytes[] = {
    /* Signature */
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00,
    /* Type section */
    0x01, 0x11, 0x03, 0x5f, 0x01, 0x78, 0x00, 0x60, 0x01, 0x7f, 0x01, 0x64, 0x00, 0x60, 0x01, 0x7f, 0x01, 0x64, 0x6c,
    /* Func section */
    0x03, 0x03, 0x02, 0x01, 0x02,
    /* Memory section */
    0x05, 0x03, 0x01, 0x04, 0x01,
    /* Export section */
    0x07, 0x14, 0x02, 0x08, 0x74, 0x6f, 0x53, 0x74, 0x72, 0x75, 0x63, 0x74, 0x00, 0x00,
    0x05, 0x74, 0x6f, 0x49, 0x33, 0x31, 0x00, 0x01,
    /* Code section */
    0x0a, 0x10, 0x02, 0x07, 0x00, 0x20, 0x00, 0xfb, 0x00, 0x00, 0x0b,
    0x06, 0x00, 0x20, 0x00, 0xfb, 0x1c, 0x0b
};

static void check(int condition, const char* message)
{
    if (!condition) {
        fprintf(stderr, "FAIL: %s\n", message);
        exit(1);
    }
}

int main(void)
{
    wasm_engine_t* engine = wasm_engine_new();
    wasm_store_t* store = wasm_store_new(engine);

    wasm_byte_vec_t binary;
    wasm_byte_vec_new(&binary, sizeof(module_bytes), (const char*)module_bytes);
    check(wasm_module_validate(store, &binary), "validate Wasm3 features");
    wasm_module_t* module = wasm_module_new(store, &binary);
    check(module != NULL, "module creation");
    wasm_byte_vec_delete(&binary);

    wasm_extern_vec_t imports = WASM_EMPTY_VEC;
    wasm_trap_t* trap = NULL;
    wasm_instance_t* instance = wasm_instance_new(store, module, &imports, &trap);
    check(instance != NULL && trap == NULL, "instantiation");

    wasm_extern_vec_t exports;
    wasm_instance_exports(instance, &exports);
    check(exports.size == 2, "export count");

    wasm_val_t param = WASM_I32_VAL(0x123456);
    wasm_val_t result1 = WASM_INIT_VAL;
    wasm_val_vec_t args = { 1, &param };
    wasm_val_vec_t results1 = { 1, &result1 };
    trap = wasm_func_call(wasm_extern_as_func(exports.data[0]), &args, &results1);
    check(trap == NULL, "call");
    wasm_val_delete(&result1);

    wasm_val_t result2 = WASM_INIT_VAL;
    wasm_val_vec_t results2 = { 1, &result2 };
    trap = wasm_func_call(wasm_extern_as_func(exports.data[1]), &args, &results2);
    check(trap == NULL, "call");
    check(result2.kind == WASM_ANYREF && result2.of.ref != NULL, "i31 result wrapper");
    wasm_val_t copied = WASM_INIT_VAL;
    wasm_val_copy(&copied, &result2);
    check(wasm_ref_same(result2.of.ref, copied.of.ref), "copied i31 identity");
    check(wasm_ref_as_foreign(result2.of.ref) == NULL, "i31 is not a foreign object");
    wasm_val_delete(&result2);
    wasm_ref_t* second_copy = wasm_ref_copy(copied.of.ref);
    check(wasm_ref_same(second_copy, copied.of.ref), "copy survives original deletion");
    wasm_val_delete(&copied);
    wasm_ref_delete(second_copy);

    wasm_val_delete(&param);
    wasm_extern_vec_delete(&exports);
    wasm_instance_delete(instance);
    wasm_module_delete(module);
    wasm_store_delete(store);
    wasm_engine_delete(engine);
    return 0;
}
