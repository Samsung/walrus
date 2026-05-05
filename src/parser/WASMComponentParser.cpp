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

#include "parser/WASMComponentParser.h"
#include "parser/WASMParser.h"
#include "runtime/Store.h"
#include "runtime/TypeStore.h"

#include "wabt/binary-reader.h"
#include "wabt/walrus/binary-reader-walrus.h"

namespace wabt {

class WASMComponentBinaryReader : public wabt::WASMComponentBinaryReaderDelegate {
private:
    struct CoreInstanceType {
        static constexpr size_t NotInline = ~static_cast<size_t>(0);

        CoreInstanceType(Walrus::Module* module)
            : module(module)
            , inlineIndex(NotInline)
        {
            ASSERT(module != nullptr);
        }

        CoreInstanceType(size_t inlineIndex)
            : module(nullptr)
            , inlineIndex(inlineIndex)
        {
        }

        Walrus::Module* module;
        size_t inlineIndex;
    };

    // Depth data for each component.
    struct ComponentTypeInfo {
        ComponentTypeInfo(ComponentTypeInfo* parent, Walrus::ComponentType* parentComponentType, Walrus::Component* parentComponent)
            : parent(parent)
            , parentComponentType(parentComponentType)
            , parentComponent(parentComponent)
        {
        }

        ComponentTypeInfo* parent;
        Walrus::ComponentType* parentComponentType;
        Walrus::Component* parentComponent;
        std::vector<Walrus::FunctionType*> coreFuncTypes;
        std::vector<bool> coreMemories;
        std::vector<Walrus::Module*> coreModuleTypes;
        std::vector<CoreInstanceType> coreInstanceTypes;
        std::vector<Walrus::ComponentTypeFunc*> funcTypes;
        std::vector<Walrus::ComponentType*> componentTypes;
        std::vector<Walrus::ComponentType*> instanceTypes;
    };

    Walrus::ComponentTypeRef::Type getValueType(const ComponentType& type)
    {
        switch (type.GetType()) {
        case ComponentType::Bool:
            return Walrus::ComponentTypeRef::Bool;
        case ComponentType::S8:
            return Walrus::ComponentTypeRef::S8;
        case ComponentType::U8:
            return Walrus::ComponentTypeRef::U8;
        case ComponentType::S16:
            return Walrus::ComponentTypeRef::S16;
        case ComponentType::U16:
            return Walrus::ComponentTypeRef::U16;
        case ComponentType::S32:
            return Walrus::ComponentTypeRef::S32;
        case ComponentType::U32:
            return Walrus::ComponentTypeRef::U32;
        case ComponentType::S64:
            return Walrus::ComponentTypeRef::S64;
        case ComponentType::U64:
            return Walrus::ComponentTypeRef::U64;
        case ComponentType::F32:
            return Walrus::ComponentTypeRef::F32;
        case ComponentType::F64:
            return Walrus::ComponentTypeRef::F64;
        case ComponentType::Char:
            return Walrus::ComponentTypeRef::Char;
        case ComponentType::String:
            return Walrus::ComponentTypeRef::String;
        default:
            ASSERT(type.GetType() == ComponentType::ErrorContext);
            return Walrus::ComponentTypeRef::ErrorContext;
        }
    }

    Walrus::ComponentSort getSort(ComponentSort sort)
    {
        switch (sort) {
        case ComponentSort::CoreFunc:
            return Walrus::ComponentSort::CoreFunc;
        case ComponentSort::CoreTable:
            return Walrus::ComponentSort::CoreTable;
        case ComponentSort::CoreMemory:
            return Walrus::ComponentSort::CoreMemory;
        case ComponentSort::CoreGlobal:
            return Walrus::ComponentSort::CoreGlobal;
        case ComponentSort::CoreType:
            return Walrus::ComponentSort::CoreType;
        case ComponentSort::CoreModule:
            return Walrus::ComponentSort::CoreModule;
        case ComponentSort::CoreInstance:
            return Walrus::ComponentSort::CoreInstance;
        case ComponentSort::Func:
            return Walrus::ComponentSort::Func;
        case ComponentSort::Value:
            return Walrus::ComponentSort::Value;
        case ComponentSort::Type:
            return Walrus::ComponentSort::Type;
        case ComponentSort::Component:
            return Walrus::ComponentSort::Component;
        case ComponentSort::Instance:
            return Walrus::ComponentSort::Instance;
        default:
            return Walrus::ComponentSort::Invalid;
        }
    }

    Walrus::ComponentTypeRef refType(const ComponentType& type)
    {
        if (type.IsIndex()) {
            Walrus::ComponentRefCounted* ref = m_current->getType(type.GetIndex());
            ref->addRef();
            return Walrus::ComponentTypeRef(ref);
        }
        return Walrus::ComponentTypeRef(getValueType(type.GetType()));
    }

    Walrus::ComponentTypeRef refAnyType(const ComponentType& type)
    {
        if (type.IsNone()) {
            return Walrus::ComponentTypeRef();
        }
        return refType(type);
    }

    Walrus::CanonicalOptions parseCanonOptions(uint32_t optionCount, ComponentCanonOption* options)
    {
        Walrus::CanonicalOptions::StringEncoding encoding = Walrus::CanonicalOptions::Utf8;
        bool isAsync = false;
        uint32_t memoryIndex = Walrus::CanonicalOptions::NoIndex;
        uint32_t reallocIndex = Walrus::CanonicalOptions::NoIndex;
        uint32_t postReturnIndex = Walrus::CanonicalOptions::NoIndex;
        uint32_t callbackIndex = Walrus::CanonicalOptions::NoIndex;

        while (optionCount > 0) {
            switch (options->option) {
            case ComponentCanonOption::StrEncUtf8:
                encoding = Walrus::CanonicalOptions::Utf8;
                break;
            case ComponentCanonOption::StrEncUtf16:
                encoding = Walrus::CanonicalOptions::Utf16;
                break;
            case ComponentCanonOption::StrEncLatin1Utf16:
                encoding = Walrus::CanonicalOptions::Latin1Utf16;
                break;
            case ComponentCanonOption::Memory:
                memoryIndex = options->index;
                break;
            case ComponentCanonOption::Realloc:
                reallocIndex = options->index;
                break;
            case ComponentCanonOption::PostReturn:
                postReturnIndex = options->index;
                break;
            case ComponentCanonOption::Async:
                isAsync = true;
                break;
            case ComponentCanonOption::Callback:
                callbackIndex = options->index;
                break;
            }
            optionCount--;
            options++;
        }
        return Walrus::CanonicalOptions(encoding, isAsync, memoryIndex, reallocIndex, postReturnIndex, callbackIndex);
    }

    Walrus::ComponentRefCounted* getTypeRef(ComponentSort sort, uint32_t index)
    {
        switch (sort) {
        case ComponentSort::Func:
            ASSERT(index < m_currentInfo->funcTypes.size());
            return m_currentInfo->funcTypes[index];
        case ComponentSort::Type:
            ASSERT(index < m_current->types().size());
            return m_current->getType(index);
        case ComponentSort::Component:
            ASSERT(index < m_currentInfo->componentTypes.size());
            return m_currentInfo->componentTypes[index];
        case ComponentSort::Instance:
            ASSERT(index < m_currentInfo->instanceTypes.size());
            return m_currentInfo->instanceTypes[index];
        default:
            return nullptr;
        }
    }

    Walrus::ComponentRefCounted* pushExternalType(const ComponentExternalInfo& externalInfo)
    {
        Walrus::ComponentRefCounted* type;
        switch (externalInfo.sort) {
        case ComponentSort::Func:
            type = m_current->getType(externalInfo.index.index)->asTypeFunc();
            m_currentInfo->funcTypes.push_back(type->asTypeFunc());
            break;
        case ComponentSort::Type:
            if (externalInfo.external == ComponentExternalDesc::TypeSubRes) {
                type = new Walrus::ComponentTypeResource(Walrus::ComponentRefCounted::ResourceKind, false);
            } else {
                type = m_current->getType(externalInfo.index.index);
                type->addRef();
            }
            m_current->pushType(type);
            break;
        case ComponentSort::Component:
            type = m_current->getType(externalInfo.index.index);
            m_currentInfo->componentTypes.push_back(type->asComponentType());
            break;
        case ComponentSort::Instance:
            type = m_current->getType(externalInfo.index.index);
            m_currentInfo->instanceTypes.push_back(type->asComponentType());
            break;
        default:
            return nullptr;
        }
        type->addRef();
        return type;
    }

public:
    WASMComponentBinaryReader(Walrus::Store* store, const std::string& filename, const uint32_t JITFlags, const uint32_t featureFlags)
        : m_store(store)
        , m_filename(filename)
        , m_JITFlags(JITFlags)
        , m_featureFlags(featureFlags)
        , m_current(nullptr)
        , m_currentComponent(nullptr)
        , m_currentInfo(nullptr)
    {
    }

    ~WASMComponentBinaryReader()
    {
        while (m_currentInfo != nullptr) {
            ComponentTypeInfo* info = m_currentInfo;
            m_currentInfo = m_currentInfo->parent;
            delete info;
        }
    }

    const std::string& filename()
    {
        return m_filename;
    }

    uint32_t featureFlags()
    {
        return m_featureFlags;
    }

    void OnCoreModule(const void* data,
                      size_t size,
                      const ReadBinaryOptions& options)
    {
        std::pair<Walrus::Optional<Walrus::Module*>, std::string> result = Walrus::WASMParser::parseBinary(m_store, m_filename, reinterpret_cast<const uint8_t*>(data), size, m_JITFlags, m_featureFlags);
        if (!result.second.empty()) {
            m_walrusParseError = result.second;
        }
        // Module has been already added to store.
        Walrus::Module* module = result.first.value();
        m_currentComponent->pushDeclaration(new Walrus::ComponentCoreModule(module));
        m_currentInfo->coreModuleTypes.push_back(module);
    }

    void BeginComponent(uint32_t version, size_t depth)
    {
        ASSERT(m_currentInfo != nullptr || (m_current == nullptr && m_currentComponent == nullptr));
        Walrus::Component* parentComponent = m_currentComponent;
        m_currentInfo = new ComponentTypeInfo(m_currentInfo, m_current, m_currentComponent);
        m_currentComponent = new Walrus::Component();
        m_current = m_currentComponent->type();

        if (parentComponent != nullptr) {
            parentComponent->pushDeclaration(m_currentComponent);
            m_currentInfo->parent->componentTypes.push_back(m_current);
        }
    }

    void EndComponent()
    {
        ComponentTypeInfo* info = m_currentInfo;
        // Keep the last component.
        if (m_currentInfo->parentComponent != nullptr) {
            m_currentComponent = m_currentInfo->parentComponent;
        }
        m_current = m_currentInfo->parentComponentType;
        ASSERT(m_current == nullptr || (m_currentComponent != nullptr && m_currentComponent->type() == m_current));
        m_currentInfo = m_currentInfo->parent;
        delete info;
    }

    void BeginCoreInstance(Index moduleIndex,
                           uint32_t argumentCount)
    {
        m_currentInfo->coreInstanceTypes.push_back(CoreInstanceType(m_currentInfo->coreModuleTypes[moduleIndex]));
        m_currentComponent->pushDeclaration(new Walrus::ComponentCoreInstantiate(moduleIndex));
    }

    void OnCoreInstanceArg(const ComponentStringLoc& name,
                           ComponentSort sort,
                           Index index)
    {
        Walrus::ComponentCoreInstantiate* instance = m_currentComponent->declarations().back()->asCoreInstantiate();
        instance->arguments().push_back(Walrus::ComponentCoreInstantiate::Argument{ name.str.to_string(), index });
    }

    void EndCoreInstance()
    {
    }

    void BeginInlineCoreInstance(uint32_t argumentCount)
    {
        m_currentInfo->coreInstanceTypes.push_back(CoreInstanceType(m_currentComponent->declarations().size()));
        m_currentComponent->pushDeclaration(new Walrus::ComponentCoreInstantiateInline());
        ASSERT(m_names.size() == 0);
    }

    void OnInlineCoreInstanceArg(const ComponentStringLoc& name,
                                 ComponentSort sort,
                                 Index index)
    {
        Walrus::ComponentCoreInstantiateInline* instance = m_currentComponent->declarations().back()->asCoreInstantiateInline();
        instance->arguments().push_back(Walrus::ComponentCoreInstantiateInline::Argument{ getSort(sort), index });
        m_names.push_back(name.str.to_string());
    }

    void EndInlineCoreInstance()
    {
        Walrus::ComponentCoreInstantiateInline* instance = m_currentComponent->declarations().back()->asCoreInstantiateInline();
        Walrus::WASMParsingResult result;
        size_t size = instance->arguments().size();
        uint32_t funcIndex = 0;
        uint32_t tableIndex = 0;
        uint32_t memoryIndex = 0;
        uint32_t globalIndex = 0;
        ASSERT(m_names.size() == size);

        for (size_t i = 0; i < size; i++) {
            Walrus::ExportType::Type exportType;
            uint32_t index;
            switch (instance->arguments()[i].sort) {
            case Walrus::ComponentSort::CoreFunc:
                index = funcIndex++;
                exportType = Walrus::ExportType::Function;
                break;
            case Walrus::ComponentSort::CoreTable:
                index = tableIndex++;
                exportType = Walrus::ExportType::Table;
                break;
            case Walrus::ComponentSort::CoreMemory:
                index = memoryIndex++;
                exportType = Walrus::ExportType::Memory;
                break;
            default:
                ASSERT(instance->arguments()[i].sort == Walrus::ComponentSort::CoreGlobal);
                index = globalIndex++;
                exportType = Walrus::ExportType::Global;
                break;
            }
            result.m_exports.push_back(new Walrus::ExportType(exportType, m_names[i], index));
        }
        Walrus::Module* module = new Walrus::Module(m_store, result);
        m_currentInfo->coreInstanceTypes.back().module = module;
        instance->setModule(module);
        m_names.clear();
    }

    void BeginInstance(Index componentIndex,
                       uint32_t argumentCount)
    {
        m_currentInfo->instanceTypes.push_back(m_currentInfo->componentTypes[componentIndex]);
        m_currentComponent->pushDeclaration(new Walrus::ComponentInstantiate(componentIndex));
    }

    void OnInstanceArg(const ComponentStringLoc& name,
                       ComponentSort sort,
                       Index index)
    {
        Walrus::ComponentInstantiate* instance = m_currentComponent->declarations().back()->asInstantiate();
        instance->arguments().push_back(Walrus::ComponentInstantiate::Argument{ name.str.to_string(), getSort(sort), index });
    }

    void EndInstance()
    {
    }

    void BeginInlineInstance(uint32_t argumentCount)
    {
        m_currentComponent->pushDeclaration(new Walrus::ComponentInstantiateInline());
    }

    void OnInlineInstanceArg(const ComponentStringLoc& name,
                             nonstd::string_view* versionSuffix,
                             ComponentSort sort,
                             Index index)
    {
        Walrus::ComponentRefCounted* type = getTypeRef(sort, index);
        if (type == nullptr) {
            return;
        }
        type->addRef();
        Walrus::ComponentInstantiateInline* instance = m_currentComponent->declarations().back()->asInstantiateInline();
        instance->type()->exports().push_back(Walrus::ComponentType::External{ name.str.to_string(), type });
        instance->arguments().push_back(Walrus::ComponentInstantiateInline::Argument{ getSort(sort), index });
    }

    void EndInlineInstance()
    {
    }

    void OnAliasExport(ComponentSort sort,
                       Index instanceIndex,
                       const ComponentStringLoc& name)
    {
        for (auto it : m_currentInfo->instanceTypes[instanceIndex]->exports()) {
            if (name.str == it.name) {
                switch (sort) {
                case ComponentSort::Func:
                    m_currentInfo->funcTypes.push_back(it.type->asTypeFunc());
                    break;
                case ComponentSort::Type:
                    it.type->addRef();
                    m_current->pushType(it.type);
                    break;
                case ComponentSort::Component:
                    m_currentInfo->componentTypes.push_back(it.type->asComponentType());
                    break;
                case ComponentSort::Instance:
                    m_currentInfo->instanceTypes.push_back(it.type->asComponentType());
                    break;
                default:
                    ASSERT(sort == ComponentSort::CoreModule);
                    break;
                }
                return;
            }
        }
    }

    void OnAliasCoreExport(ComponentSort sort,
                           Index coreInstanceIndex,
                           const ComponentStringLoc& name)
    {
        size_t exportIndex = 0;
        const CoreInstanceType& type = m_currentInfo->coreInstanceTypes[coreInstanceIndex];
        const Walrus::VectorWithFixedSize<Walrus::ExportType*, std::allocator<Walrus::ExportType*>>& exports = type.module->exports();

        while (true) {
            if (exportIndex >= exports.size()) {
                m_walrusParseError = "export not found";
                return;
            }

            if (name.str == exports[exportIndex]->name()) {
                break;
            }
            exportIndex++;
        }

        if (type.inlineIndex != CoreInstanceType::NotInline) {
            exportIndex = m_currentComponent->declarations()[type.inlineIndex]->asCoreInstantiateInline()->arguments()[exportIndex].index;

            switch (sort) {
            case ComponentSort::CoreFunc:
                m_currentInfo->coreFuncTypes.push_back(m_currentInfo->coreFuncTypes[exportIndex]);
                break;
            case ComponentSort::CoreMemory:
                m_currentInfo->coreMemories.push_back(m_currentInfo->coreMemories[exportIndex]);
                break;
            default:
                break;
            }
            return;
        }

        uint32_t itemIndex = exports[exportIndex]->itemIndex();

        switch (sort) {
        case ComponentSort::CoreFunc:
            m_currentInfo->coreFuncTypes.push_back(type.module->function(itemIndex)->functionType());
            break;
        case ComponentSort::CoreMemory:
            m_currentInfo->coreMemories.push_back(type.module->memoryType(itemIndex)->is64());
            break;
        default:
            break;
        }
    }

    void OnAliasOuter(ComponentSort sort,
                      uint32_t counter,
                      uint32_t index)
    {
        ComponentTypeInfo* info = m_currentInfo;
        Walrus::ComponentType* target = m_current;

        while (counter > 0) {
            target = info->parentComponentType;
            info = info->parent;
            counter--;
        }

        if (sort == ComponentSort::Type) {
            Walrus::ComponentRefCounted* type = target->getType(index);
            type->addRef();
            m_current->pushType(type);
        } else if (sort == ComponentSort::Component) {
            m_currentInfo->componentTypes.push_back(info->componentTypes[index]);
        }
    }

    void OnPrimitiveType(const ComponentType& type)
    {
        m_current->pushType(new Walrus::ComponentValueType(getValueType(type)));
    }

    void BeginRecordType(uint32_t fieldCount)
    {
        m_current->pushType(new Walrus::ComponentTypeItems(Walrus::ComponentRefCounted::RecordKind));
    }

    void OnRecordField(const ComponentStringLoc& fieldName,
                       const ComponentType& fieldType)
    {
        Walrus::ComponentTypeRef ref = refType(fieldType);
        m_current->lastType()->asTypeItems()->items().push_back(Walrus::ComponentTypeItems::Item{ fieldName.str.to_string(), ref });
    }

    void EndRecordType()
    {
    }

    void BeginVariantType(uint32_t caseCount)
    {
        m_current->pushType(new Walrus::ComponentTypeItems(Walrus::ComponentRefCounted::VariantKind));
    }

    void OnVariantCase(const ComponentStringLoc& caseName,
                       const ComponentType& caseType)
    {
        Walrus::ComponentTypeRef ref = refAnyType(caseType);
        m_current->lastType()->asTypeItems()->items().push_back(Walrus::ComponentTypeItems::Item{ caseName.str.to_string(), ref });
    }

    void EndVariantType()
    {
    }

    void OnListType(const ComponentType& type)
    {
        m_current->pushType(new Walrus::ComponentValueTypeRef(Walrus::ComponentRefCounted::ListKind, refType(type)));
    }

    void OnListFixedType(const ComponentType& type,
                         uint32_t size)
    {
        m_current->pushType(new Walrus::ComponentTypeListFixed(refType(type), size));
    }

    void BeginTupleType(uint32_t itemCount)
    {
        m_current->pushType(new Walrus::ComponentTypeTuple());
    }

    void OnTupleItem(const ComponentType& item)
    {
        m_current->lastType()->asTypeTuple()->items().push_back(refType(item));
    }

    void EndTupleType()
    {
    }

    void BeginFlagsType(uint32_t labelCount)
    {
        m_current->pushType(new Walrus::ComponentTypeLabels(Walrus::ComponentRefCounted::FlagsKind));
    }

    void OnFlagsLabel(const ComponentStringLoc& label)
    {
        m_current->lastType()->asTypeLabels()->labels().push_back(label.str.to_string());
    }

    void EndFlagsType()
    {
    }

    void BeginEnumType(uint32_t labelCount)
    {
        m_current->pushType(new Walrus::ComponentTypeLabels(Walrus::ComponentRefCounted::EnumKind));
    }

    void OnEnumLabel(const ComponentStringLoc& label)
    {
        m_current->lastType()->asTypeLabels()->labels().push_back(label.str.to_string());
    }

    void EndEnumType()
    {
    }

    void OnOptionType(const ComponentType& type)
    {
        m_current->pushType(new Walrus::ComponentValueTypeRef(Walrus::ComponentRefCounted::OptionKind, refType(type)));
    }

    void OnResultType(const ComponentType& resultType,
                      const ComponentType& errorType)
    {
        m_current->pushType(new Walrus::ComponentTypeResult(refAnyType(resultType), refAnyType(errorType)));
    }

    void OnOwnType(Index index)
    {
        Walrus::ComponentTypeResource* ref = m_current->getType(index)->asTypeResource();
        ref->addRef();
        m_current->pushType(new Walrus::ComponentTypeResourceRef(Walrus::ComponentRefCounted::OwnKind, ref));
    }

    void OnBorrowType(Index index)
    {
        Walrus::ComponentTypeResource* ref = m_current->getType(index)->asTypeResource();
        ref->addRef();
        m_current->pushType(new Walrus::ComponentTypeResourceRef(Walrus::ComponentRefCounted::BorrowKind, ref));
    }

    void OnStreamType(const ComponentType& type)
    {
        m_current->pushType(new Walrus::ComponentValueTypeRef(Walrus::ComponentRefCounted::StreamKind, refType(type)));
    }

    void OnFutureType(const ComponentType& type)
    {
        m_current->pushType(new Walrus::ComponentValueTypeRef(Walrus::ComponentRefCounted::FutureKind, refType(type)));
    }

    void BeginFuncType(ComponentTypeDef type,
                       uint32_t paramCount)
    {
        Walrus::ComponentRefCounted::Kind kind = Walrus::ComponentRefCounted::FuncKind;
        if (type == ComponentTypeDef::AsyncFunc) {
            kind = Walrus::ComponentRefCounted::AsyncFuncKind;
        }
        m_current->pushType(new Walrus::ComponentTypeFunc(kind));
    }

    void OnFuncParam(ComponentStringLoc name,
                     const ComponentType& type)
    {
        Walrus::ComponentTypeRef ref = refType(type);
        m_current->lastType()->asTypeFunc()->params().push_back(Walrus::ComponentTypeFunc::Param{ name.str.to_string(), ref });
    }

    void OnFuncResult(const ComponentType& type)
    {
        m_current->lastType()->asTypeFunc()->result() = refAnyType(type);
    }

    void EndFuncType()
    {
    }

    void OnResourceType(ComponentResourceRep rep,
                        Index dtor)
    {
        m_current->pushType(new Walrus::ComponentTypeResource(Walrus::ComponentRefCounted::ResourceKind, rep == ComponentResourceRep::I64));
    }

    void OnResourceAsyncType(ComponentResourceRep rep,
                             Index dtor,
                             Index callback)
    {
        m_current->pushType(new Walrus::ComponentTypeResource(Walrus::ComponentRefCounted::ResourceAsyncKind, rep == ComponentResourceRep::I64));
    }

    void BeginInstanceType(uint32_t count)
    {
        ASSERT(m_currentInfo != nullptr || m_current == nullptr);
        Walrus::ComponentType* current = m_current;
        m_currentInfo = new ComponentTypeInfo(m_currentInfo, m_current, m_currentComponent);
        m_current = new Walrus::ComponentType(Walrus::ComponentRefCounted::InstanceTypeKind);
        m_currentComponent = nullptr;
        current->pushType(m_current);
    }

    void EndInstanceType()
    {
        ComponentTypeInfo* info = m_currentInfo;
        m_current = m_currentInfo->parentComponentType;
        m_currentComponent = m_currentInfo->parentComponent;
        ASSERT(m_currentComponent == nullptr || m_currentComponent->type() == m_current);
        m_currentInfo = m_currentInfo->parent;
        delete info;
    }

    void BeginComponentType(uint32_t count)
    {
        ASSERT(m_currentInfo != nullptr || m_current == nullptr);
        Walrus::ComponentType* current = m_current;
        m_currentInfo = new ComponentTypeInfo(m_currentInfo, m_current, m_currentComponent);
        m_current = new Walrus::ComponentType(Walrus::ComponentRefCounted::ComponentTypeKind);
        m_currentComponent = nullptr;
        current->pushType(m_current);
    }

    void EndComponentType()
    {
        ComponentTypeInfo* info = m_currentInfo;
        m_current = m_currentInfo->parentComponentType;
        m_currentComponent = m_currentInfo->parentComponent;
        ASSERT(m_currentComponent == nullptr || m_currentComponent->type() == m_current);
        m_currentInfo = m_currentInfo->parent;
        delete info;
    }

    void OnCanonLift(Index coreFuncIndex,
                     uint32_t optionCount,
                     ComponentCanonOption* options,
                     Index typeIndex)
    {
        Walrus::CanonicalOptions canonOptions = parseCanonOptions(optionCount, options);
        Walrus::ComponentTypeFunc* funcType = m_current->getType(typeIndex)->asTypeFunc();
        m_currentComponent->pushDeclaration(new Walrus::ComponentCanonLift(coreFuncIndex, canonOptions, funcType));
        m_currentInfo->funcTypes.push_back(funcType);
    }

    void OnCanonLower(Index funcIndex,
                      uint32_t optionCount,
                      ComponentCanonOption* options)
    {
        Walrus::CanonicalOptions canonOptions = parseCanonOptions(optionCount, options);
        m_currentComponent->pushDeclaration(new Walrus::ComponentCanonLower(funcIndex, canonOptions));
    }

    void OnCanonType(ComponentCanon canon,
                     Index typeIndex)
    {
        Walrus::ComponentDeclaration::Kind kind = Walrus::ComponentDeclaration::CanonResourceNew;
        if (canon == ComponentCanon::ResourceDrop) {
            kind = Walrus::ComponentDeclaration::CanonResourceDrop;
        } else if (canon == ComponentCanon::ResourceRep) {
            kind = Walrus::ComponentDeclaration::CanonResourceRep;
        }

        Walrus::ComponentRefCounted* ref = m_current->getType(typeIndex);
        m_currentComponent->pushDeclaration(new Walrus::ComponentCanonType(kind, ref));
    }

    void OnImport(const ComponentStringLoc& name,
                  nonstd::string_view* versionSuffix,
                  const ComponentExternalInfo& externalInfo)
    {
        if (m_currentComponent != nullptr) {
            m_currentComponent->pushDeclaration(new Walrus::ComponentImport(static_cast<uint32_t>(m_current->imports().size())));
        }
        m_current->imports().push_back(Walrus::ComponentType::External{ name.str.to_string(), pushExternalType(externalInfo) });
    }

    void OnExport(const ComponentStringLoc& name,
                  nonstd::string_view* versionSuffix,
                  ComponentExternalInfo* externalInfo,
                  ComponentExportInfo* exportInfo)
    {
        if (externalInfo != nullptr) {
            m_current->exports().push_back(Walrus::ComponentType::External{ name.str.to_string(), pushExternalType(*externalInfo) });
            return;
        }

        ComponentExternalInfo info{ exportInfo->sort, ComponentExternalDesc::Unused, exportInfo->index };
        m_current->exports().push_back(Walrus::ComponentType::External{ name.str.to_string(), pushExternalType(info) });
    }

    Walrus::Component* parsingResult()
    {
        return m_currentComponent;
    }

private:
    Walrus::Store* m_store;
    std::string m_filename;
    uint32_t m_JITFlags;
    uint32_t m_featureFlags;

    Walrus::ComponentType* m_current;
    Walrus::Component* m_currentComponent;
    ComponentTypeInfo* m_currentInfo;
    std::vector<std::string> m_names;
};

} // namespace wabt

namespace Walrus {

std::pair<Optional<Component*>, std::string> WASMComponentParser::parseBinary(Store* store, const std::string& filename, const uint8_t* data, size_t len, const uint32_t JITFlags, const uint32_t featureFlags)
{
    wabt::WASMComponentBinaryReader delegate(store, filename, JITFlags, featureFlags);

    std::string error = ReadWasmComponentBinary(data, len, &delegate);
    if (error.length()) {
        return std::make_pair(nullptr, error);
    }

    delete delegate.parsingResult();
    return std::make_pair(nullptr, std::string());
}

} // namespace Walrus
