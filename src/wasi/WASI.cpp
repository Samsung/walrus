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

#include "wasi/WASI.h"
#include "runtime/Value.h"
#include "runtime/Memory.h"
#include "runtime/Instance.h"
#include "wasi/Fd.h"
#include "wasi/Path.h"
#include "wasi/Environ.h"

// https://github.com/WebAssembly/WASI/blob/main/legacy/preview1/docs.md

namespace Walrus {

uvwasi_t* WASI::g_uvwasi;
WASI::WasiFuncInfo WASI::g_wasiFunctions[WasiFuncIndex::FuncEnd];

void WASI::initialize(uvwasi_t* uvwasi)
{
    ASSERT(!!uvwasi);
    g_uvwasi = uvwasi;

    // fill wasi function table
#define WASI_FUNC_TABLE(NAME, FUNCTYPE)                                                       \
    g_wasiFunctions[WasiFuncIndex::NAME##FUNC].name = #NAME;                                  \
    g_wasiFunctions[WasiFuncIndex::NAME##FUNC].functionType = DefinedFunctionTypes::FUNCTYPE; \
    g_wasiFunctions[WasiFuncIndex::NAME##FUNC].ptr = &WASI::NAME;
    FOR_EACH_WASI_FUNC(WASI_FUNC_TABLE)
#undef WASI_FUNC_TABLE
}

WASI::WasiFuncInfo* WASI::find(const std::string& funcName)
{
    for (unsigned i = 0; i < WasiFuncIndex::FuncEnd; ++i) {
        if (g_wasiFunctions[i].name == funcName) {
            return &g_wasiFunctions[i];
        }
    }
    return nullptr;
}

void WASI::proc_exit(ExecutionState& state, Value* argv, Value* result, Instance* instance)
{
    ASSERT(argv[0].type() == Value::I32);
    uvwasi_proc_exit(WASI::g_uvwasi, argv[0].asI32());
    ASSERT_NOT_REACHED();
}

void WASI::proc_raise(ExecutionState& state, Value* argv, Value* result, Instance* instance)
{
    ASSERT(argv[0].type() == Value::I32);
    result[0] = Value(uvwasi_proc_raise(WASI::g_uvwasi, argv[0].asI32()));
}

void WASI::random_get(ExecutionState& state, Value* argv, Value* result, Instance* instance)
{
    ASSERT(argv[0].type() == Value::I32);
    ASSERT(argv[1].type() == Value::I32);
    if (uint64_t(argv[0].asI32()) + argv[1].asI32() >= instance->memory(0)->sizeInByte()) {
        result[0] = Value(WasiErrNo::inval);
    }

    void* buf = (void*)(instance->memory(0)->buffer() + argv[0].asI32());
    uvwasi_size_t length = argv[1].asI32();
    result[0] = Value(uvwasi_random_get(WASI::g_uvwasi, buf, length));
}
} // namespace Walrus
