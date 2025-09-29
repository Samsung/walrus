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

#ifdef ENABLE_GC
#include "GCUtil.h"
#endif /* ENABLE_GC */

namespace Walrus {

DEFINE_GLOBAL_TYPE_INFO(instanceTypeInfo, InstanceKind);

ElementSegment::ElementSegment(size_t size)
    : m_size(size)
{
    if (size == 0) {
        m_elements = nullptr;
        return;
    }

    // The references are initialized during instantiation.
#ifdef ENABLE_GC
    m_elements = reinterpret_cast<void**>(GC_MALLOC_UNCOLLECTABLE(size * sizeof(void*)));
#else
    m_elements = reinterpret_cast<void**>(malloc(size * sizeof(void*)));
#endif
}

void ElementSegment::drop()
{
    if (m_elements != nullptr) {
#ifdef ENABLE_GC
        GC_FREE(m_elements);
#else
        free(m_elements);
#endif
        m_elements = nullptr;
        m_size = 0;
    }
}

Instance* Instance::newInstance(Module* module)
{
    // Must follow the order in Module::instantiate.

    COMPILE_ASSERT((sizeof(Memory::TargetBuffer) % sizeof(void*)) == 0, "TargetBuffer must be pointer aligned");
    COMPILE_ASSERT((sizeof(DataSegment) % sizeof(void*)) == 0, "DataSegment must be pointer aligned");
    COMPILE_ASSERT((sizeof(ElementSegment) % sizeof(void*)) == 0, "ElementSegment must be pointer aligned");

    size_t numberOfRefs = module->numberOfMemoryTypes()
        + module->numberOfGlobalTypes() + module->numberOfTableTypes()
        + module->numberOfFunctions() + module->numberOfTagTypes();

    size_t totalSize = numberOfRefs * sizeof(void*)
        + module->numberOfMemoryTypes() * sizeof(Memory::TargetBuffer)
        + module->numberOfDataSegments() * sizeof(DataSegment)
        + module->numberOfElemSegments() * sizeof(ElementSegment);

    void* result = malloc(alignedSize() + totalSize);

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
    : Object(GET_GLOBAL_TYPE_INFO(instanceTypeInfo))
    , m_module(module)
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

    size = m_module->numberOfElemSegments();
    for (size_t i = 0; i < size; i++) {
        m_elementSegments[i].drop();
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
    : m_data(d->initData().data())
    , m_sizeInByte(d->initData().size())
{
}

} // namespace Walrus
