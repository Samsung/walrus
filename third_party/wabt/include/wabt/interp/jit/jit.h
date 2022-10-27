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

struct ExecutionContext {
  // Describes a function in the stack chain.
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
  typedef void (*ExternalDecl)(ExecutionContext* context, void* aligned_start);

  ~JITModuleDescriptor();

  // Reference counted because it is often copied by wabt.
  size_t ref_count = 1;
  void* machine_code = nullptr;
};

class JITCompiler;

class JITFunction {
  friend class JITCompiler;

 public:
  bool isCompiled() const { return func_entry_ != nullptr; }
  void call() const;

 private:
  void* func_entry_;
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
  JITModuleData(const JITModuleData&& other) : descriptor_(other.descriptor_) {}
  ~JITModuleData() {
    if (descriptor_ != nullptr && --descriptor_->ref_count == 0) {
      delete descriptor_;
    }
  }

 private:
  JITModuleDescriptor* descriptor_;
};

}  // namespace interp
}  // namespace wabt

#endif  // WABT_JIT_H_
