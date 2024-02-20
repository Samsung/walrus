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

void WASI::path_open(ExecutionState& state, Value* argv, Value* result, Instance* instance)
{
    uint32_t fd = argv[0].asI32();
    uint32_t dirflags = argv[1].asI32();
    uint32_t path_offset = argv[2].asI32();
    uint32_t len = argv[3].asI32();
    uint32_t oflags = argv[4].asI32();
    uint64_t rights = argv[5].asI64();
    uint64_t right_inheriting = argv[6].asI64();
    uint32_t fdflags = argv[7].asI32();
    uint32_t ret_fd_offset = argv[8].asI32();

    uvwasi_fd_t* ret_fd = reinterpret_cast<uvwasi_fd_t*>(instance->memory(0)->buffer() + ret_fd_offset);

    const char* path = reinterpret_cast<char*>(instance->memory(0)->buffer() + path_offset);

    result[0] = Value(static_cast<uint16_t>(
        uvwasi_path_open(WASI::g_uvwasi,
                         fd,
                         dirflags,
                         path,
                         len,
                         oflags,
                         rights,
                         right_inheriting,
                         fdflags,
                         ret_fd)));
}

} // namespace Walrus
