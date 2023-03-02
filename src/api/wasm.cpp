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

// Structures
struct wasm_importtype_t {
    wasm_importtype_t(std::string& m, std::string& n, own wasm_externtype_t* et)
        : externType(et)
    {
        wasm_byte_vec_new(&module, m.length(), m.data());
        wasm_byte_vec_new(&name, n.length(), n.data());
    }

    wasm_importtype_t(own wasm_name_t* m, own wasm_name_t* n, own wasm_externtype_t* et)
        : module(*m)
        , name(*n)
        , externType(et)
    {
    }

    virtual ~wasm_importtype_t()
    {
        wasm_name_delete(&module);
        wasm_name_delete(&name);
        wasm_externtype_delete(externType);
    }

    own wasm_name_t module;
    own wasm_name_t name;
    own wasm_externtype_t* externType;
};

struct wasm_exporttype_t {
    wasm_exporttype_t(std::string& n, own wasm_externtype_t* t)
        : type(t)
    {
        wasm_byte_vec_new(&name, n.length(), n.data());
    }

    wasm_exporttype_t(own wasm_name_t* n, own wasm_externtype_t* t)
        : name(*n)
        , type(t)
    {
    }

    virtual ~wasm_exporttype_t()
    {
        wasm_name_delete(&name);
        wasm_externtype_delete(type);
    }

    own wasm_name_t name;
    own wasm_externtype_t* type;
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
        delete engine;
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
        delete store;
    }

    Store* get() const
    {
        ASSERT(store);
        return store;
    }

    Store* store;
};

struct wasm_valtype_t {
    wasm_valtype_t(Value::Type t)
        : type(t)
    {
    }

    Value::Type type;
};

struct wasm_externtype_t {
    wasm_externtype_t(own const ObjectType* t)
        : type(t)
    {
    }

    virtual ~wasm_externtype_t()
    {
        ASSERT(type);
        delete type;
    }

    virtual wasm_externtype_t* clone() const = 0;

    const ObjectType* get() const
    {
        ASSERT(type);
        return type;
    }

    own const ObjectType* type;
};

struct wasm_functype_t : wasm_externtype_t {
    wasm_functype_t(own const FunctionType* type, own wasm_valtype_vec_t* params, own wasm_valtype_vec_t* results)
        : wasm_externtype_t(type)
        , params(*params)
        , results(*results)
    {
    }

    wasm_functype_t(own const FunctionType* type);

    virtual ~wasm_functype_t()
    {
        wasm_valtype_vec_delete(&params);
        wasm_valtype_vec_delete(&results);
    }

    virtual wasm_externtype_t* clone() const override
    {
        FunctionType* type = static_cast<const FunctionType*>(get())->clone();
        return new wasm_functype_t(type);
    }

    wasm_valtype_vec_t params;
    wasm_valtype_vec_t results;
};

struct wasm_globaltype_t : wasm_externtype_t {
    wasm_globaltype_t(own const GlobalType* type)
        : wasm_externtype_t(type)
        , valtype(type->type())
    {
    }

    virtual wasm_externtype_t* clone() const override
    {
        GlobalType* type = static_cast<const GlobalType*>(get())->clone();
        return new wasm_globaltype_t(type);
    }

    wasm_valtype_t valtype;
};

struct wasm_tabletype_t : wasm_externtype_t {
    wasm_tabletype_t(own const TableType* type)
        : wasm_externtype_t(type)
        , valtype(type->type())
        , limits{ type->initialSize(), type->maximumSize() }
    {
    }

    virtual wasm_externtype_t* clone() const override
    {
        TableType* type = static_cast<const TableType*>(get())->clone();
        return new wasm_tabletype_t(type);
    }

    wasm_valtype_t valtype;
    wasm_limits_t limits;
};

struct wasm_memorytype_t : wasm_externtype_t {
    wasm_memorytype_t(own const MemoryType* type)
        : wasm_externtype_t(type)
        , limits{ type->initialSize() / (uint32_t)MEMORY_PAGE_SIZE, type->maximumSize() / (uint32_t)MEMORY_PAGE_SIZE }
    {
    }

    virtual wasm_externtype_t* clone() const override
    {
        MemoryType* type = static_cast<const MemoryType*>(get())->clone();
        return new wasm_memorytype_t(type);
    }

    wasm_limits_t limits;
};

struct wasm_ref_t {
    wasm_ref_t(const Object* o)
        : obj(o)
    {
    }

    virtual ~wasm_ref_t() {}

    const Object* get() const
    {
        ASSERT(obj);
        return obj;
    }

    const Object* obj;
};

struct wasm_extern_t : wasm_ref_t {
    wasm_extern_t(const std::shared_ptr<Object>& ptr, own const ObjectType* type)
        : wasm_ref_t(ptr.get())
        , sharedPtr(ptr)
        , objectType(type)
    {
    }

    virtual ~wasm_extern_t()
    {
        delete objectType;
    }

    const std::shared_ptr<Object>& getSharedPtr() const { return sharedPtr; }

    std::shared_ptr<Object> sharedPtr;
    const ObjectType* objectType;
};

struct wasm_module_t : wasm_ref_t {
    wasm_module_t(own const Module* module)
        : wasm_ref_t(module)
    {
    }

    Module* get() const
    {
        ASSERT(obj && obj->isModule());
        return const_cast<Module*>(static_cast<const Module*>(obj));
    }
};

struct wasm_func_t : wasm_extern_t {
    wasm_func_t(const wasm_func_t& other)
        : wasm_extern_t(std::static_pointer_cast<Object>(other.getSharedPtr()), other.type()->clone())
    {
    }

    wasm_func_t(const std::shared_ptr<Function>& func, const wasm_functype_t* ft)
        : wasm_extern_t(std::static_pointer_cast<Object>(func), static_cast<const FunctionType*>(ft->get())->clone())
    {
    }

    Function* get() const
    {
        ASSERT(obj && obj->isFunction());
        return const_cast<Function*>(static_cast<const Function*>(obj));
    }

    const FunctionType* type() const
    {
        ASSERT(objectType->kind() == ObjectType::FunctionKind);
        return static_cast<const FunctionType*>(objectType);
    }
};

struct wasm_global_t : wasm_extern_t {
    wasm_global_t(const wasm_global_t& other)
        : wasm_extern_t(std::static_pointer_cast<Object>(other.getSharedPtr()), other.type()->clone())
    {
    }

    wasm_global_t(const std::shared_ptr<Global>& glob, const wasm_globaltype_t* gt)
        : wasm_extern_t(std::static_pointer_cast<Object>(glob), static_cast<const GlobalType*>(gt->get())->clone())
    {
    }

    Global* get() const
    {
        ASSERT(obj && obj->isGlobal());
        return const_cast<Global*>(static_cast<const Global*>(obj));
    }

    const GlobalType* type() const
    {
        ASSERT(objectType->kind() == ObjectType::GlobalKind);
        return static_cast<const GlobalType*>(objectType);
    }
};

struct wasm_table_t : wasm_extern_t {
    wasm_table_t(const wasm_table_t& other)
        : wasm_extern_t(std::static_pointer_cast<Object>(other.getSharedPtr()), other.type()->clone())
    {
    }

    wasm_table_t(const std::shared_ptr<Table> table, const wasm_tabletype_t* tt)
        : wasm_extern_t(std::static_pointer_cast<Object>(table), static_cast<const TableType*>(tt->get())->clone())
    {
    }

    Table* get() const
    {
        ASSERT(obj && obj->isTable());
        return const_cast<Table*>(static_cast<const Table*>(obj));
    }

    const TableType* type() const
    {
        ASSERT(objectType->kind() == ObjectType::TableKind);
        return static_cast<const TableType*>(objectType);
    }
};

struct wasm_memory_t : wasm_extern_t {
    wasm_memory_t(const wasm_memory_t& other)
        : wasm_extern_t(std::static_pointer_cast<Object>(other.getSharedPtr()), other.type()->clone())
    {
    }

    wasm_memory_t(const std::shared_ptr<Memory> mem, const wasm_memorytype_t* mt)
        : wasm_extern_t(std::static_pointer_cast<Object>(mem), static_cast<const MemoryType*>(mt->get())->clone())
    {
    }

    Memory* get() const
    {
        ASSERT(obj && obj->isMemory());
        return const_cast<Memory*>(static_cast<const Memory*>(obj));
    }

    const MemoryType* type() const
    {
        ASSERT(objectType->kind() == ObjectType::MemoryKind);
        return static_cast<const MemoryType*>(objectType);
    }
};

struct wasm_instance_t : wasm_ref_t {
    wasm_instance_t(own const Instance* ins)
        : wasm_ref_t(ins)
    {
    }

    const Instance* get() const
    {
        ASSERT(obj && obj->isInstance());
        return static_cast<const Instance*>(obj);
    }
};

struct wasm_trap_t : wasm_ref_t {
    wasm_trap_t(own const Trap* t, const wasm_message_t* msg)
        : wasm_ref_t(t)
    {
        wasm_byte_vec_copy(&message, msg);
    }

    virtual ~wasm_trap_t()
    {
        delete obj;
    }

    const Trap* get() const
    {
        ASSERT(obj && obj->isTrap());
        return static_cast<const Trap*>(obj);
    }

    wasm_message_t message;
};

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

static void FromWalrusValueTypes(const ValueTypeVector& types, wasm_valtype_vec_t* out)
{
    wasm_valtype_vec_new_uninitialized(out, types.size());
    for (size_t i = 0; i < types.size(); i++) {
        out->data[i] = wasm_valtype_new(FromWalrusValueType(types[i]));
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
        result.kind = WASM_FUNCREF;
        // FIXME reference count
        result.of.ref = new wasm_ref_t(val.asFunction());
        break;
    case Value::Type::ExternRef:
        result.kind = WASM_ANYREF;
        // FIXME reference count
        result.of.ref = new wasm_ref_t(val.asObject());
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
        return Value(const_cast<Object*>(val.of.ref->get()));
    case WASM_FUNCREF:
        return Value(static_cast<Function*>(const_cast<Object*>(val.of.ref->get())));
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

// Rest of Structure Methods
wasm_functype_t::wasm_functype_t(own const FunctionType* type)
    : wasm_externtype_t(type)
{
    FromWalrusValueTypes(type->param(), &params);
    FromWalrusValueTypes(type->result(), &results);
}

extern "C" {

// Import Types
own wasm_importtype_t* wasm_importtype_new(
    own wasm_name_t* module, own wasm_name_t* name, own wasm_externtype_t* type)
{
    return new wasm_importtype_t(module, name, type);
}

const wasm_name_t* wasm_importtype_module(const wasm_importtype_t* importType)
{
    return &importType->module;
}

const wasm_name_t* wasm_importtype_name(const wasm_importtype_t* importType)
{
    return &importType->name;
}

const wasm_externtype_t* wasm_importtype_type(const wasm_importtype_t* importType)
{
    return importType->externType;
}

// Export Types
own wasm_exporttype_t* wasm_exporttype_new(own wasm_name_t* name, own wasm_externtype_t* type)
{
    return new wasm_exporttype_t(name, type);
}

const wasm_name_t* wasm_exporttype_name(const wasm_exporttype_t* exportType)
{
    return &exportType->name;
}

const wasm_externtype_t* wasm_exporttype_type(const wasm_exporttype_t* exportType)
{
    return exportType->type;
}

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
own static ValueTypeVector* ToWalrusValueTypes(const wasm_valtype_vec_t* types)
{
    ValueTypeVector* result = new ValueTypeVector();
    result->reserve(types->size);

    for (size_t i = 0; i < types->size; i++) {
        result->push_back(types->data[i]->type);
    }

    return result;
}

own wasm_functype_t* wasm_functype_new(
    own wasm_valtype_vec_t* params, own wasm_valtype_vec_t* results)
{
    FunctionType* type = new FunctionType(ToWalrusValueTypes(params), ToWalrusValueTypes(results));
    return new wasm_functype_t(type, params, results);
}

const wasm_valtype_vec_t* wasm_functype_params(const wasm_functype_t* ft)
{
    return &ft->params;
}

const wasm_valtype_vec_t* wasm_functype_results(const wasm_functype_t* ft)
{
    return &ft->results;
}

// Global Types
own wasm_globaltype_t* wasm_globaltype_new(own wasm_valtype_t* valtype, wasm_mutability_t mut)
{
    GlobalType* type = new GlobalType(valtype->type, mut == WASM_CONST ? false : true);
    wasm_valtype_delete(valtype);

    return new wasm_globaltype_t(type);
}

const wasm_valtype_t* wasm_globaltype_content(const wasm_globaltype_t* gt)
{
    return &gt->valtype;
}

wasm_mutability_t wasm_globaltype_mutability(const wasm_globaltype_t* gt)
{
    ASSERT(gt->get()->kind() == ObjectType::GlobalKind);
    bool mut = static_cast<const GlobalType*>(gt->get())->isMutable();

    return mut ? WASM_VAR : WASM_CONST;
}

// Table Types
own wasm_tabletype_t* wasm_tabletype_new(
    own wasm_valtype_t* valtype, const wasm_limits_t* limits)
{
    TableType* type = new TableType(valtype->type, limits->min, limits->max);
    wasm_valtype_delete(valtype);

    return new wasm_tabletype_t(type);
}

const wasm_valtype_t* wasm_tabletype_element(const wasm_tabletype_t* tt)
{
    return &tt->valtype;
}

const wasm_limits_t* wasm_tabletype_limits(const wasm_tabletype_t* tt)
{
    return &tt->limits;
}

// Memory Types
own wasm_memorytype_t* wasm_memorytype_new(const wasm_limits_t* limits)
{
    MemoryType* type = new MemoryType(limits->min * MEMORY_PAGE_SIZE, limits->max * MEMORY_PAGE_SIZE);
    return new wasm_memorytype_t(type);
}

const wasm_limits_t* wasm_memorytype_limits(const wasm_memorytype_t* mt)
{
    return &mt->limits;
}

// Extern Types
wasm_externkind_t wasm_externtype_kind(const wasm_externtype_t* type)
{
    ASSERT(type);
    switch (type->get()->kind()) {
    case ObjectType::FunctionKind:
        return WASM_EXTERN_FUNC;
    case ObjectType::GlobalKind:
        return WASM_EXTERN_GLOBAL;
    case ObjectType::TableKind:
        return WASM_EXTERN_TABLE;
    case ObjectType::MemoryKind:
        return WASM_EXTERN_MEMORY;
    default:
        RELEASE_ASSERT_NOT_REACHED();
        return WASM_EXTERN_MEMORY;
    }
}

///////////////////////////////////////////////////////////////////////////////
// Runtime Objects
// Values
void wasm_val_delete(own wasm_val_t* val)
{
    // FIXME
    if (wasm_valkind_is_ref(val->kind)) {
        delete val->of.ref;
        val->of.ref = nullptr;
    }
}

void wasm_val_copy(own wasm_val_t* out, const wasm_val_t* src)
{
    if (wasm_valkind_is_ref(src->kind)) {
        out->kind = src->kind;
        // FIXME ref count
        out->of.ref = src->of.ref ? new wasm_ref_t(src->of.ref->get()) : nullptr;
    } else {
        *out = *src;
    }
}

// References
// Frames

// Traps
own wasm_trap_t* wasm_trap_new(wasm_store_t* store, const wasm_message_t* message)
{
    Trap* trap = new Trap();
    return new wasm_trap_t(trap, message);
}

void wasm_trap_message(const wasm_trap_t* trap, own wasm_message_t* out)
{
    wasm_byte_vec_copy(out, &trap->message);
}

own wasm_frame_t* wasm_trap_origin(const wasm_trap_t*)
{
    // TODO
    RELEASE_ASSERT_NOT_REACHED();
}

void wasm_trap_trace(const wasm_trap_t*, own wasm_frame_vec_t* out)
{
    // TODO
    RELEASE_ASSERT_NOT_REACHED();
}

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

void wasm_module_imports(const wasm_module_t* module, own wasm_importtype_vec_t* out)
{
    const Vector<ImportType*, std::allocator<ImportType*>>& importTypes = module->get()->imports();

    wasm_importtype_vec_new_uninitialized(out, importTypes.size());
    own wasm_externtype_t* type;

    for (size_t i = 0; i < importTypes.size(); i++) {
        ImportType* importType = importTypes[i];
        switch (importType->importType()) {
        case ImportType::Function:
            type = new wasm_functype_t(importType->functionType()->clone());
            break;
        case ImportType::Global:
            type = new wasm_globaltype_t(importType->globalType()->clone());
            break;
        case ImportType::Table:
            type = new wasm_tabletype_t(importType->tableType()->clone());
            break;
        case ImportType::Memory:
            type = new wasm_memorytype_t(importType->memoryType()->clone());
            break;
        default:
            RELEASE_ASSERT_NOT_REACHED();
            break;
        }
        out->data[i] = new wasm_importtype_t(importType->moduleName(), importType->fieldName(), type);
    }
}

void wasm_module_exports(const wasm_module_t* module, own wasm_exporttype_vec_t* out)
{
    const Module* mod = module->get();
    const Vector<ExportType*, std::allocator<ExportType*>>& exportTypes = mod->exports();

    wasm_exporttype_vec_new_uninitialized(out, exportTypes.size());
    own wasm_externtype_t* type;

    for (size_t i = 0; i < exportTypes.size(); i++) {
        uint32_t itemIndex = exportTypes[i]->itemIndex();
        switch (exportTypes[i]->exportType()) {
        case ExportType::Function:
            // FIXME function type index may be wrong
            type = new wasm_functype_t(mod->functionType(itemIndex)->clone());
            break;
        case ExportType::Global:
            type = new wasm_globaltype_t(mod->globalType(itemIndex)->clone());
            break;
        case ExportType::Table:
            type = new wasm_tabletype_t(mod->tableType(itemIndex)->clone());
            break;
        case ExportType::Memory:
            type = new wasm_memorytype_t(mod->memoryType(itemIndex)->clone());
            break;
        default:
            RELEASE_ASSERT_NOT_REACHED();
            break;
        }

        out->data[i] = new wasm_exporttype_t(exportTypes[i]->name(), type);
    }
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
    std::shared_ptr<Function> func = std::make_shared<ImportedFunction>(
        ToWalrusFunctionType(ft),
        [=](ExecutionState& state, const uint32_t argc, Value* argv, Value* result, void* d) {
            wasm_val_vec_t params, results;
            wasm_val_vec_new_uninitialized(&params, argc);
            wasm_val_vec_new_uninitialized(&results, reinterpret_cast<size_t>(d));
            FromWalrusValues(params.data, argv, argc);

            auto trap = callback(&params, &results);
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
        reinterpret_cast<void*>(ft->results.size));

    return new wasm_func_t(func, ft);
}

own wasm_func_t* wasm_func_new_with_env(
    wasm_store_t* store, const wasm_functype_t* ft, wasm_func_callback_with_env_t callback,
    void* env, void (*finalizer)(void*))
{
    // TODO: finalizer
    std::shared_ptr<Function> func = std::make_shared<ImportedFunction>(
        ToWalrusFunctionType(ft),
        [=](ExecutionState& state, const uint32_t argc, Value* argv, Value* result, void* d) {
            wasm_val_vec_t params, results;
            wasm_val_vec_new_uninitialized(&params, argc);
            wasm_val_vec_new_uninitialized(&results, reinterpret_cast<size_t>(d));
            FromWalrusValues(params.data, argv, argc);

            auto trap = callback(env, &params, &results);
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
        reinterpret_cast<void*>(ft->results.size));

    return new wasm_func_t(func, ft);
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

    return wasm_functype_new(&params, &results);
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
    walrusArgs.resize(paramNum);
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
        // FIXME
        own wasm_message_t msg;
        if (trapResult.exception->message().length()) {
            std::string& exceptionMsg = trapResult.exception->message();
            wasm_byte_vec_new(&msg, exceptionMsg.length(), exceptionMsg.data());
        } else {
            wasm_byte_vec_new_empty(&msg);
        }

        wasm_trap_t* trap = new wasm_trap_t(new Trap(), &msg);
        wasm_byte_vec_delete(&msg);
        return trap;
    }

    FromWalrusValues(results->data, walrusResults.data(), resultNum);
    return nullptr;
}

// Global Instances
own wasm_global_t* wasm_global_new(
    wasm_store_t* store, const wasm_globaltype_t* gt, const wasm_val_t* val)
{
    std::shared_ptr<Global> glob = std::make_shared<Global>(ToWalrusValue(*val));
    return new wasm_global_t(glob, gt);
}

own wasm_globaltype_t* wasm_global_type(const wasm_global_t* glob)
{
    return new wasm_globaltype_t(glob->type()->clone());
}

void wasm_global_get(const wasm_global_t* glob, own wasm_val_t* out)
{
    *out = FromWalrusValue(glob->get()->value());
}

void wasm_global_set(wasm_global_t* glob, const wasm_val_t* val)
{
    glob->get()->setValue(ToWalrusValue(*val));
}

// Table Instances
own wasm_table_t* wasm_table_new(
    wasm_store_t* store, const wasm_tabletype_t* tt, wasm_ref_t* init)
{
    std::shared_ptr<Table> table = std::make_shared<Table>(tt->valtype.type, tt->limits.min, tt->limits.max);
    return new wasm_table_t(table, tt);
}

own wasm_tabletype_t* wasm_table_type(const wasm_table_t* table)
{
    return new wasm_tabletype_t(table->type()->clone());
}

own wasm_ref_t* wasm_table_get(const wasm_table_t* table, wasm_table_size_t index)
{
    if (UNLIKELY(index >= table->get()->size())) {
        return nullptr;
    }

    Value val(table->get()->uncheckedGetElement(index));

    if (val.isNull()) {
        return nullptr;
    }

    // FIXME ref count
    return new wasm_ref_t(val.asObject());
}

bool wasm_table_set(wasm_table_t* table, wasm_table_size_t index, wasm_ref_t* ref)
{
    if (UNLIKELY(index >= table->get()->size())) {
        return false;
    }

    table->get()->uncheckedSetElement(index, ref ? const_cast<Object*>(ref->get()) : reinterpret_cast<void*>(Value::NullBits));
    return true;
}

wasm_table_size_t wasm_table_size(const wasm_table_t* table)
{
    return table->get()->size();
}

bool wasm_table_grow(wasm_table_t* table, wasm_table_size_t delta, wasm_ref_t* init)
{
    if (UNLIKELY(delta + table->get()->size() > table->get()->maximumSize())) {
        return false;
    }

    table->get()->grow(delta + table->get()->size(), init ? const_cast<Object*>(init->get()) : reinterpret_cast<void*>(Value::NullBits));
    return true;
}

// Memory Instances
own wasm_memory_t* wasm_memory_new(wasm_store_t* store, const wasm_memorytype_t* mt)
{
    std::shared_ptr<Memory> mem = std::make_shared<Memory>(mt->limits.min * MEMORY_PAGE_SIZE, mt->limits.max * MEMORY_PAGE_SIZE);
    return new wasm_memory_t(mem, mt);
}

own wasm_memorytype_t* wasm_memory_type(const wasm_memory_t* mem)
{
    return new wasm_memorytype_t(mem->type()->clone());
}

byte_t* wasm_memory_data(wasm_memory_t* mem)
{
    return reinterpret_cast<byte_t*>(mem->get()->buffer());
}

size_t wasm_memory_data_size(const wasm_memory_t* mem)
{
    return mem->get()->sizeInByte();
}

wasm_memory_pages_t wasm_memory_size(const wasm_memory_t* mem)
{
    return mem->get()->sizeInPageSize();
}

bool wasm_memory_grow(wasm_memory_t* mem, wasm_memory_pages_t delta)
{
    return mem->get()->grow(delta * MEMORY_PAGE_SIZE);
}

// Externals

wasm_externkind_t wasm_extern_kind(const wasm_extern_t* ext)
{
    switch (ext->get()->kind()) {
    case Object::FunctionKind:
        return WASM_EXTERN_FUNC;
    case Object::GlobalKind:
        return WASM_EXTERN_GLOBAL;
    case Object::TableKind:
        return WASM_EXTERN_TABLE;
    case Object::MemoryKind:
        return WASM_EXTERN_MEMORY;
    default:
        RELEASE_ASSERT_NOT_REACHED();
        return WASM_EXTERN_MEMORY;
    }
}

own wasm_externtype_t* wasm_extern_type(const wasm_extern_t* ext)
{
    switch (ext->get()->kind()) {
    case Object::FunctionKind:
        return new wasm_functype_t(static_cast<const wasm_func_t*>(ext)->type()->clone());
    case Object::GlobalKind:
        return new wasm_globaltype_t(static_cast<const wasm_global_t*>(ext)->type()->clone());
    case Object::TableKind:
        return new wasm_tabletype_t(static_cast<const wasm_table_t*>(ext)->type()->clone());
    case Object::MemoryKind:
        return new wasm_memorytype_t(static_cast<const wasm_memory_t*>(ext)->type()->clone());
    default:
        RELEASE_ASSERT_NOT_REACHED();
        return nullptr;
    }
}

// Module Instances
own wasm_instance_t* wasm_instance_new(
    wasm_store_t* store, const wasm_module_t* module, const wasm_extern_vec_t* imports,
    own wasm_trap_t** outTrap)
{
    struct RunData {
        Module* module;
        SharedObjectVector importValues;
        Instance* instance;
    } data = { module->get(), SharedObjectVector(), nullptr };

    data.importValues.reserve(imports->size);
    for (size_t i = 0; i < imports->size; i++) {
        data.importValues.push_back(imports->data[i]->getSharedPtr());
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

void wasm_instance_exports(const wasm_instance_t* ins, own wasm_extern_vec_t* out)
{
    Instance* instance = const_cast<Instance*>(ins->get());
    const Vector<ExportType*, std::allocator<ExportType*>>& exports = instance->module()->exports();

    wasm_extern_vec_new_uninitialized(out, exports.size());
    for (size_t i = 0; i < exports.size(); i++) {
        std::shared_ptr<Object> item;
        ObjectType* type;
        uint32_t itemIndex = exports[i]->itemIndex();
        switch (exports[i]->exportType()) {
        case ExportType::Function: {
            std::shared_ptr<Function>& func = instance->functionPtr(itemIndex);
            item = std::static_pointer_cast<Object>(func);
            type = const_cast<FunctionType*>(func->functionType()->clone());
            break;
        }
        case ExportType::Table: {
            item = std::static_pointer_cast<Object>(instance->tablePtr(itemIndex));
            type = instance->module()->tableType(itemIndex)->clone();
            break;
        }
        case ExportType::Memory: {
            item = std::static_pointer_cast<Object>(instance->memoryPtr(itemIndex));
            type = instance->module()->memoryType(itemIndex)->clone();
            break;
        }
        case ExportType::Global: {
            item = std::static_pointer_cast<Object>(instance->globalPtr(itemIndex));
            type = instance->module()->globalType(itemIndex)->clone();
            break;
        }
        default: {
            RELEASE_ASSERT_NOT_REACHED();
            break;
        }
        }
        out->data[i] = new wasm_extern_t(item, type);
    }
}

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
WASM_IMPL_VEC_OWN(extern);

#define WASM_IMPL_TYPE(name)                                              \
    WASM_IMPL_OWN(name)                                                   \
    WASM_IMPL_VEC_OWN(name)                                               \
    own wasm_##name##_t* wasm_##name##_copy(const wasm_##name##_t* other) \
    {                                                                     \
        return new wasm_##name##_t(*other);                               \
    }

WASM_IMPL_TYPE(valtype);
WASM_IMPL_TYPE(importtype);
WASM_IMPL_TYPE(exporttype);

#define WASM_IMPL_TYPE_CLONE(name)                                        \
    WASM_IMPL_OWN(name)                                                   \
    WASM_IMPL_VEC_OWN(name)                                               \
    own wasm_##name##_t* wasm_##name##_copy(const wasm_##name##_t* other) \
    {                                                                     \
        return static_cast<wasm_##name##_t*>(other->clone());             \
    }

WASM_IMPL_TYPE_CLONE(functype);
WASM_IMPL_TYPE_CLONE(globaltype);
WASM_IMPL_TYPE_CLONE(tabletype);
WASM_IMPL_TYPE_CLONE(memorytype);
WASM_IMPL_TYPE_CLONE(externtype);

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
WASM_IMPL_REF(global);
WASM_IMPL_REF(instance);
WASM_IMPL_REF(memory);
WASM_IMPL_REF(table);
WASM_IMPL_REF(trap);
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

WASM_IMPL_EXTERN(table);
WASM_IMPL_EXTERN(func);
WASM_IMPL_EXTERN(global);
WASM_IMPL_EXTERN(memory);

} // extern "C"

#endif // __WalrusAPI__
