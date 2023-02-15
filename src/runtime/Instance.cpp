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
    /*
    for (size_t i = 0; i < m_functions.size(); i++) {
        delete m_functions[i];
    }

    for (size_t i = 0; i < m_tables.size(); i++) {
        delete m_tables[i];
    }

    for (size_t i = 0; i < m_memories.size(); i++) {
        delete m_memories[i];
    }

    for (size_t i = 0; i < m_globals.size(); i++) {
        delete m_globals[i];
    }

    for (size_t i = 0; i < m_tags.size(); i++) {
        delete m_tags[i];
    }
    */
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

std::shared_ptr<Function> Instance::resolveExportFunction(std::string& name)
{
    auto me = resolveExportType(name);
    if (me && me->exportType() == ExportType::Function) {
        return m_functions[me->itemIndex()];
    }

    return std::shared_ptr<Function>();
}

std::shared_ptr<Tag> Instance::resolveExportTag(std::string& name)
{
    auto me = resolveExportType(name);
    if (me && me->exportType() == ExportType::Tag) {
        return m_tags[me->itemIndex()];
    }

    return std::shared_ptr<Tag>();
}

std::shared_ptr<Table> Instance::resolveExportTable(std::string& name)
{
    auto me = resolveExportType(name);
    if (me && me->exportType() == ExportType::Table) {
        return m_tables[me->itemIndex()];
    }

    return std::shared_ptr<Table>();
}

std::shared_ptr<Memory> Instance::resolveExportMemory(std::string& name)
{
    auto me = resolveExportType(name);
    if (me && me->exportType() == ExportType::Memory) {
        return m_memories[me->itemIndex()];
    }

    return std::shared_ptr<Memory>();
}

std::shared_ptr<Global> Instance::resolveExportGlobal(std::string& name)
{
    auto me = resolveExportType(name);
    if (me && me->exportType() == ExportType::Global) {
        return m_globals[me->itemIndex()];
    }

    return std::shared_ptr<Global>();
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
