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
#include "wasi/WASI02Impl.h"
#include "runtime/Memory.h"

namespace Walrus {

static void throwNoMemory(ExecutionState& state)
{
    std::string message = "out of memory";
    Trap::throwException(state, message);
}

static ComponentResourceWasiStream* asStream(ComponentHandle* handle)
{
    ASSERT(handle->kind() == ComponentHandle::ResourceWasiStreamKind);
    return reinterpret_cast<ComponentResourceWasiStream*>(handle);
}

void callWasiFunction(ExecutionState& state, Value* argv, Value* result, LiftedWasiFunction* function, CanonOptions* options)
{
    ComponentInstance* instance = function->instance();

    switch (function->type()) {
    case LiftedWasiFunction::ioOutputStreamCheckWrite02: {
        uint32_t offset = argv[1].asI32();
        uint64_t value = 0xffffffff;
        options->memory()->store(state, offset, 8, value);
        options->memory()->buffer()[offset] = 0;
        break;
    }
    case LiftedWasiFunction::ioOutputStreamWrite02: {
        uint32_t index = argv[0].asI32();
        ComponentHandle* handle = options->instance()->getHandle(state, index);
        if (handle->kind() != ComponentHandle::ResourceWasiStreamKind) {
            ComponentInstance::throwInvalidHandle(state, index);
        }

        ASSERT(!options->memory()->is64());
        uint32_t bufferStart = argv[1].asI32();
        uint32_t bufferSize = argv[2].asI32();
        options->memoryCheckRange32(state, 1, bufferStart, bufferSize);
        fwrite(options->memory()->buffer() + bufferStart, bufferSize, 1, asStream(handle)->file());

        uint32_t offset = argv[3].asI32();
        options->memory()->buffer()[offset] = 0;
        break;
    }
    case LiftedWasiFunction::ioOutputStreamBlockingFlush02: {
        uint32_t offset = argv[1].asI32();
        options->memory()->buffer()[offset] = 0;
        break;
    }
    case LiftedWasiFunction::ioOutputStreamSubscribe02: {
        uint32_t index = argv[0].asI32();
        ComponentHandle* handle = options->instance()->getHandle(state, index);
        if (handle->kind() != ComponentHandle::ResourceWasiStreamKind) {
            ComponentInstance::throwInvalidHandle(state, index);
        }

        ComponentResource* resource = new ComponentResourceWasiPollable(instance->type()->getType(0)->asTypeResource(), asStream(handle));
        result[0] = Value(static_cast<int32_t>(options->instance()->appendHandle(state, resource)));
        break;
    }
    case LiftedWasiFunction::cliGetEnvironment02: {
        uint32_t offset = argv[0].asI32();
        const std::vector<std::pair<std::string, std::string>>& environment = instance->store()->wasiData()->environment();

        ASSERT(!options->memory()->is64());
        if (environment.size() >= Memory::s_maxMemory32 >> 4) {
            throwNoMemory(state);
        }

        uint32_t length = static_cast<uint32_t>(environment.size());
        uint32_t start = options->memoryMalloc32(state, 4, length << 4);
        uint32_t* argBuffer = reinterpret_cast<uint32_t*>(options->memory()->buffer() + start);
        options->memory()->store(state, offset, 0, start);
        options->memory()->store(state, offset, 4, length);

        for (auto& it : environment) {
            if (it.first.length() >= Component::MaxStringByteLength || it.second.length() >= Component::MaxStringByteLength) {
                throwNoMemory(state);
            }
            length = static_cast<uint32_t>(it.first.length());
            start = static_cast<uint32_t>(options->storeLatin1String(state, reinterpret_cast<const uint8_t*>(it.first.data()), &length));
            *argBuffer++ = start;
            *argBuffer++ = length;
            length = static_cast<uint32_t>(it.second.length());
            start = static_cast<uint32_t>(options->storeLatin1String(state, reinterpret_cast<const uint8_t*>(it.second.data()), &length));
            *argBuffer++ = start;
            *argBuffer++ = length;
        }
        break;
    }
    case LiftedWasiFunction::cliGetArguments02: {
        uint32_t offset = argv[0].asI32();
        const std::vector<std::string>& arguments = instance->store()->wasiData()->arguments();

        ASSERT(!options->memory()->is64());
        if (arguments.size() >= Memory::s_maxMemory32 >> 3) {
            throwNoMemory(state);
        }

        uint32_t length = static_cast<uint32_t>(arguments.size());
        uint32_t start = options->memoryMalloc32(state, 4, length << 3);
        uint32_t* argBuffer = reinterpret_cast<uint32_t*>(options->memory()->buffer() + start);
        options->memory()->store(state, offset, 0, start);
        options->memory()->store(state, offset, 4, length);

        for (auto& it : arguments) {
            if (it.length() >= Component::MaxStringByteLength) {
                throwNoMemory(state);
            }
            length = static_cast<uint32_t>(it.length());
            start = static_cast<uint32_t>(options->storeLatin1String(state, reinterpret_cast<const uint8_t*>(it.data()), &length));
            *argBuffer++ = start;
            *argBuffer++ = length;
        }
        break;
    }
    case LiftedWasiFunction::cliGetStdout02: {
        ComponentResource* resource = new ComponentResourceWasiStream(instance->type()->getType(0)->asTypeResource(), stdout);
        result[0] = Value(static_cast<int32_t>(options->instance()->appendHandle(state, resource)));
        break;
    }
    case LiftedWasiFunction::cliGetTerminalStdout02: {
        ComponentResource* resource = new ComponentResourceWasiTerminal(instance->type()->getType(0)->asTypeResource(), 1);
        uint32_t offset = argv[0].asI32();
        uint32_t value = options->instance()->appendHandle(state, resource);
        options->memory()->store(state, offset, 4, value);
        options->memory()->buffer()[offset] = 1;
        break;
    }
    default:
        std::string message = "unimplemented wasi function";
        Trap::throwException(state, message);
        break;
    }
}

bool dropWasiResource(ExecutionState& state, ComponentHandle* handle)
{
    switch (handle->kind()) {
    case ComponentHandle::ResourceWasiStreamKind:
        if (asStream(handle)->pollableCount() != 0) {
            std::string message = "stream cannot be destroyed (has assigned pollable)";
            Trap::throwException(state, message);
        }
        break;
    case ComponentHandle::ResourceWasiPollableKind:
        break;
    case ComponentHandle::ResourceWasiTerminalKind:
        break;
    default:
        return false;
    }
    return true;
}

} // namespace Walrus

#endif
