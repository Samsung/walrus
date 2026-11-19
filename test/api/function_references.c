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

static wasm_foreign_t* callback_foreign;

static wasm_trap_t* return_foreign(const wasm_val_vec_t* args, wasm_val_vec_t* results)
{
    (void)args;
    results->data[0].kind = WASM_ANYREF;
    results->data[0].of.ref = wasm_foreign_as_ref(wasm_foreign_copy(callback_foreign));
    return NULL;
}

static void check_reference_type_validation(wasm_store_t* store)
{
    static const unsigned char bytes[] = {
        0, 97, 115, 109, 1, 0, 0, 0, 1, 8, 1, 96, 1, 99, 115, 1, 99, 112,
        3, 2, 1, 0, 7, 6, 1, 2, 119, 50, 0, 0, 10, 6, 1, 4, 0, 32, 0, 11,
    };
    static const unsigned char non_nullable_bytes[] = {
        0, 97, 115, 109, 1, 0, 0, 0, 1, 8, 1, 96, 1, 100, 115, 1, 99, 112,
        3, 2, 1, 0, 7, 6, 1, 2, 119, 50, 0, 0, 10, 6, 1, 4, 0, 32, 0, 11,
    };
    wasm_byte_vec_t binary;
    wasm_byte_vec_new(&binary, sizeof(bytes), (const char*)bytes);
    wasm_module_t* module = wasm_module_new(store, &binary);
    check(module != NULL, "bottom reference module");
    wasm_byte_vec_delete(&binary);

    wasm_exporttype_vec_t export_types;
    wasm_module_exports(module, &export_types);
    const wasm_functype_t* function_type =
        wasm_externtype_as_functype_const(wasm_exporttype_type(export_types.data[0]));
    check(wasm_valtype_kind(wasm_functype_params(function_type)->data[0]) == WASM_FUNCREF,
          "bottom function reference parameter kind");
    wasm_exporttype_vec_delete(&export_types);

    wasm_extern_vec_t imports = WASM_EMPTY_VEC;
    wasm_instance_t* instance = wasm_instance_new(store, module, &imports, NULL);
    check(instance != NULL, "bottom reference instance");
    wasm_extern_vec_t exports;
    wasm_instance_exports(instance, &exports);
    wasm_func_t* function = wasm_extern_as_func(exports.data[0]);

    wasm_foreign_t* foreign = wasm_foreign_new(store);
    wasm_val_t argument = WASM_INIT_VAL;
    argument.kind = WASM_ANYREF;
    argument.of.ref = wasm_foreign_as_ref(wasm_foreign_copy(foreign));
    wasm_val_t result = WASM_INIT_VAL;
    wasm_val_vec_t args = { 1, &argument };
    wasm_val_vec_t results = { 1, &result };
    wasm_trap_t* trap = wasm_func_call(function, &args, &results);
    check(trap != NULL, "reject incompatible function reference argument");
    wasm_trap_delete(trap);
    wasm_val_delete(&argument);
    wasm_val_delete(&result);

    argument.kind = WASM_FUNCREF;
    argument.of.ref = NULL;
    result = (wasm_val_t)WASM_INIT_VAL;
    trap = wasm_func_call(function, &args, &results);
    check(trap == NULL, "accept null bottom function reference");
    check(result.kind == WASM_FUNCREF && result.of.ref == NULL,
          "preserve null function reference result");
    wasm_val_delete(&result);

    wasm_byte_vec_new(&binary, sizeof(non_nullable_bytes),
                      (const char*)non_nullable_bytes);
    wasm_module_t* non_nullable_module = wasm_module_new(store, &binary);
    check(non_nullable_module != NULL, "non-nullable bottom reference module");
    wasm_byte_vec_delete(&binary);
    wasm_instance_t* non_nullable_instance =
        wasm_instance_new(store, non_nullable_module, &imports, NULL);
    check(non_nullable_instance != NULL, "non-nullable bottom reference instance");
    wasm_extern_vec_t non_nullable_exports;
    wasm_instance_exports(non_nullable_instance, &non_nullable_exports);
    result = (wasm_val_t)WASM_INIT_VAL;
    trap = wasm_func_call(wasm_extern_as_func(non_nullable_exports.data[0]),
                          &args, &results);
    check(trap != NULL, "reject null non-nullable bottom reference");
    wasm_trap_delete(trap);
    wasm_val_delete(&result);
    wasm_extern_vec_delete(&non_nullable_exports);
    wasm_instance_delete(non_nullable_instance);
    wasm_module_delete(non_nullable_module);

    wasm_val_t inconsistent = WASM_INIT_VAL;
    inconsistent.kind = WASM_FUNCREF;
    inconsistent.of.ref = wasm_foreign_as_ref(wasm_foreign_copy(foreign));
    wasm_val_vec_t inconsistent_args = { 1, &inconsistent };
    result = (wasm_val_t)WASM_INIT_VAL;
    trap = wasm_func_call(function, &inconsistent_args, &results);
    check(trap != NULL, "reject mistagged function reference argument");
    wasm_trap_delete(trap);
    wasm_val_delete(&result);

    wasm_globaltype_t* global_type = wasm_globaltype_new(
        wasm_valtype_new(WASM_FUNCREF), WASM_CONST);
    wasm_global_t* global = wasm_global_new(store, global_type, &inconsistent);
    check(global == NULL, "reject mistagged function reference global");
    wasm_globaltype_delete(global_type);

    check(wasm_ref_as_func(inconsistent.of.ref) == NULL, "reject foreign function cast");
    wasm_val_t copied = WASM_INIT_VAL;
    wasm_val_copy(&copied, &inconsistent);
    check(copied.kind == inconsistent.kind, "preserve copied reference kind");
    wasm_val_delete(&inconsistent);
    wasm_val_delete(&copied);

    callback_foreign = foreign;
    wasm_valtype_vec_t no_params = WASM_EMPTY_VEC;
    wasm_valtype_vec_t function_results;
    wasm_valtype_vec_new_uninitialized(&function_results, 1);
    function_results.data[0] = wasm_valtype_new(WASM_FUNCREF);
    wasm_functype_t* host_type = wasm_functype_new(&no_params, &function_results);
    wasm_func_t* host = wasm_func_new(store, host_type, return_foreign);
    wasm_functype_delete(host_type);
    wasm_val_vec_t no_args = WASM_EMPTY_VEC;
    result = (wasm_val_t)WASM_INIT_VAL;
    trap = wasm_func_call(host, &no_args, &results);
    check(trap != NULL, "reject incompatible callback result");
    wasm_trap_delete(trap);
    wasm_val_delete(&result);
    wasm_func_delete(host);
    callback_foreign = NULL;
    wasm_foreign_delete(foreign);

    wasm_extern_vec_delete(&exports);
    wasm_instance_delete(instance);
    wasm_module_delete(module);
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
    check_reference_type_validation(store);
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
