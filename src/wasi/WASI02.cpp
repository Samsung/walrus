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

#ifdef ENABLE_WASI

#include "wasi/WASI02.h"
#include "runtime/ComponentInstance.h"
#include "runtime/DefinedFunctionTypes.h"

namespace Walrus {

class ComponentInstanceWasi02 {
public:
    static void addExport(ComponentInstance* instance, const char* name, LiftedWasiFunction::Type type, FunctionType* functionType);
    static ComponentInstance* loadInstance(ComponentType::External& external, DefinedFunctionTypes& functionTypes);
};

void ComponentInstanceWasi02::addExport(ComponentInstance* instance, const char* name, LiftedWasiFunction::Type type, FunctionType* functionType)
{
    instance->m_type->exports().push_back(ComponentType::External{ name, nullptr, ComponentSort::Func, static_cast<uint32_t>(instance->m_funcs.size()) });
    instance->m_funcs.push_back(new LiftedWasiFunction(type, functionType));
}

ComponentInstance* ComponentInstanceWasi02::loadInstance(ComponentType::External& external, DefinedFunctionTypes& functionTypes)
{
    if (external.name == "wasi:cli/exit@0.2.6") {
        ComponentType* type = new ComponentType(ComponentType::ComponentTypeKind);
        ComponentInstance* instance = new ComponentInstance(type);
        addExport(instance, "exit", LiftedWasiFunction::cliExit026, functionTypes[DefinedFunctionTypes::I32R]);
        type->releaseRef();
        return instance;
    }
    if (external.name == "wasi:io/error@0.2.6") {
        ComponentType* type = new ComponentType(ComponentType::ComponentTypeKind);
        ComponentInstance* instance = new ComponentInstance(type);
        type->releaseRef();
        return instance;
    }
    if (external.name == "wasi:io/poll@0.2.6") {
        ComponentType* type = new ComponentType(ComponentType::ComponentTypeKind);
        ComponentInstance* instance = new ComponentInstance(type);
        addExport(instance, "[method]pollable.block", LiftedWasiFunction::ioPollableBlock026, functionTypes[DefinedFunctionTypes::I32R]);
        type->releaseRef();
        return instance;
    }
    if (external.name == "wasi:io/streams@0.2.6") {
        ComponentType* type = new ComponentType(ComponentType::ComponentTypeKind);
        ComponentInstance* instance = new ComponentInstance(type);
        addExport(instance, "[method]output-stream.check-write", LiftedWasiFunction::ioOutputStreamCheckWrite026, functionTypes[DefinedFunctionTypes::I32I32R]);
        addExport(instance, "[method]output-stream.write", LiftedWasiFunction::ioOutputStreamWrite026, functionTypes[DefinedFunctionTypes::I32I32I32I32R]);
        addExport(instance, "[method]output-stream.blocking-write-and-flush", LiftedWasiFunction::ioOutputStreamBlockingWriteAndFlush026, functionTypes[DefinedFunctionTypes::I32I32I32I32R]);
        addExport(instance, "[method]output-stream.blocking-flush", LiftedWasiFunction::ioOutputStreamBlockingFlush026, functionTypes[DefinedFunctionTypes::I32I32R]);
        addExport(instance, "[method]output-stream.subscribe", LiftedWasiFunction::ioOutputStreamSubscribe026, functionTypes[DefinedFunctionTypes::I32_RI32]);
        type->releaseRef();
        return instance;
    }
    if (external.name == "wasi:cli/stdin@0.2.6") {
        ComponentType* type = new ComponentType(ComponentType::ComponentTypeKind);
        ComponentInstance* instance = new ComponentInstance(type);
        addExport(instance, "get-stdin", LiftedWasiFunction::cliGetStdin026, functionTypes[DefinedFunctionTypes::RI32]);
        type->releaseRef();
        return instance;
    }
    if (external.name == "wasi:cli/stdout@0.2.6") {
        ComponentType* type = new ComponentType(ComponentType::ComponentTypeKind);
        ComponentInstance* instance = new ComponentInstance(type);
        addExport(instance, "get-stdout", LiftedWasiFunction::cliGetStdout026, functionTypes[DefinedFunctionTypes::RI32]);
        type->releaseRef();
        return instance;
    }
    if (external.name == "wasi:cli/stderr@0.2.6") {
        ComponentType* type = new ComponentType(ComponentType::ComponentTypeKind);
        ComponentInstance* instance = new ComponentInstance(type);
        addExport(instance, "get-stderr", LiftedWasiFunction::cliGetStderr026, functionTypes[DefinedFunctionTypes::RI32]);
        type->releaseRef();
        return instance;
    }
    if (external.name == "wasi:cli/terminal-input@0.2.6") {
        ComponentType* type = new ComponentType(ComponentType::ComponentTypeKind);
        ComponentInstance* instance = new ComponentInstance(type);
        type->releaseRef();
        return instance;
    }
    if (external.name == "wasi:cli/terminal-output@0.2.6") {
        ComponentType* type = new ComponentType(ComponentType::ComponentTypeKind);
        ComponentInstance* instance = new ComponentInstance(type);
        type->releaseRef();
        return instance;
    }
    if (external.name == "wasi:cli/terminal-stdin@0.2.6") {
        ComponentType* type = new ComponentType(ComponentType::ComponentTypeKind);
        ComponentInstance* instance = new ComponentInstance(type);
        addExport(instance, "get-terminal-stdin", LiftedWasiFunction::cliGetTerminalStdin026, functionTypes[DefinedFunctionTypes::I32R]);
        type->releaseRef();
        return instance;
    }
    if (external.name == "wasi:cli/terminal-stdout@0.2.6") {
        ComponentType* type = new ComponentType(ComponentType::ComponentTypeKind);
        ComponentInstance* instance = new ComponentInstance(type);
        addExport(instance, "get-terminal-stdout", LiftedWasiFunction::cliGetTerminalStdout026, functionTypes[DefinedFunctionTypes::I32R]);
        type->releaseRef();
        return instance;
    }
    if (external.name == "wasi:cli/terminal-stderr@0.2.6") {
        ComponentType* type = new ComponentType(ComponentType::ComponentTypeKind);
        ComponentInstance* instance = new ComponentInstance(type);
        addExport(instance, "get-terminal-stderr", LiftedWasiFunction::cliGetTerminalStderr026, functionTypes[DefinedFunctionTypes::I32R]);
        type->releaseRef();
        return instance;
    }
    return nullptr;
}

ComponentInstance* wasi02LoadInstance(ComponentType::External& external, DefinedFunctionTypes& functionTypes)
{
    return ComponentInstanceWasi02::loadInstance(external, functionTypes);
}

} // namespace Walrus

#endif
