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

#include "wabt/interp/jit/jit-allocator.h"

namespace wabt {
namespace interp {

ValueInfo LocationInfo::typeToValueInfo(Type type) {
  switch (type) {
    case Type::I32:
      return kFourByteSize;
    case Type::I64:
      return kEightByteSize;
    case Type::F32:
      return kFloat | kFourByteSize;
    case Type::F64:
      return kFloat | kEightByteSize;
    case Type::V128:
      return kSixteenByteSize;
    case Type::FuncRef:
    case Type::ExternRef:
    case Type::Reference:
      return kReference | (sizeof(void*) == 8 ? kEightByteSize : kFourByteSize);
    default:
      WABT_UNREACHABLE;
  }
  return 0;
}

StackAllocator::StackAllocator(StackAllocator* other, Index end)
    : values_(other->values_.begin(), other->values_.begin() + end),
      skip_start_(other->skip_start_),
      skip_end_(other->skip_end_) {
  while (end > 0) {
    --end;
    LocationInfo& info = values_[end];
    if (info.status & LocationInfo::kIsOffset) {
      size_ = info.value + LocationInfo::length(info.value_info);
      return;
    }
  }
}

void StackAllocator::push(ValueInfo value_info) {
  size_t mask;

  switch (value_info & LocationInfo::kSizeMask) {
    case LocationInfo::kFourByteSize:
      mask = sizeof(uint32_t) - 1;
      break;
    case LocationInfo::kEightByteSize:
      mask = sizeof(uint64_t) - 1;
      break;
    default:
      assert((value_info & LocationInfo::kSizeMask) ==
             LocationInfo::kSixteenByteSize);
      mask = sizeof(v128) - 1;
      break;
  }

  size_ = (size_ + mask) & ~mask;

  if (size_ >= skip_start_ && size_ < skip_end_) {
    size_ = (skip_end_ + mask) & ~mask;
  }

  values_.push_back(LocationInfo(size_, LocationInfo::kIsOffset, value_info));
  size_ += (mask + 1);
}

void StackAllocator::pop() {
  assert(values_.size() > 0);

  uint8_t status = values_.back().status;

  values_.pop_back();

  if (!(status & LocationInfo::kIsOffset)) {
    return;
  }

  for (size_t i = values_.size(); i > 0; --i) {
    LocationInfo& location = values_[i - 1];

    if (location.status & LocationInfo::kIsOffset) {
      size_ = location.value + LocationInfo::length(location.value_info);
      return;
    }
  }

  size_ = 0;
}

void StackAllocator::skipRange(Index start, Index end) {
  assert(skip_start_ == 0 && skip_end_ == 0);
  assert(start <= end && size_ <= start);

  if (start < end) {
    skip_start_ = start;
    skip_end_ = end;
  }

  size_ = 0;
  values_.clear();
}

void LocalsAllocator::allocate(ValueInfo value_info) {
  Index offset;

  // Allocate value at the end unless there is a free
  // space for the value. Allocating large values might
  // create free space for smaller values.
  switch (value_info & LocationInfo::kSizeMask) {
    case LocationInfo::kFourByteSize:
      if (unused_four_byte_end_ != 0) {
        offset = unused_four_byte_end_ - sizeof(uint32_t);
        unused_four_byte_end_ = 0;
        break;
      }

      if (unused_eight_byte_end_ != 0) {
        offset = unused_eight_byte_end_ - sizeof(uint64_t);
        unused_four_byte_end_ = unused_eight_byte_end_;
        unused_eight_byte_end_ = 0;
        break;
      }

      offset = size_;
      size_ += sizeof(uint32_t);
      break;
    case LocationInfo::kEightByteSize:
      if (unused_eight_byte_end_ != 0) {
        offset = unused_eight_byte_end_ - sizeof(uint64_t);
        unused_eight_byte_end_ = 0;
        break;
      }

      if (size_ & sizeof(uint32_t)) {
        // When a four byte space is allocated at the end,
        // it means there was no unused 4 byte available.
        assert(unused_four_byte_end_ == 0);
        size_ += sizeof(uint32_t);
        unused_four_byte_end_ = size_;
      }

      offset = size_;
      size_ += sizeof(uint64_t);
      break;
    default:
      assert((value_info & LocationInfo::kSizeMask) ==
             LocationInfo::kSixteenByteSize);
      if (size_ & (sizeof(v128) - 1)) {
        // Four byte alignment checked first
        // to ensure eight byte alignment.
        if (size_ & sizeof(uint32_t)) {
          assert(unused_four_byte_end_ == 0);
          size_ += sizeof(uint32_t);
          unused_four_byte_end_ = size_;
        }

        if (size_ & sizeof(uint64_t)) {
          assert(unused_eight_byte_end_ == 0);
          size_ += sizeof(uint64_t);
          unused_eight_byte_end_ = size_;
        }
      }

      offset = size_;
      size_ += sizeof(v128);
      break;
  }

  values_.push_back(LocationInfo(offset, LocationInfo::kIsOffset, value_info));
}

}  // namespace interp
}  // namespace wabt
