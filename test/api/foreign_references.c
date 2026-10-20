#include <stdio.h>
#include <stdlib.h>
#include "test_helpers.h"
#include "wat.h"

static void check(int ok, const char* message)
{
    if (!ok) {
        fprintf(stderr, "FAIL: %s\n", message);
        exit(1);
    }
}

static wasm_trap_t* identity(const wasm_val_vec_t* args, wasm_val_vec_t* results)
{
    wasm_val_copy(&results->data[0], &args->data[0]);
    return NULL;
}

/* (module (func (export "id") (param externref) (result externref) local.get 0)) */

int main(void)
{
    wasm_engine_t* engine = wasm_engine_new();
    wasm_store_t* store = wasm_store_new(engine);
    wasm_foreign_t* foreign = wasm_foreign_new(store);
    wasm_foreign_t* other = wasm_foreign_new(store);
    check(!wasm_foreign_same(foreign, other), "distinct foreign identities");
    wasm_foreign_delete(other);
    check(wasm_test_store_object_count(store) == 1, "foreign is Store-owned");
    wasm_foreign_t* copy = wasm_foreign_copy(foreign);
    check(wasm_foreign_same(foreign, copy), "foreign copy identity");
    wasm_ref_t* ref = wasm_ref_copy(wasm_foreign_as_ref(foreign));
    check(wasm_foreign_same(foreign, wasm_ref_as_foreign(ref)), "generic copy cast");
    check(wasm_foreign_same(foreign, wasm_ref_as_foreign_const(wasm_foreign_as_ref_const(copy))), "const conversions");
    check(wasm_ref_as_foreign(NULL) == NULL, "null conversion");
    wasm_foreign_delete(copy);

    wasm_val_t value = WASM_INIT_VAL;
    value.kind = WASM_ANYREF;
    value.of.ref = ref;
    wasm_globaltype_t* gt = wasm_globaltype_new(wasm_valtype_new(WASM_ANYREF), WASM_VAR);
    wasm_global_t* global = wasm_global_new(store, gt, &value);
    wasm_globaltype_delete(gt);
    wasm_limits_t limits = { 1, 2 };
    wasm_tabletype_t* tt = wasm_tabletype_new(wasm_valtype_new(WASM_ANYREF), &limits);
    wasm_table_t* table = wasm_table_new(store, tt, ref);
    wasm_tabletype_delete(tt);
    check(wasm_ref_as_foreign(wasm_global_as_ref(global)) == NULL, "reject non-foreign");
    wasm_foreign_delete(foreign);
    wasm_ref_delete(ref);
    check(wasm_test_store_object_count(store) == 3, "Global and Table keep foreign alive");

    wasm_val_t from_global = WASM_INIT_VAL;
    wasm_global_get(global, &from_global);
    wasm_ref_t* from_table = wasm_table_get(table, 0);
    check(wasm_ref_same(from_global.of.ref, from_table), "Global/Table round trip");
    check(wasm_ref_as_foreign(from_table) != NULL, "Table foreign conversion");
    check(wasm_table_grow(table, 1, from_table), "foreign Table growth");
    wasm_ref_t* grown = wasm_table_get(table, 1);
    check(wasm_ref_same(from_table, grown), "grown Table identity");
    wasm_ref_delete(grown);

    wasm_byte_vec_t bytes;
    wasm_test_wat(&bytes, "foreign_references");
    wasm_module_t* module = wasm_module_new(store, &bytes);
    check(module != NULL, "identity module");
    wasm_byte_vec_delete(&bytes);
    wasm_extern_vec_t imports = WASM_EMPTY_VEC;
    wasm_instance_t* instance = wasm_instance_new(store, module, &imports, NULL);
    check(instance != NULL, "identity instance");
    wasm_extern_vec_t exports;
    wasm_instance_exports(instance, &exports);
    wasm_functype_t* ft = wasm_functype_new_1_1(wasm_valtype_new(WASM_ANYREF),
                                                wasm_valtype_new(WASM_ANYREF));
    wasm_func_t* host = wasm_func_new(store, ft, identity);
    wasm_functype_delete(ft);
    wasm_func_t* functions[] = { wasm_extern_as_func(exports.data[0]), host };
    for (size_t i = 0; i < 2; ++i) {
        wasm_val_vec_t args = { 1, &from_global };
        wasm_val_t result = WASM_INIT_VAL;
        wasm_val_vec_t results = { 1, &result };
        check(wasm_func_call(functions[i], &args, &results) == NULL, "reference call");
        check(result.kind == WASM_ANYREF && wasm_ref_same(from_table, result.of.ref),
              "function argument/result identity");
        check(wasm_ref_as_foreign(result.of.ref) != NULL, "returned foreign conversion");
        wasm_val_delete(&result);
        wasm_val_t null_value = WASM_INIT_VAL;
        null_value.kind = WASM_ANYREF;
        null_value.of.ref = NULL;
        args.data = &null_value;
        check(wasm_func_call(functions[i], &args, &results) == NULL, "null reference call");
        check(result.kind == WASM_ANYREF && result.of.ref == NULL, "null externref preserved");
        wasm_val_delete(&result);
    }
    wasm_func_delete(host);
    wasm_extern_vec_delete(&exports);
    wasm_instance_delete(instance);
    wasm_module_delete(module);
    wasm_val_delete(&from_global);
    wasm_ref_delete(from_table);

    value.of.ref = NULL;
    wasm_global_set(global, &value);
    check(wasm_test_store_object_count(store) == 3, "Table alone keeps foreign alive");
    check(wasm_table_set(table, 0, NULL) && wasm_table_set(table, 1, NULL), "clear Table");
    check(wasm_test_store_object_count(store) == 2, "unreachable foreign reclaimed");
    wasm_global_delete(global);
    wasm_table_delete(table);
    check(wasm_test_store_object_count(store) == 0, "all Store objects reclaimed");
    wasm_store_delete(store);
    wasm_engine_delete(engine);
    puts("PASS: foreign reference identity and lifetime");
    return 0;
}
