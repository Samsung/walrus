/*
 * Copyright (c) 2022-present Samsung Electronics Co., Ltd
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __WalrusAPI__
#define __WalrusAPI__

#include "Walrus.h"
#include "wasm.h"

#include "runtime/Engine.h"
#include "runtime/Store.h"
#include "runtime/Module.h"
#include "runtime/Function.h"
#include "runtime/Table.h"
#include "runtime/Memory.h"
#include "runtime/Global.h"
#include "runtime/Instance.h"
#include "runtime/Trap.h"
#include "parser/WASMParser.h"

using namespace Walrus;

#define own

// Common Internal Methods
static wasm_valkind_t FromWalrusValueType(const Value::Type& type)
{
    switch (type) {
    case Value::Type::I32:
        return WASM_I32;
    case Value::Type::I64:
        return WASM_I64;
    case Value::Type::F32:
        return WASM_F32;
    case Value::Type::F64:
        return WASM_F64;
    case Value::Type::FuncRef:
        return WASM_FUNCREF;
    case Value::Type::ExternRef:
        return WASM_ANYREF;
    default:
        RELEASE_ASSERT_NOT_REACHED();
        return WASM_ANYREF;
    }
}

static Value::Type ToWalrusValueType(const wasm_valkind_t& kind)
{
    switch (kind) {
    case WASM_I32:
        return Value::Type::I32;
    case WASM_I64:
        return Value::Type::I64;
    case WASM_F32:
        return Value::Type::F32;
    case WASM_F64:
        return Value::Type::F64;
    case WASM_FUNCREF:
        return Value::Type::FuncRef;
    case WASM_ANYREF:
        return Value::Type::ExternRef;
    default:
        RELEASE_ASSERT_NOT_REACHED();
        return Value::Type::Void;
    }
}

static wasm_val_t FromWalrusValue(const Value& val)
{
    wasm_val_t result;
    switch (val.type()) {
    case Value::Type::I32:
        result.kind = WASM_I32;
        result.of.i32 = val.asI32();
        break;
    case Value::Type::I64:
        result.kind = WASM_I64;
        result.of.i64 = val.asI64();
        break;
    case Value::Type::F32:
        result.kind = WASM_F32;
        result.of.f32 = val.asF32();
        break;
    case Value::Type::F64:
        result.kind = WASM_F64;
        result.of.f64 = val.asF64();
        break;
    case Value::Type::FuncRef:
        // TODO
        RELEASE_ASSERT_NOT_REACHED();
        result.kind = WASM_FUNCREF;
        break;
    case Value::Type::ExternRef:
        // TODO
        RELEASE_ASSERT_NOT_REACHED();
        result.kind = WASM_ANYREF;
        break;
    default:
        RELEASE_ASSERT_NOT_REACHED();
        break;
    }
    return result;
}

static Value ToWalrusValue(const wasm_val_t& val)
{
    switch (val.kind) {
    case WASM_I32:
        return Value(val.of.i32);
    case WASM_I64:
        return Value(val.of.i64);
    case WASM_F32:
        return Value(val.of.f32);
    case WASM_F64:
        return Value(val.of.f64);
    case WASM_ANYREF:
        // TODO
        RELEASE_ASSERT_NOT_REACHED();
        return Value();
    case WASM_FUNCREF:
        // TODO
        RELEASE_ASSERT_NOT_REACHED();
        return Value();
    default:
        RELEASE_ASSERT_NOT_REACHED();
        return Value();
    }
}

static void FromWalrusValues(wasm_val_t* results, const Value* values, const uint32_t num)
{
    for (uint32_t i = 0; i < num; i++) {
        results[i] = FromWalrusValue(values[i]);
    }
}

static void ToWalrusValues(Value* results, const wasm_val_t* values, const uint32_t num)
{
    for (uint32_t i = 0; i < num; i++) {
        results[i] = ToWalrusValue(values[i]);
    }
}

// Structures
struct wasm_importtype_t {
};

struct wasm_exporttype_t {
};

struct wasm_config_t {
};

struct wasm_engine_t {
    wasm_engine_t(Engine* e)
        : engine(e)
    {
    }

    ~wasm_engine_t()
    {
        GC_FREE(engine);
        engine = nullptr;
    }

    Engine* get() const
    {
        ASSERT(engine);
        return engine;
    }

    Engine* engine;
};

struct wasm_store_t {
    wasm_store_t(Store* s)
        : store(s)
    {
    }

    ~wasm_store_t()
    {
        GC_FREE(store);
        store = nullptr;
    }

    Store* get() const
    {
        ASSERT(store);
        return store;
    }

    Store* store;
};

struct wasm_valtype_t {
    Value::Type type;
};

struct wasm_externtype_t {
};

struct wasm_functype_t : wasm_externtype_t {
    wasm_functype_t(own wasm_valtype_vec_t* params, own wasm_valtype_vec_t* results)
        : params(*params)
        , results(*results)
    {
    }

    ~wasm_functype_t()
    {
        wasm_valtype_vec_delete(&params);
        wasm_valtype_vec_delete(&results);
    }

    wasm_valtype_vec_t params;
    wasm_valtype_vec_t results;
};

struct wasm_ref_t {
    wasm_ref_t(Object* o)
        : obj(o)
    {
    }

    Object* get() const
    {
        ASSERT(obj);
        return obj;
    }

    Object* obj;
};

struct wasm_extern_t : wasm_ref_t {
    wasm_extern_t(Object* ptr)
        : wasm_ref_t(ptr)
    {
    }
};

struct wasm_module_t : wasm_ref_t {
    wasm_module_t(Module* module)
        : wasm_ref_t(module)
    {
    }

    Module* get() const
    {
        ASSERT(obj);
        return reinterpret_cast<Module*>(obj);
    }
};

struct wasm_func_t : wasm_extern_t {
    wasm_func_t(Function* func)
        : wasm_extern_t(func)
    {
    }

    Function* get() const
    {
        ASSERT(obj);
        return reinterpret_cast<Function*>(obj);
    }
};

struct wasm_instance_t : wasm_ref_t {
    wasm_instance_t(Instance* ins)
        : wasm_ref_t(ins)
    {
    }

    Instance* get() const
    {
        ASSERT(obj);
        return reinterpret_cast<Instance*>(obj);
    }
};

extern "C" {

// Import Types
own wasm_importtype_t* wasm_importtype_new(
    own wasm_name_t* module, own wasm_name_t* name, own wasm_externtype_t*);

const wasm_name_t* wasm_importtype_module(const wasm_importtype_t*);
const wasm_name_t* wasm_importtype_name(const wasm_importtype_t*);
const wasm_externtype_t* wasm_importtype_type(const wasm_importtype_t*);

// Export Types
own wasm_exporttype_t* wasm_exporttype_new(own wasm_name_t*, own wasm_externtype_t*);

const wasm_name_t* wasm_exporttype_name(const wasm_exporttype_t*);
const wasm_externtype_t* wasm_exporttype_type(const wasm_exporttype_t*);

///////////////////////////////////////////////////////////////////////////////
// Runtime Environment

// Configuration
own wasm_config_t* wasm_config_new()
{
    // TODO
    RELEASE_ASSERT_NOT_REACHED();
    return nullptr;
}

// Engine
own wasm_engine_t* wasm_engine_new()
{
    return new wasm_engine_t(new Engine());
}

own wasm_engine_t* wasm_engine_new_with_config(own wasm_config_t*)
{
    // TODO
    RELEASE_ASSERT_NOT_REACHED();
    return nullptr;
}

// Store
own wasm_store_t* wasm_store_new(wasm_engine_t* engine)
{
    ASSERT(engine);
    return new wasm_store_t(new Store(engine->get()));
}

///////////////////////////////////////////////////////////////////////////////
// Type Representations

// Value Types
own wasm_valtype_t* wasm_valtype_new(wasm_valkind_t kind)
{
    return new wasm_valtype_t{ ToWalrusValueType(kind) };
}

wasm_valkind_t wasm_valtype_kind(const wasm_valtype_t* type)
{
    return FromWalrusValueType(type->type);
}

// Function Types
own wasm_functype_t* wasm_functype_new(
    own wasm_valtype_vec_t* params, own wasm_valtype_vec_t* results)
{
    return new wasm_functype_t(params, results);
}

const wasm_valtype_vec_t* wasm_functype_params(const wasm_functype_t* f)
{
    return &f->params;
}

const wasm_valtype_vec_t* wasm_functype_results(const wasm_functype_t* f)
{
    return &f->results;
}

// Extern Types
// TODO
/*
wasm_externkind_t wasm_externtype_kind(const wasm_externtype_t*);

wasm_externtype_t* wasm_functype_as_externtype(wasm_functype_t*);
wasm_externtype_t* wasm_globaltype_as_externtype(wasm_globaltype_t*);
wasm_externtype_t* wasm_tabletype_as_externtype(wasm_tabletype_t*);
wasm_externtype_t* wasm_memorytype_as_externtype(wasm_memorytype_t*);

wasm_functype_t* wasm_externtype_as_functype(wasm_externtype_t*);
wasm_globaltype_t* wasm_externtype_as_globaltype(wasm_externtype_t*);
wasm_tabletype_t* wasm_externtype_as_tabletype(wasm_externtype_t*);
wasm_memorytype_t* wasm_externtype_as_memorytype(wasm_externtype_t*);

const wasm_externtype_t* wasm_functype_as_externtype_const(const wasm_functype_t*);
const wasm_externtype_t* wasm_globaltype_as_externtype_const(const wasm_globaltype_t*);
const wasm_externtype_t* wasm_tabletype_as_externtype_const(const wasm_tabletype_t*);
const wasm_externtype_t* wasm_memorytype_as_externtype_const(const wasm_memorytype_t*);

const wasm_functype_t* wasm_externtype_as_functype_const(const wasm_externtype_t*);
const wasm_globaltype_t* wasm_externtype_as_globaltype_const(const wasm_externtype_t*);
const wasm_tabletype_t* wasm_externtype_as_tabletype_const(const wasm_externtype_t*);
const wasm_memorytype_t* wasm_externtype_as_memorytype_const(const wasm_externtype_t*);
*/

///////////////////////////////////////////////////////////////////////////////
// Runtime Objects

// References
// Modules
own wasm_module_t* wasm_module_new(wasm_store_t* store, const wasm_byte_vec_t* binary)
{
    auto parseResult = WASMParser::parseBinary(store->get(), std::string(), reinterpret_cast<uint8_t*>(binary->data), binary->size);
    return new wasm_module_t(parseResult.first.unwrap());
}

bool wasm_module_validate(wasm_store_t* store, const wasm_byte_vec_t* binary)
{
    auto parseResult = WASMParser::parseBinary(store->get(), std::string(), reinterpret_cast<uint8_t*>(binary->data), binary->size);
    if (!parseResult.first.hasValue()) {
        return false;
    }
    return true;
}

void wasm_module_imports(const wasm_module_t*, own wasm_importtype_vec_t* out)
{
    // TODO
    RELEASE_ASSERT_NOT_REACHED();
}

void wasm_module_exports(const wasm_module_t*, own wasm_exporttype_vec_t* out)
{
    // TODO
    RELEASE_ASSERT_NOT_REACHED();
}

void wasm_module_serialize(const wasm_module_t*, own wasm_byte_vec_t* out)
{
    // TODO
    RELEASE_ASSERT_NOT_REACHED();
}

own wasm_module_t* wasm_module_deserialize(wasm_store_t*, const wasm_byte_vec_t*)
{
    // TODO
    RELEASE_ASSERT_NOT_REACHED();
    return nullptr;
}

// Function Instances
static FunctionType* ToWalrusFunctionType(const wasm_functype_t* ft)
{
    ValueTypeVector* params = new ValueTypeVector();
    ValueTypeVector* results = new ValueTypeVector();
    params->reserve(ft->params.size);
    results->reserve(ft->results.size);

    for (uint32_t i = 0; i < ft->params.size; i++) {
        params->push_back(ft->params.data[i]->type);
    }
    for (uint32_t i = 0; i < ft->results.size; i++) {
        results->push_back(ft->results.data[i]->type);
    }

    return new FunctionType(params, results);
}

own wasm_func_t* wasm_func_new(
    wasm_store_t* store, const wasm_functype_t* ft, wasm_func_callback_t callback)
{
    struct FuncData {
        wasm_func_callback_t callback;
        size_t resultSize;
    } data = { callback, ft->params.size };
    Function* func = new ImportedFunction(store->get(), ToWalrusFunctionType(ft),
                                          [](ExecutionState& state, const uint32_t argc, Value* argv, Value* result, void* d) {
                                              FuncData* data = reinterpret_cast<FuncData*>(d);
                                              wasm_val_vec_t params, results;
                                              wasm_val_vec_new_uninitialized(&params, argc);
                                              wasm_val_vec_new_uninitialized(&results, data->resultSize);
                                              FromWalrusValues(params.data, argv, argc);

                                              auto trap = data->callback(&params, &results);
                                              wasm_val_vec_delete(&params);

                                              if (trap) {
                                                  // TODO
                                                  wasm_trap_delete(trap);
                                                  RELEASE_ASSERT_NOT_REACHED();
                                                  // Can't use wasm_val_vec_delete since it wasn't populated.
                                                  delete[] results.data;
                                              }

                                              ToWalrusValues(result, results.data, results.size);
                                              wasm_val_vec_delete(&results);
                                          },
                                          &data);

    return new wasm_func_t(func);
}

own wasm_func_t* wasm_func_new_with_env(
    wasm_store_t*, const wasm_functype_t* type, wasm_func_callback_with_env_t,
    void* env, void (*finalizer)(void*))
{
    // TODO
    RELEASE_ASSERT_NOT_REACHED();
    return nullptr;
}

static own wasm_functype_t* FromWalrusFunctionType(const FunctionType* ft)
{
    const ValueTypeVector& srcParams = ft->param();
    const ValueTypeVector& srcResults = ft->result();

    wasm_valtype_vec_t params;
    wasm_valtype_vec_t results;
    wasm_valtype_vec_new_uninitialized(&params, srcParams.size());
    wasm_valtype_vec_new_uninitialized(&results, srcResults.size());

    for (uint32_t i = 0; i < srcParams.size(); i++) {
        params.data[i] = wasm_valtype_new(FromWalrusValueType(srcParams[i]));
    }
    for (uint32_t i = 0; i < srcResults.size(); i++) {
        results.data[i] = wasm_valtype_new(FromWalrusValueType(srcResults[i]));
    }

    return new wasm_functype_t(&params, &results);
}

own wasm_functype_t* wasm_func_type(const wasm_func_t* func)
{
    return FromWalrusFunctionType(func->get()->functionType());
}

size_t wasm_func_param_arity(const wasm_func_t* func)
{
    return func->get()->functionType()->param().size();
}

size_t wasm_func_result_arity(const wasm_func_t* func)
{
    return func->get()->functionType()->result().size();
}

own wasm_trap_t* wasm_func_call(
    const wasm_func_t* func, const wasm_val_vec_t* args, wasm_val_vec_t* results)
{
    size_t paramNum = wasm_func_param_arity(func);
    size_t resultNum = wasm_func_result_arity(func);

    ValueVector walrusArgs, walrusResults;
    walrusArgs.reserve(paramNum);
    walrusResults.resize(resultNum);

    ToWalrusValues(walrusArgs.data(), args->data, paramNum);

    struct RunData {
        Function* fn;
        Walrus::ValueVector& args;
        Walrus::ValueVector& results;
    } data = { func->get(), walrusArgs, walrusResults };
    Trap trap;
    auto trapResult = trap.run([](ExecutionState& state, void* d) {
        RunData* data = reinterpret_cast<RunData*>(d);
        data->fn->call(state, data->args.size(), data->args.data(), data->results.data());
    },
                               &data);

    if (trapResult.exception) {
        // TODO
        RELEASE_ASSERT_NOT_REACHED();
    }

    FromWalrusValues(results->data, walrusResults.data(), resultNum);
    return nullptr;
}

// Externals

// Module Instances
own wasm_instance_t* wasm_instance_new(
    wasm_store_t* store, const wasm_module_t* module, const wasm_extern_vec_t* imports,
    own wasm_trap_t** outTrap)
{
    struct RunData {
        Module* module;
        ObjectVector importValues;
        Instance* instance;
    } data = { module->get(), ObjectVector(), nullptr };

    data.importValues.reserve(imports->size);
    for (size_t i = 0; i < imports->size; i++) {
        data.importValues.push_back(imports->data[i]->get());
    }

    Walrus::Trap trap;
    auto trapResult = trap.run([](ExecutionState& state, void* d) {
        RunData* data = reinterpret_cast<RunData*>(d);
        data->instance = data->module->instantiate(state, data->importValues);
    },
                               &data);

    if (trapResult.exception) {
        // TODO
        return nullptr;
    }

    ASSERT(data.instance);
    return new wasm_instance_t(data.instance);
}

void wasm_instance_exports(const wasm_instance_t*, own wasm_extern_vec_t* out);

// Vector Types
#define WASM_IMPL_OWN(name)                           \
    void wasm_##name##_delete(own wasm_##name##_t* t) \
    {                                                 \
        ASSERT(t);                                    \
        delete t;                                     \
    }

//WASM_IMPL_OWN(frame);
//WASM_IMPL_OWN(config);
WASM_IMPL_OWN(engine);
WASM_IMPL_OWN(store);

#define WASM_IMPL_VEC_BASE(name, ptr_or_none)                               \
    void wasm_##name##_vec_new_empty(own wasm_##name##_vec_t* out)          \
    {                                                                       \
        wasm_##name##_vec_new_uninitialized(out, 0);                        \
    }                                                                       \
    void wasm_##name##_vec_new_uninitialized(own wasm_##name##_vec_t* vec,  \
                                             size_t size)                   \
    {                                                                       \
        vec->size = size;                                                   \
        vec->data = size ? new wasm_##name##_t ptr_or_none[size] : nullptr; \
    }

#define WASM_IMPL_VEC_PLAIN(name)                                          \
    WASM_IMPL_VEC_BASE(name, )                                             \
    void wasm_##name##_vec_new(own wasm_##name##_vec_t* vec, size_t size,  \
                               own wasm_##name##_t const src[])            \
    {                                                                      \
        wasm_##name##_vec_new_uninitialized(vec, size);                    \
        memcpy(vec->data, src, size * sizeof(wasm_##name##_t));            \
    }                                                                      \
    void wasm_##name##_vec_copy(own wasm_##name##_vec_t* out,              \
                                const wasm_##name##_vec_t* vec)            \
    {                                                                      \
        wasm_##name##_vec_new_uninitialized(out, vec->size);               \
        memcpy(out->data, vec->data, vec->size * sizeof(wasm_##name##_t)); \
    }                                                                      \
    void wasm_##name##_vec_delete(own wasm_##name##_vec_t* vec)            \
    {                                                                      \
        delete[] vec->data;                                                \
        vec->size = 0;                                                     \
    }
WASM_IMPL_VEC_PLAIN(byte);

// Special implementation for wasm_val_t, since it's weird.
WASM_IMPL_VEC_BASE(val, )
void wasm_val_vec_new(own wasm_val_vec_t* vec,
                      size_t size,
                      own wasm_val_t const src[])
{
    wasm_val_vec_new_uninitialized(vec, size);
    for (size_t i = 0; i < size; ++i) {
        vec->data[i] = src[i];
    }
}

void wasm_val_vec_copy(own wasm_val_vec_t* out, const wasm_val_vec_t* vec)
{
    wasm_val_vec_new_uninitialized(out, vec->size);
    for (size_t i = 0; i < vec->size; ++i) {
        wasm_val_copy(&out->data[i], &vec->data[i]);
    }
}

void wasm_val_vec_delete(own wasm_val_vec_t* vec)
{
    for (size_t i = 0; i < vec->size; ++i) {
        wasm_val_delete(&vec->data[i]);
    }
    delete[] vec->data;
    vec->size = 0;
}

#define WASM_IMPL_VEC_OWN(name)                                           \
    WASM_IMPL_VEC_BASE(name, *)                                           \
    void wasm_##name##_vec_new(own wasm_##name##_vec_t* vec, size_t size, \
                               own wasm_##name##_t* const src[])          \
    {                                                                     \
        wasm_##name##_vec_new_uninitialized(vec, size);                   \
        for (size_t i = 0; i < size; ++i) {                               \
            vec->data[i] = src[i];                                        \
        }                                                                 \
    }                                                                     \
    void wasm_##name##_vec_copy(own wasm_##name##_vec_t* out,             \
                                const wasm_##name##_vec_t* vec)           \
    {                                                                     \
        wasm_##name##_vec_new_uninitialized(out, vec->size);              \
        for (size_t i = 0; i < vec->size; ++i) {                          \
            out->data[i] = wasm_##name##_copy(vec->data[i]);              \
        }                                                                 \
    }                                                                     \
    void wasm_##name##_vec_delete(wasm_##name##_vec_t* vec)               \
    {                                                                     \
        for (size_t i = 0; i < vec->size; ++i) {                          \
            delete vec->data[i];                                          \
        }                                                                 \
        delete[] vec->data;                                               \
        vec->size = 0;                                                    \
    }

//WASM_IMPL_VEC_OWN(frame);
//WASM_IMPL_VEC_OWN(extern);

#define WASM_IMPL_TYPE(name)                                              \
    WASM_IMPL_OWN(name)                                                   \
    WASM_IMPL_VEC_OWN(name)                                               \
    own wasm_##name##_t* wasm_##name##_copy(const wasm_##name##_t* other) \
    {                                                                     \
        return new wasm_##name##_t(*other);                               \
    }

WASM_IMPL_TYPE(valtype);
//WASM_IMPL_TYPE(importtype);
//WASM_IMPL_TYPE(exporttype);

#define WASM_IMPL_TYPE_CLONE(name)                                      \
    WASM_IMPL_OWN(name)                                                 \
    WASM_IMPL_VEC_OWN(name)                                             \
    own wasm_##name##_t* wasm_##name##_copy(wasm_##name##_t* other)     \
    {                                                                   \
        return static_cast<wasm_##name##_t*>(other->Clone().release()); \
    }

//WASM_IMPL_TYPE_CLONE(functype);
//WASM_IMPL_TYPE_CLONE(globaltype);
//WASM_IMPL_TYPE_CLONE(tabletype);
//WASM_IMPL_TYPE_CLONE(memorytype);
//WASM_IMPL_TYPE_CLONE(externtype);

#define WASM_IMPL_REF_BASE(name)                                        \
    WASM_IMPL_OWN(name)                                                 \
    own wasm_##name##_t* wasm_##name##_copy(const wasm_##name##_t* ref) \
    {                                                                   \
        return new wasm_##name##_t(*ref);                               \
    }                                                                   \
    bool wasm_##name##_same(const wasm_##name##_t* ref,                 \
                            const wasm_##name##_t* other)               \
    {                                                                   \
        return ref->get() == other->get();                              \
    }

#define WASM_IMPL_REF(name)                                                  \
    WASM_IMPL_REF_BASE(name)                                                 \
    wasm_ref_t* wasm_##name##_as_ref(wasm_##name##_t* subclass)              \
    {                                                                        \
        return subclass;                                                     \
    }                                                                        \
    wasm_##name##_t* wasm_ref_as_##name(wasm_ref_t* ref)                     \
    {                                                                        \
        return static_cast<wasm_##name##_t*>(ref);                           \
    }                                                                        \
    const wasm_ref_t* wasm_##name##_as_ref_const(                            \
        const wasm_##name##_t* subclass)                                     \
    {                                                                        \
        return subclass;                                                     \
    }                                                                        \
    const wasm_##name##_t* wasm_ref_as_##name##_const(const wasm_ref_t* ref) \
    {                                                                        \
        return static_cast<const wasm_##name##_t*>(ref);                     \
    }

WASM_IMPL_REF_BASE(ref);

WASM_IMPL_REF(extern);
//WASM_IMPL_REF(foreign);
WASM_IMPL_REF(func);
//WASM_IMPL_REF(global);
WASM_IMPL_REF(instance);
//WASM_IMPL_REF(memory);
//WASM_IMPL_REF(table);
//WASM_IMPL_REF(trap);
WASM_IMPL_REF(module); // FIXME

#define WASM_IMPL_SHARABLE_REF(name)                                           \
    WASM_IMPL_REF(name)                                                        \
    WASM_IMPL_OWN(shared_##name)                                               \
    own wasm_shared_##name##_t* wasm_##name##_share(const wasm_##name##_t* t)  \
    {                                                                          \
        return static_cast<wasm_shared_##name##_t*>(                           \
            const_cast<wasm_##name##_t*>(t));                                  \
    }                                                                          \
    own wasm_##name##_t* wasm_##name##_obtain(wasm_store_t*,                   \
                                              const wasm_shared_##name##_t* t) \
    {                                                                          \
        return static_cast<wasm_##name##_t*>(                                  \
            const_cast<wasm_shared_##name##_t*>(t));                           \
    }

//WASM_IMPL_SHARABLE_REF(module)

#define WASM_IMPL_EXTERN(name)                                                 \
    const wasm_##name##type_t* wasm_externtype_as_##name##type_const(          \
        const wasm_externtype_t* t)                                            \
    {                                                                          \
        return static_cast<const wasm_##name##type_t*>(t);                     \
    }                                                                          \
    wasm_##name##type_t* wasm_externtype_as_##name##type(wasm_externtype_t* t) \
    {                                                                          \
        return static_cast<wasm_##name##type_t*>(t);                           \
    }                                                                          \
    wasm_externtype_t* wasm_##name##type_as_externtype(wasm_##name##type_t* t) \
    {                                                                          \
        return static_cast<wasm_externtype_t*>(t);                             \
    }                                                                          \
    const wasm_externtype_t* wasm_##name##type_as_externtype_const(            \
        const wasm_##name##type_t* t)                                          \
    {                                                                          \
        return static_cast<const wasm_externtype_t*>(t);                       \
    }                                                                          \
    wasm_extern_t* wasm_##name##_as_extern(wasm_##name##_t* name)              \
    {                                                                          \
        return static_cast<wasm_extern_t*>(name);                              \
    }                                                                          \
    const wasm_extern_t* wasm_##name##_as_extern_const(                        \
        const wasm_##name##_t* name)                                           \
    {                                                                          \
        return static_cast<const wasm_extern_t*>(name);                        \
    }                                                                          \
    wasm_##name##_t* wasm_extern_as_##name(wasm_extern_t* ext)                 \
    {                                                                          \
        return static_cast<wasm_##name##_t*>(ext);                             \
    }

//WASM_IMPL_EXTERN(table);
WASM_IMPL_EXTERN(func);
//WASM_IMPL_EXTERN(global);
//WASM_IMPL_EXTERN(memory);

} // extern "C"

#endif // __WalrusAPI__
