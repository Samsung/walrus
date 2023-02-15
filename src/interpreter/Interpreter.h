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

#ifndef __WalrusInterpreter__
#define __WalrusInterpreter__

#include "runtime/ExecutionState.h"
#include "interpreter/ByteCode.h"

namespace Walrus {

class Instance;
class Memory;
class Table;
class Global;

class Interpreter {
public:
    static ByteCodeStackOffset* interpret(ExecutionState& state,
                                          uint8_t* bp);

private:
    friend class OpcodeTable;
    static ByteCodeStackOffset* interpret(ExecutionState& state,
                                          size_t programCounter,
                                          uint8_t* bp,
                                          Instance* instance,
                                          const std::vector<std::shared_ptr<Memory>>& memories,
                                          const std::vector<std::shared_ptr<Table>>& tables,
                                          const std::vector<std::shared_ptr<Global>>& globals);

    static void callOperation(ExecutionState& state,
                              size_t& programCounter,
                              uint8_t* bp,
                              Instance* instance);

    static void callIndirectOperation(ExecutionState& state,
                                      size_t& programCounter,
                                      uint8_t* bp,
                                      Instance* instance);
};

} // namespace Walrus

#endif // __WalrusOpcode__
