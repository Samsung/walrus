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

#ifndef WABT_JIT_ALLOCATOR_H_
#define WABT_JIT_ALLOCATOR_H_

#include "wabt/common.h"

namespace wabt {
namespace interp {

// Bitset which describes type related data.
typedef uint8_t ValueInfo;

// Describes the location of a value on the stack.
struct LocationInfo {
  // Unspecified or auto-detected value.
  static const ValueInfo kNone = 0;
  // Size bits.
  static const ValueInfo kFourByteSize = 1;
  static const ValueInfo kEightByteSize = 2;
  static const ValueInfo kSixteenByteSize = 3;
  static const ValueInfo kSizeMask = 0x3;
  // Type information bits.
  static const ValueInfo kFloat = 1 << 2;
  static const ValueInfo kReference = 1 << 3;

  // Status bits
  static const uint8_t kIsOffset = 1 << 0;
  static const uint8_t kIsLocal = 1 << 1;
  static const uint8_t kIsRegister = 1 << 2;
  static const uint8_t kIsCallArg = 1 << 3;
  static const uint8_t kIsImmediate = 1 << 4;
  static const uint8_t kIsUnused = 1 << 5;

  LocationInfo(Index value, uint8_t status, ValueInfo value_info)
      : value(value), status(status), value_info(value_info) {}

  static ValueInfo typeToValueInfo(Type type);

  static Index length(ValueInfo value_info) {
    return 0x2u << (value_info & kSizeMask);
  }

  // Possible values of value:
  //  offset - when status has kIsOffset bit set
  //  non-zero - register index
  //  0 - immediate or unused value
  Index value;
  uint8_t status;
  ValueInfo value_info;
};

class StackAllocator {
 public:
  static const Index alignment = sizeof(v128) - 1;

  StackAllocator() {}
  StackAllocator(StackAllocator* other, Index end);

  void push(ValueInfo value_info);
  void pushLocal(Index local, ValueInfo value_info) {
    values_.push_back(LocationInfo(local, LocationInfo::kIsLocal, value_info));
  }
  void pushReg(Index reg, ValueInfo value_info) {
    values_.push_back(LocationInfo(reg, LocationInfo::kIsRegister, value_info));
  }
  void pushCallArg(Index offset, ValueInfo value_info) {
    values_.push_back(
        LocationInfo(offset, LocationInfo::kIsCallArg, value_info));
  }
  void pushImmediate(ValueInfo value_info) {
    values_.push_back(LocationInfo(0, LocationInfo::kIsImmediate, value_info));
  }
  void pushUnused(ValueInfo value_info) {
    values_.push_back(LocationInfo(0, LocationInfo::kIsUnused, value_info));
  }
  void pop();

  const LocationInfo& get(size_t index) { return values_[index]; }
  const LocationInfo& last() { return values_.back(); }
  std::vector<LocationInfo>& values() { return values_; }
  bool empty() { return values_.empty(); }
  Index size() { return size_; }

  // Also resets the allocator.
  void skipRange(Index start, Index end);

  static Index alignedSize(Index size) {
    return (size + (alignment - 1)) & ~(alignment - 1);
  }

 private:
  std::vector<LocationInfo> values_;
  Index size_ = 0;
  // Values cannot be allocated in the skipped region
  Index skip_start_ = 0;
  Index skip_end_ = 0;
};

class LocalsAllocator {
 public:
  static const Index alignment = StackAllocator::alignment;

  LocalsAllocator(Index start) : size_(start) {}

  void allocate(ValueInfo value_info);

  const LocationInfo& get(size_t index) { return values_[index]; }
  const LocationInfo& last() { return values_.back(); }
  Index size() { return size_; }

 private:
  std::vector<LocationInfo> values_;
  Index size_;
  // Due to the allocation algorithm, at most one 4
  // and one 8 byte space can be free at any time.
  Index unused_four_byte_end_ = 0;
  Index unused_eight_byte_end_ = 0;
};

}  // namespace interp
}  // namespace wabt

#endif  // WABT_JIT_ALLOCATOR_H_
