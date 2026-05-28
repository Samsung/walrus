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

#ifndef __WalrusWASI02Impl__
#define __WalrusWASI02Impl__

#ifdef ENABLE_WASI

#include "Walrus.h"
#include "runtime/Component.h"
#include "runtime/ComponentInstance.h"

namespace Walrus {

class WasiStoreData {
public:
    WasiStoreData(int argc, const char** argv, const char** envp);

    const std::vector<std::string>& arguments() const
    {
        return m_arguments;
    }

    const std::vector<std::pair<std::string, std::string>>& environment() const
    {
        return m_environment;
    }

    std::map<size_t, ComponentInstance*>& wasiInstances()
    {
        return m_wasiInstances;
    }

private:
    std::vector<std::string> m_arguments;
    std::vector<std::pair<std::string, std::string>> m_environment;
    std::map<size_t, ComponentInstance*> m_wasiInstances;
};

class ComponentResourceWasiStream : public ComponentResource {
    friend class ComponentResourceWasiPollable;

public:
    ComponentResourceWasiStream(ComponentTypeResource* type, FILE* file)
        : ComponentResource(ResourceWasiStreamKind, type)
        , m_file(file)
        , m_pollableCount(0)
    {
    }

    FILE* file() const
    {
        return m_file;
    }

    size_t pollableCount() const
    {
        return m_pollableCount;
    }

private:
    FILE* m_file;
    size_t m_pollableCount;
};

class ComponentResourceWasiPollable : public ComponentResource {
public:
    ComponentResourceWasiPollable(ComponentTypeResource* type, ComponentResourceWasiStream* stream)
        : ComponentResource(ResourceWasiPollableKind, type)
        , m_stream(stream)
    {
        stream->m_pollableCount++;
    }

    ~ComponentResourceWasiPollable()
    {
        m_stream->m_pollableCount--;
    }

    ComponentResourceWasiStream* stream() const
    {
        return m_stream;
    }

private:
    ComponentResourceWasiStream* m_stream;
};

class ComponentResourceWasiTerminal : public ComponentResource {
public:
    ComponentResourceWasiTerminal(ComponentTypeResource* type, int fileNo)
        : ComponentResource(ResourceWasiTerminalKind, type)
        , m_fileNo(fileNo)
    {
    }

    int fileNo() const
    {
        return m_fileNo;
    }

private:
    int m_fileNo;
};

class LiftedWasiFunction : public LiftedFunction {
public:
    enum Type {
        cliExit02,
        ioPollableBlock02,
        ioPoll02,
        ioInputStreamRead02,
        ioInputStreamSubscribe02,
        ioOutputStreamCheckWrite02,
        ioOutputStreamWrite02,
        ioOutputStreamBlockingWriteAndFlush02,
        ioOutputStreamBlockingFlush02,
        ioOutputStreamSubscribe02,
        cliGetEnvironment02,
        cliGetArguments02,
        cliGetStdin02,
        cliGetStdout02,
        cliGetStderr02,
        cliGetTerminalStdin02,
        cliGetTerminalStdout02,
        cliGetTerminalStderr02,
        clockMonotonicNow02,
        clockSubscribeDuration02,
        clockWallNow02,
        fileSystemDescriptorReadViaStream02,
        fileSystemDescriptorWriteViaStream02,
        fileSystemDescriptorAppendViaStream02,
        fileSystemDescriptorGetFlags02,
        fileSystemDescriptorStat02,
        fileSystemDescriptorOpenAt02,
        fileSystemDescriptorMetadataHash02,
        fileSystemGetDirectories02,
    };

    LiftedWasiFunction(Type type, ComponentInstance* instance, FunctionType* functionType)
        : LiftedFunction()
        , m_type(type)
        , m_instance(instance)
        , m_functionType(functionType)
    {
    }

    virtual Kind kind() const override
    {
        return WasiFunctionKind;
    }

    Type type() const
    {
        return m_type;
    }

    ComponentInstance* instance() const
    {
        return m_instance;
    }

    FunctionType* functionType() const
    {
        return m_functionType;
    }

private:
    Type m_type;
    ComponentInstance* m_instance;
    FunctionType* m_functionType;
};

} // namespace Walrus

#endif

#endif // __WalrusWASI02Impl__
