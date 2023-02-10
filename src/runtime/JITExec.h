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

#include "wabt/common.h"
#include "runtime/Value.h"

namespace Walrus {

/*
  Call frame layout for a JIT function:

  Stack grows downwards.

  The address of the next value increases in the args area and
  decreases in the call frame area. The arrows represents this.

  +-----------------------------+  \
  |                | Locals ->  |   | args area
  |      | params / locals ->   |   } (belongs to the caller)
  | results ->                  |   | size: argsSize()
  +-----------------------------+  /
  | -> locals                   |  \
  |        | stack ->           |   |
  |                             |   |
  +-----------------------------+   } call frame
  |   argument passing area     |   | size: frameSize()
  |  (same structure as above)  |   |
  +-----------------------------+  /
*/

using ValueType = wabt::Type;
using ValueTypes = std::vector<ValueType>;
struct Value;
using Values = std::vector<Value>;

class TrapHandlerList;

// Header of instance const data
struct InstanceConstData {
    TrapHandlerList* trapHandlers;
};

// Header of instance data
struct InstanceData {
    InstanceConstData* constData;
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
        // Function index if the lowest bit is set,
        // InstanceData otherwise
        uintptr_t functionIndex;
    };

    enum ErrorCodes : uint32_t {
        NoError,
        OutOfStackError,
        DivideByZeroError,
        IntegerOverflowError,
        UnreachableError,
    };

    CallFrame* lastFrame;
    InstanceData* currentInstanceData;
    InstanceConstData* currentInstanceConstData;
    ErrorCodes error;
    uint64_t tmp1;
    uint64_t tmp2;
};

class JITModule : public gc {
public:
    // Update JITCompiler::compile() after this definition is modified.
    typedef void (*ExportCall)(ExecutionContext* context,
                               void* alignedStart,
                               void* exportEntry);

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
    // Reference counted because it is often copied by wabt.
    size_t m_refCount = 1;
    InstanceConstData* m_instanceConstData;
    void* m_moduleStart;
};

class JITCompiler;

class JITFunction : public gc {
    friend class JITCompiler;

public:
    bool isCompiled() const { return m_exportEntry != nullptr; }
    void call(ExecutionState& state, const uint32_t argc, Value* argv, Value* result, const ValueTypeVector& resultTypeInfo) const;

    void initSizes(uint32_t argsSize, uint32_t frameSize)
    {
        m_argsSize = argsSize;
        m_frameSize = frameSize;
    }

    uint32_t argsSize() const { return m_argsSize; }
    uint32_t frameSize() const { return m_frameSize; }

private:
    void* m_exportEntry;
    JITModule* m_module;
    uint32_t m_argsSize;
    uint32_t m_frameSize;
};

} // namespace Walrus

#endif // __WalrusJITExec__
