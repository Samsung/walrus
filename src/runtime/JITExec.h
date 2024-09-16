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
#include "runtime/Instance.h"
#include "runtime/Memory.h"

namespace Walrus {

class Exception;
class Memory;
class InstanceConstData;

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
        CapturedException,
        OutOfStackError,
        DivideByZeroError,
        IntegerOverflowError,
        OutOfBoundsMemAccessError,
        OutOfBoundsTableAccessError,
        TypeMismatchError,
        UndefinedElementError,
        UninitializedElementError,
        IndirectCallTypeMismatchError,
        InvalidConversionToIntegerError,
        UnreachableError,
#if defined(ENABLE_EXTENDED_FEATURES)
        UnalignedAtomicError,
        ExpectedSharedMemError,
#endif /* ENABLE_EXTENDED_FEATURES */

        // These three in this order must be the last items of the list.
        GenericTrap, // Error code received in SLJIT_R0.
        ReturnToLabel, // Used for returning with an exception.
        ErrorCodesEnd,
    };

    ExecutionContext(InstanceConstData* currentInstanceConstData, ExecutionState& state, Instance* instance)
        : lastFrame(nullptr)
        , currentInstanceConstData(currentInstanceConstData)
        , state(state)
        , instance(instance)
        , capturedException(nullptr)
        , error(NoError)
    {
    }

    CallFrame* lastFrame;
    InstanceConstData* currentInstanceConstData;
    ExecutionState& state;
    Instance* instance;
    Exception* capturedException;
    Memory::TargetBuffer memory0;
    ErrorCodes error;
    uint64_t tmp1[2];
    uint64_t tmp2[2];
};

class JITModule {
    friend class JITCompiler;

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
    // Does not include m_moduleStart code block
    std::vector<void*> m_codeBlocks;
};

class JITFunction {
    friend class JITCompiler;

public:
    JITFunction()
        : m_exportEntry(nullptr)
        , m_constData(nullptr)
        , m_module(nullptr)
    {
    }

    ~JITFunction()
    {
        if (m_constData != nullptr) {
            free(m_constData);
        }
    }

    bool isCompiled() const { return m_exportEntry != nullptr; }
    ByteCodeStackOffset* call(ExecutionState& state, Instance* instance, uint8_t* bp) const;

private:
    void* m_exportEntry;
    void* m_constData;
    JITModule* m_module;
};

} // namespace Walrus

#endif // __WalrusJITExec__
