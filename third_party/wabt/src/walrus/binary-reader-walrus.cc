/*
 * Copyright 2016 WebAssembly Community Group participants
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

#include <map>
#include <set>

#include "wabt/binary-reader.h"
#include "wabt/feature.h"
#include "wabt/shared-validator.h"
#include "wabt/stream.h"

#include "wabt/walrus/binary-reader-walrus.h"

namespace wabt {

class BinaryReaderDelegateWalrus : public BinaryReaderDelegate {
public:
    BinaryReaderDelegateWalrus(WASMBinaryReaderDelegate* delegate)
        : m_externalDelegate(delegate)
    {

    }

    bool OnError(const Error&) override {
        return false;
    }

    /* Module */
    Result BeginModule(uint32_t version) override {
        m_externalDelegate->BeginModule(version);
        return Result::Ok;
    }
    Result EndModule() override {
        m_externalDelegate->EndModule();
        return Result::Ok;
    }

    Result BeginSection(Index section_index, BinarySection section_type, Offset size) override {
        return Result::Ok;
    }

    /* Custom section */
    Result BeginCustomSection(Index section_index, Offset size, std::string_view section_name) override {
        abort();
        return Result::Ok;
    }
    Result EndCustomSection() override {
        abort();
        return Result::Ok;
    }

    /* Type section */
    Result BeginTypeSection(Offset size) override {
        return Result::Ok;
    }
    Result OnTypeCount(Index count) override {
        m_externalDelegate->OnTypeCount(count);
        return Result::Ok;
    }
    Result OnFuncType(Index index, Index param_count, Type *param_types, Index result_count, Type *result_types) override {
        m_externalDelegate->OnFuncType(index, param_count, param_types, result_count, result_types);
        return Result::Ok;
    }
    Result OnStructType(Index index, Index field_count, TypeMut *fields) override {
        abort();
        return Result::Ok;
    }
    Result OnArrayType(Index index, TypeMut field) override {
        abort();
        return Result::Ok;
    }
    Result EndTypeSection() override {
        return Result::Ok;
    }

    /* Import section */
    Result BeginImportSection(Offset size) override {
        return Result::Ok;
    }
    Result OnImportCount(Index count) override {
        m_externalDelegate->OnImportCount(count);
        return Result::Ok;
    }
    Result OnImport(Index index, ExternalKind kind, std::string_view module_name, std::string_view field_name) override {
        return Result::Ok;
    }
    Result OnImportFunc(Index import_index, std::string_view module_name, std::string_view field_name, Index func_index, Index sig_index) override {
        m_externalDelegate->OnImportFunc(import_index, std::string(module_name), std::string(field_name), func_index, sig_index);
        return Result::Ok;
    }
    Result OnImportTable(Index import_index, std::string_view module_name, std::string_view field_name, Index table_index, Type elem_type, const Limits *elem_limits) override {
        abort();
        return Result::Ok;
    }
    Result OnImportMemory(Index import_index, std::string_view module_name, std::string_view field_name, Index memory_index, const Limits *page_limits) override {
        abort();
        return Result::Ok;
    }
    Result OnImportGlobal(Index import_index, std::string_view module_name, std::string_view field_name, Index global_index, Type type, bool mutable_) override {
        abort();
        return Result::Ok;
    }
    Result OnImportTag(Index import_index, std::string_view module_name, std::string_view field_name, Index tag_index, Index sig_index) override {
        abort();
        return Result::Ok;
    }
    Result EndImportSection() override {
        return Result::Ok;
    }

    /* Function section */
    Result BeginFunctionSection(Offset size) override {
        return Result::Ok;
    }
    Result OnFunctionCount(Index count) override {
        m_externalDelegate->OnFunctionCount(count);
        return Result::Ok;
    }
    Result OnFunction(Index index, Index sig_index) override {
        m_externalDelegate->OnFunction(index, sig_index);
        return Result::Ok;
    }
    Result EndFunctionSection() override {
        return Result::Ok;
    }

    /* Table section */
    Result BeginTableSection(Offset size) override {
        abort();
        return Result::Ok;
    }
    Result OnTableCount(Index count) override {
        abort();
        return Result::Ok;
    }
    Result OnTable(Index index, Type elem_type, const Limits *elem_limits) override {
        abort();
        return Result::Ok;
    }
    Result EndTableSection() override {
        abort();
        return Result::Ok;
    }

    /* Memory section */
    Result BeginMemorySection(Offset size) override {
        abort();
        return Result::Ok;
    }
    Result OnMemoryCount(Index count) override {
        abort();
        return Result::Ok;
    }
    Result OnMemory(Index index, const Limits *limits) override {
        abort();
        return Result::Ok;
    }
    Result EndMemorySection() override {
        abort();
        return Result::Ok;
    }

    /* Global section */
    Result BeginGlobalSection(Offset size) override {
        abort();
        return Result::Ok;
    }
    Result OnGlobalCount(Index count) override {
        abort();
        return Result::Ok;
    }
    Result BeginGlobal(Index index, Type type, bool mutable_) override {
        abort();
        return Result::Ok;
    }
    Result BeginGlobalInitExpr(Index index) override {
        abort();
        return Result::Ok;
    }
    Result EndGlobalInitExpr(Index index) override {
        abort();
        return Result::Ok;
    }
    Result EndGlobal(Index index) override {
        abort();
        return Result::Ok;
    }
    Result EndGlobalSection() override {
        abort();
        return Result::Ok;
    }

    /* Exports section */
    Result BeginExportSection(Offset size) override {
        abort();
        return Result::Ok;
    }
    Result OnExportCount(Index count) override {
        abort();
        return Result::Ok;
    }
    Result OnExport(Index index, ExternalKind kind, Index item_index, std::string_view name) override {
        abort();
        return Result::Ok;
    }
    Result EndExportSection() override {
        abort();
        return Result::Ok;
    }

    /* Start section */
    Result BeginStartSection(Offset size) override {
        return Result::Ok;
    }
    Result OnStartFunction(Index func_index) override {
        m_externalDelegate->OnStartFunction(func_index);
        return Result::Ok;
    }
    Result EndStartSection() override {
        return Result::Ok;
    }

    /* Code section */
    Result BeginCodeSection(Offset size) override {
        return Result::Ok;
    }
    Result OnFunctionBodyCount(Index count) override {
        return Result::Ok;
    }
    Result BeginFunctionBody(Index index, Offset size) override {
        m_externalDelegate->BeginFunctionBody(index, size);
        return Result::Ok;
    }
    Result OnLocalDeclCount(Index count) override {
        return Result::Ok;
    }
    Result OnLocalDecl(Index decl_index, Index count, Type type) override {
        abort();
        return Result::Ok;
    }

    /* Function expressions; called between BeginFunctionBody and
     EndFunctionBody */
    Result OnOpcode(Opcode opcode) override {
        Opcode::Enum e = opcode;
        m_externalDelegate->OnOpcode(e);
        return Result::Ok;
    }
    Result OnOpcodeBare() override {
        abort();
        return Result::Ok;
    }
    Result OnOpcodeIndex(Index value) override {
        return Result::Ok;
    }
    Result OnOpcodeIndexIndex(Index value, Index value2) override {
        abort();
        return Result::Ok;
    }
    Result OnOpcodeUint32(uint32_t value) override {
        return Result::Ok;
    }
    Result OnOpcodeUint32Uint32(uint32_t value, uint32_t value2) override {
        abort();
        return Result::Ok;
    }
    Result OnOpcodeUint32Uint32Uint32(uint32_t value, uint32_t value2, uint32_t value3) override {
        abort();
        return Result::Ok;
    }
    Result OnOpcodeUint32Uint32Uint32Uint32(uint32_t value, uint32_t value2, uint32_t value3, uint32_t value4) override {
        abort();
        return Result::Ok;
    }
    Result OnOpcodeUint64(uint64_t value) override {
        return Result::Ok;
    }
    Result OnOpcodeF32(uint32_t value) override {
        return Result::Ok;
    }
    Result OnOpcodeF64(uint64_t value) override {
        return Result::Ok;
    }
    Result OnOpcodeV128(v128 value) override {
        abort();
        return Result::Ok;
    }
    Result OnOpcodeBlockSig(Type sig_type) override {
        abort();
        return Result::Ok;
    }
    Result OnOpcodeType(Type type) override {
        abort();
        return Result::Ok;
    }
    Result OnAtomicLoadExpr(Opcode opcode, Index memidx, Address alignment_log2, Address offset) override {
        abort();
        return Result::Ok;
    }
    Result OnAtomicStoreExpr(Opcode opcode, Index memidx, Address alignment_log2, Address offset) override {
        abort();
        return Result::Ok;
    }
    Result OnAtomicRmwExpr(Opcode opcode, Index memidx, Address alignment_log2, Address offset) override {
        abort();
        return Result::Ok;
    }
    Result OnAtomicRmwCmpxchgExpr(Opcode opcode, Index memidx, Address alignment_log2, Address offset) override {
        abort();
        return Result::Ok;
    }
    Result OnAtomicWaitExpr(Opcode, Index, Address, Address) override {
        abort();
        return Result::Ok;
    }
    Result OnAtomicFenceExpr(uint32_t) override {
        abort();
        return Result::Ok;
    }
    Result OnAtomicNotifyExpr(Opcode, Index, Address, Address) override {
        abort();
        return Result::Ok;
    }
    Result OnBinaryExpr(Opcode opcode) override {
        abort();
        return Result::Ok;
    }
    Result OnBlockExpr(Type sig_type) override {
        abort();
        return Result::Ok;
    }
    Result OnBrExpr(Index depth) override {
        abort();
        return Result::Ok;
    }
    Result OnBrIfExpr(Index depth) override {
        abort();
        return Result::Ok;
    }
    Result OnBrTableExpr(Index num_targets, Index *target_depths, Index default_target_depth) override {
        abort();
        return Result::Ok;
    }
    Result OnCallExpr(Index func_index) override {
        m_externalDelegate->OnCallExpr(func_index);
        return Result::Ok;
    }
    Result OnCallIndirectExpr(Index sig_index, Index table_index) override {
        abort();
        return Result::Ok;
    }
    Result OnCallRefExpr() override {
        abort();
        return Result::Ok;
    }
    Result OnCatchExpr(Index tag_index) override {
        abort();
        return Result::Ok;
    }
    Result OnCatchAllExpr() override {
        abort();
        return Result::Ok;
    }
    Result OnCompareExpr(Opcode opcode) override {
        abort();
        return Result::Ok;
    }
    Result OnConvertExpr(Opcode opcode) override {
        abort();
        return Result::Ok;
    }
    Result OnDelegateExpr(Index depth) override {
        abort();
        return Result::Ok;
    }
    Result OnDropExpr() override {
        abort();
        return Result::Ok;
    }
    Result OnElseExpr() override {
        abort();
        return Result::Ok;
    }
    Result OnEndExpr() override {
        m_externalDelegate->OnEndExpr();
        return Result::Ok;
    }
    Result OnF32ConstExpr(uint32_t value_bits) override {
        float* f = reinterpret_cast<float*>(&value_bits);
        m_externalDelegate->OnF32ConstExpr(*f);
        return Result::Ok;
    }
    Result OnF64ConstExpr(uint64_t value_bits) override {
        double* f = reinterpret_cast<double*>(&value_bits);
        m_externalDelegate->OnF64ConstExpr(*f);
        return Result::Ok;
    }
    Result OnV128ConstExpr(v128 value_bits) override {
        abort();
        return Result::Ok;
    }
    Result OnGlobalGetExpr(Index global_index) override {
        abort();
        return Result::Ok;
    }
    Result OnGlobalSetExpr(Index global_index) override {
        abort();
        return Result::Ok;
    }
    Result OnI32ConstExpr(uint32_t value) override {
        m_externalDelegate->OnI32ConstExpr(value);
        return Result::Ok;
    }
    Result OnI64ConstExpr(uint64_t value) override {
        m_externalDelegate->OnI64ConstExpr(value);
        return Result::Ok;
    }
    Result OnIfExpr(Type sig_type) override {
        abort();
        return Result::Ok;
    }
    Result OnLoadExpr(Opcode opcode, Index memidx, Address alignment_log2, Address offset) override {
        abort();
        return Result::Ok;
    }
    Result OnLocalGetExpr(Index local_index) override {
        abort();
        return Result::Ok;
    }
    Result OnLocalSetExpr(Index local_index) override {
        abort();
        return Result::Ok;
    }
    Result OnLocalTeeExpr(Index local_index) override {
        abort();
        return Result::Ok;
    }
    Result OnLoopExpr(Type sig_type) override {
        abort();
        return Result::Ok;
    }
    Result OnMemoryCopyExpr(Index srcmemidx, Index destmemidx) override {
        abort();
        return Result::Ok;
    }
    Result OnDataDropExpr(Index segment_index) override {
        abort();
        return Result::Ok;
    }
    Result OnMemoryFillExpr(Index memidx) override {
        abort();
        return Result::Ok;
    }
    Result OnMemoryGrowExpr(Index memidx) override {
        abort();
        return Result::Ok;
    }
    Result OnMemoryInitExpr(Index segment_index, Index memidx) override {
        abort();
        return Result::Ok;
    }
    Result OnMemorySizeExpr(Index memidx) override {
        abort();
        return Result::Ok;
    }
    Result OnTableCopyExpr(Index dst_index, Index src_index) override {
        abort();
        return Result::Ok;
    }
    Result OnElemDropExpr(Index segment_index) override {
        abort();
        return Result::Ok;
    }
    Result OnTableInitExpr(Index segment_index, Index table_index) override {
        abort();
        return Result::Ok;
    }
    Result OnTableGetExpr(Index table_index) override {
        abort();
        return Result::Ok;
    }
    Result OnTableSetExpr(Index table_index) override {
        abort();
        return Result::Ok;
    }
    Result OnTableGrowExpr(Index table_index) override {
        abort();
        return Result::Ok;
    }
    Result OnTableSizeExpr(Index table_index) override {
        abort();
        return Result::Ok;
    }
    Result OnTableFillExpr(Index table_index) override {
        abort();
        return Result::Ok;
    }
    Result OnRefFuncExpr(Index func_index) override {
        abort();
        return Result::Ok;
    }
    Result OnRefNullExpr(Type type) override {
        abort();
        return Result::Ok;
    }
    Result OnRefIsNullExpr() override {
        abort();
        return Result::Ok;
    }
    Result OnNopExpr() override {
        abort();
        return Result::Ok;
    }
    Result OnRethrowExpr(Index depth) override {
        abort();
        return Result::Ok;
    }
    Result OnReturnCallExpr(Index sig_index) override {
        abort();
        return Result::Ok;
    }
    Result OnReturnCallIndirectExpr(Index sig_index, Index table_index) override {
        abort();
        return Result::Ok;
    }
    Result OnReturnExpr() override {
        abort();
        return Result::Ok;
    }
    Result OnSelectExpr(Index result_count, Type *result_types) override {
        abort();
        return Result::Ok;
    }
    Result OnStoreExpr(Opcode opcode, Index memidx, Address alignment_log2, Address offset) override {
        abort();
        return Result::Ok;
    }
    Result OnThrowExpr(Index depth) override {
        abort();
        return Result::Ok;
    }
    Result OnTryExpr(Type sig_type) override {
        abort();
        return Result::Ok;
    }
    Result OnUnaryExpr(Opcode opcode) override {
        abort();
        return Result::Ok;
    }
    Result OnTernaryExpr(Opcode opcode) override {
        abort();
        return Result::Ok;
    }
    Result OnUnreachableExpr() override {
        abort();
        return Result::Ok;
    }
    Result EndFunctionBody(Index index) override {
        m_externalDelegate->EndFunctionBody(index);
        return Result::Ok;
    }
    Result EndCodeSection() override {
        return Result::Ok;
    }
    Result OnSimdLaneOpExpr(Opcode opcode, uint64_t value) override {
        abort();
        return Result::Ok;
    }
    Result OnSimdLoadLaneExpr(Opcode opcode, Index memidx, Address alignment_log2, Address offset, uint64_t value) override {
        abort();
        return Result::Ok;
    }
    Result OnSimdStoreLaneExpr(Opcode opcode, Index memidx, Address alignment_log2, Address offset, uint64_t value) override {
        abort();
        return Result::Ok;
    }
    Result OnSimdShuffleOpExpr(Opcode opcode, v128 value) override {
        abort();
        return Result::Ok;
    }
    Result OnLoadSplatExpr(Opcode opcode, Index memidx, Address alignment_log2, Address offset) override {
        abort();
        return Result::Ok;
    }
    Result OnLoadZeroExpr(Opcode opcode, Index memidx, Address alignment_log2, Address offset) override {
        abort();
        return Result::Ok;
    }

    /* Elem section */
    Result BeginElemSection(Offset size) override {
        abort();
        return Result::Ok;
    }
    Result OnElemSegmentCount(Index count) override {
        abort();
        return Result::Ok;
    }
    Result BeginElemSegment(Index index, Index table_index, uint8_t flags) override {
        abort();
        return Result::Ok;
    }
    Result BeginElemSegmentInitExpr(Index index) override {
        abort();
        return Result::Ok;
    }
    Result EndElemSegmentInitExpr(Index index) override {
        abort();
        return Result::Ok;
    }
    Result OnElemSegmentElemType(Index index, Type elem_type) override {
        abort();
        return Result::Ok;
    }
    Result OnElemSegmentElemExprCount(Index index, Index count) override {
        abort();
        return Result::Ok;
    }
    Result OnElemSegmentElemExpr_RefNull(Index segment_index, Type type) override {
        abort();
        return Result::Ok;
    }
    Result OnElemSegmentElemExpr_RefFunc(Index segment_index, Index func_index) override {
        abort();
        return Result::Ok;
    }
    Result EndElemSegment(Index index) override {
        abort();
        return Result::Ok;
    }
    Result EndElemSection() override {
        abort();
        return Result::Ok;
    }

    /* Data section */
    Result BeginDataSection(Offset size) override {
        abort();
        return Result::Ok;
    }
    Result OnDataSegmentCount(Index count) override {
        abort();
        return Result::Ok;
    }
    Result BeginDataSegment(Index index, Index memory_index, uint8_t flags) override {
        abort();
        return Result::Ok;
    }
    Result BeginDataSegmentInitExpr(Index index) override {
        abort();
        return Result::Ok;
    }
    Result EndDataSegmentInitExpr(Index index) override {
        abort();
        return Result::Ok;
    }
    Result OnDataSegmentData(Index index, const void *data, Address size) override {
        abort();
        return Result::Ok;
    }
    Result EndDataSegment(Index index) override {
        abort();
        return Result::Ok;
    }
    Result EndDataSection() override {
        abort();
        return Result::Ok;
    }

    /* DataCount section */
    Result BeginDataCountSection(Offset size) override {
        abort();
        return Result::Ok;
    }
    Result OnDataCount(Index count) override {
        abort();
        return Result::Ok;
    }
    Result EndDataCountSection() override {
        abort();
        return Result::Ok;
    }

    /* Names section */
    Result BeginNamesSection(Offset size) override {
        abort();
        return Result::Ok;
    }
    Result OnModuleNameSubsection(Index index, uint32_t name_type, Offset subsection_size) override {
        abort();
        return Result::Ok;
    }
    Result OnModuleName(std::string_view name) override {
        abort();
        return Result::Ok;
    }
    Result OnFunctionNameSubsection(Index index, uint32_t name_type, Offset subsection_size) override {
        abort();
        return Result::Ok;
    }
    Result OnFunctionNamesCount(Index num_functions) override {
        abort();
        return Result::Ok;
    }
    Result OnFunctionName(Index function_index, std::string_view function_name) override {
        abort();
        return Result::Ok;
    }
    Result OnLocalNameSubsection(Index index, uint32_t name_type, Offset subsection_size) override {
        abort();
        return Result::Ok;
    }
    Result OnLocalNameFunctionCount(Index num_functions) override {
        abort();
        return Result::Ok;
    }
    Result OnLocalNameLocalCount(Index function_index, Index num_locals) override {
        abort();
        return Result::Ok;
    }
    Result OnLocalName(Index function_index, Index local_index, std::string_view local_name) override {
        abort();
        return Result::Ok;
    }
    Result EndNamesSection() override {
        abort();
        return Result::Ok;
    }

    Result OnNameSubsection(Index index, NameSectionSubsection subsection_type, Offset subsection_size) override {
        abort();
        return Result::Ok;
    }
    Result OnNameCount(Index num_names) override {
        abort();
        return Result::Ok;
    }
    Result OnNameEntry(NameSectionSubsection type, Index index, std::string_view name) override {
        abort();
        return Result::Ok;
    }

    /* Reloc section */
    Result BeginRelocSection(Offset size) override {
        abort();
        return Result::Ok;
    }
    Result OnRelocCount(Index count, Index section_code) override {
        abort();
        return Result::Ok;
    }
    Result OnReloc(RelocType type, Offset offset, Index index, uint32_t addend) override {
        abort();
        return Result::Ok;
    }
    Result EndRelocSection() override {
        abort();
        return Result::Ok;
    }

    /* Tag section */
    Result BeginTagSection(Offset size) override {
        abort();
        return Result::Ok;
    }
    Result OnTagCount(Index count) override {
        abort();
        return Result::Ok;
    }
    Result OnTagType(Index index, Index sig_index) override {
        abort();
        return Result::Ok;
    }
    Result EndTagSection() override {
        abort();
        return Result::Ok;
    }

    /* Code Metadata sections */
    Result BeginCodeMetadataSection(std::string_view name, Offset size) override {
        abort();
        return Result::Ok;
    }
    Result OnCodeMetadataFuncCount(Index count) override {
        abort();
        return Result::Ok;
    }
    Result OnCodeMetadataCount(Index function_index, Index count) override {
        abort();
        return Result::Ok;
    }
    Result OnCodeMetadata(Offset offset, const void *data, Address size) override {
        abort();
        return Result::Ok;
    }
    Result EndCodeMetadataSection() override {
        abort();
        return Result::Ok;
    }

    /* Dylink section */
    Result BeginDylinkSection(Offset size) override {
        abort();
        return Result::Ok;
    }
    Result OnDylinkInfo(uint32_t mem_size, uint32_t mem_align, uint32_t table_size, uint32_t table_align) override {
        abort();
        return Result::Ok;
    }
    Result OnDylinkNeededCount(Index count) override {
        abort();
        return Result::Ok;
    }
    Result OnDylinkNeeded(std::string_view so_name) override {
        abort();
        return Result::Ok;
    }
    Result OnDylinkImportCount(Index count) override {
        abort();
        return Result::Ok;
    }
    Result OnDylinkExportCount(Index count) override {
        abort();
        return Result::Ok;
    }
    Result OnDylinkImport(std::string_view module, std::string_view name, uint32_t flags) override {
        abort();
        return Result::Ok;
    }
    Result OnDylinkExport(std::string_view name, uint32_t flags) override {
        abort();
        return Result::Ok;
    }
    Result EndDylinkSection() override {
        abort();
        return Result::Ok;
    }

    /* target_features section */
    Result BeginTargetFeaturesSection(Offset size) override {
        abort();
        return Result::Ok;
    }
    Result OnFeatureCount(Index count) override {
        abort();
        return Result::Ok;
    }
    Result OnFeature(uint8_t prefix, std::string_view name) override {
        abort();
        return Result::Ok;
    }
    Result EndTargetFeaturesSection() override {
        abort();
        return Result::Ok;
    }

    /* Linking section */
    Result BeginLinkingSection(Offset size) override {
        abort();
        return Result::Ok;
    }
    Result OnSymbolCount(Index count) override {
        abort();
        return Result::Ok;
    }
    Result OnDataSymbol(Index index, uint32_t flags, std::string_view name, Index segment, uint32_t offset, uint32_t size) override {
        abort();
        return Result::Ok;
    }
    Result OnFunctionSymbol(Index index, uint32_t flags, std::string_view name, Index func_index) override {
        abort();
        return Result::Ok;
    }
    Result OnGlobalSymbol(Index index, uint32_t flags, std::string_view name, Index global_index) override {
        abort();
        return Result::Ok;
    }
    Result OnSectionSymbol(Index index, uint32_t flags, Index section_index) override {
        abort();
        return Result::Ok;
    }
    Result OnTagSymbol(Index index, uint32_t flags, std::string_view name, Index tag_index) override {
        abort();
        return Result::Ok;
    }
    Result OnTableSymbol(Index index, uint32_t flags, std::string_view name, Index table_index) override {
        abort();
        return Result::Ok;
    }
    Result OnSegmentInfoCount(Index count) override {
        abort();
        return Result::Ok;
    }
    Result OnSegmentInfo(Index index, std::string_view name, Address alignment, uint32_t flags) override {
        abort();
        return Result::Ok;
    }
    Result OnInitFunctionCount(Index count) override {
        abort();
        return Result::Ok;
    }
    Result OnInitFunction(uint32_t priority, Index function_index) override {
        abort();
        return Result::Ok;
    }
    Result OnComdatCount(Index count) override {
        abort();
        return Result::Ok;
    }
    Result OnComdatBegin(std::string_view name, uint32_t flags, Index count) override {
        abort();
        return Result::Ok;
    }
    Result OnComdatEntry(ComdatType kind, Index index) override {
        abort();
        return Result::Ok;
    }
    Result EndLinkingSection() override {
        abort();
        return Result::Ok;
    }

    WASMBinaryReaderDelegate* m_externalDelegate;
};

bool ReadWasmBinary(const uint8_t* data, size_t size, WASMBinaryReaderDelegate* delegate)
{
    const bool kReadDebugNames = true;
    const bool kStopOnFirstError = true;
    const bool kFailOnCustomSectionError = true;
    Features features;
    features.EnableAll();
    ReadBinaryOptions options(features, nullptr, kReadDebugNames,
                             kStopOnFirstError, kFailOnCustomSectionError);
    BinaryReaderDelegateWalrus binaryReaderDelegateEscargot(delegate);
    ReadBinary(data, size, &binaryReaderDelegateEscargot, options);

    return true;
}

}  // namespace wabt
