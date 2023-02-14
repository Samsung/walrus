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

namespace Walrus {

Instance::Instance(Module* module)
    : m_module(module)
{
    module->store()->appendInstance(this);
}

Optional<ExportType*> Instance::resolveExport(String* name)
{
    for (auto me : m_module->exports()) {
        if (me->name()->equals(name)) {
            return me;
        }
    }

    return nullptr;
}

Optional<Function*> Instance::resolveExportFunction(String* name)
{
    auto me = resolveExport(name);
    if (me && me->exportType() == ExportType::Function) {
        return m_function[me->itemIndex()];
    }

    return nullptr;
}

Optional<Tag*> Instance::resolveExportTag(String* name)
{
    auto me = resolveExport(name);
    if (me && me->exportType() == ExportType::Tag) {
        return m_tag[me->itemIndex()];
    }

    return nullptr;
}

Optional<Table*> Instance::resolveExportTable(String* name)
{
    auto me = resolveExport(name);
    if (me && me->exportType() == ExportType::Table) {
        return m_table[me->itemIndex()];
    }

    return nullptr;
}

Optional<Memory*> Instance::resolveExportMemory(String* name)
{
    auto me = resolveExport(name);
    if (me && me->exportType() == ExportType::Memory) {
        return m_memory[me->itemIndex()];
    }

    return nullptr;
}

Optional<Global*> Instance::resolveExportGlobal(String* name)
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
