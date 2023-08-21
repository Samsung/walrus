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

#include "Walrus.h"
#include "runtime/Value.h"
#include "runtime/Function.h"
#include "runtime/ObjectType.h"
#include "runtime/SpecTest.h"

namespace Walrus {

class WASI {
public:
    WASI();

    ~WASI()
    {
    }

    struct WasiFunc {
        std::string name;
        SpecTestFunctionTypes::Index functionType;
        ImportedFunction::ImportedFunctionCallback ptr;
    };

#define FOR_EACH_WASI_FUNC(F) \
    F(test, NONE)             \
    F(printI32, I32R)         \
    F(writeI32, RI32)         \
    F(proc_exit, I32R)

    enum WasiFuncName : size_t {
#define DECLARE_FUNCTION(NAME, FUNCTYPE) NAME##FUNC,
        FOR_EACH_WASI_FUNC(DECLARE_FUNCTION)
#undef DECLARE_FUNCTION
            FuncEnd,
    };

    void fillWasiFuncTable();
    WasiFunc* find(std::string funcName);

    static void test(ExecutionState& state, Value* argv, Value* result, void* data);
    static void printI32(ExecutionState& state, Value* argv, Value* result, void* data);
    static void writeI32(ExecutionState& state, Value* argv, Value* result, void* data);
    static void proc_exit(ExecutionState& state, Value* argv, Value* result, void* data);

    WasiFunc m_wasiFunctions[FuncEnd];
};

} // namespace Walrus
