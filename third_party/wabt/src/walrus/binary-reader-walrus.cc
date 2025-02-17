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
#include <limits>

#include "wabt/binary-reader.h"
#include "wabt/feature.h"
#include "wabt/shared-validator.h"
#include "wabt/stream.h"

#include "wabt/walrus/binary-reader-walrus.h"

#define EXECUTE_VALIDATOR(expr)                                      \
  do {                                                               \
    if (WABT_UNLIKELY(state->offset <= m_externalDelegate->          \
            skipValidationUntil())) {                                \
        break;                                                       \
    }                                                                \
    expr;                                                            \
  } while (0)

#undef CHECK_RESULT
#define CHECK_RESULT(expr)                                           \
  do {                                                               \
    if (WABT_UNLIKELY(state->offset <= m_externalDelegate->          \
            skipValidationUntil())) {                                \
        break;                                                       \
    }                                                                \
    if (WABT_UNLIKELY(Failed(expr))) {                               \
      return ::wabt::Result::Error;                                  \
    }                                                                \
  } while (0)

#define SHOULD_GENERATE_BYTECODE if (WABT_UNLIKELY(!m_externalDelegate->shouldContinueToGenerateByteCode())) { return Result::Ok; }

namespace wabt {

using ValueType = wabt::Type;
using ValueTypes = std::vector<ValueType>;

struct SimpleFuncType {
    ValueTypes params;
    ValueTypes results;
};

ValueTypes ToInterp(Index count, Type *types) {
    return ValueTypes(&types[0], &types[count]);
}

SegmentKind ToSegmentMode(uint8_t flags) {
    if ((flags & SegDeclared) == SegDeclared) {
        return SegmentKind::Declared;
    } else if ((flags & SegPassive) == SegPassive) {
        return SegmentKind::Passive;
    } else {
        return SegmentKind::Active;
    }
}

enum class LabelKind {
    Block, Try
};
struct Label {
    LabelKind kind;
};

static Features getFeatures() {
    Features features;
    features.enable_exceptions();
    // TODO: should use command line flag for this (--enable-threads)
    features.enable_threads();
    // TODO: should use command line flag for this (--enable-relaxed-simd)
    features.enable_relaxed_simd();
    // TODO: should use command line flag for this (--enable-multi-memory)
    features.enable_multi_memory();
    return features;
}

class BinaryReaderDelegateWalrus: public BinaryReaderDelegate {
public:
    BinaryReaderDelegateWalrus(WASMBinaryReaderDelegate *delegate, const std::string &filename) :
        m_externalDelegate(delegate), m_filename(filename), m_validator(&m_errors, ValidateOptions(getFeatures())), m_lastInitType(Type::___), m_currentElementTableIndex(0) {

    }

    Location GetLocation() const {
        Location loc;
        loc.filename = m_filename;
        loc.offset = state->offset;
        return loc;
    }

    Label* GetLabel(Index depth) {
        assert(depth < m_labelStack.size());
        return &m_labelStack[m_labelStack.size() - depth - 1];
    }

    Label* GetNearestTryLabel(Index depth) {
        for (size_t i = depth; i < m_labelStack.size(); i++) {
            Label *label = &m_labelStack[m_labelStack.size() - i - 1];
            if (label->kind == LabelKind::Try) {
                return label;
            }
        }
        return nullptr;
    }

    Label* TopLabel() {
        return GetLabel(0);
    }

    void PushLabel(LabelKind kind) {
        m_labelStack.push_back(Label { kind });
    }

    void PopLabel() {
        m_labelStack.pop_back();
    }

    Result GetDropCount(Index keep_count, size_t type_stack_limit, Index *out_drop_count) {
        assert(m_validator.type_stack_size() >= type_stack_limit);
        Index type_stack_count = m_validator.type_stack_size() - type_stack_limit;
        // The keep_count may be larger than the type_stack_count if the typechecker
        // is currently unreachable. In that case, it doesn't matter what value we
        // drop, but 0 is a reasonable choice.
        *out_drop_count = type_stack_count >= keep_count ? type_stack_count - keep_count : 0;
        return Result::Ok;
    }
    Result GetBrDropKeepCount(Index depth, Index *out_drop_count, Index *out_keep_count) {
        if (state->offset > m_externalDelegate->skipValidationUntil()) {
            SharedValidator::Label *label;
            if (WABT_UNLIKELY(Failed(m_validator.GetLabel(depth, &label)))) {
                return ::wabt::Result::Error;
            }
            Index keep_count = label->br_types().size();
            CHECK_RESULT(GetDropCount(keep_count, label->type_stack_limit, out_drop_count));
            *out_keep_count = keep_count;
        }
        return Result::Ok;
    }

    Result GetReturnDropKeepCount(Index *out_drop_count, Index *out_keep_count) {
        CHECK_RESULT(GetBrDropKeepCount(m_labelStack.size() - 1, out_drop_count, out_keep_count));
        *out_drop_count += m_validator.GetLocalCount();
        return Result::Ok;
    }

    Result GetReturnCallDropKeepCount(const SimpleFuncType &func_type, Index keep_extra, Index *out_drop_count, Index *out_keep_count) {
        Index keep_count = static_cast<Index>(func_type.params.size()) + keep_extra;
        CHECK_RESULT(GetDropCount(keep_count, 0, out_drop_count));
        *out_drop_count += m_validator.GetLocalCount();
        *out_keep_count = keep_count;
        return Result::Ok;
    }

    void OnSetState(const State* s) override {
        BinaryReaderDelegate::OnSetState(s);
        m_externalDelegate->OnSetOffsetAddress(const_cast<size_t*>(&s->offset));
        m_externalDelegate->OnSetDataAddress(s->data);
    }

    bool OnError(const Error& err) override {
        m_errors.push_back(err);
        return true;
    }

    /* Module */
    Result BeginModule(uint32_t version) override {
        m_externalDelegate->BeginModule(version);
        return Result::Ok;
    }
    Result EndModule() override {
        CHECK_RESULT(m_validator.EndModule());
        m_externalDelegate->EndModule();
        return Result::Ok;
    }

    Result BeginSection(Index section_index, BinarySection section_type, Offset size) override {
        return Result::Ok;
    }

    /* Custom section */
    Result BeginCustomSection(Index section_index, Offset size, nonstd::string_view section_name) override {
        return Result::Ok;
    }
    Result EndCustomSection() override {
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
        CHECK_RESULT(m_validator.OnFuncType(GetLocation(), param_count, param_types, result_count, result_types, index));
        m_functionTypes.push_back(SimpleFuncType( { ToInterp(param_count, param_types), ToInterp(result_count, result_types) }));
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
    Result OnImport(Index index, ExternalKind kind, nonstd::string_view module_name, nonstd::string_view field_name) override {
        return Result::Ok;
    }
    Result OnImportFunc(Index import_index, nonstd::string_view module_name, nonstd::string_view field_name, Index func_index, Index sig_index) override {
        CHECK_RESULT(m_validator.OnFunction(GetLocation(), Var(sig_index, GetLocation())));
        m_externalDelegate->OnImportFunc(import_index, std::string(module_name), std::string(field_name), func_index, sig_index);
        return Result::Ok;
    }
    Result OnImportTable(Index import_index, nonstd::string_view module_name, nonstd::string_view field_name, Index table_index, Type elem_type, const Limits *elem_limits) override {
        CHECK_RESULT(m_validator.OnTable(GetLocation(), elem_type, *elem_limits));
        m_tableTypes.push_back(elem_type);
        m_externalDelegate->OnImportTable(import_index, std::string(module_name), std::string(field_name), table_index, elem_type, elem_limits->initial, elem_limits->has_max ? elem_limits->max : std::numeric_limits<uint32_t>::max());
        return Result::Ok;
    }
    Result OnImportMemory(Index import_index, nonstd::string_view module_name, nonstd::string_view field_name, Index memory_index, const Limits *page_limits) override {
        CHECK_RESULT(m_validator.OnMemory(GetLocation(), *page_limits));
        m_externalDelegate->OnImportMemory(import_index, std::string(module_name), std::string(field_name), memory_index, page_limits->initial,
            page_limits->has_max ? page_limits->max : (page_limits->is_64? (WABT_MAX_PAGES64 - 1) : (WABT_MAX_PAGES32 - 1)), page_limits->is_shared);
        return Result::Ok;
    }
    Result OnImportGlobal(Index import_index, nonstd::string_view module_name, nonstd::string_view field_name, Index global_index, Type type, bool mutable_) override {
        CHECK_RESULT(m_validator.OnGlobalImport(GetLocation(), type, mutable_));
        m_externalDelegate->OnImportGlobal(import_index, std::string(module_name), std::string(field_name), global_index, type, mutable_);
        return Result::Ok;
    }
    Result OnImportTag(Index import_index, nonstd::string_view module_name, nonstd::string_view field_name, Index tag_index, Index sig_index) override {
        CHECK_RESULT(m_validator.OnTag(GetLocation(), Var(sig_index, GetLocation())));
        m_externalDelegate->OnImportTag(import_index, std::string(module_name), std::string(field_name), tag_index, sig_index);
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
        CHECK_RESULT(m_validator.OnFunction(GetLocation(), Var(sig_index, GetLocation())));
        m_externalDelegate->OnFunction(index, sig_index);
        return Result::Ok;
    }
    Result EndFunctionSection() override {
        return Result::Ok;
    }

    /* Table section */
    Result BeginTableSection(Offset size) override {
        return Result::Ok;
    }
    Result OnTableCount(Index count) override {
        m_externalDelegate->OnTableCount(count);
        return Result::Ok;
    }
    Result OnTable(Index index, Type elem_type, const Limits *elem_limits) override {
        CHECK_RESULT(m_validator.OnTable(GetLocation(), elem_type, *elem_limits));
        m_tableTypes.push_back(elem_type);
        m_externalDelegate->OnTable(index, elem_type, elem_limits->initial, elem_limits->has_max ? elem_limits->max : std::numeric_limits<uint32_t>::max());
        return Result::Ok;
    }
    Result EndTableSection() override {
        return Result::Ok;
    }

    /* Memory section */
    Result BeginMemorySection(Offset size) override {
        return Result::Ok;
    }
    Result OnMemoryCount(Index count) override {
        m_externalDelegate->OnMemoryCount(count);
        return Result::Ok;
    }
    Result OnMemory(Index index, const Limits *limits) override {
        CHECK_RESULT(m_validator.OnMemory(GetLocation(), *limits));
        m_externalDelegate->OnMemory(index, limits->initial,
            limits->has_max ? limits->max : (limits->is_64 ? (WABT_MAX_PAGES64 - 1) : (WABT_MAX_PAGES32 - 1)), limits->is_shared);
        return Result::Ok;
    }
    Result EndMemorySection() override {
        return Result::Ok;
    }

    /* Global section */
    Result BeginGlobalSection(Offset size) override {
        return Result::Ok;
    }
    Result OnGlobalCount(Index count) override {
        m_externalDelegate->OnGlobalCount(count);
        return Result::Ok;
    }
    Result BeginGlobal(Index index, Type type, bool mutable_) override {
        CHECK_RESULT(m_validator.OnGlobal(GetLocation(), type, mutable_));
        m_externalDelegate->BeginGlobal(index, type, mutable_);
        assert(m_lastInitType == Type::___);
        m_lastInitType = type;
        return Result::Ok;
    }
    Result BeginGlobalInitExpr(Index index) override {
        assert(m_lastInitType != Type::___);
        CHECK_RESULT(m_validator.BeginInitExpr(GetLocation(), m_lastInitType));
        PushLabel(LabelKind::Try);
        m_externalDelegate->BeginGlobalInitExpr(index);
        return Result::Ok;
    }
    Result EndGlobalInitExpr(Index index) override {
        m_lastInitType = Type::___;
        CHECK_RESULT(m_validator.EndInitExpr());
        PopLabel();
        m_externalDelegate->EndGlobalInitExpr(index);
        return Result::Ok;
    }
    Result EndGlobal(Index index) override {
        m_externalDelegate->EndGlobal(index);
        return Result::Ok;
    }
    Result EndGlobalSection() override {
        m_externalDelegate->EndGlobalSection();
        return Result::Ok;
    }

    /* Exports section */
    Result BeginExportSection(Offset size) override {
        return Result::Ok;
    }
    Result OnExportCount(Index count) override {
        m_externalDelegate->OnExportCount(count);
        return Result::Ok;
    }
    Result OnExport(Index index, ExternalKind kind, Index item_index, nonstd::string_view name) override {
        CHECK_RESULT(m_validator.OnExport(GetLocation(), kind, Var(item_index, GetLocation()), name));
        m_externalDelegate->OnExport(static_cast<int>(kind), index, std::string(name), item_index);
        return Result::Ok;
    }
    Result EndExportSection() override {
        return Result::Ok;
    }

    /* Start section */
    Result BeginStartSection(Offset size) override {
        return Result::Ok;
    }
    Result OnStartFunction(Index func_index) override {
        CHECK_RESULT(m_validator.OnStart(GetLocation(), Var(func_index, GetLocation())));
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
        m_labelStack.clear();
        CHECK_RESULT(m_validator.BeginFunctionBody(GetLocation(), index));
        PushLabel(LabelKind::Try);
        m_externalDelegate->BeginFunctionBody(index, size);
        return Result::Ok;
    }
    Result OnLocalDeclCount(Index count) override {
        m_externalDelegate->OnLocalDeclCount(count);
        return Result::Ok;
    }
    Result OnLocalDecl(Index decl_index, Index count, Type type) override {
        CHECK_RESULT(m_validator.OnLocalDecl(GetLocation(), count, type));
        m_externalDelegate->OnLocalDecl(decl_index, count, type);
        return Result::Ok;
    }

    Result OnStartReadInstructions(Offset start, Offset end) override {
        m_externalDelegate->OnStartReadInstructions(start, end);
        return Result::Ok;
    }

    Result OnStartPreprocess() override {
        m_externalDelegate->OnStartPreprocess();
        return Result::Ok;
    }

    Result OnEndPreprocess() override {
        m_externalDelegate->OnEndPreprocess();
        return Result::Ok;
    }

    bool NeedsPreprocess() override {
        return true;
    }

    /* Function expressions; called between BeginFunctionBody and
     EndFunctionBody */
    Result OnOpcode(Opcode opcode) override {
        SHOULD_GENERATE_BYTECODE;
        Opcode::Enum e = opcode;
        m_externalDelegate->OnOpcode(e);
        return Result::Ok;
    }
    Result OnOpcodeBare() override {
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
        return Result::Ok;
    }
    Result OnOpcodeUint32Uint32Uint32(uint32_t value, uint32_t value2, uint32_t value3) override {
        return Result::Ok;
    }
    Result OnOpcodeUint32Uint32Uint32Uint32(uint32_t value, uint32_t value2, uint32_t value3, uint32_t value4) override {
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
        return Result::Ok;
    }
    Result OnOpcodeBlockSig(Type sig_type) override {
        if (WABT_UNLIKELY(m_externalDelegate->resumeGenerateByteCodeAfterNBlockEnd())) {
            m_externalDelegate->setResumeGenerateByteCodeAfterNBlockEnd(m_externalDelegate->resumeGenerateByteCodeAfterNBlockEnd() + 1);
        }
        return Result::Ok;
    }
    Result OnOpcodeType(Type type) override {
        return Result::Ok;
    }
    Result OnAtomicLoadExpr(Opcode opcode, Index memidx, Address alignment_log2, Address offset) override {
        CHECK_RESULT(m_validator.OnAtomicLoad(GetLocation(), opcode, Var(memidx, GetLocation()), GetAlignment(alignment_log2), offset));
        SHOULD_GENERATE_BYTECODE;
        m_externalDelegate->OnAtomicLoadExpr(opcode, memidx, alignment_log2, offset);
        return Result::Ok;
    }
    Result OnAtomicStoreExpr(Opcode opcode, Index memidx, Address alignment_log2, Address offset) override {
        CHECK_RESULT(m_validator.OnAtomicStore(GetLocation(), opcode, Var(memidx, GetLocation()), GetAlignment(alignment_log2), offset));
        SHOULD_GENERATE_BYTECODE;
        m_externalDelegate->OnAtomicStoreExpr(opcode, memidx, alignment_log2, offset);
        return Result::Ok;
    }
    Result OnAtomicRmwExpr(Opcode opcode, Index memidx, Address alignment_log2, Address offset) override {
        CHECK_RESULT(m_validator.OnAtomicRmw(GetLocation(), opcode, Var(memidx, GetLocation()), GetAlignment(alignment_log2), offset));
        SHOULD_GENERATE_BYTECODE;
        m_externalDelegate->OnAtomicRmwExpr(opcode, memidx, alignment_log2, offset);
        return Result::Ok;
    }
    Result OnAtomicRmwCmpxchgExpr(Opcode opcode, Index memidx, Address alignment_log2, Address offset) override {
        CHECK_RESULT(m_validator.OnAtomicRmwCmpxchg(GetLocation(), opcode, Var(memidx, GetLocation()), GetAlignment(alignment_log2), offset));
        SHOULD_GENERATE_BYTECODE;
        m_externalDelegate->OnAtomicCmpxchgExpr(opcode, memidx, alignment_log2, offset);
        return Result::Ok;
    }
    Result OnAtomicWaitExpr(Opcode opcode, Index memidx, Address alignment_log2, Address offset) override {
        CHECK_RESULT(m_validator.OnAtomicWait(GetLocation(), opcode, Var(memidx, GetLocation()), GetAlignment(alignment_log2), offset));
        SHOULD_GENERATE_BYTECODE;
        m_externalDelegate->OnAtomicWaitExpr(opcode, memidx, alignment_log2, offset);
        return Result::Ok;
    }
    Result OnAtomicFenceExpr(uint32_t consistency_model) override {
        CHECK_RESULT(m_validator.OnAtomicFence(GetLocation(), consistency_model));
        SHOULD_GENERATE_BYTECODE;
        m_externalDelegate->OnAtomicFenceExpr(consistency_model);
        return Result::Ok;
    }
    Result OnAtomicNotifyExpr(Opcode opcode, Index memidx, Address alignment_log2, Address offset) override {
        CHECK_RESULT(m_validator.OnAtomicNotify(GetLocation(), opcode, Var(memidx, GetLocation()), GetAlignment(alignment_log2), offset));
        SHOULD_GENERATE_BYTECODE;
        m_externalDelegate->OnAtomicNotifyExpr(opcode, memidx, alignment_log2, offset);
        return Result::Ok;
    }
    Result OnBinaryExpr(Opcode opcode) override {
        CHECK_RESULT(m_validator.OnBinary(GetLocation(), opcode));
        SHOULD_GENERATE_BYTECODE;
        m_externalDelegate->OnBinaryExpr(opcode);
        return Result::Ok;
    }
    Result OnBlockExpr(Type sig_type) override {
        CHECK_RESULT(m_validator.OnBlock(GetLocation(), sig_type));
        EXECUTE_VALIDATOR(PushLabel(LabelKind::Block));
        SHOULD_GENERATE_BYTECODE;
        m_externalDelegate->OnBlockExpr(sig_type);
        return Result::Ok;
    }
    Result OnBrExpr(Index depth) override {
        Index drop_count, keep_count, catch_drop_count;
        CHECK_RESULT(GetBrDropKeepCount(depth, &drop_count, &keep_count));
        CHECK_RESULT(m_validator.GetCatchCount(depth, &catch_drop_count));
        CHECK_RESULT(m_validator.OnBr(GetLocation(), Var(depth, GetLocation())));
        SHOULD_GENERATE_BYTECODE;
        m_externalDelegate->OnBrExpr(depth);
        return Result::Ok;
    }
    Result OnBrIfExpr(Index depth) override {
        Index drop_count, keep_count, catch_drop_count;
        CHECK_RESULT(m_validator.OnBrIf(GetLocation(), Var(depth, GetLocation())));
        CHECK_RESULT(GetBrDropKeepCount(depth, &drop_count, &keep_count));
        CHECK_RESULT(m_validator.GetCatchCount(depth, &catch_drop_count));
        SHOULD_GENERATE_BYTECODE;
        m_externalDelegate->OnBrIfExpr(depth);
        return Result::Ok;
    }
    Result OnBrTableExpr(Index num_targets, Index *target_depths, Index default_target_depth) override {
        CHECK_RESULT(m_validator.BeginBrTable(GetLocation()));
        Index drop_count, keep_count, catch_drop_count;

        for (Index i = 0; i < num_targets; ++i) {
            Index depth = target_depths[i];
            CHECK_RESULT(m_validator.OnBrTableTarget(GetLocation(), Var(depth, GetLocation())));
            CHECK_RESULT(GetBrDropKeepCount(depth, &drop_count, &keep_count));
            CHECK_RESULT(m_validator.GetCatchCount(depth, &catch_drop_count));
        }
        CHECK_RESULT(m_validator.OnBrTableTarget(GetLocation(), Var(default_target_depth, GetLocation())));
        CHECK_RESULT(GetBrDropKeepCount(default_target_depth, &drop_count, &keep_count));
        CHECK_RESULT(m_validator.GetCatchCount(default_target_depth, &catch_drop_count));
        CHECK_RESULT(m_validator.EndBrTable(GetLocation()));

        SHOULD_GENERATE_BYTECODE;
        m_externalDelegate->OnBrTableExpr(num_targets, target_depths, default_target_depth);
        return Result::Ok;
    }
    Result OnCallExpr(Index func_index) override {
        CHECK_RESULT(m_validator.OnCall(GetLocation(), Var(func_index, GetLocation())));
        SHOULD_GENERATE_BYTECODE;
        m_externalDelegate->OnCallExpr(func_index);
        return Result::Ok;
    }
    Result OnCallIndirectExpr(Index sig_index, Index table_index) override {
        CHECK_RESULT(m_validator.OnCallIndirect(GetLocation(), Var(sig_index, GetLocation()), Var(table_index, GetLocation())));
        SHOULD_GENERATE_BYTECODE;
        m_externalDelegate->OnCallIndirectExpr(sig_index, table_index);
        return Result::Ok;
    }
    Result OnCallRefExpr() override {
        abort();
        return Result::Ok;
    }
    void SubBlockCheck() {
        if (WABT_UNLIKELY(m_externalDelegate->resumeGenerateByteCodeAfterNBlockEnd() == 1)) {
            m_externalDelegate->setResumeGenerateByteCodeAfterNBlockEnd(0);
            m_externalDelegate->setShouldContinueToGenerateByteCode(true);
        }
    }
    Result OnCatchExpr(Index tag_index) override {
        CHECK_RESULT(m_validator.OnCatch(GetLocation(), Var(tag_index, GetLocation()), false));
        SubBlockCheck();
        SHOULD_GENERATE_BYTECODE;
        m_externalDelegate->OnCatchExpr(tag_index);
        return Result::Ok;
    }
    Result OnCatchAllExpr() override {
        CHECK_RESULT(m_validator.OnCatch(GetLocation(), Var(), true));
        SubBlockCheck();
        SHOULD_GENERATE_BYTECODE;
        m_externalDelegate->OnCatchAllExpr();
        return Result::Ok;
    }
    Result OnCompareExpr(Opcode opcode) override {
        CHECK_RESULT(m_validator.OnCompare(GetLocation(), opcode));
        SHOULD_GENERATE_BYTECODE;
        m_externalDelegate->OnBinaryExpr(opcode);
        return Result::Ok;
    }
    Result OnConvertExpr(Opcode opcode) override {
        CHECK_RESULT(m_validator.OnConvert(GetLocation(), opcode));
        SHOULD_GENERATE_BYTECODE;
        m_externalDelegate->OnUnaryExpr(opcode);
        return Result::Ok;
    }
    Result OnDelegateExpr(Index depth) override {
        CHECK_RESULT(m_validator.OnDelegate(GetLocation(), Var(depth, GetLocation())));
        EXECUTE_VALIDATOR(PopLabel());
        abort();
        return Result::Ok;
    }
    Result OnDropExpr() override {
        CHECK_RESULT(m_validator.OnDrop(GetLocation()));
        SHOULD_GENERATE_BYTECODE;
        m_externalDelegate->OnDropExpr();
        return Result::Ok;
    }
    Result OnElseExpr() override {
        CHECK_RESULT(m_validator.OnElse(GetLocation()));
        SubBlockCheck();
        SHOULD_GENERATE_BYTECODE;
        m_externalDelegate->OnElseExpr();
        return Result::Ok;
    }
    Result OnEndExpr() override {
        if (state->offset > m_externalDelegate->skipValidationUntil() && m_labelStack.size() != 1) {
            CHECK_RESULT(m_validator.OnEnd(GetLocation()));
            PopLabel();
        }
        if (WABT_UNLIKELY(m_externalDelegate->resumeGenerateByteCodeAfterNBlockEnd())) {
            m_externalDelegate->setResumeGenerateByteCodeAfterNBlockEnd(m_externalDelegate->resumeGenerateByteCodeAfterNBlockEnd() - 1);
            if (m_externalDelegate->resumeGenerateByteCodeAfterNBlockEnd() == 0) {
                m_externalDelegate->setShouldContinueToGenerateByteCode(true);
            }
        }
        SHOULD_GENERATE_BYTECODE;
        m_externalDelegate->OnEndExpr();
        return Result::Ok;
    }
    Result OnF32ConstExpr(uint32_t value_bits) override {
        CHECK_RESULT(m_validator.OnConst(GetLocation(), Type::F32));
        SHOULD_GENERATE_BYTECODE;
        m_externalDelegate->OnF32ConstExpr(value_bits);
        return Result::Ok;
    }
    Result OnF64ConstExpr(uint64_t value_bits) override {
        CHECK_RESULT(m_validator.OnConst(GetLocation(), Type::F64));
        SHOULD_GENERATE_BYTECODE;
        m_externalDelegate->OnF64ConstExpr(value_bits);
        return Result::Ok;
    }
    Result OnV128ConstExpr(v128 value_bits) override {
        CHECK_RESULT(m_validator.OnConst(GetLocation(), Type::V128));
        SHOULD_GENERATE_BYTECODE;
        m_externalDelegate->OnV128ConstExpr(value_bits.v);
        return Result::Ok;
    }
    Result OnGlobalGetExpr(Index global_index) override {
        CHECK_RESULT(m_validator.OnGlobalGet(GetLocation(), Var(global_index, GetLocation())));
        SHOULD_GENERATE_BYTECODE;
        m_externalDelegate->OnGlobalGetExpr(global_index);
        return Result::Ok;
    }
    Result OnGlobalSetExpr(Index global_index) override {
        CHECK_RESULT(m_validator.OnGlobalSet(GetLocation(), Var(global_index, GetLocation())));
        SHOULD_GENERATE_BYTECODE;
        m_externalDelegate->OnGlobalSetExpr(global_index);
        return Result::Ok;
    }
    Result OnI32ConstExpr(uint32_t value) override {
        CHECK_RESULT(m_validator.OnConst(GetLocation(), Type::I32));
        SHOULD_GENERATE_BYTECODE;
        m_externalDelegate->OnI32ConstExpr(value);
        return Result::Ok;
    }
    Result OnI64ConstExpr(uint64_t value) override {
        CHECK_RESULT(m_validator.OnConst(GetLocation(), Type::I64));
        SHOULD_GENERATE_BYTECODE;
        m_externalDelegate->OnI64ConstExpr(value);
        return Result::Ok;
    }
    Result OnIfExpr(Type sig_type) override {
        CHECK_RESULT(m_validator.OnIf(GetLocation(), sig_type));
        EXECUTE_VALIDATOR(PushLabel(LabelKind::Block));
        SHOULD_GENERATE_BYTECODE;
        m_externalDelegate->OnIfExpr(sig_type);
        return Result::Ok;
    }
    Result OnLoadExpr(Opcode opcode, Index memidx, Address alignment_log2, Address offset) override {
        CHECK_RESULT(m_validator.OnLoad(GetLocation(), opcode, Var(memidx, GetLocation()), GetAlignment(alignment_log2), offset));
        SHOULD_GENERATE_BYTECODE;
        m_externalDelegate->OnLoadExpr(opcode, memidx, alignment_log2, offset);
        return Result::Ok;
    }
    Index TranslateLocalIndex(Index local_index) {
        return m_validator.type_stack_size() + m_validator.GetLocalCount() - local_index;
    }
    Result OnLocalGetExpr(Index local_index) override {
        CHECK_RESULT(m_validator.OnLocalGet(GetLocation(), Var(local_index, GetLocation())));
        SHOULD_GENERATE_BYTECODE;
        m_externalDelegate->OnLocalGetExpr(local_index);
        return Result::Ok;
    }
    Result OnLocalSetExpr(Index local_index) override {
        CHECK_RESULT(m_validator.OnLocalSet(GetLocation(), Var(local_index, GetLocation())));
        SHOULD_GENERATE_BYTECODE;
        m_externalDelegate->OnLocalSetExpr(local_index);
        return Result::Ok;
    }
    Result OnLocalTeeExpr(Index local_index) override {
        CHECK_RESULT(m_validator.OnLocalTee(GetLocation(), Var(local_index, GetLocation())));
        SHOULD_GENERATE_BYTECODE;
        m_externalDelegate->OnLocalTeeExpr(local_index);
        return Result::Ok;
    }
    Result OnLoopExpr(Type sig_type) override {
        CHECK_RESULT(m_validator.OnLoop(GetLocation(), sig_type));
        EXECUTE_VALIDATOR(PushLabel(LabelKind::Block));
        SHOULD_GENERATE_BYTECODE;
        m_externalDelegate->OnLoopExpr(sig_type);
        return Result::Ok;
    }
    Result OnMemoryCopyExpr(Index srcmemidx, Index destmemidx) override {
        CHECK_RESULT(m_validator.OnMemoryCopy(GetLocation(), Var(srcmemidx, GetLocation()), Var(destmemidx, GetLocation())));
        SHOULD_GENERATE_BYTECODE;
        m_externalDelegate->OnMemoryCopyExpr(srcmemidx, destmemidx);
        return Result::Ok;
    }
    Result OnDataDropExpr(Index segment_index) override {
        CHECK_RESULT(m_validator.OnDataDrop(GetLocation(), Var(segment_index, GetLocation())));
        SHOULD_GENERATE_BYTECODE;
        m_externalDelegate->OnDataDropExpr(segment_index);
        return Result::Ok;
    }
    Result OnMemoryFillExpr(Index memidx) override {
        CHECK_RESULT(m_validator.OnMemoryFill(GetLocation(), Var(memidx, GetLocation())));
        SHOULD_GENERATE_BYTECODE;
        m_externalDelegate->OnMemoryFillExpr(memidx);
        return Result::Ok;
    }
    Result OnMemoryGrowExpr(Index memidx) override {
        CHECK_RESULT(m_validator.OnMemoryGrow(GetLocation(), Var(memidx, GetLocation())));
        SHOULD_GENERATE_BYTECODE;
        m_externalDelegate->OnMemoryGrowExpr(memidx);
        return Result::Ok;
    }
    Result OnMemoryInitExpr(Index segment_index, Index memidx) override {
        CHECK_RESULT(m_validator.OnMemoryInit(GetLocation(), Var(segment_index, GetLocation()), Var(memidx, GetLocation())));
        SHOULD_GENERATE_BYTECODE;
        m_externalDelegate->OnMemoryInitExpr(segment_index, memidx);
        return Result::Ok;
    }
    Result OnMemorySizeExpr(Index memidx) override {
        CHECK_RESULT(m_validator.OnMemorySize(GetLocation(), Var(memidx, GetLocation())));
        SHOULD_GENERATE_BYTECODE;
        m_externalDelegate->OnMemorySizeExpr(memidx);
        return Result::Ok;
    }
    Result OnTableCopyExpr(Index dst_index, Index src_index) override {
        CHECK_RESULT(m_validator.OnTableCopy(GetLocation(), Var(dst_index, GetLocation()), Var(src_index, GetLocation())));
        SHOULD_GENERATE_BYTECODE;
        m_externalDelegate->OnTableCopyExpr(dst_index, src_index);
        return Result::Ok;
    }
    Result OnElemDropExpr(Index segment_index) override {
        CHECK_RESULT(m_validator.OnElemDrop(GetLocation(), Var(segment_index, GetLocation())));
        SHOULD_GENERATE_BYTECODE;
        m_externalDelegate->OnElemDropExpr(segment_index);
        return Result::Ok;
    }
    Result OnTableInitExpr(Index segment_index, Index table_index) override {
        CHECK_RESULT(m_validator.OnTableInit(GetLocation(), Var(segment_index, GetLocation()), Var(table_index, GetLocation())));
        SHOULD_GENERATE_BYTECODE;
        m_externalDelegate->OnTableInitExpr(segment_index, table_index);
        return Result::Ok;
    }
    Result OnTableGetExpr(Index table_index) override {
        CHECK_RESULT(m_validator.OnTableGet(GetLocation(), Var(table_index, GetLocation())));
        SHOULD_GENERATE_BYTECODE;
        m_externalDelegate->OnTableGetExpr(table_index);
        return Result::Ok;
    }
    Result OnTableSetExpr(Index table_index) override {
        CHECK_RESULT(m_validator.OnTableSet(GetLocation(), Var(table_index, GetLocation())));
        SHOULD_GENERATE_BYTECODE;
        m_externalDelegate->OnTableSetExpr(table_index);
        return Result::Ok;
    }
    Result OnTableGrowExpr(Index table_index) override {
        CHECK_RESULT(m_validator.OnTableGrow(GetLocation(), Var(table_index, GetLocation())));
        SHOULD_GENERATE_BYTECODE;
        m_externalDelegate->OnTableGrowExpr(table_index);
        return Result::Ok;
    }
    Result OnTableSizeExpr(Index table_index) override {
        CHECK_RESULT(m_validator.OnTableSize(GetLocation(), Var(table_index, GetLocation())));
        SHOULD_GENERATE_BYTECODE;
        m_externalDelegate->OnTableSizeExpr(table_index);
        return Result::Ok;
    }
    Result OnTableFillExpr(Index table_index) override {
        CHECK_RESULT(m_validator.OnTableFill(GetLocation(), Var(table_index, GetLocation())));
        SHOULD_GENERATE_BYTECODE;
        m_externalDelegate->OnTableFillExpr(table_index);
        return Result::Ok;
    }
    Result OnRefFuncExpr(Index func_index) override {
        CHECK_RESULT(m_validator.OnRefFunc(GetLocation(), Var(func_index, GetLocation())));
        SHOULD_GENERATE_BYTECODE;
        m_externalDelegate->OnRefFuncExpr(func_index);
        return Result::Ok;
    }
    Result OnRefNullExpr(Type type) override {
        CHECK_RESULT(m_validator.OnRefNull(GetLocation(), type));
        SHOULD_GENERATE_BYTECODE;
        m_externalDelegate->OnRefNullExpr(type);
        return Result::Ok;
    }
    Result OnRefIsNullExpr() override {
        CHECK_RESULT(m_validator.OnRefIsNull(GetLocation()));
        SHOULD_GENERATE_BYTECODE;
        m_externalDelegate->OnRefIsNullExpr();
        return Result::Ok;
    }
    Result OnNopExpr() override {
        CHECK_RESULT(m_validator.OnNop(GetLocation()));
#if !defined(NDEBUG)
        m_externalDelegate->OnNopExpr();
#endif /* !NDEBUG */
        return Result::Ok;
    }
    Result OnRethrowExpr(Index depth) override {
        Index catch_depth;
        CHECK_RESULT(m_validator.OnRethrow(GetLocation(), Var(depth, GetLocation())));
        CHECK_RESULT(m_validator.GetCatchCount(depth, &catch_depth));
        abort();
        return Result::Ok;
    }
    Result OnReturnCallExpr(Index func_index) override {
        CHECK_RESULT(m_validator.OnReturnCall(GetLocation(), Var(func_index, GetLocation())));

        SimpleFuncType &func_type = m_functionTypes[func_index];

        Index drop_count, keep_count, catch_drop_count;
        CHECK_RESULT(GetReturnCallDropKeepCount(func_type, 0, &drop_count, &keep_count));
        CHECK_RESULT(m_validator.GetCatchCount(m_labelStack.size() - 1, &catch_drop_count));
        // The validator must be run after we get the drop/keep counts, since it
        // will change the type stack.
        CHECK_RESULT(m_validator.OnReturnCall(GetLocation(), Var(func_index, GetLocation())));

        abort();
        return Result::Ok;
    }
    Result OnReturnCallIndirectExpr(Index sig_index, Index table_index) override {
        CHECK_RESULT(m_validator.OnReturnCallIndirect(GetLocation(), Var(sig_index, GetLocation()), Var(table_index, GetLocation())));

        SimpleFuncType &func_type = m_functionTypes[sig_index];

        Index drop_count, keep_count, catch_drop_count;
        // +1 to include the index of the function.
        CHECK_RESULT(GetReturnCallDropKeepCount(func_type, +1, &drop_count, &keep_count));
        CHECK_RESULT(m_validator.GetCatchCount(m_labelStack.size() - 1, &catch_drop_count));
        // The validator must be run after we get the drop/keep counts, since it
        // changes the type stack.
        CHECK_RESULT(m_validator.OnReturnCallIndirect(GetLocation(), Var(sig_index, GetLocation()), Var(table_index, GetLocation())));
        abort();
        return Result::Ok;
    }
    Result OnReturnExpr() override {
        Index drop_count, keep_count, catch_drop_count;
        CHECK_RESULT(GetReturnDropKeepCount(&drop_count, &keep_count));
        CHECK_RESULT(m_validator.GetCatchCount(m_labelStack.size() - 1, &catch_drop_count));
        CHECK_RESULT(m_validator.OnReturn(GetLocation()));
        SHOULD_GENERATE_BYTECODE;
        m_externalDelegate->OnReturnExpr();
        return Result::Ok;
    }
    Result OnSelectExpr(Index result_count, Type *result_types) override {
        CHECK_RESULT(m_validator.OnSelect(GetLocation(), result_count, result_types));
        SHOULD_GENERATE_BYTECODE;
        m_externalDelegate->OnSelectExpr(result_count, result_types);
        return Result::Ok;
    }
    Result OnStoreExpr(Opcode opcode, Index memidx, Address alignment_log2, Address offset) override {
        CHECK_RESULT(m_validator.OnStore(GetLocation(), opcode, Var(memidx, GetLocation()), GetAlignment(alignment_log2), offset));
        SHOULD_GENERATE_BYTECODE;
        m_externalDelegate->OnStoreExpr(opcode, memidx, alignment_log2, offset);
        return Result::Ok;
    }
    Result OnThrowExpr(Index tag_index) override {
        CHECK_RESULT(m_validator.OnThrow(GetLocation(), Var(tag_index, GetLocation())));
        SHOULD_GENERATE_BYTECODE;
        m_externalDelegate->OnThrowExpr(tag_index);
        return Result::Ok;
    }
    Result OnTryExpr(Type sig_type) override {
        uint32_t exn_stack_height;
        CHECK_RESULT(m_validator.GetCatchCount(m_labelStack.size() - 1, &exn_stack_height));
        CHECK_RESULT(m_validator.OnTry(GetLocation(), sig_type));
        EXECUTE_VALIDATOR(PushLabel(LabelKind::Try));
        SHOULD_GENERATE_BYTECODE;
        m_externalDelegate->OnTryExpr(sig_type);
        return Result::Ok;
    }
    Result OnUnaryExpr(Opcode opcode) override {
        CHECK_RESULT(m_validator.OnUnary(GetLocation(), opcode));
        SHOULD_GENERATE_BYTECODE;
        m_externalDelegate->OnUnaryExpr(opcode);
        return Result::Ok;
    }
    Result OnTernaryExpr(Opcode opcode) override {
        CHECK_RESULT(m_validator.OnTernary(GetLocation(), opcode));
        SHOULD_GENERATE_BYTECODE;
        m_externalDelegate->OnTernaryExpr(opcode);
        return Result::Ok;
    }
    Result OnUnreachableExpr() override {
        CHECK_RESULT(m_validator.OnUnreachable(GetLocation()));
        SHOULD_GENERATE_BYTECODE;
        m_externalDelegate->OnUnreachableExpr();
        return Result::Ok;
    }
    Result EndFunctionBody(Index index) override {
        Index drop_count, keep_count;
        CHECK_RESULT(GetReturnDropKeepCount(&drop_count, &keep_count));
        CHECK_RESULT(m_validator.EndFunctionBody(GetLocation()));
        EXECUTE_VALIDATOR(PopLabel());
        m_externalDelegate->EndFunctionBody(index);
        return Result::Ok;
    }
    Result EndCodeSection() override {
        return Result::Ok;
    }
    Result OnSimdLaneOpExpr(Opcode opcode, uint64_t value) override {
        CHECK_RESULT(m_validator.OnSimdLaneOp(GetLocation(), opcode, value));
        SHOULD_GENERATE_BYTECODE;
        m_externalDelegate->OnSimdLaneOpExpr(opcode, value);
        return Result::Ok;
    }
    uint32_t GetAlignment(Address alignment_log2) {
        return alignment_log2 < 32 ? 1 << alignment_log2 : ~0u;
    }
    Result OnSimdLoadLaneExpr(Opcode opcode, Index memidx, Address alignment_log2, Address offset, uint64_t value) override {
        CHECK_RESULT(m_validator.OnSimdLoadLane(GetLocation(), opcode, Var(memidx, GetLocation()), GetAlignment(alignment_log2), offset, value));
        SHOULD_GENERATE_BYTECODE;
        m_externalDelegate->OnSimdLoadLaneExpr(opcode, memidx, alignment_log2, offset, value);
        return Result::Ok;
    }
    Result OnSimdStoreLaneExpr(Opcode opcode, Index memidx, Address alignment_log2, Address offset, uint64_t value) override {
        CHECK_RESULT(m_validator.OnSimdStoreLane(GetLocation(), opcode, Var(memidx, GetLocation()), GetAlignment(alignment_log2), offset, value));
        SHOULD_GENERATE_BYTECODE;
        m_externalDelegate->OnSimdStoreLaneExpr(opcode, memidx, alignment_log2, offset, value);
        return Result::Ok;
    }
    Result OnSimdShuffleOpExpr(Opcode opcode, v128 value) override {
        CHECK_RESULT(m_validator.OnSimdShuffleOp(GetLocation(), opcode, value));
        SHOULD_GENERATE_BYTECODE;
        m_externalDelegate->OnSimdShuffleOpExpr(opcode, value.v);
        return Result::Ok;
    }
    Result OnLoadSplatExpr(Opcode opcode, Index memidx, Address alignment_log2, Address offset) override {
        CHECK_RESULT(m_validator.OnLoadSplat(GetLocation(), opcode, Var(memidx, GetLocation()), GetAlignment(alignment_log2), offset));
        SHOULD_GENERATE_BYTECODE;
        m_externalDelegate->OnLoadSplatExpr(opcode, memidx, alignment_log2, offset);
        return Result::Ok;
    }
    Result OnLoadZeroExpr(Opcode opcode, Index memidx, Address alignment_log2, Address offset) override {
        CHECK_RESULT(m_validator.OnLoadZero(GetLocation(), opcode, Var(memidx, GetLocation()), GetAlignment(alignment_log2), offset));
        SHOULD_GENERATE_BYTECODE;
        m_externalDelegate->OnLoadZeroExpr(opcode, memidx, alignment_log2, offset);
        return Result::Ok;
    }

    /* Elem section */
    Result BeginElemSection(Offset size) override {
        return Result::Ok;
    }
    Result OnElemSegmentCount(Index count) override {
        m_externalDelegate->OnElemSegmentCount(count);
        return Result::Ok;
    }
    Result BeginElemSegment(Index index, Index table_index, uint8_t flags) override {
        auto mode = ToSegmentMode(flags);
        CHECK_RESULT(m_validator.OnElemSegment(GetLocation(), Var(table_index, GetLocation()), mode));
        m_externalDelegate->BeginElemSegment(index, table_index, flags);
        m_lastInitType = Type::I32;
        m_currentElementTableIndex = table_index;
        return Result::Ok;
    }
    Result BeginElemSegmentInitExpr(Index index) override {
        assert(m_lastInitType != Type::___);
        CHECK_RESULT(m_validator.BeginInitExpr(GetLocation(), m_lastInitType));
        PushLabel(LabelKind::Try);
        m_externalDelegate->BeginElemSegmentInitExpr(index);
        return Result::Ok;
    }
    Result EndElemSegmentInitExpr(Index index) override {
        m_lastInitType = Type::___;
        CHECK_RESULT(m_validator.EndInitExpr());
        PopLabel();
        m_externalDelegate->EndElemSegmentInitExpr(index);
        return Result::Ok;
    }
    Result OnElemSegmentElemType(Index index, Type elem_type) override {
        m_lastInitType = elem_type;
        CHECK_RESULT(m_validator.OnElemSegmentElemType(GetLocation(), elem_type));
        m_externalDelegate->OnElemSegmentElemType(index, elem_type);
        return Result::Ok;
    }
    Result OnElemSegmentElemExprCount(Index index, Index count) override {
        m_externalDelegate->OnElemSegmentElemExprCount(index, count);
        return Result::Ok;
    }
    Result BeginElemExpr(Index elem_index, Index expr_index) override {
        assert(m_lastInitType != Type::___);
        CHECK_RESULT(m_validator.BeginInitExpr(GetLocation(), m_lastInitType));
        PushLabel(LabelKind::Try);
        m_externalDelegate->BeginElemExpr(elem_index, expr_index);
        return Result::Ok;
    }
    Result EndElemExpr(Index elem_index, Index expr_index) override {
        CHECK_RESULT(m_validator.EndInitExpr());
        PopLabel();
        m_externalDelegate->EndElemExpr(elem_index, expr_index);
        return Result::Ok;
    }
    Result EndElemSegment(Index index) override {
        m_lastInitType = Type::___;
        m_externalDelegate->EndElemSegment(index);
        return Result::Ok;
    }
    Result EndElemSection() override {
        return Result::Ok;
    }

    /* Data section */
    Result BeginDataSection(Offset size) override {
        return Result::Ok;
    }
    Result OnDataSegmentCount(Index count) override {
        m_externalDelegate->OnDataSegmentCount(count);
        return Result::Ok;
    }
    Result BeginDataSegment(Index index, Index memory_index, uint8_t flags) override {
        auto mode = ToSegmentMode(flags);
        CHECK_RESULT(m_validator.OnDataSegment(GetLocation(), Var(memory_index, GetLocation()), mode));
        m_externalDelegate->BeginDataSegment(index, memory_index, flags);
        m_lastInitType = Type::I32;
        return Result::Ok;
    }
    Result BeginDataSegmentInitExpr(Index index) override {
        assert(m_lastInitType != Type::___);
        CHECK_RESULT(m_validator.BeginInitExpr(GetLocation(), m_lastInitType));
        PushLabel(LabelKind::Try);
        m_externalDelegate->BeginDataSegmentInitExpr(index);
        return Result::Ok;
    }
    Result EndDataSegmentInitExpr(Index index) override {
        m_lastInitType = Type::___;
        CHECK_RESULT(m_validator.EndInitExpr());
        PopLabel();
        m_externalDelegate->EndDataSegmentInitExpr(index);
        return Result::Ok;
    }
    Result OnDataSegmentData(Index index, const void *data, Address size) override {
        m_externalDelegate->OnDataSegmentData(index, data, size);
        return Result::Ok;
    }
    Result EndDataSegment(Index index) override {
        m_externalDelegate->EndDataSegment(index);
        return Result::Ok;
    }
    Result EndDataSection() override {
        return Result::Ok;
    }

    /* DataCount section */
    Result BeginDataCountSection(Offset size) override {
        return Result::Ok;
    }
    Result OnDataCount(Index count) override {
        m_validator.OnDataCount(count);
        return Result::Ok;
    }
    Result EndDataCountSection() override {
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
    Result OnModuleName(nonstd::string_view name) override {
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
    Result OnFunctionName(Index function_index, nonstd::string_view function_name) override {
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
    Result OnLocalName(Index function_index, Index local_index, nonstd::string_view local_name) override {
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
    Result OnNameEntry(NameSectionSubsection type, Index index, nonstd::string_view name) override {
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
        return Result::Ok;
    }
    Result OnTagCount(Index count) override {
        m_externalDelegate->OnTagCount(count);
        return Result::Ok;
    }
    Result OnTagType(Index index, Index sig_index) override {
        CHECK_RESULT(m_validator.OnTag(GetLocation(), Var(sig_index, GetLocation())));
        m_externalDelegate->OnTagType(index, sig_index);
        return Result::Ok;
    }
    Result EndTagSection() override {
        return Result::Ok;
    }

    /* Code Metadata sections */
    Result BeginCodeMetadataSection(nonstd::string_view name, Offset size) override {
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
    Result OnDylinkNeeded(nonstd::string_view so_name) override {
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
    Result OnDylinkImport(nonstd::string_view module, nonstd::string_view name, uint32_t flags) override {
        abort();
        return Result::Ok;
    }
    Result OnDylinkExport(nonstd::string_view name, uint32_t flags) override {
        abort();
        return Result::Ok;
    }
    Result EndDylinkSection() override {
        abort();
        return Result::Ok;
    }

    /* target_features section */
    Result BeginTargetFeaturesSection(Offset size) override {
        return Result::Ok;
    }
    Result OnFeatureCount(Index count) override {
        m_externalDelegate->OnFeatureCount(count);
        return Result::Ok;
    }
    Result OnFeature(uint8_t prefix, nonstd::string_view name) override {
        return m_externalDelegate->OnFeature(prefix, std::string(name)) ? Result::Ok : Result::Error;
    }
    Result EndTargetFeaturesSection() override {
        return Result::Ok;
    }

    /* Generic custom section */
    Result BeginGenericCustomSection(Offset size) override {
        return Result::Ok;
    }
    Result OnGenericCustomSection(nonstd::string_view name, const void* data, Offset size) override {
        return Result::Ok;
    }
    Result EndGenericCustomSection() override {
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
    Result OnDataSymbol(Index index, uint32_t flags, nonstd::string_view name, Index segment, uint32_t offset, uint32_t size) override {
        abort();
        return Result::Ok;
    }
    Result OnFunctionSymbol(Index index, uint32_t flags, nonstd::string_view name, Index func_index) override {
        abort();
        return Result::Ok;
    }
    Result OnGlobalSymbol(Index index, uint32_t flags, nonstd::string_view name, Index global_index) override {
        abort();
        return Result::Ok;
    }
    Result OnSectionSymbol(Index index, uint32_t flags, Index section_index) override {
        abort();
        return Result::Ok;
    }
    Result OnTagSymbol(Index index, uint32_t flags, nonstd::string_view name, Index tag_index) override {
        abort();
        return Result::Ok;
    }
    Result OnTableSymbol(Index index, uint32_t flags, nonstd::string_view name, Index table_index) override {
        abort();
        return Result::Ok;
    }
    Result OnSegmentInfoCount(Index count) override {
        abort();
        return Result::Ok;
    }
    Result OnSegmentInfo(Index index, nonstd::string_view name, Address alignment, uint32_t flags) override {
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
    Result OnComdatBegin(nonstd::string_view name, uint32_t flags, Index count) override {
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

    WASMBinaryReaderDelegate *m_externalDelegate;
    const std::string &m_filename;
    Errors m_errors;
    SharedValidator m_validator;
    std::vector<Label> m_labelStack;
    std::vector<SimpleFuncType> m_functionTypes;
    Type m_lastInitType;
    std::vector<Type> m_tableTypes;
    Index m_currentElementTableIndex;
};

std::string ReadWasmBinary(const std::string &filename, const uint8_t *data, size_t size, WASMBinaryReaderDelegate *delegate) {
    const bool kReadDebugNames = false;
    const bool kStopOnFirstError = true;
    const bool kFailOnCustomSectionError = true;
    ReadBinaryOptions options(getFeatures(), nullptr, kReadDebugNames, kStopOnFirstError, kFailOnCustomSectionError);
    BinaryReaderDelegateWalrus binaryReaderDelegateWalrus(delegate, filename);
    Result result = ReadBinary(data, size, &binaryReaderDelegateWalrus, options);

    if (WABT_UNLIKELY(binaryReaderDelegateWalrus.m_errors.size())) {
        return std::move(binaryReaderDelegateWalrus.m_errors.begin()->message);
    }

    if (WABT_UNLIKELY(result != ::wabt::Result::Ok)) {
        return std::string("read wasm error");
    }

    return std::string();
}

}  // namespace wabt
