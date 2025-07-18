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

#ifndef __WalrusWASMParser__
#define __WalrusWASMParser__

#include "runtime/Module.h"

namespace Walrus {

class Module;
class Store;

struct WASMParsingResult {
    // should be allocated in the stack
    MAKE_STACK_ALLOCATED();

    WASMParsingResult();

    void clear();

    bool m_seenStartAttribute;
    bool m_typesAddedToStore;
    uint32_t m_version;
    uint32_t m_start;

    Vector<ImportType*> m_imports;
    Vector<ExportType*> m_exports;

    Vector<ModuleFunction*> m_functions;

    Vector<Data*> m_datas;
    Vector<Element*> m_elements;

    Vector<FunctionType*> m_functionTypes;
    Vector<GlobalType*> m_globalTypes;
    Vector<TableType*> m_tableTypes;
    Vector<MemoryType*> m_memoryTypes;
    Vector<TagType*> m_tagTypes;
};

class WASMParser {
public:
    // returns <result, error>
    static std::pair<Optional<Module*>, std::string> parseBinary(Store* store, const std::string& filename, const uint8_t* data, size_t len, const uint32_t JITFlags = 0, const uint32_t featureFlags = 0);
};

} // namespace Walrus

#endif // __WalrusParser__
