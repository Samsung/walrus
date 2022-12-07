/*
 * Copyright 2020 WebAssembly Community Group participants
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

#include "wabt/interp/jit/binary-reader-jit.h"
#include "wabt/binary-reader-nop.h"
#include "wabt/interp/jit/jit-compiler.h"
#include "wabt/interp/jit/jit.h"
#include "wabt/shared-validator.h"

namespace wabt {
namespace interp {

class BinaryReaderJIT : public BinaryReaderNop {
 public:
  BinaryReaderJIT(ModuleDesc* module,
                  std::string_view filename,
                  Errors* errors,
                  const Features& features,
                  int verbose_level);

  // Implement BinaryReader.
  bool OnError(const Error&) override;

  Result EndModule() override;

  Result OnTypeCount(Index count) override;
  Result OnFuncType(Index index,
                    Index param_count,
                    Type* param_types,
                    Index result_count,
                    Type* result_types) override;

#if 0
  Result OnImportFunc(Index import_index,
                      std::string_view module_name,
                      std::string_view field_name,
                      Index func_index,
                      Index sig_index) override;
  Result OnImportTable(Index import_index,
                       std::string_view module_name,
                       std::string_view field_name,
                       Index table_index,
                       Type elem_type,
                       const Limits* elem_limits) override;
  Result OnImportMemory(Index import_index,
                        std::string_view module_name,
                        std::string_view field_name,
                        Index memory_index,
                        const Limits* page_limits) override;
  Result OnImportGlobal(Index import_index,
                        std::string_view module_name,
                        std::string_view field_name,
                        Index global_index,
                        Type type,
                        bool mutable_) override;
  Result OnImportTag(Index import_index,
                     std::string_view module_name,
                     std::string_view field_name,
                     Index tag_index,
                     Index sig_index) override;

#endif
  Result OnFunctionCount(Index count) override;
  Result OnFunction(Index index, Index sig_index) override;
#if 0

  Result OnTableCount(Index count) override;
  Result OnTable(Index index,
                 Type elem_type,
                 const Limits* elem_limits) override;

  Result OnMemoryCount(Index count) override;
  Result OnMemory(Index index, const Limits* limits) override;

  Result OnGlobalCount(Index count) override;
  Result BeginGlobal(Index index, Type type, bool mutable_) override;
  Result BeginGlobalInitExpr(Index index) override;
  Result EndGlobalInitExpr(Index index) override;

  Result OnTagCount(Index count) override;
  Result OnTagType(Index index, Index sig_index) override;

#endif
  Result OnExport(Index index,
                  ExternalKind kind,
                  Index item_index,
                  std::string_view name) override;

#if 0
  Result OnStartFunction(Index func_index) override;
#endif
  Result BeginFunctionBody(Index index, Offset size) override;
  // Result OnLocalDeclCount(Index count) override;
  Result OnLocalDecl(Index decl_index, Index count, Type type) override;
#if 0

  Result OnOpcode(Opcode Opcode) override;
  Result OnAtomicLoadExpr(Opcode opcode,
                          Index memidx,
                          Address alignment_log2,
                          Address offset) override;
  Result OnAtomicStoreExpr(Opcode opcode,
                           Index memidx,
                           Address alignment_log2,
                           Address offset) override;
  Result OnAtomicRmwExpr(Opcode opcode,
                         Index memidx,
                         Address alignment_log2,
                         Address offset) override;
  Result OnAtomicRmwCmpxchgExpr(Opcode opcode,
                                Index memidx,
                                Address alignment_log2,
                                Address offset) override;
  Result OnAtomicWaitExpr(Opcode opcode,
                          Index memidx,
                          Address alignment_log2,
                          Address offset) override;
  Result OnAtomicFenceExpr(uint32_t consistency_model) override;
  Result OnAtomicNotifyExpr(Opcode opcode,
                            Index memidx,
                            Address alignment_log2,
                            Address offset) override;
#endif
  Result OnBinaryExpr(Opcode opcode) override;
  Result OnBlockExpr(Type sig_type) override;
  Result OnBrExpr(Index depth) override;
  Result OnBrIfExpr(Index depth) override;
  Result OnBrTableExpr(Index num_targets,
                       Index* target_depths,
                       Index default_target_depth) override;
#if 0
  Result OnCallExpr(Index func_index) override;
  Result OnCallIndirectExpr(Index sig_index, Index table_index) override;
  Result OnCatchExpr(Index tag_index) override;
  Result OnCatchAllExpr() override;
  Result OnDelegateExpr(Index depth) override;
  Result OnReturnCallExpr(Index func_index) override;
  Result OnReturnCallIndirectExpr(Index sig_index, Index table_index) override;
#endif
  Result OnCompareExpr(Opcode opcode) override;
  Result OnConvertExpr(Opcode opcode) override;
  Result OnDropExpr() override;
  Result OnElseExpr() override;
  Result OnEndExpr() override;
#if 0
  Result OnF32ConstExpr(uint32_t value_bits) override;
  Result OnF64ConstExpr(uint64_t value_bits) override;
  Result OnV128ConstExpr(v128 value_bits) override;
  Result OnGlobalGetExpr(Index global_index) override;
  Result OnGlobalSetExpr(Index global_index) override;
#endif
  Result OnI32ConstExpr(uint32_t value) override;
  Result OnI64ConstExpr(uint64_t value) override;
  Result OnIfExpr(Type sig_type) override;
#if 0
  Result OnLoadExpr(Opcode opcode,
                    Index memidx,
                    Address alignment_log2,
                    Address offset) override;
#endif
  Result OnLocalGetExpr(Index local_index) override;
  Result OnLocalSetExpr(Index local_index) override;
  Result OnLocalTeeExpr(Index local_index) override;
  Result OnLoopExpr(Type sig_type) override;
#if 0
  Result OnMemoryCopyExpr(Index srcmemidx, Index destmemidx) override;
  Result OnDataDropExpr(Index segment_index) override;
  Result OnMemoryGrowExpr(Index memidx) override;
  Result OnMemoryFillExpr(Index memidx) override;
  Result OnMemoryInitExpr(Index segment_index, Index memidx) override;
  Result OnMemorySizeExpr(Index memidx) override;
  Result OnRefFuncExpr(Index func_index) override;
  Result OnRefNullExpr(Type type) override;
  Result OnRefIsNullExpr() override;
  Result OnNopExpr() override;
  Result OnRethrowExpr(Index depth) override;
  Result OnReturnExpr() override;
  Result OnSelectExpr(Index result_count, Type* result_types) override;
  Result OnStoreExpr(Opcode opcode,
                     Index memidx,
                     Address alignment_log2,
                     Address offset) override;
#endif
  Result OnUnaryExpr(Opcode opcode) override;
#if 0
  Result OnTableCopyExpr(Index dst_index, Index src_index) override;
  Result OnTableGetExpr(Index table_index) override;
  Result OnTableSetExpr(Index table_index) override;
  Result OnTableGrowExpr(Index table_index) override;
  Result OnTableSizeExpr(Index table_index) override;
  Result OnTableFillExpr(Index table_index) override;
  Result OnElemDropExpr(Index segment_index) override;
  Result OnTableInitExpr(Index segment_index, Index table_index) override;
  Result OnTernaryExpr(Opcode opcode) override;
  Result OnThrowExpr(Index tag_index) override;
  Result OnTryExpr(Type sig_type) override;
  Result OnUnreachableExpr() override;
#endif
  Result EndFunctionBody(Index index) override;
#if 0
  Result OnSimdLaneOpExpr(Opcode opcode, uint64_t value) override;
  Result OnSimdLoadLaneExpr(Opcode opcode,
                            Index memidx,
                            Address alignment_log2,
                            Address offset,
                            uint64_t value) override;
  Result OnSimdStoreLaneExpr(Opcode opcode,
                             Index memidx,
                             Address alignment_log2,
                             Address offset,
                             uint64_t value) override;
  Result OnSimdShuffleOpExpr(Opcode opcode, v128 value) override;
  Result OnLoadSplatExpr(Opcode opcode,
                         Index memidx,
                         Address alignment_log2,
                         Address offset) override;
  Result OnLoadZeroExpr(Opcode opcode,
                        Index memidx,
                        Address alignment_log2,
                        Address offset) override;

  Result OnElemSegmentCount(Index count) override;
  Result BeginElemSegment(Index index,
                          Index table_index,
                          uint8_t flags) override;
  Result BeginElemSegmentInitExpr(Index index) override;
  Result EndElemSegmentInitExpr(Index index) override;
  Result OnElemSegmentElemType(Index index, Type elem_type) override;
  Result OnElemSegmentElemExprCount(Index index, Index count) override;
  Result OnElemSegmentElemExpr_RefNull(Index segment_index, Type type) override;
  Result OnElemSegmentElemExpr_RefFunc(Index segment_index,
                                       Index func_index) override;

  Result OnDataCount(Index count) override;
  Result BeginDataSegmentInitExpr(Index index) override;
  Result EndDataSegmentInitExpr(Index index) override;
  Result BeginDataSegment(Index index,
                          Index memory_index,
                          uint8_t flags) override;
  Result OnDataSegmentData(Index index,
                           const void* data,
                           Address size) override;
#endif
 private:
  Location GetLocation() const;
  void pushLabel(Opcode opcode, Type sig_type);

  int verbose_level_;
  Errors* errors_ = nullptr;
  ModuleDesc& module_;

  SharedValidator validator_;
  std::string_view filename_;

  Index function_body_index_;
  JITCompiler compiler_;

  // Includes imported and defined.
  std::vector<FuncType> func_types_;
};

BinaryReaderJIT::BinaryReaderJIT(ModuleDesc* module,
                                 std::string_view filename,
                                 Errors* errors,
                                 const Features& features,
                                 int verbose_level)
    : verbose_level_(verbose_level),
      errors_(errors),
      module_(*module),
      validator_(errors, ValidateOptions(features)),
      filename_(filename) {}

Location BinaryReaderJIT::GetLocation() const {
  Location loc;
  loc.filename = filename_;
  loc.offset = state->offset;
  return loc;
}

void BinaryReaderJIT::pushLabel(Opcode opcode, Type sig_type) {
  Index param_count = 0;
  Index result_count = 0;

  if (sig_type.IsIndex()) {
    FuncType& func_type = module_.func_types[sig_type.GetIndex()];
    param_count = func_type.params.size();
    result_count = func_type.results.size();
  } else if (sig_type != Type::Void) {
    result_count = 1;
  }

  compiler_.pushLabel(opcode, param_count, result_count);
}

bool BinaryReaderJIT::OnError(const Error& error) {
  errors_->push_back(error);
  return true;
}

Result BinaryReaderJIT::EndModule() {
  CHECK_RESULT(validator_.EndModule());
  compiler_.compile();
  return Result::Ok;
}

Result BinaryReaderJIT::OnTypeCount(Index count) {
  return Result::Ok;
}

Result BinaryReaderJIT::OnFuncType(Index index,
                                   Index param_count,
                                   Type* param_types,
                                   Index result_count,
                                   Type* result_types) {
  CHECK_RESULT(validator_.OnFuncType(GetLocation(), param_count, param_types,
                                     result_count, result_types, index));

  Type* param_end = param_types + param_count;
  Type* result_end = result_types + result_count;

  module_.func_types.push_back(FuncType(ValueTypes(param_types, param_end),
                                        ValueTypes(result_types, result_end)));
  return Result::Ok;
}

Result BinaryReaderJIT::OnFunctionCount(Index count) {
  module_.funcs.reserve(count);
  return Result::Ok;
}

Result BinaryReaderJIT::OnFunction(Index index, Index sig_index) {
  CHECK_RESULT(
      validator_.OnFunction(GetLocation(), Var(sig_index, GetLocation())));
  FuncType& func_type = module_.func_types[sig_index];
  module_.funcs.push_back(
      FuncDesc{func_type, {}, Istream::kInvalidOffset, {}, {}});
  func_types_.push_back(func_type);
  return Result::Ok;
}

Result BinaryReaderJIT::OnExport(Index index,
                                 ExternalKind kind,
                                 Index item_index,
                                 std::string_view name) {
  CHECK_RESULT(validator_.OnExport(GetLocation(), kind,
                                   Var(item_index, GetLocation()), name));

  std::unique_ptr<ExternType> type;
  switch (kind) {
    case ExternalKind::Func: {
      type = func_types_[item_index].Clone();
      break;
    }
    default: {
      return Result::Ok;
    }
  }
  module_.exports.push_back(
      ExportDesc{ExportType(std::string(name), std::move(type)), item_index});
  return Result::Ok;
}

Result BinaryReaderJIT::BeginFunctionBody(Index index, Offset size) {
  CHECK_RESULT(validator_.BeginFunctionBody(GetLocation(), index));

  function_body_index_ = index;

  FuncType& func_type = module_.funcs[index].type;

  compiler_.pushLabel(Opcode::Block, 0, func_type.results.size());

  for (auto it : func_type.params) {
    compiler_.locals().push_back(LocationInfo::typeToValueInfo(it));
  }

  return Result::Ok;
}

Result BinaryReaderJIT::OnLocalDecl(Index decl_index, Index count, Type type) {
  CHECK_RESULT(validator_.OnLocalDecl(GetLocation(), count, type));

  if (count <= 0) {
    return Result::Ok;
  }

  ValueInfo value_info = LocationInfo::typeToValueInfo(type);

  do {
    compiler_.locals().push_back(value_info);
  } while (--count != 0);

  return Result::Ok;
}

Result BinaryReaderJIT::OnBinaryExpr(Opcode opcode) {
  CHECK_RESULT(validator_.OnBinary(GetLocation(), opcode));
  compiler_.append(Instruction::Binary, opcode, 2, LocationInfo::kFourByteSize);
  return Result::Ok;
}

Result BinaryReaderJIT::OnUnaryExpr(Opcode opcode) {
  CHECK_RESULT(validator_.OnUnary(GetLocation(), opcode));
  compiler_.append(Instruction::Unary, opcode, 1, LocationInfo::kFourByteSize);
  return Result::Ok;
}

Result BinaryReaderJIT::OnBlockExpr(Type sig_type) {
  CHECK_RESULT(validator_.OnBlock(GetLocation(), sig_type));

  pushLabel(Opcode::Block, sig_type);
  return Result::Ok;
}

Result BinaryReaderJIT::OnBrExpr(Index depth) {
  CHECK_RESULT(validator_.OnBr(GetLocation(), Var(depth, GetLocation())));
  compiler_.appendBranch(Opcode::Br, depth);
  return Result::Ok;
}

Result BinaryReaderJIT::OnBrIfExpr(Index depth) {
  CHECK_RESULT(validator_.OnBrIf(GetLocation(), Var(depth, GetLocation())));
  compiler_.appendBranch(Opcode::BrIf, depth);
  return Result::Ok;
}

Result BinaryReaderJIT::OnBrTableExpr(Index num_targets,
                                      Index* target_depths,
                                      Index default_target_depth) {
  CHECK_RESULT(validator_.BeginBrTable(GetLocation()));
  for (Index i = 0; i < num_targets; ++i) {
    CHECK_RESULT(validator_.OnBrTableTarget(
        GetLocation(), Var(target_depths[i], GetLocation())));
  }
  CHECK_RESULT(validator_.OnBrTableTarget(
      GetLocation(), Var(default_target_depth, GetLocation())));
  CHECK_RESULT(validator_.EndBrTable(GetLocation()));
  compiler_.appendBrTable(num_targets, target_depths, default_target_depth);
  return Result::Ok;
}

Result BinaryReaderJIT::OnCompareExpr(Opcode opcode) {
  CHECK_RESULT(validator_.OnCompare(GetLocation(), opcode));
  compiler_.append(Instruction::Compare, opcode, 2,
                   LocationInfo::kFourByteSize);
  return Result::Ok;
}

Result BinaryReaderJIT::OnConvertExpr(Opcode opcode) {
  CHECK_RESULT(validator_.OnConvert(GetLocation(), opcode));
  if (opcode == Opcode::I32Eqz || opcode == Opcode::I64Eqz) {
    compiler_.append(Instruction::Compare, opcode, 1,
                     LocationInfo::kFourByteSize);
  }
  return Result::Ok;
}

Result BinaryReaderJIT::OnDropExpr() {
  CHECK_RESULT(validator_.OnDrop(GetLocation()));
  compiler_.append(Instruction::Any, Opcode::Drop, 1);
  return Result::Ok;
}

Result BinaryReaderJIT::OnElseExpr() {
  CHECK_RESULT(validator_.OnElse(GetLocation()));
  compiler_.appendElseLabel();
  return Result::Ok;
}

Result BinaryReaderJIT::OnEndExpr() {
  if (compiler_.labelCount() > 1) {
    CHECK_RESULT(validator_.OnEnd(GetLocation()));
  }

  compiler_.popLabel();
  return Result::Ok;
}

Result BinaryReaderJIT::OnI32ConstExpr(uint32_t value) {
  CHECK_RESULT(validator_.OnConst(GetLocation(), Type::I32));

  Instruction* instr = compiler_.append(
      Instruction::Immediate, Opcode::I32Const, 0, LocationInfo::kFourByteSize);
  if (instr != nullptr) {
    instr->value().value32 = value;
  }
  return Result::Ok;
}

Result BinaryReaderJIT::OnI64ConstExpr(uint64_t value) {
  CHECK_RESULT(validator_.OnConst(GetLocation(), Type::I64));

  Instruction* instr =
      compiler_.append(Instruction::Immediate, Opcode::I64Const, 0,
                       LocationInfo::kEightByteSize);
  if (instr != nullptr) {
    instr->value().value64 = value;
  }
  return Result::Ok;
}

Result BinaryReaderJIT::OnIfExpr(Type sig_type) {
  CHECK_RESULT(validator_.OnIf(GetLocation(), sig_type));

  pushLabel(Opcode::If, sig_type);
  return Result::Ok;
}

Result BinaryReaderJIT::OnLocalGetExpr(Index local_index) {
  CHECK_RESULT(
      validator_.OnLocalGet(GetLocation(), Var(local_index, GetLocation())));
  Instruction* instr =
      compiler_.append(Instruction::LocalMove, Opcode::LocalGet, 0,
                       compiler_.local(local_index));
  if (instr != nullptr) {
    instr->value().local_index = local_index;
  }
  return Result::Ok;
}

Result BinaryReaderJIT::OnLocalSetExpr(Index local_index) {
  CHECK_RESULT(
      validator_.OnLocalSet(GetLocation(), Var(local_index, GetLocation())));
  Instruction* instr =
      compiler_.append(Instruction::LocalMove, Opcode::LocalSet, 1);
  if (instr != nullptr) {
    instr->value().local_index = local_index;
  }
  return Result::Ok;
}

Result BinaryReaderJIT::OnLocalTeeExpr(Index local_index) {
  CHECK_RESULT(
      validator_.OnLocalTee(GetLocation(), Var(local_index, GetLocation())));
  Instruction* instr =
      compiler_.append(Instruction::LocalMove, Opcode::LocalSet, 1);
  if (instr != nullptr) {
    instr->value().local_index = local_index;
    instr = compiler_.append(Instruction::LocalMove, Opcode::LocalGet, 0,
                             compiler_.local(local_index));
    assert(instr != nullptr);
    instr->value().local_index = local_index;
  }
  return Result::Ok;
}

Result BinaryReaderJIT::OnLoopExpr(Type sig_type) {
  CHECK_RESULT(validator_.OnLoop(GetLocation(), sig_type));

  pushLabel(Opcode::Loop, sig_type);
  return Result::Ok;
}

Result BinaryReaderJIT::EndFunctionBody(Index index) {
  CHECK_RESULT(validator_.EndFunctionBody(GetLocation()));

  FuncType& func_type = module_.funcs[function_body_index_].type;
  compiler_.append(Instruction::Return, Opcode::Return,
                   func_type.results.size());

  compiler_.buildParamDependencies();

  if (verbose_level_ >= 1) {
    printf("++++++++++++++++++++++++++++++\nFunction: %d\n",
           static_cast<int>(index));
    compiler_.dump(verbose_level_ >= 2, false);
  }

  compiler_.reduceLocalAndConstantMoves();
  compiler_.optimizeBlocks();
  compiler_.computeOperandLocations(func_type.params.size(), func_type.results);

  if (verbose_level_ >= 1) {
    printf("------------------------------\n");
    compiler_.dump(verbose_level_ >= 2, true);
  }

  compiler_.appendFunction(&module_.funcs[function_body_index_].jit_func, true);

  compiler_.clear();
  return Result::Ok;
}

Result ReadBinaryJIT(std::string_view filename,
                     const void* data,
                     size_t size,
                     const ReadBinaryOptions& options,
                     Errors* errors,
                     ModuleDesc* out_module,
                     int verbose_level) {
  BinaryReaderJIT reader(out_module, filename, errors, options.features,
                         verbose_level);
  return ReadBinary(data, size, &reader, options);
}

}  // namespace interp
}  // namespace wabt
