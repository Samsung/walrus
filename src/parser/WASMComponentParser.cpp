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
#include "Walrus.h"

#include "parser/WASMComponentParser.h"
#include "runtime/Store.h"
#include "runtime/TypeStore.h"

#include "wabt/binary-reader.h"
#include "wabt/walrus/binary-reader-walrus.h"

namespace wabt {

class WASMComponentBinaryReader : public wabt::WASMComponentBinaryReaderDelegate {
private:
    Walrus::TypeStore& m_typeStore;

public:
    WASMComponentBinaryReader(Walrus::TypeStore& typeStore)
        : m_typeStore(typeStore)
    {
    }

    void OnCoreModule(const void* data,
                      size_t size,
                      const ReadBinaryOptions& options)
    {
    }

    void BeginComponent(uint32_t version, size_t depth)
    {
    }

    void EndComponent()
    {
    }

    void BeginCoreInstance(Index module_index,
                           uint32_t argument_count)
    {
    }

    void OnCoreInstanceArg(const ComponentStringLoc& name,
                           ComponentSort sort,
                           Index index)
    {
    }

    void EndCoreInstance()
    {
    }

    void BeginInlineCoreInstance(uint32_t argument_count)
    {
    }

    void OnInlineCoreInstanceArg(const ComponentStringLoc& name,
                                 ComponentSort sort,
                                 Index index)
    {
    }

    void EndInlineCoreInstance()
    {
    }

    void BeginInstance(Index component_index,
                       uint32_t argument_count)
    {
    }

    void OnInstanceArg(const ComponentStringLoc& name,
                       ComponentSort sort,
                       Index index)
    {
    }

    void EndInstance()
    {
    }

    void BeginInlineInstance(uint32_t argument_count)
    {
    }

    void OnInlineInstanceArg(const ComponentStringLoc& name,
                             nonstd::string_view* version_suffix,
                             ComponentSort sort,
                             Index index)
    {
    }

    void EndInlineInstance()
    {
    }

    void OnAliasExport(ComponentSort sort,
                       Index instance_index,
                       const ComponentStringLoc& name)
    {
    }

    void OnAliasCoreExport(ComponentSort sort,
                           Index core_instance_index,
                           const ComponentStringLoc& name)
    {
    }

    void OnAliasOuter(ComponentSort sort,
                      uint32_t counter,
                      uint32_t index)
    {
    }

    void OnPrimitiveType(const ComponentType& type)
    {
    }

    void BeginRecordType(uint32_t field_count)
    {
    }

    void OnRecordField(const ComponentStringLoc& field_name,
                       const ComponentType& field_type)
    {
    }

    void EndRecordType()
    {
    }

    void BeginVariantType(uint32_t case_count)
    {
    }

    void OnVariantCase(const ComponentStringLoc& case_name,
                       const ComponentType& case_type)
    {
    }

    void EndVariantType()
    {
    }

    void OnListType(const ComponentType& type)
    {
    }

    void OnListFixedType(const ComponentType& type,
                         uint32_t size)
    {
    }

    void BeginTupleType(uint32_t item_count)
    {
    }

    void OnTupleItem(const ComponentType& item)
    {
    }

    void EndTupleType()
    {
    }

    void BeginFlagsType(uint32_t label_count)
    {
    }

    void OnFlagsLabel(const ComponentStringLoc& label)
    {
    }

    void EndFlagsType()
    {
    }

    void BeginEnumType(uint32_t label_count)
    {
    }

    void OnEnumLabel(const ComponentStringLoc& label)
    {
    }

    void EndEnumType()
    {
    }

    void OnOptionType(const ComponentType& type)
    {
    }

    void OnResultType(const ComponentType& result_type,
                      const ComponentType& error_type)
    {
    }

    void OnOwnType(Index index)
    {
    }

    void OnBorrowType(Index index)
    {
    }

    void OnStreamType(const ComponentType& type)
    {
    }

    void OnFutureType(const ComponentType& type)
    {
    }

    void BeginFuncType(ComponentTypeDef type,
                       uint32_t param_count)
    {
    }

    void OnFuncParam(ComponentStringLoc name,
                     const ComponentType& type)
    {
    }

    void OnFuncResult(const ComponentType& type)
    {
    }

    void EndFuncType()
    {
    }

    void OnResourceType(ComponentResourceRep rep,
                        Index dtor)
    {
    }

    void OnResourceAsyncType(ComponentResourceRep rep,
                             Index dtor,
                             Index callback)
    {
    }

    void BeginInstanceType(uint32_t count)
    {
    }

    void EndInstanceType()
    {
    }

    void BeginComponentType(uint32_t count)
    {
    }

    void EndComponentType()
    {
    }

    void OnCanonLift(Index core_func_index,
                     uint32_t option_count,
                     ComponentCanonOption* options,
                     Index type_index)
    {
    }

    void OnCanonLower(Index func_index,
                      uint32_t option_count,
                      ComponentCanonOption* options)
    {
    }

    void OnCanonType(ComponentCanon canon,
                     Index type_index)
    {
    }

    void OnImport(const ComponentStringLoc& name,
                  nonstd::string_view* version_suffix,
                  const ComponentExternalInfo& external_info)
    {
    }

    void OnExport(const ComponentStringLoc& name,
                  nonstd::string_view* version_suffix,
                  ComponentExternalInfo* external_info,
                  ComponentExportInfo* export_info)
    {
    }
};

} // namespace wabt

namespace Walrus {

std::pair<Optional<Component*>, std::string> WASMComponentParser::parseBinary(Store* store, const std::string& filename, const uint8_t* data, size_t len, const uint32_t JITFlags, const uint32_t featureFlags)
{
    wabt::WASMComponentBinaryReader delegate(store->getTypeStore());

    std::string error = ReadWasmComponentBinary(filename, data, len, &delegate, featureFlags);
    if (error.length()) {
        return std::make_pair(nullptr, error);
    }

    return std::make_pair(nullptr, std::string());
}

} // namespace Walrus
