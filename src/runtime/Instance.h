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

    static Instance* newInstance(Module* module);
    static void freeInstance(Instance* instance);

    virtual Object::Kind kind() const override
    {
        return Object::InstanceKind;
    }

    virtual bool isInstance() const override
    {
        return true;
    }

    Module* module() const { return m_module; }

    Function* function(uint32_t index) const { return m_functions[index]; }
    Memory* memory(uint32_t index) const
    {
        // now only one memory is allowed for each Instance/Module
        ASSERT(index == 0);
        return m_memories[index];
    }
    Table* table(uint32_t index) const { return m_tables[index]; }
    Tag* tag(uint32_t index) const { return m_tags[index]; }
    Global* global(uint32_t index) const { return m_globals[index]; }

    DataSegment& dataSegment(uint32_t index) { return m_dataSegments[index]; }
    ElementSegment& elementSegment(uint32_t index) { return m_elementSegments[index]; }

    Optional<ExportType*> resolveExportType(std::string& name);

    Function* resolveExportFunction(std::string& name);
    Tag* resolveExportTag(std::string& name);
    Table* resolveExportTable(std::string& name);
    Memory* resolveExportMemory(std::string& name);
    Global* resolveExportGlobal(std::string& name);

    const Function* const* functions() { return m_functions; }

private:
    Instance(Module* module);
    ~Instance() {}

    static size_t alignedSize()
    {
        return (sizeof(Instance) + sizeof(void*) - 1) & ~(sizeof(void*) - 1);
    }

    Module* m_module;

    // The initialization in Module::instantiate and Instance::newInstance must follow this order.
    // Ordered in use frequency order.
    Memory** m_memories;
    Global** m_globals;
    Table** m_tables;
    Function** m_functions;
    Tag** m_tags;

    VectorWithFixedSize<DataSegment, std::allocator<DataSegment>> m_dataSegments;
    VectorWithFixedSize<ElementSegment, std::allocator<ElementSegment>> m_elementSegments;
};
} // namespace Walrus

#endif // __WalrusInstance__
