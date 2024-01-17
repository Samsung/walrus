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

    if (!WASI::checkMemOffset(instance->memory(0), iovptr, iovcnt)) {
        result[0] = Value(static_cast<int16_t>(WASI::wasi_errno::inval));
        return;
    }

    std::vector<uvwasi_ciovec_t> iovs(iovcnt);
    for (uint32_t i = 0; i < iovcnt; i++) {
        iovs[i].buf = instance->memory(0)->buffer() + *reinterpret_cast<uint32_t*>(instance->memory(0)->buffer() + iovptr + i * 8);
        iovs[i].buf_len = *reinterpret_cast<uint32_t*>(instance->memory(0)->buffer() + iovptr + 4 + i * 8);
    }

    uvwasi_size_t* out_addr = (uvwasi_size_t*)(instance->memory(0)->buffer() + out);

    result[0] = Value(static_cast<int16_t>(uvwasi_fd_write(WASI::m_uvwasi, fd, iovs.data(), iovs.size(), out_addr)));
    *(instance->memory(0)->buffer() + out) = *out_addr;
}

void WASI::fd_read(ExecutionState& state, Value* argv, Value* result, Instance* instance)
{
    uint32_t fd = argv[0].asI32();
    uint32_t iovptr = argv[1].asI32();
    uint32_t iovcnt = argv[2].asI32();
    uint32_t out = argv[3].asI32();

    std::vector<uvwasi_iovec_t> iovs(iovcnt);
    for (uint32_t i = 0; i < iovcnt; i++) {
        iovs[i].buf = instance->memory(0)->buffer() + *reinterpret_cast<uint32_t*>(instance->memory(0)->buffer() + iovptr + i * 8);
        iovs[i].buf_len = *reinterpret_cast<uint32_t*>(instance->memory(0)->buffer() + iovptr + 4 + i * 8);
    }

    uvwasi_size_t* out_addr = (uvwasi_size_t*)(instance->memory(0)->buffer() + out);

    result[0] = Value(static_cast<int16_t>(uvwasi_fd_read(WASI::m_uvwasi, fd, iovs.data(), iovs.size(), out_addr)));
    *(instance->memory(0)->buffer() + out) = *out_addr;
}

void WASI::fd_close(ExecutionState& state, Value* argv, Value* result, Instance* instance)
{
    uint32_t fd = argv[0].asI32();

    result[0] = Value(static_cast<int16_t>(uvwasi_fd_close(WASI::m_uvwasi, fd)));
}

void WASI::fd_seek(ExecutionState& state, Value* argv, Value* result, Instance* instance)
{
    uint32_t fd = argv[0].asI32();
    uint64_t fileDelta = argv[1].asI32();
    uint32_t whence = argv[2].asI32();
    uint64_t newOffset = argv[3].asI32();

    uvwasi_filesize_t out_addr = *(instance->memory(0)->buffer() + newOffset);

    result[0] = Value(static_cast<int16_t>(uvwasi_fd_seek(WASI::m_uvwasi, fd, fileDelta, whence, &out_addr)));
    *(instance->memory(0)->buffer() + newOffset) = out_addr;
}

} // namespace Walrus
