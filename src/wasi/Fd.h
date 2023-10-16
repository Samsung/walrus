/*
 * Copyright (c) 2023-present Samsung Electronics Co., Ltd
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

namespace Walrus {

void WASI::fd_write(ExecutionState& state, Value* argv, Value* result, Instance* instance)
{
    uint32_t fd = argv[0].asI32();
    uint32_t iovptr = argv[1].asI32();
    uint32_t iovcnt = argv[2].asI32();
    uint32_t out = argv[3].asI32();
    WASI::wasi_iovec_t wasi_iovs;

    if (!WASI::checkMemOffset(instance->memory(0), iovptr, iovcnt)) {
        result[0] = Value(static_cast<int16_t>(WASI::wasi_errno::inval));
        result[1] = Value(static_cast<int32_t>(0));
        return;
    }

    uint32_t offset = *reinterpret_cast<uint32_t*>(instance->memory(0)->buffer() + iovptr);
    wasi_iovs.buf = reinterpret_cast<uint8_t*>(instance->memory(0)->buffer() + offset);
    wasi_iovs.len = *reinterpret_cast<uint32_t*>(instance->memory(0)->buffer() + iovptr + sizeof(uvwasi_size_t));

    std::vector<uvwasi_ciovec_t> iovs(iovcnt);
    for (uint32_t i = 0; i < iovcnt; i++) {
        iovs[i].buf_len = wasi_iovs.len;
        iovs[0].buf = wasi_iovs.buf;
    }

    uvwasi_size_t out_addr;
    result[0] = Value(static_cast<int16_t>(uvwasi_fd_write(WASI::m_uvwasi, fd, iovs.data(), iovs.size(), &out_addr)));
    *(instance->memory(0)->buffer() + out) = out_addr;
}

} // namespace Walrus
