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

/* Readable WAT reproductions from Samsung/escargot live in test/api/wat. */

struct Case {
    const char* name;
    int valid;
    int expected;
};
static const struct Case cases[] = {
    { "issue_1713_0", 0, 0 },
    { "issue_1713_1", 0, 0 },
    { "issue_1713_2", 1, 0 },
    { "issue_1713_3", 1, 0 },
    { "issue_1714_0", 1, 0 },
    { "issue_1714_1", 1, 0 },
    { "issue_1714_2", 1, 0 },
    { "issue_1715_0", 1, 0 },
    { "issue_1715_1", 1, 0 },
    { "issue_1715_2", 1, 5 },
    { "issue_1715_3", 1, 0 },
    { "issue_1718_0", 0, 0 },
    { "issue_1718_1", 1, 0 },
    { "issue_1719_0", 1, 0 },
    { "issue_1719_1", 1, 0 },
    { "issue_1719_2", 1, 0 },
    { "issue_1722_0", 1, 0 },
    { "issue_1722_1", 1, 0 },
    { "issue_1722_2", 1, 7 },
    { "issue_1725_0", 1, 0 },
    { "issue_1725_1", 1, 0 },
    { "issue_1725_2", 1, 0 },
    { "issue_1711_0", 1, 0 },
    { "issue_1712_0", 1, 0 },
    { "issue_1727_0", 1, 0 },
};

int main(void)
{
    wasm_engine_t* engine = wasm_engine_new();
    wasm_store_t* store = wasm_store_new(engine);
    /* Exercise the public tag constructors and owned type copies, not only
     * module reflection. Tags must retain their canonical function type. */
    wasm_tagtype_t* tt = wasm_tagtype_new(wasm_functype_new_0_0());
    wasm_tagtype_t* copied = wasm_tagtype_copy(tt);
    wasm_tag_t* tag = wasm_tag_new(store, copied);
    wasm_tagtype_delete(tt);
    wasm_tagtype_delete(copied);
    check(tag != NULL, "host tag construction");
    wasm_tag_t* tag_copy = wasm_tag_copy(tag);
    check(wasm_tag_same(tag, tag_copy), "host tag identity");
    check(wasm_extern_as_tag_const(wasm_tag_as_extern_const(tag)) == tag,
          "const tag conversion");
    tt = wasm_tag_type(tag);
    check(wasm_functype_params(wasm_tagtype_type(tt))->size == 0, "tag parameters");
    check(wasm_functype_results(wasm_tagtype_type(tt))->size == 0, "tag results");
    wasm_tagtype_t* tag_types[] = { tt };
    wasm_tagtype_vec_t types, types_copy;
    wasm_tagtype_vec_new(&types, 1, tag_types);
    wasm_tagtype_vec_copy(&types_copy, &types);
    wasm_tagtype_vec_delete(&types);
    check(wasm_functype_params(wasm_tagtype_type(types_copy.data[0]))->size == 0,
          "owned tag type vector copy");
    wasm_tagtype_vec_delete(&types_copy);
    wasm_tag_delete(tag);
    wasm_tag_delete(tag_copy);
    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        const struct Case* c = &cases[i];
        printf("%s\n", c->name);
        fflush(stdout);
        wasm_byte_vec_t bytes;
        wasm_test_wat(&bytes, c->name);
        check(wasm_module_validate(store, &bytes) == c->valid, "validation");
        wasm_module_t* module = wasm_module_new(store, &bytes);
        check((module != NULL) == c->valid, "module construction");
        wasm_byte_vec_delete(&bytes);
        if (c->expected) {
            wasm_extern_vec_t imports = WASM_EMPTY_VEC;
            wasm_instance_t* instance = wasm_instance_new(store, module, &imports, NULL);
            check(instance != NULL, "instantiation");
            wasm_extern_vec_t exports;
            wasm_instance_exports(instance, &exports);
            wasm_val_t arg = WASM_I64_VAL(0);
            wasm_val_vec_t args = { c->expected == 7 ? 1 : 0, &arg };
            wasm_val_t results[3] = { WASM_INIT_VAL, WASM_INIT_VAL, WASM_INIT_VAL };
            size_t count = c->expected == 7 ? 2 : c->expected == 1 ? 3 : 1;
            wasm_val_vec_t outputs = { count, results };
            check(wasm_func_call(wasm_extern_as_func(exports.data[0]), &args, &outputs) == NULL, "call");
            for (size_t j = 0; j < count; ++j) {
                int expected = c->expected == 7 ? (j == 0 ? 7 : 9) : c->expected + j;
                check(results[j].kind == WASM_I32 && results[j].of.i32 == expected, "branch/catch result");
            }
            wasm_extern_vec_delete(&exports);
            wasm_instance_delete(instance);
        } else if (!strcmp(c->name, "issue_1711_0") || !strcmp(c->name, "issue_1712_0")) {
            wasm_extern_vec_t imports = WASM_EMPTY_VEC;
            wasm_instance_t* instance = wasm_instance_new(store, module, &imports, NULL);
            check(instance != NULL, "instantiation");
            wasm_extern_vec_t exports;
            wasm_instance_exports(instance, &exports);
            wasm_extern_vec_delete(&exports);
            wasm_instance_delete(instance);
        }
        if (module) {
            wasm_importtype_vec_t imports;
            wasm_module_imports(module, &imports);
            if (!strcmp(c->name, "issue_1727_0")) {
                check(imports.size == 1, "issue 1727 import count");
                check(wasm_externtype_kind(wasm_importtype_type(imports.data[0])) == WASM_EXTERN_TAG, "issue 1727 import kind");
                const wasm_tagtype_t* imported_type = wasm_externtype_as_tagtype_const(wasm_importtype_type(imports.data[0]));
                wasm_tag_t* imported_tag = wasm_tag_new(store, imported_type);
                wasm_extern_t* values[] = { wasm_tag_as_extern(imported_tag) };
                wasm_extern_vec_t import_values = { 1, values };
                wasm_instance_t* instance = wasm_instance_new(store, module, &import_values, NULL);
                check(instance != NULL, "issue 1727 tag import instantiation");
                wasm_tag_delete(imported_tag);
                wasm_instance_delete(instance);
            }
            wasm_importtype_vec_delete(&imports);
            wasm_module_delete(module);
        }
    }
    wasm_store_delete(store);
    wasm_engine_delete(engine);
    puts("PASS: parser regressions");
    return 0;
}
