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

enum { FOREIGN,
       GLOBAL,
       TABLE,
       PAYLOAD_KINDS };
enum { SET,
       GET,
       CLEAR,
       TAKE,
       MAKE,
       READ,
       REPLACE,
       TAKE_BOX,
       TAKE_LARGE,
       TAKE_TAIL,
       TAKE_TRAP,
       EXPORT_COUNT };

struct callback_data {
    wasm_store_t* store;
    wasm_func_t* nested;
    size_t calls;
};

static wasm_trap_t* collect_during_call(void* env, const wasm_val_vec_t* args, wasm_val_vec_t* results)
{
    (void)args;
    (void)results;
    struct callback_data* data = env;
    size_t before = wasm_test_store_collection_count(data->store);
    size_t scans = wasm_test_store_stack_scan_count(data->store);
    wasm_foreign_t* garbage = wasm_foreign_new(data->store);
    wasm_foreign_delete(garbage);
    wasm_val_vec_t empty = WASM_EMPTY_VEC;
    check(!wasm_func_call(data->nested, &empty, &empty), "reentrant Wasm call");
    check(wasm_test_store_collection_count(data->store) > before, "collect while reference is on Wasm stack");
    check(wasm_test_store_stack_scan_count(data->store) > scans, "scan native roots during callback");
    ++data->calls;
    return NULL;
}

static wasm_val_t call(wasm_func_t* func, wasm_val_t* args, size_t count, int has_result)
{
    wasm_val_vec_t arguments = { count, args };
    wasm_val_t result = WASM_INIT_VAL;
    wasm_val_vec_t results = { has_result ? 1 : 0, has_result ? &result : NULL };
    check(!wasm_func_call(func, &arguments, &results), "Wasm call");
    return result;
}

static wasm_ref_t* payload(wasm_store_t* store, int kind)
{
    if (kind == FOREIGN) {
        return wasm_foreign_as_ref(wasm_foreign_new(store));
    }
    if (kind == GLOBAL) {
        wasm_val_t value = WASM_I32_VAL(1234567);
        wasm_globaltype_t* type = wasm_globaltype_new(wasm_valtype_new(WASM_I32), WASM_VAR);
        wasm_global_t* global = wasm_global_new(store, type, &value);
        wasm_globaltype_delete(type);
        return wasm_global_as_ref(global);
    }
    wasm_limits_t limits = { 3, 3 };
    wasm_tabletype_t* type = wasm_tabletype_new(wasm_valtype_new(WASM_ANYREF), &limits);
    wasm_table_t* table = wasm_table_new(store, type, NULL);
    wasm_tabletype_delete(type);
    return wasm_table_as_ref(table);
}

static void check_payload(wasm_ref_t* ref, int kind)
{
    check(ref != NULL, "non-null external object survives");
    if (kind == FOREIGN) {
        check(wasm_ref_as_foreign(ref) != NULL, "foreign object survives");
    } else if (kind == GLOBAL) {
        wasm_val_t value = WASM_INIT_VAL;
        wasm_global_get(wasm_ref_as_global(ref), &value);
        check(value.kind == WASM_I32 && value.of.i32 == 1234567, "external Global retains its value");
        wasm_val_delete(&value);
    } else {
        check(wasm_table_size(wasm_ref_as_table(ref)) == 3, "external Table retains its size");
    }
}

struct result_data {
    wasm_store_t* store;
    int kind;
};

static wasm_trap_t* make_results(void* env, const wasm_val_vec_t* args, wasm_val_vec_t* results)
{
    (void)args;
    struct result_data* data = env;
    for (size_t i = 0; i < results->size; ++i) {
        results->data[i].kind = WASM_ANYREF;
        results->data[i].of.ref = payload(data->store, data->kind);
    }
    return NULL;
}

static void callback_results(wasm_store_t* store, int kind)
{
    size_t baseline = wasm_test_store_object_count(store);
    struct result_data data = { store, kind };
    wasm_valtype_t* types[] = { wasm_valtype_new(WASM_ANYREF), wasm_valtype_new(WASM_ANYREF) };
    wasm_valtype_vec_t params = WASM_EMPTY_VEC, results;
    wasm_valtype_vec_new(&results, 2, types);
    wasm_functype_t* type = wasm_functype_new(&params, &results);
    wasm_func_t* func = wasm_func_new_with_env(store, type, make_results, &data, NULL);
    wasm_functype_delete(type);
    wasm_val_vec_t empty = WASM_EMPTY_VEC;
    wasm_val_t values[] = { WASM_INIT_VAL, WASM_INIT_VAL };
    wasm_val_vec_t outputs = { 2, values };
    check(!wasm_func_call(func, &empty, &outputs), "return newly allocated references from host callback");
    wasm_func_delete(func);
    for (size_t i = 0; i < 2; ++i) {
        check_payload(values[i].of.ref, kind);
        wasm_val_delete(&values[i]);
    }
    check(wasm_test_store_object_count(store) == baseline, "callback result bridge releases all roots");
}

static void global_struct(wasm_store_t* store, wasm_func_t** funcs, struct callback_data* data, int kind, int take)
{
    size_t baseline = wasm_test_store_object_count(store);
    wasm_val_t arg = WASM_INIT_VAL;
    arg.of.ref = payload(store, kind);
    call(funcs[SET], &arg, 1, 0);
    wasm_val_t result = call(funcs[GET], NULL, 0, 1);
    check(wasm_ref_same(arg.of.ref, result.of.ref), "struct preserves external object identity");
    wasm_val_delete(&result);
    wasm_val_delete(&arg);
    check(wasm_test_store_object_count(store) == baseline + 1, "global -> struct retains external object without C handle");

    result = call(funcs[GET], NULL, 0, 1);
    check_payload(result.of.ref, kind);
    wasm_val_delete(&result);

    size_t callbacks = data->calls;
    size_t frames = wasm_test_store_owned_frame_scan_count(store);
    result = call(funcs[take], NULL, 0, 1);
    check(data->calls == callbacks + 1, "stack-only reference crosses collecting callback");
    check_payload(result.of.ref, kind);
    if (take == TAKE_LARGE) {
        check(wasm_test_store_owned_frame_scan_count(store) > frames, "scan separately allocated Wasm frame");
    }
    check(wasm_test_store_object_count(store) == baseline + 1, "return value becomes a root before collection");
    wasm_val_delete(&result);
    check(wasm_test_store_object_count(store) == baseline, "collect object after last returned reference is dropped");
}

static void nested_gc_root(wasm_store_t* store, wasm_func_t** funcs, int kind, int route)
{
    size_t baseline = wasm_test_store_object_count(store);
    wasm_val_t arg = WASM_INIT_VAL;
    arg.of.ref = payload(store, kind);
    wasm_val_t root = call(funcs[MAKE], &arg, 1, 1);
    wasm_val_t result = call(funcs[READ], &root, 1, 1);
    check(wasm_ref_same(arg.of.ref, result.of.ref), "nested GC fields preserve external object identity");
    wasm_val_delete(&result);
    wasm_val_delete(&arg);
    check(wasm_test_store_object_count(store) == baseline + 1, "API GC root retains nested external object");

    if (route == 1) {
        wasm_globaltype_t* type = wasm_globaltype_new(wasm_valtype_new(WASM_ANYREF), WASM_VAR);
        wasm_global_t* global = wasm_global_new(store, type, &root);
        wasm_globaltype_delete(type);
        wasm_val_delete(&root);
        check(wasm_test_store_object_count(store) == baseline + 2, "Global roots nested GC fields");
        wasm_global_get(global, &root);
        wasm_global_delete(global);
    } else if (route == 2) {
        wasm_limits_t limits = { 1, 1 };
        wasm_tabletype_t* type = wasm_tabletype_new(wasm_valtype_new(WASM_ANYREF), &limits);
        wasm_table_t* table = wasm_table_new(store, type, NULL);
        wasm_tabletype_delete(type);
        check(wasm_table_set(table, 0, root.of.ref), "Table accepts GC reference");
        wasm_val_delete(&root);
        check(wasm_test_store_object_count(store) == baseline + 2, "Table roots nested GC fields");
        root.kind = WASM_ANYREF;
        root.of.ref = wasm_table_get(table, 0);
        wasm_table_delete(table);
    }

    result = call(funcs[READ], &root, 1, 1);
    check_payload(result.of.ref, kind);
    wasm_val_delete(&result);

    // Mutating an array must replace cached edges even through a cyclic GC graph.
    wasm_val_t replacement = WASM_INIT_VAL;
    replacement.of.ref = payload(store, kind);
    wasm_val_t args[] = { root, replacement };
    call(funcs[REPLACE], args, 2, 0);
    check(wasm_test_store_object_count(store) == baseline + 1, "GC mutation collects former external object");
    result = call(funcs[READ], &root, 1, 1);
    check(wasm_ref_same(replacement.of.ref, result.of.ref), "GC mutation preserves replacement identity");
    wasm_val_delete(&result);
    wasm_val_delete(&replacement);

    wasm_val_t copy = WASM_INIT_VAL;
    wasm_val_copy(&copy, &root);
    wasm_val_delete(&root);
    result = call(funcs[READ], &copy, 1, 1);
    check_payload(result.of.ref, kind);
    wasm_val_delete(&result);
    wasm_val_delete(&copy);
    check(wasm_test_store_object_count(store) == baseline, "dropping GC roots releases external object despite array cycle");
}

int main(void)
{
    wasm_engine_t* engine = wasm_engine_new();
    wasm_store_t* store = wasm_store_new(engine);
    struct callback_data data = { store, NULL, 0 };
    wasm_functype_t* type = wasm_functype_new_0_0();
    wasm_func_t* callback = wasm_func_new_with_env(store, type, collect_during_call, &data, NULL);
    wasm_functype_delete(type);
    wasm_byte_vec_t bytes;
    wasm_test_wat(&bytes, "gc_references");
    wasm_module_t* module = wasm_test_module_new_gc(store, &bytes);
    wasm_byte_vec_delete(&bytes);
    check(module != NULL, "compile GC reference fixture");
    wasm_extern_t* imported = wasm_func_as_extern(callback);
    wasm_extern_vec_t imports = { 1, &imported };
    wasm_trap_t* trap = NULL;
    wasm_instance_t* instance = wasm_instance_new(store, module, &imports, &trap);
    check(instance != NULL && trap == NULL, "instantiate GC reference fixture");
    wasm_extern_vec_t exports;
    wasm_instance_exports(instance, &exports);
    check(exports.size == EXPORT_COUNT, "fixture exports");
    wasm_func_t* funcs[EXPORT_COUNT];
    for (size_t i = 0; i < EXPORT_COUNT; ++i) {
        funcs[i] = wasm_extern_as_func(exports.data[i]);
    }
    data.nested = funcs[CLEAR];
    wasm_func_delete(callback);
    wasm_instance_delete(instance);
    wasm_module_delete(module);

    for (int kind = 0; kind < PAYLOAD_KINDS; ++kind) {
        callback_results(store, kind);
        global_struct(store, funcs, &data, kind, TAKE);
        global_struct(store, funcs, &data, kind, TAKE_BOX);
        global_struct(store, funcs, &data, kind, TAKE_LARGE);
        global_struct(store, funcs, &data, kind, TAKE_TAIL);
        for (int route = 0; route < 3; ++route) {
            nested_gc_root(store, funcs, kind, route);
        }
        size_t baseline = wasm_test_store_object_count(store);
        wasm_val_t arg = WASM_INIT_VAL;
        arg.of.ref = payload(store, kind);
        call(funcs[SET], &arg, 1, 0);
        wasm_val_delete(&arg);
        wasm_val_vec_t empty = WASM_EMPTY_VEC;
        trap = wasm_func_call(funcs[TAKE_TRAP], &empty, &empty);
        check(trap != NULL, "trap after collecting with a live frame");
        wasm_trap_delete(trap);
        check(wasm_test_store_object_count(store) == baseline, "unwind releases frame roots");
        global_struct(store, funcs, &data, kind, TAKE_LARGE);
    }
    wasm_extern_vec_delete(&exports);
    check(wasm_test_store_object_count(store) == 0, "all GC reference test objects reclaimed");
    wasm_store_delete(store);
    wasm_engine_delete(engine);
    puts("PASS: GC references through globals, structs, arrays, API roots and reentrant calls");
    return 0;
}
