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

#ifndef __WalrusJITExec__
#define __WalrusJITExec__

#include "interpreter/ByteCode.h"

namespace Walrus {

class TrapHandlerList;

// Header of instance const data
struct InstanceConstData {
    TrapHandlerList* trapHandlers;
};

struct ExecutionContext {
    // Describes a function in the call frame
    // chain (stored on the machine stack).
    struct CallFrame {
        // Previous call frame.
        CallFrame* prevFrame;
        // Start address of the data used by
        // the function (also stored in kFrameReg).
        void* frameStart;
    };

    enum ErrorCodes : uint32_t {
        NoError,
        OutOfStackError,
        DivideByZeroError,
        IntegerOverflowError,
        UnreachableError,
    };

    ExecutionContext(InstanceConstData* currentInstanceConstData)
        : lastFrame(nullptr)
        , currentInstanceConstData(currentInstanceConstData)
        , error(NoError)
    {
    }

    CallFrame* lastFrame;
    InstanceConstData* currentInstanceConstData;
    ErrorCodes error;
    uint64_t tmp1;
    uint64_t tmp2;
};

class JITModule {
public:
    // Update JITCompiler::compile() after this definition is modified.
    typedef ByteCodeStackOffset* (*ExportCall)(ExecutionContext* context, void* alignedStart, void* exportEntry);

    JITModule(InstanceConstData* instanceConstData, void* moduleStart)
        : m_instanceConstData(instanceConstData)
        , m_moduleStart(moduleStart)
    {
    }
    ~JITModule();

    ExportCall exportCall()
    {
        return reinterpret_cast<ExportCall>(m_moduleStart);
    }

    InstanceConstData* instanceConstData() { return m_instanceConstData; }

private:
    InstanceConstData* m_instanceConstData;
    void* m_moduleStart;
};

class JITCompiler;

class JITFunction {
    friend class JITCompiler;

public:
    JITFunction()
        : m_exportEntry(nullptr)
        , m_branchList(nullptr)
        , m_module(nullptr)
    {
    }

    ~JITFunction()
    {
        if (m_branchList != nullptr) {
            free(m_branchList);
        }
    }

    bool isCompiled() const { return m_exportEntry != nullptr; }
    ByteCodeStackOffset* call(ExecutionState& state, uint8_t* bp) const;

private:
    void* m_exportEntry;
    void* m_branchList;
    JITModule* m_module;
};

} // namespace Walrus

#endif // __WalrusJITExec__
