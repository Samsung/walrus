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

namespace Walrus {

WASI::WasiFunc* WASI::find(std::string funcName)
{
    for (unsigned i = 0; i < WasiFuncName::FuncEnd; ++i) {
        if (m_wasiFunctions[i].name == funcName) {
            return &m_wasiFunctions[i];
        }
    }
    return nullptr;
}

void WASI::test(ExecutionState& state, Value* argv, Value* result, void* data)
{
    printf("No argument test succesful.\n");
}

void WASI::printI32(ExecutionState& state, Value* argv, Value* result, void* data)
{
    printf("Recieved number: %d.\n", argv[0].asI32());
}

void WASI::writeI32(ExecutionState& state, Value* argv, Value* result, void* data)
{
    printf("Writing 42 to stack.\n");
    result[0] = Value((int32_t)42);
}

void WASI::proc_exit(ExecutionState& state, Value* argv, Value* result, void* data)
{
    ASSERT(argv[0].type() == Value::I32);
    exit(argv[0].asI32());
    ASSERT_NOT_REACHED();
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
}

} // namespace Walrus
