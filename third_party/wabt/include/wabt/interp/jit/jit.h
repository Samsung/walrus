/*
 * Copyright 2020 WebAssembly Community Group participants
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

#ifndef WABT_JIT_H_
#define WABT_JIT_H_

#include "wabt/common.h"

namespace wabt {
namespace interp {

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

struct ExecutionContext {
  // Describes a function in the call frame
  // chain (stored on the machine stack).
  struct CallFrame {
    // Previous call frame.
    CallFrame* prev_frame;
    // Start address of the data used by the function.
    void* frame_start;
  };

  CallFrame* last_frame;
};

struct JITModuleDescriptor {
  // Update JITCompiler::compile() after this definition is modified.
  typedef void (*ExternalDecl)(ExecutionContext* context,
                               void* aligned_start,
                               void* func_entry);

  JITModuleDescriptor(void* machine_code) : machine_code(machine_code) {}
  ~JITModuleDescriptor();

  // Reference counted because it is often copied by wabt.
  size_t ref_count = 1;
  void* machine_code;
};

class JITCompiler;

class JITFunction {
  friend class JITCompiler;

 public:
  bool isCompiled() const { return func_entry_ != nullptr; }
  void call(const ValueTypes& param_types,
            const Values& params,
            const ValueTypes& result_types,
            Values& results) const;

  void initSizes(Index args_size, Index frame_size) {
    args_size_ = args_size;
    frame_size_ = frame_size;
  }

  Index argsSize() const { return args_size_; }
  Index frameSize() const { return frame_size_; }

 private:
  void* func_entry_;
  JITModuleDescriptor* module_;
  Index args_size_;
  Index frame_size_;
};

class JITModuleData {
  friend class JITCompiler;

 public:
  JITModuleData() : descriptor_(nullptr) {}
  JITModuleData(const JITModuleData& other) : descriptor_(other.descriptor_) {
    if (descriptor_ != nullptr) {
      descriptor_->ref_count++;
    }
  }
  JITModuleData(JITModuleData&& other) : descriptor_(other.descriptor_) {
    other.descriptor_ = nullptr;
  }
  ~JITModuleData() {
    if (descriptor_ != nullptr && --descriptor_->ref_count == 0) {
      delete descriptor_;
    }
  }

  void setDescriptor(JITModuleDescriptor* descriptor) {
    descriptor_ = descriptor;
  }

 private:
  JITModuleDescriptor* descriptor_;
};

}  // namespace interp
}  // namespace wabt

#endif  // WABT_JIT_H_
