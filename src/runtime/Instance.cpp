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

Instance::Instance(Module* module)
    : m_module(module)
{
    module->store()->appendInstance(this);
}

Instance::~Instance()
{
    for (size_t i = 0; i < m_function.size(); i++) {
        delete m_function[i];
    }

    for (size_t i = 0; i < m_table.size(); i++) {
        delete m_table[i];
    }

    for (size_t i = 0; i < m_memory.size(); i++) {
        delete m_memory[i];
    }

    for (size_t i = 0; i < m_global.size(); i++) {
        delete m_global[i];
    }

    for (size_t i = 0; i < m_tag.size(); i++) {
        delete m_tag[i];
    }
}

Optional<ExportType*> Instance::resolveExport(std::string& name)
{
    for (auto me : m_module->exports()) {
        if (me->name() == name) {
            return me;
        }
    }

    return nullptr;
}

Optional<Function*> Instance::resolveExportFunction(std::string& name)
{
    auto me = resolveExport(name);
    if (me && me->exportType() == ExportType::Function) {
        return m_function[me->itemIndex()];
    }

    return nullptr;
}

Optional<Tag*> Instance::resolveExportTag(std::string& name)
{
    auto me = resolveExport(name);
    if (me && me->exportType() == ExportType::Tag) {
        return m_tag[me->itemIndex()];
    }

    return nullptr;
}

Optional<Table*> Instance::resolveExportTable(std::string& name)
{
    auto me = resolveExport(name);
    if (me && me->exportType() == ExportType::Table) {
        return m_table[me->itemIndex()];
    }

    return nullptr;
}

Optional<Memory*> Instance::resolveExportMemory(std::string& name)
{
    auto me = resolveExport(name);
    if (me && me->exportType() == ExportType::Memory) {
        return m_memory[me->itemIndex()];
    }

    return nullptr;
}

Optional<Global*> Instance::resolveExportGlobal(std::string& name)
{
    auto me = resolveExport(name);
    if (me && me->exportType() == ExportType::Global) {
        return m_global[me->itemIndex()];
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
