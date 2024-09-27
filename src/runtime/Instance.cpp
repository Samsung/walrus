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

#include "Instance.h"
#include "runtime/Store.h"
#include "runtime/Module.h"
#include "runtime/Function.h"
#include "runtime/Table.h"
#include "runtime/Memory.h"
#include "runtime/Global.h"
#include "runtime/Tag.h"

namespace Walrus {

Instance* Instance::newInstance(Module* module)
{
    // Must follow the order in Module::instantiate.

    size_t numberOfRefs = module->numberOfMemoryTypes()
        + Memory::TargetBuffer::sizeInPointers(module->numberOfMemoryTypes())
        + module->numberOfGlobalTypes() + module->numberOfTableTypes()
        + module->numberOfFunctions() + module->numberOfTagTypes();

    void* result = malloc(alignedSize() + numberOfRefs * sizeof(void*));

    // Placement new.
    new (result) Instance(module);

    // Initialize data.
    return reinterpret_cast<Instance*>(result);
}

void Instance::freeInstance(Instance* instance)
{
    instance->~Instance();

    free(reinterpret_cast<void*>(instance));
}

Instance::Instance(Module* module)
    : m_module(module)
    , m_memories(nullptr)
    , m_globals(nullptr)
    , m_tables(nullptr)
    , m_functions(nullptr)
    , m_tags(nullptr)
{
    module->store()->appendInstance(this);
}

Instance::~Instance()
{
    size_t size = m_module->numberOfMemoryTypes();
    Memory::TargetBuffer* targetBuffers = reinterpret_cast<Memory::TargetBuffer*>(alignedEnd() + size);

    for (size_t i = 0; i < size; i++) {
        targetBuffers[i].deque(m_memories[i]);
    }
}

Optional<ExportType*> Instance::resolveExportType(std::string& name)
{
    for (auto me : m_module->exports()) {
        if (me->name() == name) {
            return me;
        }
    }

    return nullptr;
}

Function* Instance::resolveExportFunction(std::string& name)
{
    auto me = resolveExportType(name);
    if (me && me->exportType() == ExportType::Function) {
        return m_functions[me->itemIndex()];
    }

    return nullptr;
}

Tag* Instance::resolveExportTag(std::string& name)
{
    auto me = resolveExportType(name);
    if (me && me->exportType() == ExportType::Tag) {
        return m_tags[me->itemIndex()];
    }

    return nullptr;
}

Table* Instance::resolveExportTable(std::string& name)
{
    auto me = resolveExportType(name);
    if (me && me->exportType() == ExportType::Table) {
        return m_tables[me->itemIndex()];
    }

    return nullptr;
}

Memory* Instance::resolveExportMemory(std::string& name)
{
    auto me = resolveExportType(name);
    if (me && me->exportType() == ExportType::Memory) {
        return m_memories[me->itemIndex()];
    }

    return nullptr;
}

Global* Instance::resolveExportGlobal(std::string& name)
{
    auto me = resolveExportType(name);
    if (me && me->exportType() == ExportType::Global) {
        return m_globals[me->itemIndex()];
    }

    return nullptr;
}

DataSegment::DataSegment(Data* d)
    : m_data(d)
    , m_sizeInByte(m_data->initData().size())
{
}

ElementSegment::ElementSegment(Element* elem)
    : m_element(elem)
{
}

} // namespace Walrus
