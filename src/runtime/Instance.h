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

#ifndef __WalrusInstance__
#define __WalrusInstance__

#include "util/Vector.h"
#include "runtime/Value.h"
#include "runtime/Tag.h"

namespace Walrus {

class Data;
class DataSegment;
class Element;
class ElementSegment;
class Function;
class FunctionType;
class Memory;
class Module;
class ModuleExport;
class Table;

class Instance : public gc {
    friend class Module;
    Instance(Module* module)
        : m_module(module)
    {
    }

public:
    typedef Vector<Instance*, GCUtil::gc_malloc_allocator<Instance*>> InstanceVector;

    Module* module() const { return m_module; }

    Function* function(uint32_t index) const { return m_function[index]; }
    Memory* memory(uint32_t index) const { return m_memory[index]; }
    Table* table(uint32_t index) const { return m_table[index]; }
    Tag* tag(uint32_t index) const { return m_tag[index]; }
    Value& global(uint32_t index) { return m_global[index]; }
    DataSegment& dataSegment(uint32_t index) { return m_dataSegment[index]; }
    ElementSegment& elementSegment(uint32_t index) { return m_elementSegment[index]; }

    Optional<ModuleExport*> resolveExport(String* name);
    Optional<Function*> resolveExportFunction(String* name);
    Optional<Tag*> resolveExportTag(String* name);
    Optional<Table*> resolveExportTable(String* name);

private:
    Module* m_module;

    Vector<Function*, GCUtil::gc_malloc_allocator<Function*>> m_function;
    Vector<Table*, GCUtil::gc_malloc_allocator<Table*>> m_table;
    Vector<Memory*, GCUtil::gc_malloc_allocator<Memory*>> m_memory;
    Vector<DataSegment, GCUtil::gc_malloc_allocator<DataSegment>> m_dataSegment;
    Vector<ElementSegment, GCUtil::gc_malloc_allocator<ElementSegment>> m_elementSegment;
    ValueVector m_global;
    Vector<Tag*, GCUtil::gc_malloc_allocator<Tag*>> m_tag;
};

class DataSegment : public gc {
public:
    DataSegment(Data* data);

    Data* data() const
    {
        return m_data;
    }

    void drop()
    {
        m_sizeInByte = 0;
    }

    size_t sizeInByte() const
    {
        return m_sizeInByte;
    }

private:
    Data* m_data;
    size_t m_sizeInByte;
};

class ElementSegment : public gc {
public:
    ElementSegment(Element* elem);

    Optional<Element*> element() const
    {
        return m_element;
    }

    void drop()
    {
        m_element = nullptr;
    }

private:
    Optional<Element*> m_element;
};

} // namespace Walrus

#endif // __WalrusInstance__
