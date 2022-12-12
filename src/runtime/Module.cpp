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

ModuleFunction::ModuleFunction(Module* module, uint32_t functionTypeIndex)
    : m_module(module)
    , m_functionTypeIndex(functionTypeIndex)
    , m_requiredStackSize(0)
    , m_requiredStackSizeDueToLocal(0)
{
    if (m_functionTypeIndex != g_invalidFunctionTypeIndex) {
        auto ft = module->functionType(m_functionTypeIndex);
        m_requiredStackSize = std::max(ft->paramStackSize(), ft->resultStackSize());
    }
}

Instance* Module::instantiate(ExecutionState& state, const ValueVector& imports)
{
    Instance* instance = new Instance(this);

    instance->m_function.reserve(m_function.size());
    instance->m_table.reserve(m_table.size());
    instance->m_memory.reserve(m_memory.size());
    instance->m_global.reserve(m_global.size());
    instance->m_tag.reserve(m_tag.size());

    size_t importFuncCount = 0;
    size_t importTableCount = 0;
    size_t importMemCount = 0;
    size_t importGlobCount = 0;
    size_t importTagCount = 0;

    for (size_t i = 0; i < m_import.size(); i++) {
        auto type = m_import[i]->type();

        switch (type) {
        case ModuleImport::Function: {
            ASSERT(m_import[i]->functionIndex() == instance->m_function.size());
            instance->m_function.push_back(imports[i].asFunction());
            importFuncCount++;
            break;
        }
        case ModuleImport::Table: {
            ASSERT(m_import[i]->tableIndex() == instance->m_table.size());
            instance->m_table.push_back(imports[i].asTable());
            importTableCount++;
            break;
        }
        case ModuleImport::Memory: {
            ASSERT(m_import[i]->memoryIndex() == instance->m_table.size());
            instance->m_memory.push_back(imports[i].asMemory());
            importMemCount++;
            break;
        }
        case ModuleImport::Global: {
            ASSERT(m_import[i]->globalIndex() == instance->m_global.size());
            instance->m_global.push_back(imports[i].asGlobal());
            importGlobCount++;
            break;
        }
        case ModuleImport::Tag: {
            ASSERT(m_import[i]->tagIndex() == instance->m_tag.size());
            instance->m_tag.push_back(reinterpret_cast<Tag*>(imports[i].asExternal()));
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
    for (size_t i = importFuncCount; i < m_function.size(); i++) {
        ASSERT(i == instance->m_function.size());
        instance->m_function.push_back(new DefinedFunction(m_store, functionType(m_function[i]->functionTypeIndex()), instance, function(i)));
    }

    // init table
    for (size_t i = importTableCount; i < m_table.size(); i++) {
        ASSERT(i == instance->m_table.size());
        instance->m_table.pushBack(new Table(std::get<0>(m_table[i]), std::get<1>(m_table[i]), std::get<2>(m_table[i])));
    }

    // init memory
    for (size_t i = importMemCount; i < m_memory.size(); i++) {
        ASSERT(i == instance->m_memory.size());
        instance->m_memory.pushBack(new Memory(m_memory[i].first * Memory::s_memoryPageSize, m_memory[i].second * Memory::s_memoryPageSize));
    }

    // init global
    for (size_t i = importGlobCount; i < m_global.size(); i++) {
        ASSERT(i == instance->m_global.size());
        instance->m_global.pushBack(new Global(Value(std::get<0>(m_global[i]))));
    }

    // init tag
    for (size_t i = importTagCount; i < m_tag.size(); i++) {
        ASSERT(i == instance->m_tag.size());
        instance->m_tag.push_back(new Tag(functionType(m_tag[i])));
    }

    // init global
    if (m_globalInitBlock) {
        struct RunData {
            Instance* instance;
            Module* module;
        } data = { instance, this };
        Walrus::Trap trap;
        trap.run([](Walrus::ExecutionState& state, void* d) {
            RunData* data = reinterpret_cast<RunData*>(d);
            uint8_t* functionStackBase = ALLOCA(data->module->m_globalInitBlock->requiredStackSize(), uint8_t);
            uint8_t* functionStackPointer = functionStackBase;

            DefinedFunction fakeFunction(data->module->m_store, nullptr, data->instance,
                                         data->module->m_globalInitBlock.value());
            ExecutionState newState(state, &fakeFunction);

            Interpreter::interpret(newState, functionStackBase, functionStackPointer);
        },
                 &data);
    }

    // init table(elem segment)
    instance->m_elementSegment.reserve(m_element.size());
    for (auto elem : m_element) {
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
                    uint8_t* functionStackPointer = functionStackBase;

                    DefinedFunction fakeFunction(data->module->m_store, nullptr, data->instance,
                                                 data->elem->moduleFunction());
                    ExecutionState newState(state, &fakeFunction);

                    Interpreter::interpret(newState, functionStackBase, functionStackPointer);

                    functionStackPointer = functionStackPointer - valueSizeInStack(Value::I32);
                    uint8_t* resultStackPointer = functionStackPointer;
                    Value offset(Value::I32, resultStackPointer);
                    data->index = offset.asI32();
                },
                         &data);
            }

            if (UNLIKELY(elem->tableIndex() >= instance->m_table.size() || index >= instance->m_table[elem->tableIndex()]->size() || index + elem->functionIndex().size() > instance->m_table[elem->tableIndex()]->size())) {
                Trap::throwException("out of bounds table access");
            }

            const auto& fi = elem->functionIndex();
            Table* table = instance->m_table[elem->tableIndex()];
            for (size_t i = 0; i < fi.size(); i++) {
                if (fi[i] != std::numeric_limits<uint32_t>::max()) {
                    table->setElement(i + index, Value(instance->m_function[fi[i]]));
                } else {
                    table->setElement(i + index, Value(Value::FuncRef, Value::Null));
                }
            }

            instance->m_elementSegment.back().drop();
        } else if (elem->mode() == SegmentMode::Declared) {
            instance->m_elementSegment.back().drop();
        }
    }

    // init memory
    instance->m_dataSegment.reserve(m_data.size());
    for (auto init : m_data) {
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
                uint8_t* functionStackPointer = functionStackBase;

                DefinedFunction fakeFunction(data->module->m_store, nullptr, data->instance,
                                             data->init->moduleFunction());
                ExecutionState newState(state, &fakeFunction);

                Interpreter::interpret(newState, functionStackBase, functionStackPointer);

                functionStackPointer = functionStackPointer - valueSizeInStack(Value::I32);
                uint8_t* resultStackPointer = functionStackPointer;
                Value offset(Value::I32, resultStackPointer);
                Memory* m = data->instance->memory(0);
                const auto& initData = data->init->initData();
                if (m->sizeInByte() >= initData.size() && (offset.asI32() + initData.size()) <= m->sizeInByte() && offset.asI32() >= 0) {
                    memcpyEndianAware(m->buffer(), initData.data(), m->sizeInByte(), initData.size(), offset.asI32(), 0, initData.size());
                } else {
                    Trap::throwException("out of bounds memory access");
                }
            }
        },
                               &data);

        if (result.exception) {
            Trap::throwException(std::move(result.exception));
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
    printf("module %p, function type index %u\n", m_module, m_functionTypeIndex);
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
