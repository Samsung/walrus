#include <stdio.h>
#include <stdlib.h>

#include "wasm.h"

/*
 * (module
 *   (func $hidden (param i32) (result i32)
 *     local.get 0 i32.const 1 i32.add)
 *   (table (export "table") 2 funcref)
 *   (elem (i32.const 0) $hidden)
 *   (func (export "getref") (result funcref) ref.func $hidden)
 *   (global (export "global") funcref (ref.func $hidden))
 *   (func (export "getnull") (result funcref) ref.null func))
 * The target function is deliberately not exported.
 */
static const unsigned char module_bytes[] = {
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x0a, 0x02, 0x60,
    0x01, 0x7f, 0x01, 0x7f, 0x60, 0x00, 0x01, 0x70, 0x03, 0x04, 0x03, 0x00,
    0x01, 0x01, 0x04, 0x04, 0x01, 0x70, 0x00, 0x02, 0x06, 0x06, 0x01, 0x70,
    0x00, 0xd2, 0x00, 0x0b, 0x07, 0x25, 0x04, 0x05, 0x74, 0x61, 0x62, 0x6c,
    0x65, 0x01, 0x00, 0x06, 0x67, 0x65, 0x74, 0x72, 0x65, 0x66, 0x00, 0x01,
    0x06, 0x67, 0x6c, 0x6f, 0x62, 0x61, 0x6c, 0x03, 0x00, 0x07, 0x67, 0x65,
    0x74, 0x6e, 0x75, 0x6c, 0x6c, 0x00, 0x02, 0x09, 0x07, 0x01, 0x00, 0x41,
    0x00, 0x0b, 0x01, 0x00, 0x0a, 0x13, 0x03, 0x07, 0x00, 0x20, 0x00, 0x41,
    0x01, 0x6a, 0x0b, 0x04, 0x00, 0xd2, 0x00, 0x0b, 0x04, 0x00, 0xd0, 0x70,
    0x0b,
};

static void check(int condition, const char* message)
{
    if (!condition) {
        fprintf(stderr, "FAIL: %s\n", message);
        exit(1);
    }
}

static void check_function(wasm_ref_t* ref)
{
    check(ref != NULL, "non-null function reference");
    wasm_func_t* func = wasm_ref_as_func(ref);
    check(func != NULL, "function reference cast");
    wasm_functype_t* type = wasm_func_type(func);
    const wasm_valtype_vec_t* params = wasm_functype_params(type);
    const wasm_valtype_vec_t* results = wasm_functype_results(type);
    check(params->size == 1 && results->size == 1, "function arity");
    check(wasm_valtype_kind(params->data[0]) == WASM_I32, "parameter type");
    check(wasm_valtype_kind(results->data[0]) == WASM_I32, "result type");
    wasm_functype_delete(type);

    wasm_val_t arg = WASM_I32_VAL(41);
    wasm_val_t result = WASM_INIT_VAL;
    wasm_val_vec_t args = { 1, &arg };
    wasm_val_vec_t outputs = { 1, &result };
    wasm_trap_t* trap = wasm_func_call(func, &args, &outputs);
    check(trap == NULL, "function call must not trap");
    check(result.kind == WASM_I32 && result.of.i32 == 42, "function result");
}

int main(void)
{
    wasm_engine_t* engine = wasm_engine_new();
    wasm_store_t* store = wasm_store_new(engine);
    wasm_byte_vec_t binary;
    wasm_byte_vec_new(&binary, sizeof(module_bytes), (const char*)module_bytes);
    wasm_module_t* module = wasm_module_new(store, &binary);
    check(module != NULL, "module creation");
    wasm_byte_vec_delete(&binary);
    wasm_extern_vec_t imports = WASM_EMPTY_VEC;
    wasm_trap_t* trap = NULL;
    wasm_instance_t* instance = wasm_instance_new(store, module, &imports, &trap);
    check(instance != NULL && trap == NULL, "instantiation");
    wasm_extern_vec_t exports;
    wasm_instance_exports(instance, &exports);
    check(exports.size == 4, "export count");

    wasm_table_t* table = wasm_extern_as_table(exports.data[0]);
    wasm_ref_t* ref = wasm_table_get(table, 0);
    check_function(ref);
    check(wasm_table_get(table, 1) == NULL, "null table entry");
    check(wasm_table_get(table, 2) == NULL, "out-of-range table entry");
    wasm_func_t* copied_func = wasm_func_copy(wasm_ref_as_func(ref));
    wasm_ref_delete(ref);
    check_function(wasm_func_as_ref(copied_func));

    wasm_val_t original = WASM_INIT_VAL;
    original.kind = WASM_FUNCREF;
    original.of.ref = wasm_func_as_ref(copied_func);
    wasm_val_t copied = WASM_INIT_VAL;
    wasm_val_copy(&copied, &original);
    wasm_val_delete(&original);
    check(copied.kind == WASM_FUNCREF, "copied funcref kind");
    check_function(copied.of.ref);
    wasm_val_delete(&copied);

    wasm_val_vec_t no_args = WASM_EMPTY_VEC;
    wasm_val_t returned = WASM_INIT_VAL;
    wasm_val_vec_t outputs = { 1, &returned };
    trap = wasm_func_call(wasm_extern_as_func(exports.data[1]), &no_args, &outputs);
    check(trap == NULL && returned.kind == WASM_FUNCREF, "funcref return");
    check_function(returned.of.ref);
    wasm_val_delete(&returned);

    wasm_val_t global_value = WASM_INIT_VAL;
    wasm_global_get(wasm_extern_as_global(exports.data[2]), &global_value);
    check(global_value.kind == WASM_FUNCREF, "global funcref kind");
    check_function(global_value.of.ref);
    wasm_val_delete(&global_value);

    returned = (wasm_val_t)WASM_INIT_VAL;
    trap = wasm_func_call(wasm_extern_as_func(exports.data[3]), &no_args, &outputs);
    check(trap == NULL && returned.kind == WASM_FUNCREF && returned.of.ref == NULL,
          "null funcref return");
    wasm_val_delete(&returned);

    original.kind = WASM_FUNCREF;
    original.of.ref = NULL;
    wasm_val_copy(&copied, &original);
    check(copied.kind == WASM_FUNCREF && copied.of.ref == NULL, "null funcref copy");
    wasm_val_delete(&copied);
    original = (wasm_val_t)WASM_I32_VAL(42);
    wasm_val_copy(&copied, &original);
    check(copied.kind == WASM_I32 && copied.of.i32 == 42, "scalar copy");

    wasm_extern_vec_delete(&exports);
    wasm_instance_delete(instance);
    wasm_module_delete(module);
    wasm_store_delete(store);
    wasm_engine_delete(engine);
    puts("PASS: function reference types and copies");
    return 0;
}
