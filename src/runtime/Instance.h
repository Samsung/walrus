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

#include "runtime/Object.h"

namespace Walrus {

class Store;
class Data;
class Element;
class Function;
class FunctionType;
class Memory;
class Global;
class Module;
class ExportType;
class Table;

class DataSegment {
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

class ElementSegment {
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

class Instance : public Object {
    friend class Module;
    friend class Interpreter;

public:
    typedef Vector<Instance*, std::allocator<Instance*>> InstanceVector;

    Instance(Module* module);
    ~Instance();

    virtual Object::Kind kind() const override
    {
        return Object::InstanceKind;
    }

    virtual bool isInstance() const override
    {
        return true;
    }

    Function* function(uint32_t index) const { return m_functions[index].get(); }
    Memory* memory(uint32_t index) const
    {
        // now only one memory is allowed for each Instance/Module
        ASSERT(index == 0);
        return m_memories[index].get();
    }
    Table* table(uint32_t index) const { return m_tables[index].get(); }
    Tag* tag(uint32_t index) const { return m_tags[index].get(); }
    Global* global(uint32_t index) const { return m_globals[index].get(); }

    DataSegment& dataSegment(uint32_t index) { return m_dataSegments[index]; }
    ElementSegment& elementSegment(uint32_t index) { return m_elementSegments[index]; }

    Optional<ExportType*> resolveExportType(std::string& name);

    std::shared_ptr<Function> resolveExportFunction(std::string& name);
    std::shared_ptr<Tag> resolveExportTag(std::string& name);
    std::shared_ptr<Table> resolveExportTable(std::string& name);
    std::shared_ptr<Memory> resolveExportMemory(std::string& name);
    std::shared_ptr<Global> resolveExportGlobal(std::string& name);

private:
    Module* m_module;

    std::vector<std::shared_ptr<Function>> m_functions;
    std::vector<std::shared_ptr<Table>> m_tables;
    std::vector<std::shared_ptr<Memory>> m_memories;
    std::vector<std::shared_ptr<Global>> m_globals;
    std::vector<std::shared_ptr<Tag>> m_tags;

    Vector<DataSegment, std::allocator<DataSegment>> m_dataSegments;
    Vector<ElementSegment, std::allocator<ElementSegment>> m_elementSegments;
};
} // namespace Walrus

#endif // __WalrusInstance__
