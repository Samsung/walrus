#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wasm.h"
#include "wat.h"
static void check(int ok, const char* what)
{
    if (!ok) {
        fprintf(stderr, "FAIL: %s\n", what);
        exit(1);
    }
}

struct Case {
    const char* name;
    int issue;
    int variant;
};
static const struct Case cases[] = {
    { "issue_1728_0", 1728, 0 },
    { "issue_1716_0", 1716, 0 },
    { "issue_1716_1", 1716, 1 },
    { "issue_1717_0", 1717, 0 },
    { "issue_1717_1", 1717, 1 },
    { "issue_1720_0", 1720, 0 },
    { "issue_1720_1", 1720, 1 },
    { "issue_1721_0", 1721, 0 },
};
int main(int argc, char** argv)
{
    wasm_engine_t* engine = wasm_engine_new();
    wasm_store_t* store = wasm_store_new(engine);
    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        const struct Case* c = &cases[i];
        if (argc > 1 && strcmp(argv[1], c->name)) {
            continue;
        }
        printf("%s\n", c->name);
        fflush(stdout);
        wasm_byte_vec_t bytes;
        wasm_test_wat(&bytes, c->name);
        check(wasm_module_validate(store, &bytes), "validation");
        wasm_module_t* module = wasm_module_new(store, &bytes);
        check(module != NULL, "module construction");
        wasm_byte_vec_delete(&bytes);
        if (c->issue == 1717 || c->issue == 1721) {
            wasm_extern_vec_t imports = WASM_EMPTY_VEC;
            wasm_instance_t* instance = wasm_instance_new(store, module, &imports, NULL);
            check(instance != NULL, "instantiation");
            wasm_extern_vec_t exports;
            wasm_instance_exports(instance, &exports);
            for (int input = 0; input < 2; ++input) {
                wasm_val_t arg = WASM_I32_VAL(input), result = WASM_INIT_VAL;
                wasm_val_vec_t args = { c->issue == 1721 ? 1 : 0, &arg };
                wasm_val_vec_t outputs = { c->issue == 1721 ? 1 : 0, &result };
                wasm_trap_t* trap = wasm_func_call(wasm_extern_as_func(exports.data[0]), &args, &outputs);
                check((trap != NULL) == (c->variant == 0), "trap behavior");
                if (trap) {
                    wasm_trap_delete(trap);
                }
            }
            wasm_extern_vec_delete(&exports);
            wasm_instance_delete(instance);
        }
        if (c->issue == 1728) {
            wasm_extern_vec_t imports = WASM_EMPTY_VEC;
            wasm_instance_t* instance = wasm_instance_new(store, module, &imports, NULL);
            check(instance != NULL, "v128 loop instantiation");
            wasm_extern_vec_t exports;
            wasm_instance_exports(instance, &exports);
            wasm_val_t values[] = { WASM_F32_VAL(0), WASM_F64_VAL(0) };
            wasm_val_vec_t args = { 2, values };
            wasm_val_t result = WASM_INIT_VAL;
            wasm_val_vec_t results = { 1, &result };
            check(wasm_func_call(wasm_extern_as_func(exports.data[0]), &args, &results) == NULL,
                  "v128 loop call");
            check(result.kind == WASM_F64 && result.of.f64 == 0, "v128 loop result");
            wasm_extern_vec_delete(&exports);
            wasm_instance_delete(instance);
        }
        wasm_module_delete(module);
    }
    wasm_store_delete(store);
    wasm_engine_delete(engine);
    puts("PASS: JIT regressions");
    return 0;
}
