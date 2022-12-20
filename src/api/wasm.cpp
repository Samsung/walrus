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

// Cast References
#define WASM_REF_LIST(F) \
    F(Engine, engine)    \
    F(Store, store)      \
    F(Module, module)    \
    F(Function, func)

#define WASM_REF_CAST(Name, name)                                   \
    static inline Name* toImpl(wasm_##name##_t* v)                  \
    {                                                               \
        return reinterpret_cast<Name*>(v);                          \
    }                                                               \
    static inline Name* toImpl(const wasm_##name##_t* v)            \
    {                                                               \
        return const_cast<Name*>(reinterpret_cast<const Name*>(v)); \
    }                                                               \
    static inline wasm_##name##_t* toRef(Name* v)                   \
    {                                                               \
        return reinterpret_cast<wasm_##name##_t*>(v);               \
    }

WASM_REF_LIST(WASM_REF_CAST)


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

extern "C" {

// Import Types
struct wasm_importtype_t {
};

own wasm_importtype_t* wasm_importtype_new(
    own wasm_name_t* module, own wasm_name_t* name, own wasm_externtype_t*);

const wasm_name_t* wasm_importtype_module(const wasm_importtype_t*);
const wasm_name_t* wasm_importtype_name(const wasm_importtype_t*);
const wasm_externtype_t* wasm_importtype_type(const wasm_importtype_t*);

// Export Types
struct wasm_exporttype_t {
};

own wasm_exporttype_t* wasm_exporttype_new(own wasm_name_t*, own wasm_externtype_t*);

const wasm_name_t* wasm_exporttype_name(const wasm_exporttype_t*);
const wasm_externtype_t* wasm_exporttype_type(const wasm_exporttype_t*);

///////////////////////////////////////////////////////////////////////////////
// Runtime Environment

// Configuration
struct wasm_config_t {
};

own wasm_config_t* wasm_config_new()
{
    // TODO
    RELEASE_ASSERT_NOT_REACHED();
    return nullptr;
}

// Engine
struct wasm_engine_t {
};

own wasm_engine_t* wasm_engine_new()
{
    return toRef(new Engine());
}

own wasm_engine_t* wasm_engine_new_with_config(own wasm_config_t*)
{
    // TODO
    RELEASE_ASSERT_NOT_REACHED();
    return nullptr;
}

// Store
struct wasm_store_t {
};

own wasm_store_t* wasm_store_new(wasm_engine_t* engine)
{
    return toRef(new Store(toImpl(engine)));
}

///////////////////////////////////////////////////////////////////////////////
// Type Representations

// Value Types
struct wasm_valtype_t {
    Value::Type type;
};

own wasm_valtype_t* wasm_valtype_new(wasm_valkind_t kind)
{
    return new wasm_valtype_t{ ToWalrusValueType(kind) };
}

wasm_valkind_t wasm_valtype_kind(const wasm_valtype_t* type)
{
    return FromWalrusValueType(type->type);
}

// Function Types
struct wasm_functype_t {
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

///////////////////////////////////////////////////////////////////////////////
// Runtime Objects

// Modules
own wasm_module_t* wasm_module_new(wasm_store_t* store, const wasm_byte_vec_t* binary)
{
    auto parseResult = WASMParser::parseBinary(toImpl(store), std::string(), reinterpret_cast<uint8_t*>(binary->data), binary->size);
    return toRef(parseResult.first.unwrap());
}

bool wasm_module_validate(wasm_store_t* store, const wasm_byte_vec_t* binary)
{
    auto parseResult = WASMParser::parseBinary(toImpl(store), std::string(), reinterpret_cast<uint8_t*>(binary->data), binary->size);
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
struct wasm_func_t {
};

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
    Function* func = new ImportedFunction(toImpl(store), ToWalrusFunctionType(ft),
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

    return toRef(func);
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
    return FromWalrusFunctionType(toImpl(func)->functionType());
}

size_t wasm_func_param_arity(const wasm_func_t* func)
{
    return toImpl(func)->functionType()->param().size();
}

size_t wasm_func_result_arity(const wasm_func_t* func)
{
    return toImpl(func)->functionType()->result().size();
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
    } data = { toImpl(func), walrusArgs, walrusResults };
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

// Vector Types
#define WASM_IMPL_OWN(name)                           \
    void wasm_##name##_delete(own wasm_##name##_t* t) \
    {                                                 \
        ASSERT(t);                                    \
        delete t;                                     \
    }

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


} // extern "C"

#endif // __WalrusAPI__
