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

#include "runtime/ComponentInstance.h"
#include "runtime/DefinedFunctionTypes.h"
#include "runtime/Instance.h"
#include "runtime/Global.h"
#include "runtime/Memory.h"
#include "runtime/Table.h"
#include "runtime/Tag.h"
#include "runtime/Store.h"
#include "wasi/WASI02.h"

namespace Walrus {

DEFINE_GLOBAL_TYPE_INFO(componentInstanceTypeInfo, ComponentInstanceKind);

LoweredFunction* LoweredFunction::createLoweredFunction(Store* store, FunctionType* functionType, LiftedFunction* liftedFunction)
{
    LoweredFunction* func = new LoweredFunction(functionType, liftedFunction);
    store->appendExtern(func);
    return func;
}

void LoweredFunction::call(ExecutionState& state, Value* argv, Value* result)
{
    ASSERT(0);
}

CanonFunction* CanonFunction::createCanonFunction(Store* store, FunctionType* functionType, Type type)
{
    CanonFunction* func = new CanonFunction(functionType, type);
    store->appendExtern(func);
    return func;
}

void CanonFunction::call(ExecutionState& state, Value* argv, Value* result)
{
    ASSERT(0);
}

void ComponentInstance::coreInstantiate(ExecutionState& state, Store* store, Component* component, ComponentCoreInstantiate* instantiate)
{
    ExternVector imports;
    Module* module = component->modules()[instantiate->moduleIndex()];

    for (auto import : module->imports()) {
        Instance* instance = nullptr;
        uint32_t inlineIndex = 0;

        for (auto argument : instantiate->arguments()) {
            if (import->moduleName() == argument.name) {
                if (argument.index < m_coreInstances.size()) {
                    instance = m_coreInstances[argument.index];
                } else {
                    inlineIndex = argument.index;
                    ASSERT(inlineIndex > 0);
                }
                break;
            }
        }

        Extern* value = nullptr;

        if (instance != nullptr) {
            ExportType* exportType = nullptr;
            for (auto field : instance->module()->exports()) {
                if (import->fieldName() == field->name()) {
                    exportType = field;
                    break;
                }
            }

            if (exportType == nullptr) {
                std::string message = "Cannot import field \"";
                message.append(import->fieldName());
                message.append("\" from module: ");
                message.append(import->moduleName());
                Trap::throwException(state, message);
            }

            switch (import->importType()) {
            case ImportType::Function:
                if (exportType->exportType() == ExportType::Function) {
                    Function* func = instance->function(exportType->itemIndex());
                    if (func->functionType()->equals(import->functionType())) {
                        value = func;
                    }
                }
                break;
            case ImportType::Table:
                if (exportType->exportType() == ExportType::Table) {
                    Table* table = instance->table(exportType->itemIndex());
                    if (table->type() == import->tableType()->type()) {
                        value = table;
                    }
                }
                break;
            case ImportType::Memory:
                if (exportType->exportType() == ExportType::Memory) {
                    Memory* memory = instance->memory(exportType->itemIndex());
                    if (import->memoryType()->initialSize() < memory->sizeInPageSize()
                        && import->memoryType()->maximumSize() >= memory->maximumSizeInPageSize()
                        && import->memoryType()->isShared() == memory->isShared()
                        && import->memoryType()->is64() == memory->is64()) {
                        value = memory;
                    }
                }
                break;
            case ImportType::Global:
                if (exportType->exportType() == ExportType::Global) {
                    Global* global = instance->global(exportType->itemIndex());
                    if (global->type() == import->globalType()->type()) {
                        value = global;
                    }
                }
                break;
            case ImportType::Tag:
                if (exportType->exportType() == ExportType::Tag) {
                    Tag* tag = instance->tag(exportType->itemIndex());
                    if (tag->functionType()->equals(import->tagType()->functionType())) {
                        value = tag;
                    }
                }
                break;
            default:
                RELEASE_ASSERT_NOT_REACHED();
                break;
            }
        } else {
            if (inlineIndex == 0) {
                std::string message = "Cannot import module: ";
                message.append(import->moduleName());
                Trap::throwException(state, message);
            }

            size_t inlineStart = component->coreInlineExportsStart(~inlineIndex);
            size_t inlineEnd = component->coreInlineExportsEnd(~inlineIndex);
            std::vector<Walrus::Component::InlineExport>& exports = component->coreInlineExports();
            ComponentSort sort = ComponentSort::Invalid;
            uint32_t exportIndex = 0;

            for (size_t i = inlineStart; i < inlineEnd; i++) {
                if (exports[i].name == import->fieldName()) {
                    sort = exports[i].sort;
                    exportIndex = exports[i].index;
                    break;
                }
            }

            if (sort == ComponentSort::Invalid) {
                std::string message = "Cannot import field \"";
                message.append(import->fieldName());
                message.append("\" from module: ");
                message.append(import->moduleName());
                Trap::throwException(state, message);
            }

            switch (import->importType()) {
            case ImportType::Function:
                if (sort == ComponentSort::CoreFunc) {
                    Function* func = m_coreFuncs[exportIndex];
                    if (func->functionType()->equals(import->functionType())) {
                        value = func;
                    }
                }
                break;
            case ImportType::Table:
                if (sort == ComponentSort::CoreTable) {
                    Table* table = m_coreTables[exportIndex];
                    if (table->type() == import->tableType()->type()) {
                        value = table;
                    }
                }
                break;
            case ImportType::Memory:
                if (sort == ComponentSort::CoreMemory) {
                    Memory* memory = m_coreMemories[exportIndex];
                    if (import->memoryType()->initialSize() < memory->sizeInPageSize()
                        && import->memoryType()->maximumSize() >= memory->maximumSizeInPageSize()
                        && import->memoryType()->isShared() == memory->isShared()
                        && import->memoryType()->is64() == memory->is64()) {
                        value = memory;
                    }
                }
                break;
            case ImportType::Global:
                if (sort == ComponentSort::CoreGlobal) {
                    Global* global = m_coreGlobals[exportIndex];
                    if (global->type() == import->globalType()->type()) {
                        value = global;
                    }
                }
                break;
            case ImportType::Tag:
                if (sort == ComponentSort::CoreTag) {
                    Tag* tag = m_coreTags[exportIndex];
                    if (tag->functionType()->equals(import->tagType()->functionType())) {
                        value = tag;
                    }
                }
                break;
            default:
                RELEASE_ASSERT_NOT_REACHED();
                break;
            }
        }

        if (value == nullptr) {
            std::string message = "Type mismatch for field \"";
            message.append(import->fieldName());
            message.append("\" form module: ");
            message.append(import->moduleName());
            Trap::throwException(state, message);
        }

        imports.push_back(value);
    }

    m_coreInstances.push_back(module->instantiate(state, imports));
}

void ComponentInstance::aliasExport(ComponentAliasExport* alias)
{
    ComponentInstance* instance = m_instances[alias->instanceIndex()];

    for (auto it : instance->type()->exports()) {
        if (alias->name() == it.name) {
            ASSERT(alias->sort() == it.sort);
            switch (it.sort) {
            case ComponentSort::Func: {
                LiftedFunction* func = instance->m_funcs[it.exportIndex];
                m_funcs.push_back(func);
                func->addRef();
                break;
            }
            case ComponentSort::Instance:
                m_instances.push_back(instance->m_instances[it.exportIndex]);
                break;
            default:
                RELEASE_ASSERT_NOT_REACHED();
                break;
            }
            return;
        }
    }
    ASSERT(0);
}

void ComponentInstance::aliasCoreExport(ComponentAliasExport* alias)
{
    Instance* instance = m_coreInstances[alias->instanceIndex()];

    for (auto it : instance->module()->exports()) {
        if (alias->name() == it->name()) {
            switch (it->exportType()) {
            case ExportType::Function:
                ASSERT(alias->sort() == ComponentSort::CoreFunc);
                m_coreFuncs.push_back(instance->function(it->itemIndex()));
                break;
            case ExportType::Table:
                ASSERT(alias->sort() == ComponentSort::CoreTable);
                m_coreTables.push_back(instance->table(it->itemIndex()));
                break;
            case ExportType::Memory:
                ASSERT(alias->sort() == ComponentSort::CoreMemory);
                m_coreMemories.push_back(instance->memory(it->itemIndex()));
                break;
            case ExportType::Global:
                ASSERT(alias->sort() == ComponentSort::CoreGlobal);
                m_coreGlobals.push_back(instance->global(it->itemIndex()));
                break;
            case ExportType::Tag:
                ASSERT(alias->sort() == ComponentSort::CoreTag);
                m_coreTags.push_back(instance->tag(it->itemIndex()));
                break;
            default:
                RELEASE_ASSERT_NOT_REACHED();
                break;
            }
            return;
        }
    }
    ASSERT(0);
}

void ComponentInstance::aliasInline(ComponentAliasInline* alias)
{
    switch (alias->sort()) {
    case ComponentSort::CoreFunc:
        m_coreFuncs.push_back(m_coreFuncs[alias->exportIndex()]);
        break;
    case ComponentSort::CoreTable:
        m_coreTables.push_back(m_coreTables[alias->exportIndex()]);
        break;
    case ComponentSort::CoreMemory:
        m_coreMemories.push_back(m_coreMemories[alias->exportIndex()]);
        break;
    case ComponentSort::CoreGlobal:
        m_coreGlobals.push_back(m_coreGlobals[alias->exportIndex()]);
        break;
    case ComponentSort::CoreTag:
        m_coreTags.push_back(m_coreTags[alias->exportIndex()]);
        break;
    default:
        RELEASE_ASSERT_NOT_REACHED();
        break;
    }
}

void ComponentInstance::lowerFunction(Store* store, ComponentCanonLower* lower)
{
#ifdef ENABLE_WASI
    LiftedFunction* func = m_funcs[lower->funcIndex()];
    m_coreFuncs.push_back(LoweredFunction::createLoweredFunction(store, func->asLiftedWasiFunction()->functionType(), func));
#endif /* ENABLE_WASI */
}

ComponentInstance* ComponentInstance::instantiate(ExecutionState& state, Store* store, DefinedFunctionTypes& functionTypes, Component* component)
{
    ComponentInstance* instance = new ComponentInstance(component->type());

    for (auto it : component->declarations()) {
        switch (it->kind()) {
        case ComponentDeclaration::CoreInstantiateKind: {
            instance->coreInstantiate(state, store, component, it->asCoreInstantiate());
            break;
        }
        case ComponentDeclaration::AliasExportKind: {
            instance->aliasExport(it->asAliasExport());
            break;
        }
        case ComponentDeclaration::AliasCoreExportKind: {
            instance->aliasCoreExport(it->asAliasExport());
            break;
        }
        case ComponentDeclaration::AliasInlineKind: {
            instance->aliasInline(it->asAliasInline());
            break;
        }
        case ComponentDeclaration::CanonLowerKind: {
            instance->lowerFunction(store, it->asCanonLower());
            break;
        }
        case ComponentDeclaration::CanonResourceDrop: {
            instance->m_coreFuncs.push_back(CanonFunction::createCanonFunction(store, functionTypes[DefinedFunctionTypes::I32R], CanonFunction::ResourceDrop));
            break;
        }
        case ComponentDeclaration::ImportKind: {
            uint32_t index = it->asComponentImport()->importIndex();
            ComponentType::External& external = component->type()->imports()[index];
            bool success = false;

            switch (external.sort) {
            case ComponentSort::Instance: {
#ifdef ENABLE_WASI
                ComponentInstance* importedInstance = wasi02LoadInstance(external, functionTypes);
                if (importedInstance != nullptr) {
                    store->appendComponentInstance(importedInstance);
                    instance->m_instances.push_back(importedInstance);
                    success = true;
                    break;
                }
#endif /* ENABLE_WASI */
                break;
            }
            default:
                break;
            }

            if (!success) {
                std::string message = "Cannot import: ";
                message.append(external.name);
                Trap::throwException(state, message);
            }
            break;
        }
        default:
            // TODO: Implement more declarations.
            store->appendComponentInstance(instance);
            return instance;
        }
    }

    store->appendComponentInstance(instance);
    return instance;
}

ComponentInstance::ComponentInstance(ComponentType* type)
    : Object(GET_GLOBAL_TYPE_INFO(componentInstanceTypeInfo))
    , m_type(type)
{
    type->addRef();
}

ComponentInstance::~ComponentInstance()
{
    m_type->releaseRef();
    for (auto it : m_funcs) {
        it->releaseRef();
    }
}

} // namespace Walrus
