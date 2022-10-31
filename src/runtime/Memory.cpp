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
#include "Walrus.h"

#include "Memory.h"

namespace Walrus {

Memory::Memory(size_t initialSizeInByte, size_t maximumSizeInByte)
    : m_sizeInByte(initialSizeInByte)
    , m_maximumSizeInByte(maximumSizeInByte)
    , m_buffer(reinterpret_cast<uint8_t*>(calloc(1, initialSizeInByte)))
{
    RELEASE_ASSERT(m_buffer);
    GC_REGISTER_FINALIZER_NO_ORDER(this, [](void* obj, void* cd) {
        free(reinterpret_cast<Memory*>(obj)->m_buffer);
    },
                                   nullptr, nullptr, nullptr);
}

bool Memory::grow(size_t growSizeInByte)
{
    size_t newSizeInByte = growSizeInByte + m_sizeInByte;
    if (newSizeInByte > m_sizeInByte && newSizeInByte <= m_maximumSizeInByte) {
        uint8_t* newBuffer = reinterpret_cast<uint8_t*>(calloc(1, newSizeInByte));
        if (newBuffer) {
            memcpy(newBuffer, m_buffer, m_sizeInByte);
            free(m_buffer);
            m_buffer = newBuffer;
            m_sizeInByte = newSizeInByte;
            return true;
        }
    }
    return false;
}

} // namespace Walrus
