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
#include "Walrus.h"

#include "runtime/Store.h"
#include "runtime/Module.h"
#include "runtime/Instance.h"
#include "runtime/Function.h"
#include "runtime/Global.h"
#include "runtime/Table.h"
#include "runtime/Memory.h"
#include "runtime/Tag.h"
#include "runtime/Trap.h"
#include "runtime/JITExec.h"
#include "interpreter/ByteCode.h"
#include "interpreter/Interpreter.h"
#include "parser/WASMParser.h"
#include "wasi/WASI.h"

namespace Walrus {

DEFINE_GLOBAL_TYPE_INFO(moduleTypeInfo, ModuleKind);

ModuleFunction::ModuleFunction(FunctionType* functionType)
    : m_hasTryCatch(false)
    , m_requiredStackSize(std::max(functionType->paramStackSize(), functionType->resultStackSize()))
    , m_functionType(functionType)
#if defined(WALRUS_ENABLE_JIT)
    , m_jitFunction(nullptr)
#endif
{
}

Module::Module(Store* store, WASMParsingResult& result)
    : Object(GET_GLOBAL_TYPE_INFO(moduleTypeInfo))
    , m_store(store)
    , m_seenStartAttribute(result.m_seenStartAttribute)
    , m_version(result.m_version)
    , m_start(result.m_start)
    , m_imports(std::move(result.m_imports))
    , m_exports(std::move(result.m_exports))
    , m_functions(std::move(result.m_functions))
    , m_datas(std::move(result.m_datas))
    , m_elements(std::move(result.m_elements))
    , m_compositeTypes(std::move(result.m_compositeTypes))
    , m_globalTypes(std::move(result.m_globalTypes))
    , m_tableTypes(std::move(result.m_tableTypes))
    , m_memoryTypes(std::move(result.m_memoryTypes))
    , m_tagTypes(std::move(result.m_tagTypes))
#if defined(WALRUS_ENABLE_JIT)
    , m_jitModule(nullptr)
#endif
{
    store->appendModule(this);
}

ModuleFunction::~ModuleFunction()
{
#if defined(WALRUS_ENABLE_JIT)
    if (m_jitFunction != nullptr) {
        delete m_jitFunction;
    }
#endif
}

Module::~Module()
{
    // Types are freed by the type store.

    for (size_t i = 0; i < m_imports.size(); i++) {
        delete m_imports[i];
    }

    for (size_t i = 0; i < m_exports.size(); i++) {
        delete m_exports[i];
    }

    for (size_t i = 0; i < m_functions.size(); i++) {
        delete m_functions[i];
    }

    for (size_t i = 0; i < m_datas.size(); i++) {
        delete m_datas[i];
    }

    for (size_t i = 0; i < m_elements.size(); i++) {
        delete m_elements[i];
    }

    for (size_t i = 0; i < m_globalTypes.size(); i++) {
        delete m_globalTypes[i];
    }

    for (size_t i = 0; i < m_tableTypes.size(); i++) {
        delete m_tableTypes[i];
    }

    for (size_t i = 0; i < m_memoryTypes.size(); i++) {
        delete m_memoryTypes[i];
    }

    for (size_t i = 0; i < m_tagTypes.size(); i++) {
        delete m_tagTypes[i];
    }

#if defined(WALRUS_ENABLE_JIT)
    if (m_jitModule != nullptr) {
        delete m_jitModule;
    }
#endif
}

Instance* Module::instantiate(ExecutionState& state, const ExternVector& imports)
{
    Instance* instance = Instance::newInstance(this);

    void** references = instance->alignedEnd();

    // Must follow the order in Instance::newInstance.
    instance->m_memories = reinterpret_cast<Memory**>(references);
    references += numberOfMemoryTypes();
    Memory::TargetBuffer* targetBuffers = reinterpret_cast<Memory::TargetBuffer*>(references);
    references += numberOfMemoryTypes() * (sizeof(Memory::TargetBuffer) / sizeof(void*));
    instance->m_globals = reinterpret_cast<Global**>(references);
    references += numberOfGlobalTypes();
    instance->m_tables = reinterpret_cast<Table**>(references);
    references += numberOfTableTypes();
    instance->m_functions = reinterpret_cast<Function**>(references);
    references += numberOfFunctions();
    instance->m_tags = reinterpret_cast<Tag**>(references);
    references += numberOfTagTypes();
    instance->m_dataSegments = reinterpret_cast<DataSegment*>(references);
    references += numberOfDataSegments() * (sizeof(DataSegment) / sizeof(void*));
    instance->m_elementSegments = reinterpret_cast<ElementSegment*>(references);

    size_t funcIndex = 0;
    size_t globIndex = 0;
    size_t tableIndex = 0;
    size_t memIndex = 0;
    size_t tagIndex = 0;

    for (size_t i = 0; i < numberOfMemoryTypes(); i++) {
        targetBuffers[i].setUninitialized();
    }

    for (size_t i = 0; i < m_elements.size(); i++) {
        instance->m_elementSegments[i].setUninitialized();
    }

    if (imports.size() < m_imports.size()) {
        std::string message = "Insufficient import(s):\n";

        for (uint32_t i = imports.size(); i < m_imports.size(); i++) {
            message.append("  " + m_imports[i]->moduleName() + " : " + m_imports[i]->fieldName() + " -- Type:" + m_imports[i]->typeToString() + "\n");
        }
        Trap::throwException(state, message);
    }

    for (size_t i = 0; i < m_imports.size(); i++) {
        auto type = m_imports[i]->importType();

        switch (type) {
        case ImportType::Function: {
            if (UNLIKELY(imports[i]->kind() != Object::FunctionKind
                         || !imports[i]->asFunction()->functionType()->equals(m_imports[i]->functionType(), true))) {
                Trap::throwException(state, "incompatible import type");
            }

            instance->m_functions[funcIndex] = imports[i]->asFunction();
            if (imports[i]->asFunction()->isWasiFunction()) {
                instance->m_functions[funcIndex]->asWasiFunction()->setRunningInstance(instance);
            }
            funcIndex++;
            break;
        }
        case ImportType::Global: {
            if (UNLIKELY(imports[i]->kind() != Object::GlobalKind
                         || !m_imports[i]->globalType()->type().isSubType(imports[i]->asGlobal()->type())
                         || m_imports[i]->globalType()->isMutable() != imports[i]->asGlobal()->isMutable())) {
                Trap::throwException(state, "incompatible import type");
            }

            instance->m_globals[globIndex++] = imports[i]->asGlobal();
            break;
        }
        case ImportType::Table: {
            if (UNLIKELY(imports[i]->kind() != Object::TableKind
                         || m_imports[i]->tableType()->type() != imports[i]->asTable()->type()
                         || m_imports[i]->tableType()->initialSize() > imports[i]->asTable()->size()
                         || m_imports[i]->tableType()->maximumSize() < imports[i]->asTable()->maximumSize())) {
                Trap::throwException(state, "incompatible import type");
            }

            instance->m_tables[tableIndex++] = imports[i]->asTable();
            break;
        }
        case ImportType::Memory: {
            if (UNLIKELY(imports[i]->kind() != Object::MemoryKind
                         || m_imports[i]->memoryType()->initialSize() > imports[i]->asMemory()->sizeInPageSize()
                         || m_imports[i]->memoryType()->maximumSize() < imports[i]->asMemory()->maximumSizeInPageSize())) {
                Trap::throwException(state, "incompatible import type");
            }

            instance->m_memories[memIndex++] = imports[i]->asMemory();
            break;
        }
        case ImportType::Tag: {
            if (UNLIKELY(imports[i]->kind() != Object::TagKind)) {
                Trap::throwException(state, "incompatible import type");
            }
            instance->m_tags[tagIndex++] = imports[i]->asTag();
            break;
        }
        default: {
            RELEASE_ASSERT_NOT_REACHED();
            break;
        }
        }
    }

    // init defined function
    while (funcIndex < m_functions.size()) {
        instance->m_functions[funcIndex] = DefinedFunction::createDefinedFunction(m_store, instance, function(funcIndex));
        funcIndex++;
    }

    // init table
    while (tableIndex < m_tableTypes.size()) {
        TableType* tableType = m_tableTypes[tableIndex];
        void* initValue = nullptr;

        if (tableType->function()) {
            struct RunData {
                Instance* instance;
                Module* module;
                ModuleFunction* mf;
                void* initValue;
            } data = { instance, this, tableType->function(), nullptr };
            Walrus::Trap trap;
            trap.run([](Walrus::ExecutionState& state, void* d) {
                RunData* data = reinterpret_cast<RunData*>(d);
                DefinedFunctionWithTryCatch fakeFunction(data->instance, data->mf);
                Value result;
                fakeFunction.call(state, nullptr, &result);
                data->initValue = result.asReference();
            },
                     &data);
            initValue = data.initValue;
        }

        instance->m_tables[tableIndex] = Table::createTable(m_store, tableType->type(), tableType->initialSize(), tableType->maximumSize(), initValue);
        tableIndex++;
    }

    // init memory
    while (memIndex < m_memoryTypes.size()) {
        instance->m_memories[memIndex] = Memory::createMemory(m_store, m_memoryTypes[memIndex]->initialSize() * Memory::s_memoryPageSize, m_memoryTypes[memIndex]->maximumSize() * Memory::s_memoryPageSize, m_memoryTypes[memIndex]->isShared());
        memIndex++;
    }

    // init tag
    while (tagIndex < m_tagTypes.size()) {
        instance->m_tags[tagIndex] = Tag::createTag(m_store, m_compositeTypes[m_tagTypes[tagIndex]->sigIndex()]->asFunction());
        tagIndex++;
    }

    // All memories are resolved, enque them.
    for (size_t i = 0; i < numberOfMemoryTypes(); i++) {
        targetBuffers[i].enque(instance->m_memories[i]);
    }

    // init global
    while (globIndex < m_globalTypes.size()) {
        GlobalType* globalType = m_globalTypes[globIndex];
        instance->m_globals[globIndex] = Global::createGlobal(m_store, Value(globalType->type()), globalType->type());

        if (globalType->function()) {
            struct RunData {
                Instance* instance;
                Module* module;
                ModuleFunction* mf;
                size_t index;
            } data = { instance, this, globalType->function(), globIndex };
            Walrus::Trap trap;
            trap.run([](Walrus::ExecutionState& state, void* d) {
                RunData* data = reinterpret_cast<RunData*>(d);
                DefinedFunctionWithTryCatch fakeFunction(data->instance, data->mf);
                Value result;
                fakeFunction.call(state, nullptr, &result);
                data->instance->m_globals[data->index]->setValue(result);
            },
                     &data);
        }

        globIndex++;
    }

    // init table(elem segment)
    for (size_t i = 0; i < m_elements.size(); i++) {
        Element* elem = m_elements[i];
        const auto& exprs = elem->exprFunctions();

        new (instance->m_elementSegments + i) ElementSegment(exprs.size());
        void** result = instance->m_elementSegments[i].elements();

        for (size_t j = 0; j < exprs.size(); j++) {
            struct RunData {
                Instance* instance;
                ModuleFunction* exprFunc;
                void* ref;
            } data = { instance, exprs[j], nullptr };
            Walrus::Trap trap;
            trap.run([](Walrus::ExecutionState& state, void* d) {
                RunData* data = reinterpret_cast<RunData*>(d);
                DefinedFunctionWithTryCatch fakeFunction(data->instance, data->exprFunc);
                Value func;
                fakeFunction.call(state, nullptr, &func);
                data->ref = func.asReference();
            },
                     &data);

            result[j] = data.ref;
        }

        if (elem->mode() == SegmentMode::Active) {
            uint32_t offset = 0;
            if (elem->hasOffsetFunction()) {
                struct RunData {
                    Instance* instance;
                    ModuleFunction* offsetFunc;
                    uint32_t& offset;
                } data = { instance, elem->offsetFunction(), offset };
                Walrus::Trap trap;
                trap.run([](Walrus::ExecutionState& state, void* d) {
                    RunData* data = reinterpret_cast<RunData*>(d);
                    DefinedFunctionWithTryCatch fakeFunction(data->instance, data->offsetFunc);
                    Value offset;
                    fakeFunction.call(state, nullptr, &offset);
                    data->offset = offset.asI32();
                },
                         &data);
            }

            if (UNLIKELY(elem->tableIndex() >= numberOfTableTypes())) {
                Trap::throwException(state, "out of bounds table access");
            }

            uint32_t size = instance->m_tables[elem->tableIndex()]->size();
            if (UNLIKELY(offset > size || (size - offset) < elem->exprFunctions().size())) {
                Trap::throwException(state, "out of bounds table access");
            }

            instance->m_tables[elem->tableIndex()]->initTable(instance->m_elementSegments + i, offset, 0, exprs.size());
            instance->m_elementSegments[i].drop();
        } else if (elem->mode() == SegmentMode::Declared) {
            instance->m_elementSegments[i].drop();
        }
    }

    // init memory
    for (size_t i = 0; i < m_datas.size(); i++) {
        Data* init = m_datas[i];
        instance->m_dataSegments[i] = DataSegment(init);
        struct RunData {
            Data* init;
            Instance* instance;
            Module* module;
        } data = { init, instance, this };
        Walrus::Trap trap;
        auto result = trap.run([](Walrus::ExecutionState& state, void* d) {
            RunData* data = reinterpret_cast<RunData*>(d);
            if (data->init->moduleFunction()->currentByteCodeSize()) {
                DefinedFunctionWithTryCatch fakeFunction(data->instance,
                                                         data->init->moduleFunction());
                Value offset;
                fakeFunction.call(state, nullptr, &offset);

                Memory* m = data->instance->memory(data->init->memIndex());
                const auto& initData = data->init->initData();
                if (m->sizeInByte() >= initData.size() && (offset.asI32() + initData.size()) <= m->sizeInByte() && offset.asI32() >= 0) {
                    memcpyEndianAware(m->buffer(), initData.data(), m->sizeInByte(), initData.size(), offset.asI32(), 0, initData.size());
                } else {
                    Trap::throwException(state, "out of bounds memory access");
                }
            }
        },
                               &data);

        if (result.exception) {
            Trap::throwException(state, std::move(result.exception));
        }
    }

#ifndef NDEBUG
    // check total size of data in vector
    ASSERT(funcIndex == numberOfFunctions());
    ASSERT(globIndex == numberOfGlobalTypes());
    ASSERT(tableIndex == numberOfTableTypes());
    ASSERT(memIndex == numberOfMemoryTypes());
    ASSERT(tagIndex == numberOfTagTypes());
#endif

    if (m_seenStartAttribute) {
        ASSERT(instance->m_functions[m_start]->functionType()->param().size() == 0);
        ASSERT(instance->m_functions[m_start]->functionType()->result().size() == 0);
        instance->m_functions[m_start]->call(state, nullptr, nullptr);
    }

    return instance;
}

#if !defined(NDEBUG)

static const char* typeName(Value::Type v)
{
    switch (v) {
    case Value::I32:
        return "i32";
    case Value::I64:
        return "i64";
    case Value::F32:
        return "f32";
    case Value::F64:
        return "f64";
    case Value::V128:
        return "v128";
    case Value::AnyRef:
        return "AnyRef";
    case Value::NoAnyRef:
        return "NoAnyRef";
    case Value::EqRef:
        return "EqRef";
    case Value::I31Ref:
        return "I31Ref";
    case Value::StructRef:
        return "StructRef";
    case Value::ArrayRef:
        return "ArrayRef";
    case Value::ExternRef:
        return "ExternRef";
    case Value::NoExternRef:
        return "NoExternRef";
    case Value::FuncRef:
        return "FuncRef";
    case Value::DefinedRef:
        return "DefinedRef";
    case Value::NoFuncRef:
        return "NoFuncRef";
    case Value::NullAnyRef:
        return "NullAnyRef";
    case Value::NullNoAnyRef:
        return "NullNoAnyRef";
    case Value::NullEqRef:
        return "NullEqRef";
    case Value::NullI31Ref:
        return "NullI31Ref";
    case Value::NullStructRef:
        return "NullStructRef";
    case Value::NullArrayRef:
        return "NullArrayRef";
    case Value::NullExternRef:
        return "NullExternRef";
    case Value::NullNoExternRef:
        return "NullNoExternRef";
    case Value::NullFuncRef:
        return "NullFuncRef";
    case Value::NullNoFuncRef:
        return "NullNoFuncRef";
    case Value::NullDefinedRef:
        return "NullDefinedRef";
    default:
        RELEASE_ASSERT_NOT_REACHED();
    }
}

static void dumpValue(Value v)
{
    switch (v.type()) {
    case Value::I32:
        printf("%" PRId32, v.asI32());
        break;
    case Value::I64:
        printf("%" PRId64, v.asI64());
        break;
    case Value::F32:
        printf("%f", v.asF32());
        break;
    case Value::F64:
        printf("%lf", v.asF64());
        break;
    case Value::V128:
        printf("%" PRIu8, v.asV128().m_data[0]);
        printf(" %" PRIu8, v.asV128().m_data[1]);
        printf(" %" PRIu8, v.asV128().m_data[2]);
        printf(" %" PRIu8, v.asV128().m_data[3]);
        printf(" %" PRIu8, v.asV128().m_data[4]);
        printf(" %" PRIu8, v.asV128().m_data[5]);
        printf(" %" PRIu8, v.asV128().m_data[6]);
        printf(" %" PRIu8, v.asV128().m_data[7]);
        printf(" %" PRIu8, v.asV128().m_data[8]);
        printf(" %" PRIu8, v.asV128().m_data[9]);
        printf(" %" PRIu8, v.asV128().m_data[10]);
        printf(" %" PRIu8, v.asV128().m_data[11]);
        printf(" %" PRIu8, v.asV128().m_data[12]);
        printf(" %" PRIu8, v.asV128().m_data[13]);
        printf(" %" PRIu8, v.asV128().m_data[14]);
        printf(" %" PRIu8, v.asV128().m_data[15]);
        break;
    default:
        if (v.isRef()) {
            break;
        }
        RELEASE_ASSERT_NOT_REACHED();
    }
    printf(", %s", typeName(v.type()));
}

void ModuleFunction::dumpByteCode()
{
    printf("\n");
    printf("required stack size: %u bytes\n", m_requiredStackSize);
    printf("stack: [");
    size_t pos = 0;
    const TypeVector::Types& param = m_functionType->param().types();
    for (size_t i = 0; i < param.size(); i++) {
        printf("(parameter %zu, %s, pos %zu) ", i, typeName(param[i]), pos);
        pos += valueStackAllocatedSize(param[i]);
    }
    for (size_t i = 0; i < m_local.size(); i++) {
        if (m_localDebugData[i] == UINT16_MAX) {
            printf("(local %zu, %s, Unused) ", i, typeName(m_local[i]));
        } else {
            printf("(local %zu, %s, pos %zu) ", i, typeName(m_local[i]), m_localDebugData[i]);
        }
    }
    printf("\n");
    for (size_t i = 0; i < m_constantDebugData.size(); i++) {
        printf("(constant ");
        dumpValue(m_constantDebugData[i].first);
        printf(", pos %zu) ", m_constantDebugData[i].second);
    }
    printf("....]\n");

    printf("bytecode size: %zu bytes\n", m_byteCode.size());
    printf("\n");

    size_t idx = 0;
    while (idx < m_byteCode.size()) {
        ByteCode* code = reinterpret_cast<ByteCode*>(&m_byteCode[idx]);
        printf("%6zu ", idx);

        switch (code->opcode()) {
#define GENERATE_BYTECODE_CASE(name, ...)    \
    case ByteCode::name##Opcode:             \
        static_cast<name*>(code)->dump(idx); \
        break;

            FOR_EACH_BYTECODE(GENERATE_BYTECODE_CASE);
#undef GENERATE_BYTECODE_CASE
        default:
            ASSERT_NOT_REACHED();
            break;
        }

        printf("\n");
        idx += code->getSize();
    }
    printf("\n");
}
#endif

} // namespace Walrus
