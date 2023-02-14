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

#include "runtime/Module.h"
#include "runtime/Function.h"
#include "runtime/Instance.h"
#include "runtime/Store.h"
#include "runtime/Trap.h"
#include "runtime/Memory.h"
#include "runtime/Table.h"
#include "runtime/Global.h"
#include "interpreter/ByteCode.h"
#include "interpreter/Interpreter.h"

namespace Walrus {

ModuleFunction::ModuleFunction(FunctionType* functionType)
    : m_functionType(functionType)
    , m_requiredStackSize(std::max(m_functionType->paramStackSize(), m_functionType->resultStackSize()))
    , m_requiredStackSizeDueToLocal(0)
{
}

Module::Module(Store* store)
    : m_store(store)
    , m_seenStartAttribute(false)
    , m_version(0)
    , m_start(0)
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

    for (size_t i = 0; i < m_tableTypes.size(); i++) {
        delete m_tableTypes[i];
    }

    for (size_t i = 0; i < m_memoryTypes.size(); i++) {
        delete m_memoryTypes[i];
    }

    for (size_t i = 0; i < m_tagTypes.size(); i++) {
        delete m_tagTypes[i];
    }

    for (size_t i = 0; i < m_globalInfos.size(); i++) {
        if (m_globalInfos[i].second) {
            delete m_globalInfos[i].second.value();
            m_globalInfos[i].second.reset();
        }
    }
}

FunctionType* Module::initIndexFunctionType()
{
    return initGlobalFunctionType(Value::I32);
}

FunctionType* Module::initGlobalFunctionType(Value::Type type)
{
    static FunctionType* info[Value::Type::Void];
    switch (type) {
    case Value::Type::I32:
        if (!info[type])
            info[type] = new (NoGC) FunctionType(new ValueTypeVector(), new ValueTypeVector({ Value::I32 }));
        break;
    case Value::Type::I64:
        if (!info[type])
            info[type] = new (NoGC) FunctionType(new ValueTypeVector(), new ValueTypeVector({ Value::I64 }));
        break;
    case Value::Type::F32:
        if (!info[type])
            info[type] = new (NoGC) FunctionType(new ValueTypeVector(), new ValueTypeVector({ Value::F32 }));
        break;
    case Value::Type::F64:
        if (!info[type])
            info[type] = new (NoGC) FunctionType(new ValueTypeVector(), new ValueTypeVector({ Value::F64 }));
        break;
    case Value::Type::V128:
        if (!info[type])
            info[type] = new (NoGC) FunctionType(new ValueTypeVector(), new ValueTypeVector({ Value::V128 }));
        break;
    case Value::Type::FuncRef:
        if (!info[type])
            info[type] = new (NoGC) FunctionType(new ValueTypeVector(), new ValueTypeVector({ Value::FuncRef }));
        break;
    case Value::Type::ExternRef:
        if (!info[type])
            info[type] = new (NoGC) FunctionType(new ValueTypeVector(), new ValueTypeVector({ Value::ExternRef }));
        break;
    default:
        RELEASE_ASSERT_NOT_REACHED();
    }
    return info[type];
}

Instance* Module::instantiate(ExecutionState& state, const ObjectVector& imports)
{
    Instance* instance = new Instance(this);

    instance->m_function.reserve(m_functions.size());
    instance->m_table.reserve(m_tableTypes.size());
    instance->m_memory.reserve(m_memoryTypes.size());
    instance->m_global.reserve(m_globalInfos.size());
    instance->m_tag.reserve(m_tagTypes.size());

    size_t importFuncCount = 0;
    size_t importTableCount = 0;
    size_t importMemCount = 0;
    size_t importGlobCount = 0;
    size_t importTagCount = 0;

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
            instance->m_function.push_back(imports[i]->asFunction());
            importFuncCount++;
            break;
        }
        case ImportType::Table: {
            if (imports[i]->kind() != Object::TableKind
                || m_imports[i]->tableType()->type() != imports[i]->asTable()->type()
                || m_imports[i]->tableType()->initialSize() > imports[i]->asTable()->size()) {
                Trap::throwException(state, "incompatible import type");
            }

            if (m_imports[i]->tableType()->maximumSize() != std::numeric_limits<uint32_t>::max()) {
                if (imports[i]->asTable()->maximumSize() == std::numeric_limits<uint32_t>::max()
                    || imports[i]->asTable()->maximumSize() > m_imports[i]->tableType()->maximumSize())
                    Trap::throwException(state, "incompatible import type");
            }
            instance->m_table.push_back(imports[i]->asTable());
            importTableCount++;
            break;
        }
        case ImportType::Memory: {
            if (imports[i]->kind() != Object::MemoryKind
                || m_imports[i]->memoryType()->initialSize() > imports[i]->asMemory()->sizeInPageSize()) {
                Trap::throwException(state, "incompatible import type");
            }

            if (m_imports[i]->memoryType()->maximumSize() != std::numeric_limits<uint32_t>::max()) {
                if (imports[i]->asMemory()->maximumSizeInPageSize() == std::numeric_limits<uint32_t>::max()
                    || imports[i]->asMemory()->maximumSizeInPageSize() > m_imports[i]->memoryType()->maximumSize())
                    Trap::throwException(state, "incompatible import type");
            }
            instance->m_memory.push_back(imports[i]->asMemory());
            importMemCount++;
            break;
        }
        case ImportType::Global: {
            if (imports[i]->kind() != Object::GlobalKind) {
                Trap::throwException(state, "incompatible import type");
            }
            instance->m_global.push_back(imports[i]->asGlobal());
            importGlobCount++;
            break;
        }
        case ImportType::Tag: {
            if (imports[i]->kind() != Object::TagKind) {
                Trap::throwException(state, "incompatible import type");
            }
            instance->m_tag.push_back(reinterpret_cast<Tag*>(imports[i]->asTag()));
            importTagCount++;
            break;
        }
        default: {
            RELEASE_ASSERT_NOT_REACHED();
            break;
        }
        }
    }

    // init defined function
    for (size_t i = importFuncCount; i < m_functions.size(); i++) {
        ASSERT(i == instance->m_function.size());
        instance->m_function.push_back(new DefinedFunction(instance, function(i)));
    }

    // init table
    for (size_t i = importTableCount; i < m_tableTypes.size(); i++) {
        ASSERT(i == instance->m_table.size());
        instance->m_table.pushBack(new Table(m_tableTypes[i]->type(), m_tableTypes[i]->initialSize(), m_tableTypes[i]->maximumSize()));
    }

    // init memory
    for (size_t i = importMemCount; i < m_memoryTypes.size(); i++) {
        ASSERT(i == instance->m_memory.size());
        instance->m_memory.pushBack(new Memory(m_memoryTypes[i]->initialSize() * Memory::s_memoryPageSize, m_memoryTypes[i]->maximumSize() * Memory::s_memoryPageSize));
    }

    // init tag
    for (size_t i = importTagCount; i < m_tagTypes.size(); i++) {
        ASSERT(i == instance->m_tag.size());
        instance->m_tag.push_back(new Tag(m_functionTypes[m_tagTypes[i]->sigIndex()]));
    }

    // init global
    for (size_t i = importGlobCount; i < m_globalInfos.size(); i++) {
        ASSERT(i == instance->m_global.size());
        auto& globalData = m_globalInfos[i];
        instance->m_global.pushBack(new Global(Value(globalData.first.type())));

        if (globalData.second) {
            struct RunData {
                Instance* instance;
                Module* module;
                Value::Type type;
                ModuleFunction* mf;
            } data = { instance, this, globalData.first.type(), globalData.second.value() };
            Walrus::Trap trap;
            trap.run([](Walrus::ExecutionState& state, void* d) {
                RunData* data = reinterpret_cast<RunData*>(d);
                uint8_t* functionStackBase = ALLOCA(data->mf->requiredStackSize(), uint8_t);

                DefinedFunction fakeFunction(data->instance, data->mf);
                ExecutionState newState(state, &fakeFunction);
                auto resultOffset = Interpreter::interpret(newState, functionStackBase);
                data->instance->m_global.back()->setValue(Value(data->type, functionStackBase + resultOffset[0]));
            },
                     &data);
        }
    }

    // init table(elem segment)
    instance->m_elementSegment.reserve(m_elements.size());
    for (auto elem : m_elements) {
        instance->m_elementSegment.pushBack(ElementSegment(elem));

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
                    uint8_t* functionStackBase = ALLOCA(data->elem->moduleFunction()->requiredStackSize(), uint8_t);

                    DefinedFunction fakeFunction(data->instance,
                                                 data->elem->moduleFunction());
                    ExecutionState newState(state, &fakeFunction);

                    auto resultOffset = Interpreter::interpret(newState, functionStackBase);
                    Value offset(Value::I32, functionStackBase + resultOffset[0]);
                    data->index = offset.asI32();
                },
                         &data);
            }

            if (UNLIKELY(elem->tableIndex() >= instance->m_table.size() || index >= instance->m_table[elem->tableIndex()]->size() || index + elem->functionIndex().size() > instance->m_table[elem->tableIndex()]->size())) {
                Trap::throwException(state, "out of bounds table access");
            }

            const auto& fi = elem->functionIndex();
            Table* table = instance->m_table[elem->tableIndex()];
            for (size_t i = 0; i < fi.size(); i++) {
                if (fi[i] != std::numeric_limits<uint32_t>::max()) {
                    table->setElement(state, i + index, instance->m_function[fi[i]]);
                } else {
                    table->setElement(state, i + index, reinterpret_cast<void*>(Value::NullBits));
                }
            }

            instance->m_elementSegment.back().drop();
        } else if (elem->mode() == SegmentMode::Declared) {
            instance->m_elementSegment.back().drop();
        }
    }

    // init memory
    instance->m_dataSegment.reserve(m_datas.size());
    for (auto init : m_datas) {
        instance->m_dataSegment.pushBack(DataSegment(init));
        struct RunData {
            Data* init;
            Instance* instance;
            Module* module;
        } data = { init, instance, this };
        Walrus::Trap trap;
        auto result = trap.run([](Walrus::ExecutionState& state, void* d) {
            RunData* data = reinterpret_cast<RunData*>(d);
            if (data->init->moduleFunction()->currentByteCodeSize()) {
                uint8_t* functionStackBase = ALLOCA(data->init->moduleFunction()->requiredStackSize(), uint8_t);

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

    if (m_seenStartAttribute) {
        ASSERT(instance->m_function[m_start]->functionType()->param().size() == 0);
        ASSERT(instance->m_function[m_start]->functionType()->result().size() == 0);
        instance->m_function[m_start]->call(state, 0, nullptr, nullptr);
    }

    return instance;
}

#if !defined(NDEBUG)
void ModuleFunction::dumpByteCode()
{
    printf("function type %p\n", m_functionType);
    printf("requiredStackSize %u, requiredStackSizeDueToLocal %u\n", m_requiredStackSize, m_requiredStackSizeDueToLocal);

    size_t idx = 0;
    while (idx < m_byteCode.size()) {
        ByteCode* code = reinterpret_cast<ByteCode*>(&m_byteCode[idx]);
        printf("%zu: ", idx);
        printf("%s ", g_byteCodeInfo[code->opcode()].m_name);
        code->dump(idx);
        printf("\n");
        idx += code->byteCodeSize();
    }
}
#endif

} // namespace Walrus
