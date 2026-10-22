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

static void check_exception_references(wasm_store_t* store)
{
    /* Export an exnref identity function, table, mutable global, and tag. */
    static const unsigned char bytes[] = {
        0, 97, 115, 109, 1, 0, 0, 0, 1, 10, 2, 96, 1, 105, 1, 105,
        96, 1, 105, 0, 3, 2, 1, 0, 4, 5, 1, 105, 1, 1, 2, 13,
        3, 1, 0, 1, 6, 6, 1, 105, 1, 208, 105, 11, 7, 17, 4, 1,
        102, 0, 0, 1, 116, 1, 0, 1, 103, 3, 0, 1, 120, 4, 0, 10,
        6, 1, 4, 0, 32, 0, 11,
    };
    wasm_byte_vec_t binary;
    wasm_byte_vec_new(&binary, sizeof(bytes), (const char*)bytes);
    check(wasm_module_validate(store, &binary), "validate exception references");
    wasm_module_t* module = wasm_module_new(store, &binary);
    check(module != NULL, "exception reference module");
    wasm_byte_vec_delete(&binary);
    wasm_exporttype_vec_t types;
    wasm_module_exports(module, &types);
    check(types.size == 4, "exception reference export types");
    const wasm_functype_t* function = wasm_externtype_as_functype_const(wasm_exporttype_type(types.data[0]));
    check(wasm_valtype_kind(wasm_functype_params(function)->data[0]) == WASM_EXNREF,
          "exception parameter kind");
    check(wasm_valtype_kind(wasm_functype_results(function)->data[0]) == WASM_EXNREF,
          "exception result kind");
    wasm_exporttype_vec_delete(&types);
    wasm_extern_vec_t imports = WASM_EMPTY_VEC;
    wasm_trap_t* trap = NULL;
    wasm_instance_t* instance = wasm_instance_new(store, module, &imports, &trap);
    check(instance != NULL && trap == NULL, "exception reference instantiation");
    wasm_extern_vec_t exports;
    wasm_instance_exports(instance, &exports);
    wasm_val_t argument = WASM_INIT_VAL;
    argument.kind = WASM_EXNREF;
    wasm_val_t result = WASM_INIT_VAL;
    wasm_val_vec_t args = { 1, &argument };
    wasm_val_vec_t results = { 1, &result };
    trap = wasm_func_call(wasm_extern_as_func(exports.data[0]), &args, &results);
    check(trap == NULL && result.kind == WASM_EXNREF && result.of.ref == NULL,
          "exception reference identity call");
    wasm_val_t copy = WASM_INIT_VAL;
    wasm_val_copy(&copy, &result);
    check(copy.kind == WASM_EXNREF && copy.of.ref == NULL, "copy null exception reference");
    wasm_val_delete(&copy);
    wasm_val_delete(&result);
    wasm_global_t* global = wasm_extern_as_global(exports.data[2]);
    wasm_global_set(global, &argument);
    wasm_global_get(global, &result);
    check(result.kind == WASM_EXNREF && result.of.ref == NULL, "exception reference global");
    wasm_val_delete(&result);
    wasm_val_delete(&argument);
    wasm_extern_vec_delete(&exports);
    wasm_instance_delete(instance);
    wasm_module_delete(module);
}

int main(void)
{
    wasm_engine_t* engine = wasm_engine_new();
    wasm_store_t* store = wasm_store_new(engine);
    check_exception_references(store);

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
