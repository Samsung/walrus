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
#include "interpreter/ByteCode.h"

namespace Walrus {

Instance* Module::instantiate(const ValueVector& imports)
{
    Instance* instance = new Instance(this);
    instance->m_function.resize(m_function.size(), nullptr);

    for (size_t i = 0; i < m_import.size(); i++) {
        auto type = m_import[i]->type();

        switch (type) {
        case ModuleImport::Function: {
            instance->m_function[m_import[i]->functionIndex()] = imports[i].asFunction();
            break;
        }
        default: {
            RELEASE_ASSERT_NOT_REACHED();
            break;
        }
        }
    }

    // init defined function
    for (size_t i = 0; i < m_function.size(); i++) {
        auto idx = m_function[i]->functionIndex();
        if (!instance->m_function[idx]) {
            // TODO if there is no function at function(idx), throw exception
            instance->m_function[idx] = new DefinedFunction(m_store, functionType(m_function[i]->functionTypeIndex()), instance, function(idx));
        }
    }

    // init memory
    for (size_t i = 0; i < m_memory.size(); i++) {
        instance->m_memory.pushBack(new Memory(m_memory[i].first * Memory::s_memoryPageSize, m_memory[i].second * Memory::s_memoryPageSize));
    }

    // init table
    for (size_t i = 0; i < m_table.size(); i++) {
        instance->m_table.pushBack(new Table(std::get<0>(m_table[i]), std::get<1>(m_table[i]), std::get<2>(m_table[i])));
    }

    if (m_seenStartAttribute) {
        ASSERT(instance->m_function[m_start]->functionType()->param().size() == 0);
        ASSERT(instance->m_function[m_start]->functionType()->result().size() == 0);
        struct RunData {
            Instance* instance;
            Module* module;
        } data = { instance, this };
        Walrus::Trap trap;
        trap.run([](Walrus::ExecutionState& state, void* d) {
            RunData* data = reinterpret_cast<RunData*>(d);
            data->instance->m_function[data->module->m_start]->call(state, 0, nullptr, nullptr);
        },
                 &data);
    }

    return instance;
}

#if !defined(NDEBUG)
void ModuleFunction::dumpByteCode()
{
    printf("module %p, function index %u, function type index %u\n", m_module, m_functionIndex, m_functionTypeIndex);
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
