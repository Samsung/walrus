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

void WASI::environ_sizes_get(ExecutionState& state, Value* argv, Value* result, Instance* instance)
{
    uint32_t count = argv[0].asI32();
    uint32_t buf = argv[1].asI32();

    uvwasi_size_t* uvCount = reinterpret_cast<uvwasi_size_t*>(instance->memory(0)->buffer() + count);
    uvwasi_size_t* uvBufSize = reinterpret_cast<uvwasi_size_t*>(instance->memory(0)->buffer() + buf);

    result[0] = Value(static_cast<uint16_t>(uvwasi_environ_sizes_get(WASI::g_uvwasi, uvCount, uvBufSize)));
}

void WASI::environ_get(ExecutionState& state, Value* argv, Value* result, Instance* instance)
{
    uint32_t env = argv[0].asI32();
    uint32_t environBuf = argv[1].asI32();

    char** uvEnviron = reinterpret_cast<char**>(instance->memory(0)->buffer() + env);
    char* uvEnvironBuf = reinterpret_cast<char*>(instance->memory(0)->buffer() + environBuf);

    result[0] = Value(static_cast<uint16_t>(uvwasi_environ_get(WASI::g_uvwasi, uvEnviron, uvEnvironBuf)));
}

} // namespace Walrus
