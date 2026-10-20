#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "test_helpers.h"

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

static wasm_module_t* compile(wasm_store_t* store, const unsigned char* bytes, size_t size)
{
    wasm_byte_vec_t binary = { size, (wasm_byte_t*)bytes };
    wasm_module_t* module = wasm_module_new(store, &binary);
    check(module != NULL, "compile");
    return module;
}

static wasm_trap_t* noop(const wasm_val_vec_t* args, wasm_val_vec_t* results)
{
    (void)args;
    (void)results;
    return NULL;
}

static void immediate(wasm_store_t* store)
{
    wasm_byte_vec_t binary = { sizeof(module_bytes), (wasm_byte_t*)module_bytes };
    check(wasm_module_validate(store, &binary), "validate");
    check(wasm_test_store_object_count(store) == 0, "validation frees temporary module");
    wasm_limits_t limits = { 1, 1 };
    wasm_memorytype_t* type = wasm_memorytype_new(&limits);
    for (int i = 0; i < 100; ++i) {
        wasm_memory_t* memory = wasm_memory_new(store, type);
        wasm_memory_t* copy = wasm_memory_copy(memory);
        wasm_memory_delete(memory);
        check(wasm_test_store_object_count(store) == 1, "copy roots memory");
        wasm_memory_data(copy)[0] = 42;
        wasm_memory_delete(copy);
        check(wasm_test_store_object_count(store) == 0, "memory freed without another allocation");
    }
    wasm_memorytype_delete(type);
}

static void value_batch(wasm_store_t* store)
{
    wasm_functype_t* ft = wasm_functype_new_0_0();
    wasm_val_vec_t values;
    wasm_val_vec_new_uninitialized(&values, 10);
    for (size_t i = 0; i < values.size; ++i) {
        values.data[i].kind = WASM_FUNCREF;
        values.data[i].of.ref = wasm_func_as_ref(wasm_func_new(store, ft, noop));
    }
    wasm_functype_delete(ft);
    size_t before = wasm_test_store_collection_count(store);
    wasm_val_vec_delete(&values);
    check(wasm_test_store_collection_count(store) == before + 1, "value vector collected once");
    check(wasm_test_store_object_count(store) == 0, "value vector freed immediately");
}

static void references(wasm_store_t* store)
{
    wasm_module_t* module = compile(store, module_bytes, sizeof(module_bytes));
    wasm_extern_vec_t imports = WASM_EMPTY_VEC;
    wasm_trap_t* trap = NULL;
    wasm_instance_t* instance = wasm_instance_new(store, module, &imports, &trap);
    check(instance && !trap, "instantiate");
    wasm_instance_set_host_info(instance, store);
    wasm_extern_vec_t exports;
    wasm_instance_exports(instance, &exports);
    wasm_func_t* getref = wasm_func_copy(wasm_extern_as_func(exports.data[1]));
    wasm_extern_vec_delete(&exports);
    wasm_instance_delete(instance);
    wasm_module_delete(module);
    wasm_val_vec_t args = WASM_EMPTY_VEC;
    wasm_val_t result = WASM_INIT_VAL;
    wasm_val_vec_t results = { 1, &result };
    check(!wasm_func_call(getref, &args, &results), "return reference across collection");
    wasm_func_delete(getref);
    check(wasm_func_get_host_info(wasm_ref_as_func(result.of.ref)) == store, "returned function retains defining instance");
    wasm_limits_t limits = { 1, 1 };
    wasm_tabletype_t* tt = wasm_tabletype_new(wasm_valtype_new(WASM_FUNCREF), &limits);
    wasm_table_t* table = wasm_table_new(store, tt, result.of.ref);
    wasm_tabletype_delete(tt);
    wasm_globaltype_t* gt = wasm_globaltype_new(wasm_valtype_new(WASM_FUNCREF), WASM_VAR);
    wasm_global_t* global = wasm_global_new(store, gt, &result);
    wasm_globaltype_delete(gt);
    wasm_val_delete(&result);
    check(wasm_table_set(table, 0, NULL), "clear table reference");
    check(wasm_test_store_object_count(store) > 2, "global still retains instance");
    wasm_val_t nullref = WASM_INIT_VAL;
    nullref.kind = WASM_FUNCREF;
    nullref.of.ref = NULL;
    wasm_global_set(global, &nullref);
    check(wasm_test_store_object_count(store) == 2, "reference mutation collects instance cycle");
    wasm_global_delete(global);
    wasm_table_delete(table);
    check(wasm_test_store_object_count(store) == 0, "all references collected");
}

struct callback_data {
    wasm_store_t* store;
    wasm_store_t* other;
    wasm_func_t* self;
    wasm_memory_t* memory;
};

static wasm_trap_t* delete_during_call(void* env, const wasm_val_vec_t* args, wasm_val_vec_t* results)
{
    (void)args;
    (void)results;
    struct callback_data* data = (struct callback_data*)env;
    size_t own_before = wasm_test_store_collection_count(data->store);
    size_t other_before = wasm_test_store_collection_count(data->other);
    wasm_func_delete(data->self);
    wasm_memory_delete(data->memory);
    wasm_functype_t* ft = wasm_functype_new_0_0();
    wasm_func_t* nested = wasm_func_new(data->store, ft, noop);
    wasm_functype_delete(ft);
    wasm_val_vec_t empty = WASM_EMPTY_VEC;
    check(!wasm_func_call(nested, &empty, &empty), "nested callback");
    wasm_func_delete(nested);
    check(wasm_test_store_collection_count(data->store) == own_before, "defer self deletion");
    check(wasm_test_store_collection_count(data->other) == other_before, "defer other store deletion");
    return NULL;
}

static void callback(wasm_engine_t* engine, wasm_store_t* store)
{
    wasm_store_t* other = wasm_store_new(engine);
    wasm_limits_t limits = { 1, 1 };
    wasm_memorytype_t* mt = wasm_memorytype_new(&limits);
    struct callback_data data = { store, other, NULL, wasm_memory_new(other, mt) };
    wasm_memorytype_delete(mt);
    wasm_functype_t* ft = wasm_functype_new_0_0();
    data.self = wasm_func_new_with_env(store, ft, delete_during_call, &data, NULL);
    wasm_functype_delete(ft);
    check(wasm_func_get_host_info(data.self) == &data, "callback host info");
    wasm_val_vec_t empty = WASM_EMPTY_VEC;
    check(!wasm_func_call(data.self, &empty, &empty), "callback call");
    check(wasm_test_store_object_count(store) == 0, "self collected at outer return");
    check(wasm_test_store_object_count(other) == 0, "other store collected at outer return");
    wasm_store_delete(other);
}

/* (module (import "m" "f" (func)) (memory 1) (table 1 funcref)) */
static const unsigned char import_failure_bytes[] = {
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x04, 0x01, 0x60,
    0x00, 0x00, 0x02, 0x07, 0x01, 0x01, 0x6d, 0x01, 0x66, 0x00, 0x00, 0x04,
    0x04, 0x01, 0x70, 0x00, 0x01, 0x05, 0x03, 0x01, 0x00, 0x01,
};

/* (module (memory 1) (table 1 funcref) (func $f) (elem (i32.const 3) $f)) */
static const unsigned char element_failure_bytes[] = {
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x04, 0x01, 0x60,
    0x00, 0x00, 0x03, 0x02, 0x01, 0x00, 0x04, 0x04, 0x01, 0x70, 0x00, 0x01,
    0x05, 0x03, 0x01, 0x00, 0x01, 0x09, 0x07, 0x01, 0x00, 0x41, 0x03, 0x0b,
    0x01, 0x00, 0x0a, 0x04, 0x01, 0x02, 0x00, 0x0b,
};

static void failed_start(wasm_store_t* store)
{
    wasm_module_t* module = compile(store, start_bytes, sizeof(start_bytes));
    wasm_extern_vec_t imports = WASM_EMPTY_VEC;
    for (int i = 0; i < 100; ++i) {
        wasm_trap_t* trap = NULL;
        check(!wasm_instance_new(store, module, &imports, &trap) && trap, "start trap");
        wasm_trap_delete(trap);
        check(wasm_test_store_object_count(store) == 1, "failed instance immediately collected");
    }
    wasm_module_delete(module);
    check(wasm_test_store_object_count(store) == 0, "failed start cleanup");
    const unsigned char* bytes[] = { import_failure_bytes, element_failure_bytes };
    size_t sizes[] = { sizeof(import_failure_bytes), sizeof(element_failure_bytes) };
    for (size_t i = 0; i < 2; ++i) {
        module = compile(store, bytes[i], sizes[i]);
        wasm_trap_t* trap = NULL;
        check(!wasm_instance_new(store, module, &imports, &trap) && trap, "partial instantiation failure");
        wasm_trap_delete(trap);
        check(wasm_test_store_object_count(store) == 1, "partly initialized instance freed");
        wasm_module_delete(module);
    }
}

#ifdef WALRUS_GC_TESTS
/* (module (type $t (struct (field (mut funcref)))) (func $f) (elem declare func $f) (func (export "make") (result externref) (local (ref null $t)) struct.new_default $t local.tee 0 ref.func $f struct.set $t 0 local.get 0 extern.convert_any)) */
static const unsigned char struct_bytes[] = {
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x0c, 0x03, 0x5f,
    0x01, 0x70, 0x01, 0x60, 0x00, 0x00, 0x60, 0x00, 0x01, 0x6f, 0x03, 0x03,
    0x02, 0x01, 0x02, 0x07, 0x08, 0x01, 0x04, 0x6d, 0x61, 0x6b, 0x65, 0x00,
    0x01, 0x09, 0x05, 0x01, 0x03, 0x00, 0x01, 0x00, 0x0a, 0x19, 0x02, 0x02,
    0x00, 0x0b, 0x14, 0x01, 0x01, 0x63, 0x00, 0xfb, 0x01, 0x00, 0x22, 0x00,
    0xd2, 0x00, 0xfb, 0x05, 0x00, 0x00, 0x20, 0x00, 0xfb, 0x1b, 0x0b,
};

/* (module (type $t (array funcref)) (func $f) (elem declare func $f) (func (export "make") (result externref) ref.func $f i32.const 2 array.new $t extern.convert_any)) */
static const unsigned char array_bytes[] = {
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x0b, 0x03, 0x5e,
    0x70, 0x00, 0x60, 0x00, 0x00, 0x60, 0x00, 0x01, 0x6f, 0x03, 0x03, 0x02,
    0x01, 0x02, 0x07, 0x08, 0x01, 0x04, 0x6d, 0x61, 0x6b, 0x65, 0x00, 0x01,
    0x09, 0x05, 0x01, 0x03, 0x00, 0x01, 0x00, 0x0a, 0x10, 0x02, 0x02, 0x00,
    0x0b, 0x0b, 0x00, 0xd2, 0x00, 0x41, 0x02, 0xfb, 0x06, 0x00, 0xfb, 0x1b,
    0x0b,
};

static void gc_references(wasm_store_t* store)
{
    const unsigned char* bytes[] = { struct_bytes, array_bytes };
    size_t sizes[] = { sizeof(struct_bytes), sizeof(array_bytes) };
    for (size_t i = 0; i < 2; ++i) {
        wasm_byte_vec_t binary = { sizes[i], (wasm_byte_t*)bytes[i] };
        wasm_module_t* module = wasm_test_module_new_gc(store, &binary);
        check(module != NULL, "GC compile");
        wasm_extern_vec_t empty = WASM_EMPTY_VEC;
        wasm_trap_t* trap = NULL;
        wasm_instance_t* instance = wasm_instance_new(store, module, &empty, &trap);
        check(instance && !trap, "GC instantiate");
        wasm_extern_vec_t exports;
        wasm_instance_exports(instance, &exports);
        wasm_val_vec_t args = WASM_EMPTY_VEC;
        wasm_val_t result = WASM_INIT_VAL;
        wasm_val_vec_t results = { 1, &result };
        check(!wasm_func_call(wasm_extern_as_func(exports.data[0]), &args, &results), "GC return");
        wasm_extern_vec_delete(&exports);
        wasm_instance_delete(instance);
        wasm_module_delete(module);
        check(wasm_test_store_object_count(store) > 0, "API GC root retains function and instance");
        wasm_globaltype_t* gt = wasm_globaltype_new(wasm_valtype_new(WASM_ANYREF), WASM_VAR);
        wasm_global_t* global = wasm_global_new(store, gt, &result);
        wasm_globaltype_delete(gt);
        wasm_val_delete(&result);
        check(wasm_test_store_object_count(store) > 1, "global GC root retains function and instance");
        wasm_val_t nullref = WASM_INIT_VAL;
        nullref.kind = WASM_ANYREF;
        nullref.of.ref = NULL;
        wasm_global_set(global, &nullref);
        check(wasm_test_store_object_count(store) == 1, "clearing GC root collects instance");
        wasm_global_delete(global);
    }
}
#endif

static void cached_deletion(wasm_store_t* store)
{
    enum { count = 1000, slots = 100000 };
    wasm_limits_t limits = { slots, slots };
    wasm_tabletype_t* tt = wasm_tabletype_new(wasm_valtype_new(WASM_FUNCREF), &limits);
    wasm_table_t* table = wasm_table_new(store, tt, NULL);
    wasm_tabletype_delete(tt);
    wasm_globaltype_t* gt = wasm_globaltype_new(wasm_valtype_new(WASM_I32), WASM_VAR);
    wasm_val_t value = WASM_I32_VAL(0);
    wasm_global_t* globals[count];
    for (size_t i = 0; i < count; ++i) {
        globals[i] = wasm_global_new(store, gt, &value);
    }
    wasm_globaltype_delete(gt);
    size_t builds = wasm_test_store_graph_build_count(store);
    size_t scans = wasm_test_store_table_slot_count(store);
    size_t traces = wasm_test_store_graph_trace_count(store);
    // Alternate ends to exercise swapped vector entries and their saved indices.
    for (size_t i = 0; i < count; ++i) {
        size_t index = i % 2 ? count - 1 - i / 2 : i / 2;
        wasm_global_delete(globals[index]);
        check(wasm_test_store_object_count(store) == count - i, "individual delete immediately frees object");
    }
    check(wasm_test_store_graph_build_count(store) == builds + 1, "one graph build for 1000 deletes");
    check(wasm_test_store_table_slot_count(store) == scans + slots, "100000 slots scanned once, not 1000 times");
    check(wasm_test_store_graph_trace_count(store) == traces + 1, "zero incoming reference fast path");
    check(wasm_test_store_graph_edge_count(store) == 0, "null slots produce no edges");
    wasm_table_delete(table);
    check(wasm_test_store_object_count(store) == 0, "cached table freed");

    // A fresh, unreferenced table must not be scanned just to populate a cache
    // that is about to be discarded.
    tt = wasm_tabletype_new(wasm_valtype_new(WASM_FUNCREF), &limits);
    table = wasm_table_new(store, tt, NULL);
    wasm_tabletype_delete(tt);
    scans = wasm_test_store_table_slot_count(store);
    wasm_table_delete(table);
    check(wasm_test_store_table_slot_count(store) == scans, "dead table requires no slot scan");
    check(wasm_test_store_object_count(store) == 0, "dead table freed immediately");
}

static void duplicate_edges(wasm_store_t* store)
{
    wasm_functype_t* ft = wasm_functype_new_0_0();
    wasm_func_t* func = wasm_func_new(store, ft, noop);
    wasm_functype_delete(ft);
    wasm_limits_t limits = { 100000, 100001 };
    wasm_tabletype_t* tt = wasm_tabletype_new(wasm_valtype_new(WASM_FUNCREF), &limits);
    wasm_table_t* table = wasm_table_new(store, tt, wasm_func_as_ref(func));
    wasm_tabletype_delete(tt);
    wasm_func_delete(func);
    check(wasm_test_store_graph_edge_count(store) == 1, "duplicate slots share one edge");
    check(wasm_table_set(table, 0, NULL), "remove one duplicate");
    check(wasm_test_store_graph_edge_count(store) == 1, "other slots retain edge");
    wasm_ref_t* ref = wasm_table_get(table, 1);
    check(ref != NULL, "remaining slot is callable");
    wasm_val_vec_t empty = WASM_EMPTY_VEC;
    check(!wasm_func_call(wasm_ref_as_func(ref), &empty, &empty), "call retained duplicate");
    wasm_ref_delete(ref);
    size_t builds = wasm_test_store_graph_build_count(store);
    size_t scans = wasm_test_store_table_slot_count(store);
    wasm_table_delete(table);
    check(wasm_test_store_object_count(store) == 0, "table deletion cascades to last function");
    check(wasm_test_store_graph_build_count(store) == builds, "cascade uses cached graph");
    check(wasm_test_store_table_slot_count(store) == scans, "cascade does not rescan slots");
}

static void grow_invalidates(wasm_store_t* store)
{
    wasm_functype_t* ft = wasm_functype_new_0_0();
    wasm_func_t* first = wasm_func_new(store, ft, noop);
    wasm_func_t* second = wasm_func_new(store, ft, noop);
    wasm_functype_delete(ft);
    wasm_limits_t limits = { 1, 2 };
    wasm_tabletype_t* tt = wasm_tabletype_new(wasm_valtype_new(WASM_FUNCREF), &limits);
    wasm_table_t* table = wasm_table_new(store, tt, wasm_func_as_ref(first));
    wasm_tabletype_delete(tt);
    wasm_func_delete(first); // Build a snapshot that does not yet link second.
    size_t builds = wasm_test_store_graph_build_count(store);
    check(wasm_table_grow(table, 1, wasm_func_as_ref(second)), "grow table");
    check(wasm_test_store_graph_build_count(store) == builds + 1, "grow rebuilds cached edges");
    wasm_func_delete(second);
    check(wasm_test_store_object_count(store) == 3, "grown slot retains second function");
    check(wasm_table_set(table, 0, NULL), "remove first function");
    check(wasm_test_store_object_count(store) == 2, "mutation removes old cached edge");
    wasm_table_delete(table);
    check(wasm_test_store_object_count(store) == 0, "grown table cleanup");
}

static void cached_cycle(wasm_store_t* store)
{
    wasm_module_t* module = compile(store, module_bytes, sizeof(module_bytes));
    wasm_extern_vec_t imports = WASM_EMPTY_VEC;
    wasm_trap_t* trap = NULL;
    wasm_instance_t* instance = wasm_instance_new(store, module, &imports, &trap);
    check(instance && !trap, "cycle instance");
    wasm_extern_vec_t exports;
    wasm_instance_exports(instance, &exports);
    wasm_val_t value = WASM_INIT_VAL;
    value.kind = WASM_FUNCREF;
    value.of.ref = wasm_extern_as_ref(exports.data[1]);
    wasm_globaltype_t* gt = wasm_globaltype_new(wasm_valtype_new(WASM_FUNCREF), WASM_VAR);
    wasm_global_t* holder = wasm_global_new(store, gt, &value);
    wasm_globaltype_delete(gt);
    wasm_extern_vec_delete(&exports);
    wasm_instance_delete(instance);
    wasm_module_delete(module);
    size_t builds = wasm_test_store_graph_build_count(store);
    size_t traces = wasm_test_store_graph_trace_count(store);
    size_t scans = wasm_test_store_table_slot_count(store);
    // Holder itself has no incoming edges, but deleting it disconnects a cycle.
    wasm_global_delete(holder);
    check(wasm_test_store_object_count(store) == 0, "fast deletion also collects disconnected cycles");
    check(wasm_test_store_graph_build_count(store) == builds, "cycle uses cached graph");
    check(wasm_test_store_graph_trace_count(store) == traces + 1, "disconnected cycle triggers reachability check");
    check(wasm_test_store_table_slot_count(store) == scans, "cycle tracing skips original table slots");
}

/* (module (import "m" "table" (table 1 funcref)) (func (export "clear") i32.const 0 ref.null func table.set 0)) */
static const unsigned char clear_table_bytes[] = {
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x04, 0x01, 0x60,
    0x00, 0x00, 0x02, 0x0d, 0x01, 0x01, 0x6d, 0x05, 0x74, 0x61, 0x62, 0x6c,
    0x65, 0x01, 0x70, 0x00, 0x01, 0x03, 0x02, 0x01, 0x00, 0x07, 0x09, 0x01,
    0x05, 0x63, 0x6c, 0x65, 0x61, 0x72, 0x00, 0x00, 0x0a, 0x0a, 0x01, 0x08,
    0x00, 0x41, 0x00, 0xd0, 0x70, 0x26, 0x00, 0x0b,
};

static void wasm_mutation_invalidates(wasm_store_t* store)
{
    wasm_functype_t* ft = wasm_functype_new_0_0();
    wasm_func_t* target = wasm_func_new(store, ft, noop);
    wasm_functype_delete(ft);
    wasm_limits_t limits = { 1, 1 };
    wasm_tabletype_t* tt = wasm_tabletype_new(wasm_valtype_new(WASM_FUNCREF), &limits);
    wasm_table_t* table = wasm_table_new(store, tt, wasm_func_as_ref(target));
    wasm_tabletype_delete(tt);
    wasm_module_t* module = compile(store, clear_table_bytes, sizeof(clear_table_bytes));
    wasm_extern_t* imported = wasm_table_as_extern(table);
    wasm_extern_vec_t imports = { 1, &imported };
    wasm_trap_t* trap = NULL;
    wasm_instance_t* instance = wasm_instance_new(store, module, &imports, &trap);
    check(instance && !trap, "table mutator instance");
    wasm_extern_vec_t exports;
    wasm_instance_exports(instance, &exports);
    wasm_func_delete(target);
    size_t objects = wasm_test_store_object_count(store);
    size_t builds = wasm_test_store_graph_build_count(store);
    wasm_val_vec_t empty = WASM_EMPTY_VEC;
    check(!wasm_func_call(wasm_extern_as_func(exports.data[0]), &empty, &empty), "wasm clears table");
    check(wasm_test_store_graph_build_count(store) == builds + 1, "wasm/JIT mutation invalidates snapshot");
    check(wasm_test_store_object_count(store) == objects - 1, "wasm mutation collects former target before return");
    wasm_extern_vec_delete(&exports);
    wasm_instance_delete(instance);
    wasm_module_delete(module);
    wasm_table_delete(table);
    check(wasm_test_store_object_count(store) == 0, "mutator cleanup");
}

static void benchmark(wasm_store_t* store)
{
    const size_t count = 1000;
    const unsigned table_sizes[] = { 0, 10000, 100000 };
    wasm_globaltype_t* gt = wasm_globaltype_new(wasm_valtype_new(WASM_I32), WASM_VAR);
    wasm_val_t value = WASM_I32_VAL(0);
    for (size_t t = 0; t < 3; ++t) {
        wasm_limits_t limits = { table_sizes[t], table_sizes[t] };
        wasm_tabletype_t* tt = wasm_tabletype_new(wasm_valtype_new(WASM_FUNCREF), &limits);
        wasm_table_t* table = wasm_table_new(store, tt, NULL);
        wasm_tabletype_delete(tt);
        for (int batch = 0; batch <= 1; ++batch) {
            wasm_extern_vec_t values;
            wasm_extern_vec_new_uninitialized(&values, count);
            for (size_t i = 0; i < count; ++i) {
                values.data[i] = wasm_global_as_extern(wasm_global_new(store, gt, &value));
            }
            size_t before = wasm_test_store_collection_count(store);
            size_t builds_before = wasm_test_store_graph_build_count(store);
            size_t slots_before = wasm_test_store_table_slot_count(store);
            size_t traces_before = wasm_test_store_graph_trace_count(store);
            clock_t start = clock();
            if (batch) {
                wasm_extern_vec_delete(&values);
            } else {
                for (size_t i = 0; i < count; ++i) {
                    wasm_extern_delete(values.data[i]);
                    values.data[i] = NULL;
                }
                wasm_extern_vec_delete(&values);
            }
            double ms = (double)(clock() - start) * 1000 / CLOCKS_PER_SEC;
            size_t collections = wasm_test_store_collection_count(store) - before;
            check(collections == (batch ? 1 : count), "bulk deletion runs one collection");
            check(wasm_test_store_object_count(store) == 1, "globals freed before delete returns");
            size_t builds = wasm_test_store_graph_build_count(store) - builds_before;
            size_t slots = wasm_test_store_table_slot_count(store) - slots_before;
            size_t traces = wasm_test_store_graph_trace_count(store) - traces_before;
            check(builds == 1, "consecutive deletes reuse graph");
            check(slots == table_sizes[t], "table slots scanned once");
            check(traces == 1, "independent deletes avoid graph tracing");
            printf("table=%u delete=%s objects=%zu collections=%zu builds=%zu slots=%zu traces=%zu cpu_ms=%.3f\n",
                   table_sizes[t], batch ? "vector" : "individual", count, collections, builds, slots, traces, ms);
        }
        wasm_module_t* module = compile(store, module_bytes, sizeof(module_bytes));
        wasm_extern_vec_t imports = WASM_EMPTY_VEC;
        wasm_trap_t* trap = NULL;
        wasm_instance_t* instance = wasm_instance_new(store, module, &imports, &trap);
        check(instance && !trap, "benchmark instance");
        wasm_extern_vec_t exports;
        wasm_instance_exports(instance, &exports);
        wasm_func_t* fn = wasm_func_copy(wasm_extern_as_func(exports.data[3]));
        wasm_extern_vec_delete(&exports);
        wasm_instance_delete(instance);
        wasm_module_delete(module);
        wasm_val_vec_t empty = WASM_EMPTY_VEC;
        wasm_val_t result = WASM_INIT_VAL;
        wasm_val_vec_t results = { 1, &result };
        clock_t start = clock();
        for (size_t i = 0; i < count; ++i) {
            check(!wasm_func_call(fn, &empty, &results), "repeated wasm calls");
            check(result.of.ref == NULL, "null return");
        }
        printf("table=%u calls=%zu cpu_ms=%.3f\n", table_sizes[t], count,
               (double)(clock() - start) * 1000 / CLOCKS_PER_SEC);
        wasm_func_delete(fn);
        wasm_table_delete(table);
    }
    wasm_globaltype_delete(gt);
}

static void delete_after_store(wasm_engine_t* engine)
{
    wasm_store_t* store = wasm_store_new(engine);
    wasm_module_t* module = compile(store, module_bytes, sizeof(module_bytes));
    wasm_extern_vec_t imports = WASM_EMPTY_VEC;
    wasm_trap_t* trap = NULL;
    wasm_instance_t* instance = wasm_instance_new(store, module, &imports, &trap);
    check(instance && !trap, "instantiate before store deletion");
    wasm_extern_vec_t exports;
    wasm_instance_exports(instance, &exports);
    wasm_func_t* function = wasm_func_copy(wasm_extern_as_func(exports.data[1]));
    wasm_limits_t limits = { 1, 1 };
    wasm_memorytype_t* type = wasm_memorytype_new(&limits);
    wasm_memory_t* memory = wasm_memory_new(store, type);
    wasm_memorytype_delete(type);

    /* Embedders may finalize wrappers after destroying the store. */
    wasm_store_delete(store);
    wasm_func_delete(function);
    wasm_extern_vec_delete(&exports);
    wasm_memory_delete(memory);
    wasm_instance_delete(instance);
    wasm_module_delete(module);
}

int main(int argc, char** argv)
{
    wasm_engine_t* engine = wasm_engine_new();
    wasm_store_t* store = wasm_store_new(engine);
    immediate(store);
    value_batch(store);
    references(store);
    callback(engine, store);
    failed_start(store);
    cached_deletion(store);
    duplicate_edges(store);
    grow_invalidates(store);
    cached_cycle(store);
    wasm_mutation_invalidates(store);
#ifdef WALRUS_GC_TESTS
    gc_references(store);
#endif
    if (argc > 1 && strcmp(argv[1], "--benchmark") == 0) {
        benchmark(store);
    }
    check(wasm_test_store_object_count(store) == 0, "empty store");
    wasm_store_delete(store);
    delete_after_store(engine);
    wasm_engine_delete(engine);
    puts("store collection: PASS");
    return 0;
}
