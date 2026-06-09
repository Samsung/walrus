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
    WasiStoreData(int argc, const char** argv, const char** envp, Wasi02DirMap& preOpens);

    uint64_t prevNow() const
    {
        return m_prevNow;
    }

    void setPrevNow(uint64_t value)
    {
        m_prevNow = value;
    }

    clock_t prevClockNow() const
    {
        return m_prevClockNow;
    }

    void setPrevClockNow(clock_t value)
    {
        m_prevClockNow = value;
    }

    const std::vector<std::string>& arguments() const
    {
        return m_arguments;
    }

    const std::vector<std::pair<std::string, std::string>>& environment() const
    {
        return m_environment;
    }

    const std::vector<std::pair<std::string, std::string>>& preOpens() const
    {
        return m_preOpens;
    }

    std::map<size_t, ComponentInstance*>& wasiInstances()
    {
        return m_wasiInstances;
    }

private:
    uint64_t m_prevNow;
    clock_t m_prevClockNow;
    std::vector<std::string> m_arguments;
    std::vector<std::pair<std::string, std::string>> m_environment;
    std::vector<std::pair<std::string, std::string>> m_preOpens;
    std::map<size_t, ComponentInstance*> m_wasiInstances;
};

class WasiRefCountedFile {
public:
    WasiRefCountedFile(FILE* file)
        : m_file(file)
        , m_refCount(1)
    {
    }

    FILE* file()
    {
        return m_file;
    }

    void addRef()
    {
        m_refCount++;
    }

    void releaseRef()
    {
        if (--m_refCount == 0) {
            destroyFile();
        }
    }

private:
    void destroyFile();

    FILE* m_file;
    size_t m_refCount;
};

class ComponentResourceWasiStream : public ComponentResource {
    friend class ComponentResourceWasiPollable;

public:
    static constexpr long int NoSeek = -1;

    ComponentResourceWasiStream(ComponentTypeResource* type, Kind kind, WasiRefCountedFile* file, long int offset)
        : ComponentResource(kind, type)
        , m_file(file)
        , m_pollableCount(0)
        , m_offset(offset)
    {
        ASSERT(kind == ResourceWasiInputStreamKind || kind == ResourceWasiOutputStreamKind);
    }

    virtual ~ComponentResourceWasiStream() override
    {
        if (m_file != nullptr) {
            m_file->releaseRef();
        }
    }

    bool isClosed() const
    {
        return m_file == nullptr;
    }

    FILE* file() const
    {
        ASSERT(!isClosed());
        return m_file->file();
    }

    size_t pollableCount() const
    {
        return m_pollableCount;
    }

    bool seekNeeded() const
    {
        return m_offset != NoSeek;
    }

    long int offset() const
    {
        return m_offset;
    }

    void advanceOffset(size_t bytes)
    {
        if (m_offset != NoSeek) {
            m_offset += static_cast<long int>(bytes);
        }
    }

    void dropFileRef()
    {
        ASSERT(!isClosed());
        m_file->releaseRef();
        m_file = nullptr;
    }

private:
    // The m_file is nullptr for closed streams.
    WasiRefCountedFile* m_file;
    size_t m_pollableCount;
    long int m_offset;
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

class ComponentResourceWasiFile : public ComponentResource {
    friend class ComponentResourceWasiPollable;

public:
    ComponentResourceWasiFile(ComponentTypeResource* type, WasiRefCountedFile* file)
        : ComponentResource(ResourceWasiFileKind, type)
        , m_file(file)
    {
    }

    ~ComponentResourceWasiFile()
    {
        m_file->releaseRef();
    }

    FILE* file() const
    {
        return m_file->file();
    }

    WasiRefCountedFile* addFileRef()
    {
        m_file->addRef();
        return m_file;
    }

private:
    WasiRefCountedFile* m_file;
};

class ComponentResourceWasiDirectory : public ComponentResource {
public:
    ComponentResourceWasiDirectory(ComponentTypeResource* type, const std::string& mappedPath, const std::string& realPath)
        : ComponentResource(ResourceWasiDirectoryKind, type)
        , m_mappedPath(mappedPath)
        , m_realPath(realPath)
    {
    }

    const std::string mappedPath() const
    {
        return m_mappedPath;
    }

    std::string realPath()
    {
        return m_realPath;
    }

private:
    std::string m_mappedPath;
    std::string m_realPath;
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
        neuralNetworkTensorConstructor02,
        neuralNetworkInferenceGraphExecutionContextCompute,
        neuralNetworkGraphInitExectionContext02,
        neuralNetworkGraphLoad02,
    };

    LiftedWasiFunction(Type type, ComponentInstance* instance, FunctionType* functionType)
        : LiftedFunction()
        , m_type(type)
        , m_instance(instance)
        , m_functionType(functionType)
    {
    }

    virtual ~LiftedWasiFunction() override;

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
