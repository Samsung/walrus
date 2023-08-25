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
#include "interpreter/ByteCode.h"
#include "interpreter/Interpreter.h"
#include "parser/WASMParser.h"

namespace Walrus {

ModuleFunction::ModuleFunction(FunctionType* functionType)
    : m_functionType(functionType)
    , m_requiredStackSize(std::max(m_functionType->paramStackSize(), m_functionType->resultStackSize()))
    , m_requiredStackSizeDueToParameterAndLocal(0)
{
}

Module::Module(Store* store, WASMParsingResult& result)
    : m_store(store)
    , m_seenStartAttribute(result.m_seenStartAttribute)
    , m_version(result.m_version)
    , m_start(result.m_start)
    , m_imports(std::move(result.m_imports))
    , m_exports(std::move(result.m_exports))
    , m_functions(std::move(result.m_functions))
    , m_datas(std::move(result.m_datas))
    , m_elements(std::move(result.m_elements))
    , m_functionTypes(std::move(result.m_functionTypes))
    , m_globalTypes(std::move(result.m_globalTypes))
    , m_tableTypes(std::move(result.m_tableTypes))
    , m_memoryTypes(std::move(result.m_memoryTypes))
    , m_tagTypes(std::move(result.m_tagTypes))
{
    store->appendModule(this);
}

Module::~Module()
{
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

    for (size_t i = 0; i < m_functionTypes.size(); i++) {
        delete m_functionTypes[i];
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
}

Instance* Module::instantiate(ExecutionState& state, const ExternVector& imports)
{
    Instance* instance = Instance::newInstance(this);

    void** references = reinterpret_cast<void**>(reinterpret_cast<uintptr_t>(instance) + Instance::alignedSize());

    // Must follow the order in Instance::newInstance.
    instance->m_memories = reinterpret_cast<Memory**>(references);
    references += numberOfMemoryTypes();
    instance->m_globals = reinterpret_cast<Global**>(references);
    references += numberOfGlobalTypes();
    instance->m_tables = reinterpret_cast<Table**>(references);
    references += numberOfTableTypes();
    instance->m_functions = reinterpret_cast<Function**>(references);
    references += numberOfFunctions();
    instance->m_tags = reinterpret_cast<Tag**>(references);

    size_t funcIndex = 0;
    size_t globIndex = 0;
    size_t tableIndex = 0;
    size_t memIndex = 0;
    size_t tagIndex = 0;

    if (imports.size() < m_imports.size()) {
        Trap::throwException(state, "Insufficient import");
    }

    for (size_t i = 0; i < m_imports.size(); i++) {
        auto type = m_imports[i]->importType();

        switch (type) {
        case ImportType::Function: {
            if (imports[i]->kind() != Object::FunctionKind) {
                Trap::throwException(state, "incompatible import type");
            }
            if (!imports[i]->asFunction()->functionType()->equals(m_imports[i]->functionType())) {
                Trap::throwException(state, "imported function type mismatch");
            }
            instance->m_functions[funcIndex++] = imports[i]->asFunction();
            break;
        }
        case ImportType::Global: {
            if (imports[i]->kind() != Object::GlobalKind) {
                Trap::throwException(state, "incompatible import type");
            }
            instance->m_globals[globIndex++] = imports[i]->asGlobal();
            break;
        }
        case ImportType::Table: {
            if (imports[i]->kind() != Object::TableKind
                || m_imports[i]->tableType()->type() != imports[i]->asTable()->type()
                || m_imports[i]->tableType()->initialSize() > imports[i]->asTable()->size()) {
                Trap::throwException(state, "incompatible import type");
            }

            if (imports[i]->asTable()->maximumSize() > m_imports[i]->tableType()->maximumSize()) {
                Trap::throwException(state, "incompatible import type");
            }
            instance->m_tables[tableIndex++] = imports[i]->asTable();
            break;
        }
        case ImportType::Memory: {
            if (imports[i]->kind() != Object::MemoryKind
                || m_imports[i]->memoryType()->initialSize() > imports[i]->asMemory()->sizeInPageSize()) {
                Trap::throwException(state, "incompatible import type");
            }

            if (imports[i]->asMemory()->maximumSizeInPageSize() > m_imports[i]->memoryType()->maximumSize()) {
                Trap::throwException(state, "incompatible import type");
            }
            instance->m_memories[memIndex++] = imports[i]->asMemory();
            break;
        }
        case ImportType::Tag: {
            if (imports[i]->kind() != Object::TagKind) {
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
        instance->m_tables[tableIndex] = Table::createTable(m_store, m_tableTypes[tableIndex]->type(), m_tableTypes[tableIndex]->initialSize(), m_tableTypes[tableIndex]->maximumSize());
        tableIndex++;
    }

    // init memory
    while (memIndex < m_memoryTypes.size()) {
        instance->m_memories[memIndex] = Memory::createMemory(m_store, m_memoryTypes[memIndex]->initialSize() * Memory::s_memoryPageSize, m_memoryTypes[memIndex]->maximumSize() * Memory::s_memoryPageSize);
        memIndex++;
    }

    // init tag
    while (tagIndex < m_tagTypes.size()) {
        instance->m_tags[tagIndex] = Tag::createTag(m_store, m_functionTypes[m_tagTypes[tagIndex]->sigIndex()]);
        tagIndex++;
    }

    // init global
    while (globIndex < m_globalTypes.size()) {
        GlobalType* globalType = m_globalTypes[globIndex];
        instance->m_globals[globIndex] = Global::createGlobal(m_store, Value(globalType->type()));

        if (globalType->function()) {
            struct RunData {
                Instance* instance;
                Module* module;
                Value::Type type;
                ModuleFunction* mf;
                size_t index;
            } data = { instance, this, globalType->type(), globalType->function(), globIndex };
            Walrus::Trap trap;
            trap.run([](Walrus::ExecutionState& state, void* d) {
                RunData* data = reinterpret_cast<RunData*>(d);
                ALLOCA(uint8_t, functionStackBase, data->mf->requiredStackSize());

                DefinedFunction fakeFunction(data->instance, data->mf);
                ExecutionState newState(state, &fakeFunction);
                auto resultOffset = Interpreter::interpret(newState, functionStackBase);
                data->instance->m_globals[data->index]->setValue(Value(data->type, functionStackBase + resultOffset[0]));
            },
                     &data);
        }

        globIndex++;
    }

    // init table(elem segment)
    instance->m_elementSegments.reserve(m_elements.size());
    for (size_t i = 0; i < m_elements.size(); i++) {
        Element* elem = m_elements[i];
        instance->m_elementSegments[i] = ElementSegment(elem);

        if (elem->mode() == SegmentMode::Active) {
            uint32_t index = 0;
            if (elem->hasModuleFunction()) {
                struct RunData {
                    Element* elem;
                    Instance* instance;
                    Module* module;
                    uint32_t& index;
                } data = { elem, instance, this, index };
                Walrus::Trap trap;
                trap.run([](Walrus::ExecutionState& state, void* d) {
                    RunData* data = reinterpret_cast<RunData*>(d);
                    ALLOCA(uint8_t, functionStackBase, data->elem->moduleFunction()->requiredStackSize());

                    DefinedFunction fakeFunction(data->instance,
                                                 data->elem->moduleFunction());
                    ExecutionState newState(state, &fakeFunction);

                    auto resultOffset = Interpreter::interpret(newState, functionStackBase);
                    Value offset(Value::I32, functionStackBase + resultOffset[0]);
                    data->index = offset.asI32();
                },
                         &data);
            }

            if (UNLIKELY(elem->tableIndex() >= numberOfTableTypes() || index + elem->functionIndex().size() > instance->m_tables[elem->tableIndex()]->size())) {
                Trap::throwException(state, "out of bounds table access");
            }

            const auto& fi = elem->functionIndex();
            Table* table = instance->m_tables[elem->tableIndex()];
            for (size_t i = 0; i < fi.size(); i++) {
                if (fi[i] != std::numeric_limits<uint32_t>::max()) {
                    table->setElement(state, i + index, instance->m_functions[fi[i]]);
                } else {
                    table->setElement(state, i + index, reinterpret_cast<void*>(Value::NullBits));
                }
            }

            instance->m_elementSegments[i].drop();
        } else if (elem->mode() == SegmentMode::Declared) {
            instance->m_elementSegments[i].drop();
        }
    }

    // init memory
    instance->m_dataSegments.reserve(m_datas.size());
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
                ALLOCA(uint8_t, functionStackBase, data->init->moduleFunction()->requiredStackSize());

                DefinedFunction fakeFunction(data->instance,
                                             data->init->moduleFunction());
                ExecutionState newState(state, &fakeFunction);

                auto resultOffset = Interpreter::interpret(newState, functionStackBase);
                Value offset(Value::I32, functionStackBase + resultOffset[0]);

                Memory* m = data->instance->memory(0);
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
    ASSERT(m_datas.size() == instance->m_dataSegments.size());
    ASSERT(m_elements.size() == instance->m_elementSegments.size());
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
    case Value::FuncRef:
        return "FuncRef";
    case Value::ExternRef:
        return "ExternRef";
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
    case Value::FuncRef:
        break;
    case Value::ExternRef:
        break;
    default:
        RELEASE_ASSERT_NOT_REACHED();
    }
    printf(", %s", typeName(v.type()));
}

void ModuleFunction::dumpByteCode()
{
    printf("\n");
    printf("required stack size: %u bytes\n", m_requiredStackSize);
    printf("required stack size due to parameter and local: %u bytes\n", m_requiredStackSizeDueToParameterAndLocal);
    printf("stack: [");
    size_t pos = 0;
    for (size_t i = 0; i < m_functionType->param().size(); i++) {
        printf("%zu(parameter %zu, %s) ", pos, i, typeName(m_functionType->param()[i]));
        pos += valueStackAllocatedSize(m_functionType->param()[i]);
    }
    for (size_t i = 0; i < m_local.size(); i++) {
        printf("%zu(local %zu, %s) ", pos, i, typeName(m_local[i]));
        pos += valueStackAllocatedSize(m_local[i]);
    }
    for (size_t i = 0; i < m_constantDebugData.size(); i++) {
        printf("%zu(constant ", pos);
        dumpValue(m_constantDebugData[i]);
        printf(") ");
        pos += valueStackAllocatedSize(m_constantDebugData[i].type());
    }
    printf("%zu(%" PRIu32 " bytes for general operation)]\n", pos, (m_requiredStackSize - m_requiredStackSizeDueToParameterAndLocal));
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
