#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "test_helpers.h"

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
    wasm_trap_t* second_copy = wasm_trap_copy(copy);
    check(wasm_trap_same(copy, second_copy), "copy of copy shares trap identity");
    wasm_trap_delete(copy);
    wasm_trap_message(second_copy, &message);
    check(message.size > 1 && message.data[message.size - 1] == '\0', "second copy owns message");
    wasm_byte_vec_delete(&message);
    wasm_trap_delete(second_copy);
}

static int callback_mode;

static wasm_trap_t* host_callback(const wasm_val_vec_t* args, wasm_val_vec_t* results)
{
    check(args->size == 1 && results->size == 2, "callback arity");
    check(args->data[0].kind == WASM_ANYREF, "callback reference argument");
    if (callback_mode == 0) {
        wasm_trap_trigger("host callback", strlen("host callback"));
    }
    wasm_val_copy(&results->data[0], &args->data[0]);
    if (callback_mode == 1) {
        wasm_trap_trigger("partial results", strlen("partial results"));
    }
    wasm_val_copy(&results->data[1], &args->data[0]);
    return NULL;
}

static wasm_trap_t* host_callback_with_env(void* env, const wasm_val_vec_t* args, wasm_val_vec_t* results)
{
    check(env == &callback_mode, "callback environment");
    return host_callback(args, results);
}

static void check_callback_cleanup(wasm_store_t* store)
{
    size_t initial_objects = wasm_test_store_object_count(store);
    wasm_valtype_t* param_types[] = { wasm_valtype_new(WASM_ANYREF) };
    wasm_valtype_t* result_types[] = { wasm_valtype_new(WASM_ANYREF), wasm_valtype_new(WASM_ANYREF) };
    wasm_valtype_vec_t params, results;
    wasm_valtype_vec_new(&params, 1, param_types);
    wasm_valtype_vec_new(&results, 2, result_types);
    wasm_functype_t* type = wasm_functype_new(&params, &results);
    wasm_func_t* functions[] = {
        wasm_func_new(store, type, host_callback),
        wasm_func_new_with_env(store, type, host_callback_with_env, &callback_mode, NULL)
    };
    wasm_functype_delete(type);

    wasm_foreign_t* foreign = wasm_foreign_new(store);
    wasm_val_t arg = WASM_INIT_VAL;
    arg.of.ref = wasm_ref_copy(wasm_foreign_as_ref(foreign));
    wasm_foreign_delete(foreign);
    wasm_val_vec_t args = { 1, &arg };
    for (size_t i = 0; i < 2; ++i) {
        for (int repeat = 0; repeat < 10; ++repeat) {
            for (callback_mode = 0; callback_mode < 3; ++callback_mode) {
                wasm_val_t values[] = { WASM_INIT_VAL, WASM_INIT_VAL };
                wasm_val_vec_t outputs = { 2, values };
                wasm_trap_t* trap = wasm_func_call(functions[i], &args, &outputs);
                if (callback_mode < 2) {
                    check_message(trap);
                } else {
                    check(trap == NULL, "callback recovery after trap");
                    for (size_t j = 0; j < 2; ++j) {
                        check(values[j].kind == WASM_ANYREF && wasm_ref_same(arg.of.ref, values[j].of.ref), "callback result identity");
                        wasm_val_delete(&values[j]);
                    }
                }
            }
        }
        wasm_func_delete(functions[i]);
    }
    wasm_val_delete(&arg);
    /* Leaked callback vectors retain owning reference copies. This assertion
     * detects the leak even without LeakSanitizer. */
    check(wasm_test_store_object_count(store) == initial_objects, "callback vectors released all reference roots");
}

int main(void)
{
    wasm_engine_t* engine = wasm_engine_new();
    wasm_store_t* store = wasm_store_new(engine);
    check_callback_cleanup(store);
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
    const char text[] = "host-created trap";
    wasm_message_t message;
    wasm_byte_vec_new(&message, sizeof(text), text);
    wasm_trap_t* host_trap = wasm_trap_new(store, &message);
    wasm_byte_vec_delete(&message);
    wasm_store_delete(store);
    check_message(host_trap);
    wasm_engine_delete(engine);
    return 0;
}
