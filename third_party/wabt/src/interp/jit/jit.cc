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
#include "wabt/interp/interp.h"

namespace wabt {
namespace interp {

void JITFunction::call(const ValueTypes& param_types,
                       const Values& params,
                       const ValueTypes& result_types,
                       Values& results) const {
  if (export_entry_ == nullptr) {
    return;
  }

  size_t size = 4096;

  const size_t alignment = LocalsAllocator::alignment;
  void* data = malloc(size + alignment + sizeof(ExecutionContext));
  uintptr_t start = reinterpret_cast<uintptr_t>(data);

  start = (start + (alignment - 1)) & ~(alignment - 1);

  ExecutionContext* context = reinterpret_cast<ExecutionContext*>(start);

  context->last_frame = nullptr;
  context->error = ExecutionContext::NoError;

  void* args = reinterpret_cast<void*>(start + size - argsSize());
  module_->exportCall()(context + 1, args, export_entry_);

  StackAllocator* stackAllocator = new StackAllocator();
  for (ValueType result_type : result_types) {
    stackAllocator->push(LocationInfo::typeToValueInfo(result_type));
  }

  int result_index = 0;
  std::vector<LocationInfo>& offsets = stackAllocator->values();
#define push_result(type)                                  \
  results.push_back(Value::Make(*(reinterpret_cast<type*>( \
      reinterpret_cast<u8*>(args) + offsets[result_index].value))));

  for (ValueType result_type : result_types) {
    switch (result_type) {
      case Type::I32:
        push_result(s32);
        break;
      case Type::I64:
        push_result(s64);
        break;
      case Type::F32:
        push_result(f32);
        break;
      case Type::F64:
        push_result(f64);
        break;
      case Type::V128:
        push_result(v128);
        break;
      case Type::FuncRef:
      case Type::ExternRef:
      case Type::Reference:
      case Type::Func:
        push_result(Ref);
        break;
      default:
        WABT_UNREACHABLE;
    }
    result_index++;
  }
#undef push_result
  free(data);
}

}  // namespace interp
}  // namespace wabt
