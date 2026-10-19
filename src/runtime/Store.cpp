/*
 * Copyright (c) 2023-present Samsung Electronics Co., Ltd
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

#include "runtime/Store.h"
#include "runtime/Module.h"
#include "runtime/Instance.h"
#include "runtime/Component.h"
#include "runtime/ComponentInstance.h"
#include "runtime/Exception.h"
#include "runtime/ObjectType.h"
#include "runtime/Function.h"
#include "runtime/Table.h"
#include "runtime/Global.h"
#include "runtime/Memory.h"
#include "runtime/Tag.h"
#include "runtime/GCArray.h"
#include "runtime/GCException.h"
#include <algorithm>
#include <unordered_map>
#include <unordered_set>

#ifdef ENABLE_GC
#include "GCUtil.h"
#endif /* ENABLE_GC */

namespace Walrus {

#ifndef NDEBUG
size_t Extern::g_externCount;
size_t Exception::g_exceptionCount;
#endif

static const FunctionType g_defaultFunctionTypes[] = {
#define DEFINE_RESULT_TYPE(name) FunctionType(Value::name),
    FOR_EACH_VALUE_TYPE(DEFINE_RESULT_TYPE)
#undef DEFINE_RESULT_TYPE
};

#ifdef ENABLE_GC
static NEVER_INLINE void clearStack()
{
    void* data[1024];
    memset(data, 0, sizeof(data));
    // Without this call, Clang does not perfrom the memset operation.
    GCBase::stackArg(data);
}
#endif /* ENABLE_GC */

// C API stores and calls are confined to their owning thread.
static thread_local std::vector<Store*> s_apiStores;
static thread_local size_t s_collectionDepth;
static thread_local bool s_collecting;
struct APIGCReference {
    size_t count = 0;
#ifdef ENABLE_GC
    const Object** root = nullptr;
#endif
};
static thread_local std::unordered_map<const Object*, APIGCReference> s_apiGCReferences;

Store::Store(Engine* engine)
    : m_engine(engine)
#ifdef ENABLE_WASI
    , m_wasiData(nullptr)
#endif
{
    memset(m_definedFuncTypes, 0, sizeof(m_definedFuncTypes));
#ifdef ENABLE_GC
    GC_INIT();
#endif /* ENABLE_GC */
}

Store::~Store()
{
    // Invalidate wrappers before freeing objects or invoking GC finalizers.
    *m_apiLifetime = false;
    if (m_apiCollection) {
        auto it = std::find(s_apiStores.begin(), s_apiStores.end(), this);
        ASSERT(it != s_apiStores.end());
        s_apiStores.erase(it);
    }
    for (size_t i = 0; i < FUNC_TYPES_NUM; i++) {
        FunctionType* type = m_definedFuncTypes[i];
        if (type != nullptr) {
            TypeStore::ReleaseRef(type->subTypeList());
        }
    }

    // deallocate Modules and Instances
    for (size_t i = 0; i < m_instances.size(); i++) {
        Instance::freeInstance(m_instances[i]);
    }

    for (size_t i = 0; i < m_modules.size(); i++) {
        getTypeStore().releaseTypes(m_modules[i]->m_compositeTypes);
        delete m_modules[i];
    }

    for (size_t i = 0; i < m_componentInstances.size(); i++) {
        delete m_componentInstances[i];
    }

    for (size_t i = 0; i < m_components.size(); i++) {
        delete m_components[i];
    }

    for (size_t i = 0; i < m_externs.size(); i++) {
        delete m_externs[i];
    }

    for (size_t i = 0; i < m_waiterList.size(); i++) {
        delete m_waiterList[i];
    }

    Store::finalize();

#ifdef ENABLE_GC
    clearStack();
    GC_gcollect_and_unmap();
    GC_invoke_finalizers();
#endif /* ENABLE_GC */

#ifndef NDEBUG
    // Check if all Extern objects and excetions have been deallocated.
    ASSERT(Extern::g_externCount == 0);
    ASSERT(Exception::g_exceptionCount == 0);
#endif
}


void Store::appendModule(Module* module)
{
    module->setOwnerStore(this);
    module->m_storeIndex = m_modules.size();
    m_modules.push_back(module);
    m_referenceGraphDirty = true;
}

void Store::appendInstance(Instance* instance)
{
    instance->setOwnerStore(this);
    instance->m_storeIndex = m_instances.size();
    m_instances.push_back(instance);
    m_referenceGraphDirty = true;
}

void Store::appendExtern(Extern* ext)
{
    ext->setOwnerStore(this);
    ext->m_storeIndex = m_externs.size();
    ext->m_refCount = 0;
    m_externs.push_back(ext);
    m_referenceGraphDirty = true;
}

Store::CollectionScope::CollectionScope()
{
    ++s_collectionDepth;
}

Store::CollectionScope::~CollectionScope()
{
    ASSERT(s_collectionDepth);
    if (--s_collectionDepth == 0) {
        collectPending();
    }
}

void Store::enableAPICollection()
{
    ASSERT(!m_apiCollection);
    m_apiCollection = true;
    s_apiStores.push_back(this);
}

void Store::requestCollection(bool referencesChanged)
{
    if (m_apiCollection) {
        m_referenceGraphDirty |= referencesChanged;
        m_collectionPending = true;
        collectPending();
    }
}

void Store::collectPending()
{
    if (s_collectionDepth || s_collecting) {
        return;
    }
    s_collecting = true;
    for (Store* store : s_apiStores) {
        if (store->m_collectionPending) {
            store->m_collectionPending = false;
            store->collectUnreferenced();
        }
    }
    s_collecting = false;
}

static StoreObject* apiStoreObject(const Object* object)
{
    if (!object || (reinterpret_cast<uintptr_t>(object) & 1)) {
        return nullptr;
    }
    switch (object->kind()) {
    case Object::ModuleKind:
    case Object::InstanceKind:
    case Object::FunctionKind:
    case Object::MemoryKind:
    case Object::TagKind:
    case Object::ForeignKind:
        return static_cast<StoreObject*>(const_cast<Object*>(object));
    default:
        return nullptr;
    }
}

static bool isGCReference(const Object* object)
{
    return object && !(reinterpret_cast<uintptr_t>(object) & 1)
        && (object->kind() == Object::StructKind || object->kind() == Object::ArrayKind
            || object->kind() == Object::ExceptionKind);
}

std::shared_ptr<bool> Store::addAPIReference(const Object* object)
{
    if (StoreObject* owned = apiStoreObject(object)) {
        ++owned->m_refCount;
        return owned->ownerStore()->m_apiLifetime;
    } else if (isGCReference(object)) {
        auto& reference = s_apiGCReferences[object];
#ifdef ENABLE_GC
        if (!reference.count) {
            reference.root = static_cast<const Object**>(GC_MALLOC_UNCOLLECTABLE(sizeof(Object*)));
            *reference.root = object;
        }
#endif
        if (!reference.count) {
            // GC objects are not StoreObjects. A new API root can introduce
            // reachability through their fields into any API store.
            for (Store* store : s_apiStores) {
                store->m_referenceGraphDirty = true;
            }
        }
        ++reference.count;
    }
    return nullptr;
}

void Store::removeAPIReference(const Object* object)
{
    if (StoreObject* owned = apiStoreObject(object)) {
        ASSERT(owned->m_refCount);
        if (--owned->m_refCount == 0) {
            Store* store = owned->ownerStore();
            if (store->m_apiCollection) {
                store->m_collectionCandidates.insert(owned);
                store->requestCollection(false);
            }
        }
    } else if (isGCReference(object)) {
        auto it = s_apiGCReferences.find(object);
        ASSERT(it != s_apiGCReferences.end());
        if (--it->second.count == 0) {
#ifdef ENABLE_GC
            GC_FREE(it->second.root);
#endif
            s_apiGCReferences.erase(it);
            CollectionScope scope;
            for (Store* store : s_apiStores) {
                store->requestCollection();
            }
        }
    }
}

// Expand GC fields into StoreObject edges. The cache contains no GC object
// pointers, so it does not keep otherwise unreachable GC objects alive.
void Store::traceReferences(Object* source, std::vector<StoreObject*>& targets)
{
    std::vector<Object*> gcStack;
    std::unordered_set<Object*> visited;
    std::unordered_set<StoreObject*> distinct;
    auto mark = [&](StoreObject* object) {
        if (!object || object->ownerStore() != this) {
            return;
        }
        // Most objects have only a handful of edges. Avoid a hash allocation
        // for those, but keep large tables with distinct targets linear-time.
        if (targets.size() < 8) {
            if (std::find(targets.begin(), targets.end(), object) == targets.end()) {
                targets.push_back(object);
            }
        } else {
            if (distinct.empty()) {
                distinct.insert(targets.begin(), targets.end());
            }
            if (distinct.insert(object).second) {
                targets.push_back(object);
            }
        }
    };
    auto markReference = [&](void* reference) {
        if (Value::isNull(reference) || (reinterpret_cast<uintptr_t>(reference) & 1)) {
            return;
        }
        Object* object = static_cast<Object*>(reference);
        if (StoreObject* owned = apiStoreObject(object)) {
            mark(owned);
        } else if (isGCReference(object) && visited.insert(object).second) {
            gcStack.push_back(object);
        }
    };
    if (isGCReference(source)) {
        markReference(source);
    } else {
        Object* object = source;
        switch (object->kind()) {
        case Object::InstanceKind: {
            Instance* instance = object->asInstance();
            Module* module = instance->module();
            mark(module);
            for (size_t i = 0; i < module->numberOfMemoryTypes(); ++i) {
                mark(instance->memory(i));
            }
            for (size_t i = 0; i < module->numberOfGlobalTypes(); ++i) {
                mark(instance->global(i));
            }
            for (size_t i = 0; i < module->numberOfTableTypes(); ++i) {
                mark(instance->table(i));
            }
            for (size_t i = 0; i < module->numberOfFunctions(); ++i) {
                mark(instance->function(i));
            }
            for (size_t i = 0; i < module->numberOfTagTypes(); ++i) {
                mark(instance->tag(i));
            }
            for (size_t i = 0; i < module->numberOfElemSegments(); ++i) {
                ElementSegment* segment = instance->elementSegment(i);
                for (size_t j = 0; j < segment->size(); ++j) {
                    markReference(segment->element(j));
                }
            }
            break;
        }
        case Object::FunctionKind: {
            Function* function = object->asFunction();
            if (function->kind() == Function::DefinedFunctionKind) {
                mark(function->asDefinedFunction()->instance());
            }
            break;
        }
        case Object::TableKind: {
            Table* table = object->asTable();
#ifdef WALRUS_API_TESTS
            m_tableSlotCount += table->size();
#endif
            for (uint64_t i = 0; i < table->size(); ++i) {
                void* reference = table->is64() ? table->uncheckedGetElementM64(i) : table->uncheckedGetElement(static_cast<uint32_t>(i));
                // Keep the common empty-slot path outside the general tracer.
                if (!Value::isNull(reference)) {
                    markReference(reference);
                }
            }
            break;
        }
        case Object::GlobalKind: {
            const Value& value = object->asGlobal()->value();
            if (Value::isRefType(value.type())) {
                markReference(value.asReference());
            }
            break;
        }
        default:
            break;
        }
    }
    while (!gcStack.empty()) {
        Object* object = gcStack.back();
        gcStack.pop_back();
        if (object->kind() == Object::StructKind) {
            const StructType* type = static_cast<const StructType*>(object->objectTypeInfo());
            const auto& fields = type->fields().types();
            for (size_t i = 0; i < fields.size(); ++i) {
                if (Value::isRefType(fields[i].type())) {
                    void* ref;
                    memcpy(&ref, reinterpret_cast<uint8_t*>(object) + type->fieldOffsets()[i], sizeof(ref));
                    markReference(ref);
                }
            }
        } else if (object->kind() == Object::ArrayKind) {
            const ArrayType* type = static_cast<const ArrayType*>(object->objectTypeInfo());
            if (Value::isRefType(type->field().type())) {
                GCArray* array = static_cast<GCArray*>(object);
                for (uint32_t i = 0; i < array->length(); ++i) {
                    void* ref;
                    // All reference elements use the same pointer-sized layout.
                    GCArray::get(reinterpret_cast<uint8_t*>(&ref), reinterpret_cast<uint8_t*>(array), i, Value::ExternRef, false);
                    markReference(ref);
                }
            }
        } else {
            Exception* exception = static_cast<GCException*>(object)->exception();
            if (exception->tag()) {
                Tag* tag = exception->tag().value();
                mark(tag);
                const uint8_t* data = exception->userExceptionData().data();
                for (Value::Type type : tag->functionType()->param().types()) {
                    if (Value::isRefType(type)) {
                        void* ref;
                        memcpy(&ref, data, sizeof(ref));
                        markReference(ref);
                    }
                    data += valueStackAllocatedSize(type);
                }
            }
        }
    }
}

void Store::rebuildReferenceGraph(std::vector<StoreObject*>& dead)
{
#ifdef WALRUS_API_TESTS
    ++m_graphBuildCount;
    ++m_graphTraceCount;
#endif
    // Reuse live nodes and edge buffers. Trace only reachable sources: an
    // unreferenced table can be freed without reading any of its slots.
    for (auto& entry : m_referenceGraph) {
        entry.second.targets.clear();
        entry.second.incoming = 0;
    }
    m_gcRootTargets.clear();
    std::vector<StoreObject*> stack;
    auto mark = [&](StoreObject* object) {
        if (!object->m_marked) {
            object->m_marked = true;
            if (m_referenceGraph.find(object) == m_referenceGraph.end()) {
                m_referenceGraph.emplace(object, ReferenceNode());
            }
            stack.push_back(object);
        }
    };
    for (Module* module : m_modules) {
        if (module->m_refCount) {
            mark(module);
        }
    }
    for (Instance* instance : m_instances) {
        if (instance->m_refCount) {
            mark(instance);
        }
    }
    for (Extern* ext : m_externs) {
        if (ext->m_refCount) {
            mark(ext);
        }
    }
    std::unordered_set<StoreObject*> roots;
    for (const auto& root : s_apiGCReferences) {
        std::vector<StoreObject*> targets;
        traceReferences(const_cast<Object*>(root.first), targets);
        for (StoreObject* target : targets) {
            if (roots.insert(target).second) {
                mark(target);
                m_gcRootTargets.push_back(target);
                ++m_referenceGraph.find(target)->second.incoming;
            }
        }
    }
    while (!stack.empty()) {
        StoreObject* object = stack.back();
        stack.pop_back();
        // unordered_map rehash preserves references to nodes and their vectors.
        auto& targets = m_referenceGraph.find(object)->second.targets;
        traceReferences(object, targets);
        for (StoreObject* target : targets) {
            mark(target);
            ++m_referenceGraph.find(target)->second.incoming;
        }
    }
    auto finish = [&](StoreObject* object) {
        if (object->m_marked) {
            object->m_marked = false;
        } else {
            // Incoming counts were rebuilt using reachable sources only.
            m_referenceGraph.erase(object);
            dead.push_back(object);
        }
    };
    for (Module* module : m_modules) {
        finish(module);
    }
    for (Instance* instance : m_instances) {
        finish(instance);
    }
    for (Extern* ext : m_externs) {
        finish(ext);
    }
    m_referenceGraphDirty = false;
}

void Store::eraseGraphObject(StoreObject* object)
{
    auto it = m_referenceGraph.find(object);
    ASSERT(it != m_referenceGraph.end());
    for (StoreObject* target : it->second.targets) {
        auto targetIt = m_referenceGraph.find(target);
        if (targetIt != m_referenceGraph.end()) {
            ASSERT(targetIt->second.incoming);
            --targetIt->second.incoming;
        }
    }
    m_referenceGraph.erase(it);
}

void Store::collectCycles(std::vector<StoreObject*>& dead)
{
#ifdef WALRUS_API_TESTS
    ++m_graphTraceCount;
#endif
    std::vector<StoreObject*> stack;
    auto mark = [&](StoreObject* object) {
        if (!object->m_marked) {
            object->m_marked = true;
            stack.push_back(object);
        }
    };
    for (const auto& entry : m_referenceGraph) {
        if (entry.first->m_refCount) {
            mark(entry.first);
        }
    }
    for (StoreObject* root : m_gcRootTargets) {
        mark(root);
    }
    while (!stack.empty()) {
        StoreObject* object = stack.back();
        stack.pop_back();
        for (StoreObject* target : m_referenceGraph.find(object)->second.targets) {
            mark(target);
        }
    }
    size_t firstDead = dead.size();
    for (const auto& entry : m_referenceGraph) {
        if (entry.first->m_marked) {
            entry.first->m_marked = false;
        } else {
            dead.push_back(entry.first);
        }
    }
    for (size_t i = firstDead; i < dead.size(); ++i) {
        eraseGraphObject(dead[i]);
    }
}

template <typename T>
static void removeStoreObject(Vector<T*>& objects, T* object)
{
    size_t index = object->m_storeIndex;
    ASSERT(index < objects.size() && objects[index] == object);
    T* last = objects.back();
    objects[index] = last;
    last->m_storeIndex = index;
    // Vector::pop_back() reallocates and copies; shrinking this pointer vector
    // through resizeWithUninitializedValues keeps the remaining entries in place.
    objects.resizeWithUninitializedValues(objects.size() - 1);
}

void Store::freeObjects(std::vector<StoreObject*>& dead)
{
    // Instances unlink memory buffers and read module metadata while dying.
    for (StoreObject*& object : dead) {
        if (object->kind() == Object::InstanceKind) {
            Instance* instance = object->asInstance();
            removeStoreObject(m_instances, instance);
            Instance::freeInstance(instance);
            object = nullptr;
        }
    }
    for (StoreObject*& object : dead) {
        if (object && object->kind() != Object::ModuleKind) {
            Extern* ext = static_cast<Extern*>(object);
            removeStoreObject(m_externs, ext);
            delete ext;
            object = nullptr;
        }
    }
    for (StoreObject* object : dead) {
        if (object) {
            Module* module = object->asModule();
            removeStoreObject(m_modules, module);
            getTypeStore().releaseTypes(module->m_compositeTypes);
            delete module;
        }
    }
}

void Store::collectUnreferenced()
{
#ifdef WALRUS_API_TESTS
    ++m_collectionCount;
#endif
    std::vector<StoreObject*> dead;
    if (m_referenceGraphDirty) {
        // Wasm can mutate tables, globals and GC fields directly, including
        // through JIT stores. Rebuild at the outer safe point before using counts.
        rebuildReferenceGraph(dead);
    } else {
        std::vector<StoreObject*> work(m_collectionCandidates.begin(), m_collectionCandidates.end());
        bool mayContainCycles = false;
        while (!work.empty()) {
            StoreObject* object = work.back();
            work.pop_back();
            auto it = m_referenceGraph.find(object);
            if (it == m_referenceGraph.end() || object->m_refCount) {
                continue;
            }
            if (it->second.incoming) {
                mayContainCycles = true;
                continue;
            }
            // No API or internal reference reaches this object. Removing its
            // outgoing edges can make more objects eligible for the fast path.
            for (StoreObject* target : it->second.targets) {
                auto targetIt = m_referenceGraph.find(target);
                ASSERT(targetIt != m_referenceGraph.end() && targetIt->second.incoming);
                --targetIt->second.incoming;
                if (!target->m_refCount) {
                    if (!targetIt->second.incoming) {
                        work.push_back(target);
                    } else {
                        // The removed edge may have been the only external
                        // path into a cycle, even though its count stays nonzero.
                        mayContainCycles = true;
                    }
                }
            }
            m_referenceGraph.erase(it);
            dead.push_back(object);
        }
        if (mayContainCycles) {
            collectCycles(dead);
        }
    }
    m_collectionCandidates.clear();
    freeObjects(dead);
}

void Store::finalize()
{
}

FunctionType* Store::getDefaultFunctionType(Value::Type type)
{
    return const_cast<FunctionType*>(g_defaultFunctionTypes + static_cast<size_t>(type));
}

Waiter* Store::getWaiter(void* address)
{
    std::lock_guard<std::mutex> guard(m_waiterListLock);
    for (size_t i = 0; i < m_waiterList.size(); i++) {
        if (m_waiterList[i]->m_address == address) {
            return m_waiterList[i];
        }
    }

    Waiter* waiter = new Waiter(address);
    m_waiterList.push_back(waiter);
    return waiter;
}

FunctionType* Store::createDefinedFunctionType(DefinedFunctionType type)
{
    const CompositeType** noIndex = reinterpret_cast<const CompositeType**>(TypeStore::NoIndex);
    FunctionType* functionType;
    TypeVector* param;
    TypeVector* result;

    switch (type) {
    case NONE:
        functionType = new FunctionType(0, 0, 0, 0, true, noIndex);
        break;
    case I32R:
        functionType = new FunctionType(1, 0, 0, 0, true, noIndex);
        param = functionType->initParam();
        param->setType(0, Value::Type::I32);
        break;
    case I32_RI32:
        functionType = new FunctionType(1, 0, 1, 0, true, noIndex);
        param = functionType->initParam();
        result = functionType->initResult();
        param->setType(0, Value::Type::I32);
        result->setType(0, Value::Type::I32);
        break;
    case I32I32_RI32:
        functionType = new FunctionType(2, 0, 1, 0, true, noIndex);
        param = functionType->initParam();
        result = functionType->initResult();
        param->setType(0, Value::Type::I32);
        param->setType(1, Value::Type::I32);
        result->setType(0, Value::Type::I32);
        break;
    case I32I64I64_RI32:
        functionType = new FunctionType(3, 0, 1, 0, true, noIndex);

        param = functionType->initParam();
        result = functionType->initResult();

        param->setType(0, Value::Type::I32);
        param->setType(1, Value::Type::I64);
        param->setType(2, Value::Type::I64);

        result->setType(0, Value::Type::I32);
        break;
    case I32I64I32_RI32:
        functionType = new FunctionType(3, 0, 1, 0, true, noIndex);
        param = functionType->initParam();
        result = functionType->initResult();
        param->setType(0, Value::Type::I32);
        param->setType(1, Value::Type::I64);
        param->setType(2, Value::Type::I32);
        result->setType(0, Value::Type::I32);
        break;
    case I32I32I32_RI32:
        functionType = new FunctionType(3, 0, 1, 0, true, noIndex);
        param = functionType->initParam();
        result = functionType->initResult();
        param->setType(0, Value::Type::I32);
        param->setType(1, Value::Type::I32);
        param->setType(2, Value::Type::I32);
        result->setType(0, Value::Type::I32);
        break;
    case I32I32I32I32_RI32:
        functionType = new FunctionType(4, 0, 1, 0, true, noIndex);
        param = functionType->initParam();
        result = functionType->initResult();
        param->setType(0, Value::Type::I32);
        param->setType(1, Value::Type::I32);
        param->setType(2, Value::Type::I32);
        param->setType(3, Value::Type::I32);
        result->setType(0, Value::Type::I32);
        break;
    case I32I64_RI32:
        functionType = new FunctionType(2, 0, 1, 0, true, noIndex);
        param = functionType->initParam();
        result = functionType->initResult();
        param->setType(0, Value::Type::I32);
        param->setType(1, Value::Type::I64);
        result->setType(0, Value::Type::I32);
        break;
    case I32I64I32I32_RI32:
        functionType = new FunctionType(4, 0, 1, 0, true, noIndex);
        param = functionType->initParam();
        result = functionType->initResult();
        param->setType(0, Value::Type::I32);
        param->setType(1, Value::Type::I64);
        param->setType(2, Value::Type::I32);
        param->setType(3, Value::Type::I32);
        result->setType(0, Value::Type::I32);
        break;
    case I32I64I64I32_RI32:
        functionType = new FunctionType(4, 0, 1, 0, true, noIndex);
        param = functionType->initParam();
        result = functionType->initResult();
        param->setType(0, Value::Type::I32);
        param->setType(1, Value::Type::I64);
        param->setType(2, Value::Type::I64);
        param->setType(3, Value::Type::I32);
        result->setType(0, Value::Type::I32);
        break;
    case I32I32I32I32I32_RI32:
        functionType = new FunctionType(5, 0, 1, 0, true, noIndex);
        param = functionType->initParam();
        result = functionType->initResult();
        param->setType(0, Value::Type::I32);
        param->setType(1, Value::Type::I32);
        param->setType(2, Value::Type::I32);
        param->setType(3, Value::Type::I32);
        param->setType(4, Value::Type::I32);
        result->setType(0, Value::Type::I32);
        break;
    case I32I32I32I64I32_RI32:
        functionType = new FunctionType(5, 0, 1, 0, true, noIndex);
        param = functionType->initParam();
        result = functionType->initResult();
        param->setType(0, Value::Type::I32);
        param->setType(1, Value::Type::I32);
        param->setType(2, Value::Type::I32);
        param->setType(3, Value::Type::I64);
        param->setType(4, Value::Type::I32);
        result->setType(0, Value::Type::I32);
        break;
    case I32I32I32I32I32I32_RI32:
        functionType = new FunctionType(6, 0, 1, 0, true, noIndex);
        param = functionType->initParam();
        result = functionType->initResult();
        param->setType(0, Value::Type::I32);
        param->setType(1, Value::Type::I32);
        param->setType(2, Value::Type::I32);
        param->setType(3, Value::Type::I32);
        param->setType(4, Value::Type::I32);
        param->setType(5, Value::Type::I32);
        result->setType(0, Value::Type::I32);
        break;
    case I32I32I32I32I32I32I32_RI32:
        functionType = new FunctionType(7, 0, 1, 0, true, noIndex);
        param = functionType->initParam();
        result = functionType->initResult();
        param->setType(0, Value::Type::I32);
        param->setType(1, Value::Type::I32);
        param->setType(2, Value::Type::I32);
        param->setType(3, Value::Type::I32);
        param->setType(4, Value::Type::I32);
        param->setType(5, Value::Type::I32);
        param->setType(6, Value::Type::I32);
        result->setType(0, Value::Type::I32);
        break;
    case I32I32I32I32I64I64I32_RI32:
        functionType = new FunctionType(7, 0, 1, 0, true, noIndex);
        param = functionType->initParam();
        result = functionType->initResult();
        param->setType(0, Value::Type::I32);
        param->setType(1, Value::Type::I32);
        param->setType(2, Value::Type::I32);
        param->setType(3, Value::Type::I32);
        param->setType(4, Value::Type::I64);
        param->setType(5, Value::Type::I64);
        param->setType(6, Value::Type::I32);
        result->setType(0, Value::Type::I32);
        break;
    case I32I32I32I32I32I64I64I32I32_RI32:
        functionType = new FunctionType(9, 0, 1, 0, true, noIndex);
        param = functionType->initParam();
        result = functionType->initResult();
        param->setType(0, Value::Type::I32);
        param->setType(1, Value::Type::I32);
        param->setType(2, Value::Type::I32);
        param->setType(3, Value::Type::I32);
        param->setType(4, Value::Type::I32);
        param->setType(5, Value::Type::I64);
        param->setType(6, Value::Type::I64);
        param->setType(7, Value::Type::I32);
        param->setType(8, Value::Type::I32);
        result->setType(0, Value::Type::I32);
        break;
    case RI32:
        functionType = new FunctionType(0, 0, 1, 0, true, noIndex);
        result = functionType->initResult();
        result->setType(0, Value::Type::I32);
        break;
    case I64R:
        functionType = new FunctionType(1, 0, 0, 0, true, noIndex);
        param = functionType->initParam();
        param->setType(0, Value::Type::I64);
        break;
    case F32R:
        functionType = new FunctionType(1, 0, 0, 0, true, noIndex);
        param = functionType->initParam();
        param->setType(0, Value::Type::F32);
        break;
    case F64R:
        functionType = new FunctionType(1, 0, 0, 0, true, noIndex);
        param = functionType->initParam();
        param->setType(0, Value::Type::F64);
        break;
    case I32F32R:
        functionType = new FunctionType(2, 0, 0, 0, true, noIndex);
        param = functionType->initParam();
        param->setType(0, Value::Type::I32);
        param->setType(1, Value::Type::F32);
        break;
    case F64F64R:
        functionType = new FunctionType(2, 0, 0, 0, true, noIndex);
        param = functionType->initParam();
        param->setType(0, Value::Type::F64);
        param->setType(1, Value::Type::F64);
        break;
    default:
        ASSERT(type == INVALID);
        functionType = new FunctionType(1, 0, 0, 0, true, noIndex);
        param = functionType->initParam();
        // Temporary types cannot be used as params
        param->setType(0, Value::Type::Void);
        break;
    }

    functionType->initDone();

    Vector<CompositeType*> typeList;
    typeList.push_back(functionType);
    m_typeStore.updateTypes(typeList);
    functionType = typeList[0]->asFunction();

    m_definedFuncTypes[type] = functionType;
    return functionType;
}

} // namespace Walrus
