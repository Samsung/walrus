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

#include "wasi/Wasi.h"
#include "wasi/Fd.h"

// https://github.com/WebAssembly/WASI/blob/main/legacy/preview1/docs.md

namespace Walrus {

WASI::WasiFunc WASI::m_wasiFunctions[FuncEnd];
uvwasi_t* WASI::m_uvwasi;

WASI::WasiFunc* WASI::find(std::string funcName)
{
    for (unsigned i = 0; i < WasiFuncName::FuncEnd; ++i) {
        if (m_wasiFunctions[i].name == funcName) {
            return &m_wasiFunctions[i];
        }
    }
    return nullptr;
}

bool WASI::checkStr(Memory* memory, uint32_t memoryOffset, std::string& str)
{
    for (uint32_t i = memoryOffset; i < memory->sizeInByte(); ++i) {
        if (memoryOffset >= memory->sizeInByte()) {
            return false;
        } else if (*reinterpret_cast<char*>(memory->buffer() + memoryOffset + i) == '\0') {
            str = std::string(reinterpret_cast<char*>(memory->buffer() + memoryOffset));
            break;
        }
    }

    return true;
}

bool WASI::checkMemOffset(Memory* memory, uint32_t memoryOffset, uint32_t length)
{
    if (memoryOffset + length >= memory->sizeInByte()) {
        return false;
    }

    return true;
}

void WASI::proc_exit(ExecutionState& state, Value* argv, Value* result, Instance* instance)
{
    ASSERT(argv[0].type() == Value::I32);
    exit(argv[0].asI32());
    ASSERT_NOT_REACHED();
}

void WASI::proc_raise(ExecutionState& state, Value* argv, Value* result, Instance* instance)
{
    ASSERT(argv[0].type() == Value::I32);
    result[0] = Value(uvwasi_proc_raise(WASI::m_uvwasi, argv[0].asI32()));
}

void WASI::random_get(ExecutionState& state, Value* argv, Value* result, Instance* instance)
{
    ASSERT(argv[0].type() == Value::I32);
    ASSERT(argv[1].type() == Value::I32);
    if (!WASI::checkMemOffset(instance->memory(0), argv[0].asI32(), argv[1].asI32())) {
        result[0] = Value(WASI::wasi_errno::inval);
    }

    void* buf = (void*)(instance->memory(0)->buffer() + argv[0].asI32());
    uvwasi_size_t length = argv[1].asI32();
    result[0] = Value(uvwasi_random_get(WASI::m_uvwasi, buf, length));
}

void WASI::fillWasiFuncTable()
{
#define WASI_FUNC_TABLE(NAME, FUNCTYPE)                                                             \
    m_wasiFunctions[WASI::WasiFuncName::NAME##FUNC].name = #NAME;                                   \
    m_wasiFunctions[WASI::WasiFuncName::NAME##FUNC].functionType = SpecTestFunctionTypes::FUNCTYPE; \
    m_wasiFunctions[WASI::WasiFuncName::NAME##FUNC].ptr = &WASI::NAME;
    FOR_EACH_WASI_FUNC(WASI_FUNC_TABLE)
#undef WASI_FUNC_TABLE
}

WASI::WASI()
{
    fillWasiFuncTable();

    uvwasi_t uvwasi;
    WASI::m_uvwasi = reinterpret_cast<uvwasi_t*>(malloc(sizeof(uvwasi_t)));

    uvwasi_options_t init_options;
    init_options.in = 0;
    init_options.out = 1;
    init_options.err = 2;
    init_options.fd_table_size = 3;
    init_options.argc = 0;
    init_options.argv = nullptr;
    init_options.envp = nullptr;
    init_options.preopenc = 0;
    init_options.preopen_socketc = 0;
    init_options.allocator = nullptr;

    uvwasi_errno_t err = uvwasi_init(WASI::m_uvwasi, &init_options);
    assert(err == UVWASI_ESUCCESS);
}

} // namespace Walrus
