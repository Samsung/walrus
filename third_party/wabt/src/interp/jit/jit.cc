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

#include "wabt/interp/jit/jit.h"
#include "wabt/interp/jit/jit-backend.h"

namespace wabt {
namespace interp {

void JITFunction::call() const {
  if (func_entry_ == nullptr) {
    return;
  }

  size_t size = 4096;

  const size_t alignment = LocalsAllocator::alignment;
  void* data = malloc(size + alignment + sizeof(ExecutionContext));
  uintptr_t start = reinterpret_cast<uintptr_t>(data);

  start = (start + (alignment - 1)) & ~(alignment - 1);

  ExecutionContext* context = reinterpret_cast<ExecutionContext*>(start + size);

  context->last_frame = nullptr;

  union {
    void* func_entry;
    JITModuleDescriptor::ExternalDecl code;
  } func;

  func.func_entry = func_entry_;
  func.code(context, reinterpret_cast<void*>(start));
  free(data);
}

}  // namespace interp
}  // namespace wabt
