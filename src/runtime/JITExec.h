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
    TrapHandlerList* trap_handlers;
};

// Header of instance data
struct InstanceData {
    InstanceConstData* const_data;
};

struct ExecutionContext {
    // Describes a function in the call frame
    // chain (stored on the machine stack).
    struct CallFrame {
        // Previous call frame.
        CallFrame* prev_frame;
        // Start address of the data used by
        // the function (also stored in kFrameReg).
        void* frame_start;
        // Function index if the lowest bit is set,
        // InstanceData otherwise
        uintptr_t function_index;
    };

    enum ErrorCodes : uint32_t {
        NoError,
        OutOfStackError,
        DivideByZeroError,
        IntegerOverflowError,
        UnreachableError,
    };

    CallFrame* last_frame;
    InstanceData* current_instance_data;
    InstanceConstData* current_instance_const_data;
    ErrorCodes error;
    uint64_t tmp1;
    uint64_t tmp2;
};

class JITModule : public gc {
public:
    // Update JITCompiler::compile() after this definition is modified.
    typedef void (*ExportCall)(ExecutionContext* context,
                               void* aligned_start,
                               void* export_entry);

    JITModule(InstanceConstData* instance_const_data, void* module_start)
        : instance_const_data_(instance_const_data)
        , module_start_(module_start)
    {
    }
    ~JITModule();

    ExportCall exportCall()
    {
        return reinterpret_cast<ExportCall>(module_start_);
    }

    InstanceConstData* instanceConstData() { return instance_const_data_; }

private:
    // Reference counted because it is often copied by wabt.
    size_t ref_count_ = 1;
    InstanceConstData* instance_const_data_;
    void* module_start_;
};

class JITCompiler;

class JITFunction : public gc {
    friend class JITCompiler;

public:
    bool isCompiled() const { return export_entry_ != nullptr; }
    void call(ExecutionState& state, const uint32_t argc, Value* argv, Value* result, const ValueTypeVector& resultTypeInfo) const;

    void initSizes(uint32_t args_size, uint32_t frame_size)
    {
        args_size_ = args_size;
        frame_size_ = frame_size;
    }

    uint32_t argsSize() const { return args_size_; }
    uint32_t frameSize() const { return frame_size_; }

private:
    void* export_entry_;
    JITModule* module_;
    uint32_t args_size_;
    uint32_t frame_size_;
};

} // namespace Walrus

#endif // __WalrusJITExec__
