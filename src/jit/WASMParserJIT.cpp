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

#include "jit/Compiler.h"
#include "parser/WASMParser.h"
#include "runtime/JITExec.h"
#include "runtime/Module.h"
#include "runtime/Store.h"

#include "wabt/walrus/binary-reader-walrus.h"

namespace wabt {

static Walrus::Value::Type toValueKind(Type type)
{
    switch (type) {
    case Type::I32:
        return Walrus::Value::Type::I32;
    case Type::I64:
        return Walrus::Value::Type::I64;
    case Type::F32:
        return Walrus::Value::Type::F32;
    case Type::F64:
        return Walrus::Value::Type::F64;
    case Type::FuncRef:
        return Walrus::Value::Type::FuncRef;
    case Type::ExternRef:
        return Walrus::Value::Type::ExternRef;
    default:
        RELEASE_ASSERT_NOT_REACHED();
    }
}

class WASMBinaryReaderJIT : public wabt::WASMBinaryReaderDelegate {
public:
    WASMBinaryReaderJIT(Walrus::Module* module, int verboseLevel);

    virtual void OnSetOffsetAddress(size_t* o) override {}
    /* Module */
    virtual void BeginModule(uint32_t version) override {}
    virtual void EndModule() override;

    virtual void OnTypeCount(Index count) override {}
    virtual void OnFuncType(Index index, Index paramCount, Type* paramTypes, Index resultCount, Type* resultTypes) override;

    virtual void OnImportCount(Index count) override {}
    virtual void OnImportFunc(Index importIndex, std::string moduleName, std::string fieldName, Index funcIndex, Index sigIndex) override {}
    virtual void OnImportGlobal(Index importIndex, std::string moduleName, std::string fieldName, Index globalIndex, Type type, bool mutable_) override {}
    virtual void OnImportTable(Index importIndex, std::string moduleName, std::string fieldName, Index tableIndex, Type type, size_t initialSize, size_t maximumSize) override {}
    virtual void OnImportMemory(Index importIndex, std::string moduleName, std::string fieldName, Index memoryIndex, size_t initialSize, size_t maximumSize) override {}
    virtual void OnImportTag(Index importIndex, std::string moduleName, std::string fieldName, Index tagIndex, Index sigIndex) override {}

    virtual void OnExportCount(Index count) override {}
    virtual void OnExport(int kind, Index exportIndex, std::string name, Index itemIndex) override;

    virtual void OnMemoryCount(Index count) override {}
    virtual void OnMemory(Index index, size_t initialSize, size_t maximumSize) override {}

    virtual void OnDataSegmentCount(Index count) override {}
    virtual void BeginDataSegment(Index index, Index memoryIndex, uint8_t flags) override {}
    virtual void BeginDataSegmentInitExpr(Index index) override {}
    virtual void EndDataSegmentInitExpr(Index index) override {}
    virtual void OnDataSegmentData(Index index, const void* data, Address size) override {}
    virtual void EndDataSegment(Index index) override {}

    virtual void OnTableCount(Index count) override {}
    virtual void OnTable(Index index, Type type, size_t initialSize, size_t maximumSize) override {}

    virtual void OnElemSegmentCount(Index count) override {}
    virtual void BeginElemSegment(Index index, Index tableIndex, uint8_t flags) override {}
    virtual void BeginElemSegmentInitExpr(Index index) override {}
    virtual void EndElemSegmentInitExpr(Index index) override {}
    virtual void OnElemSegmentElemType(Index index, Type elemType) override {}
    virtual void OnElemSegmentElemExprCount(Index index, Index count) override {}
    virtual void OnElemSegmentElemExpr_RefNull(Index segmentIndex, Type type) override {}
    virtual void OnElemSegmentElemExpr_RefFunc(Index segmentIndex, Index funcIndex) override {}
    virtual void EndElemSegment(Index index) override {}

    virtual void OnFunctionCount(Index count) override;
    virtual void OnFunction(Index index, Index sigIndex) override;

    virtual void OnGlobalCount(Index count) override {}
    virtual void BeginGlobal(Index index, Type type, bool mutable_) override {}
    virtual void BeginGlobalInitExpr(Index index) override {}
    virtual void EndGlobalInitExpr(Index index) override {}
    virtual void EndGlobal(Index index) override {}
    virtual void EndGlobalSection() override {}

    virtual void OnTagCount(Index count) override {}
    virtual void OnTagType(Index index, Index sigIndex) override {}

    virtual void OnStartFunction(Index funcIndex) override {}

    virtual void BeginFunctionBody(Index index, Offset size) override;

    virtual void OnLocalDeclCount(Index count) override {}
    virtual void OnLocalDecl(Index declIndex, Index count, Type type) override;

    virtual void OnStartReadInstructions() override {}

    virtual void OnOpcode(uint32_t opcode) override {}

    virtual void OnCallExpr(Index index) override;
    virtual void OnCallIndirectExpr(Index sigIndex, Index tableIndex) override;
    virtual void OnI32ConstExpr(uint32_t value) override;
    virtual void OnI64ConstExpr(uint64_t value) override;
    virtual void OnF32ConstExpr(uint32_t value) override;
    virtual void OnF64ConstExpr(uint64_t value) override;
    virtual void OnLocalGetExpr(Index localIndex) override;
    virtual void OnLocalSetExpr(Index localIndex) override;
    virtual void OnLocalTeeExpr(Index localIndex) override;
    virtual void OnGlobalGetExpr(Index globalIndex) override {}
    virtual void OnGlobalSetExpr(Index globalIndex) override {}
    virtual void OnDropExpr() override;
    virtual void OnBinaryExpr(uint32_t opcode) override;
    virtual void OnUnaryExpr(uint32_t opcode) override;
    virtual void OnIfExpr(Type sigType) override;
    virtual void OnElseExpr() override;
    virtual void OnLoopExpr(Type sigType) override;
    virtual void OnBlockExpr(Type sigType) override;
    virtual void OnBrExpr(Index depth) override;
    virtual void OnBrIfExpr(Index depth) override;
    virtual void OnBrTableExpr(Index numTargets, Index* targetDepths, Index defaultTargetDepth) override;
    virtual void OnSelectExpr(Index resultCount, Type* resultTypes) override;
    virtual void OnThrowExpr(Index tagIndex) override {}
    virtual void OnTryExpr(Type sigType) override {}
    virtual void OnCatchExpr(Index tagIndex) override {}
    virtual void OnCatchAllExpr() override {}
    virtual void OnMemoryGrowExpr(Index memidx) override {}
    virtual void OnMemoryInitExpr(Index segmentIndex, Index memidx) override {}
    virtual void OnMemoryCopyExpr(Index srcMemIndex, Index dstMemIndex) override {}
    virtual void OnMemoryFillExpr(Index memidx) override {}
    virtual void OnDataDropExpr(Index segmentIndex) override {}
    virtual void OnMemorySizeExpr(Index memidx) override {}
    virtual void OnTableGetExpr(Index tableIndex) override {}
    virtual void OnTableSetExpr(Index tableIndex) override {}
    virtual void OnTableGrowExpr(Index tableIndex) override {}
    virtual void OnTableSizeExpr(Index tableIndex) override {}
    virtual void OnTableCopyExpr(Index dst_index, Index src_index) override {}
    virtual void OnTableFillExpr(Index tableIndex) override {}
    virtual void OnElemDropExpr(Index segmentIndex) override {}
    virtual void OnTableInitExpr(Index segmentIndex, Index tableIndex) override {}
    virtual void OnLoadExpr(int opcode, Index memidx, Address alignmentLog2, Address offset) override {}
    virtual void OnStoreExpr(int opcode, Index memidx, Address alignmentLog2, Address offset) override {}
    virtual void OnReturnExpr() override {}
    virtual void OnRefFuncExpr(Index func_index) override {}
    virtual void OnRefNullExpr(Type type) override {}
    virtual void OnRefIsNullExpr() override {}
    virtual void OnNopExpr() override {}
    virtual void OnEndExpr() override;
    virtual void OnUnreachableExpr() override;
    virtual void EndFunctionBody(Index index) override;

private:
    void pushLabel(Walrus::OpcodeKind opcode, Type sigType);

    Walrus::JITCompiler m_compiler;
    Walrus::Module* m_module;

    Index m_functionBodyIndex;
    size_t m_functionImportEnd = 0;

    std::vector<bool> m_functionIsExported;
};

WASMBinaryReaderJIT::WASMBinaryReaderJIT(Walrus::Module* module, int verboseLevel)
    : m_compiler(verboseLevel)
    , m_module(module)
{
}

void WASMBinaryReaderJIT::pushLabel(Walrus::OpcodeKind opcode, Type sigType)
{
    Index paramCount = 0;
    Index resultCount = 0;

    if (sigType.IsIndex()) {
        Walrus::FunctionType* functionType = m_module->functionType(sigType);
        paramCount = functionType->param().size();
        resultCount = functionType->result().size();
    } else if (sigType != Type::Void) {
        resultCount = 1;
    }

    m_compiler.pushLabel(opcode, paramCount, resultCount);
}

void WASMBinaryReaderJIT::EndModule()
{
    m_module->setJITModule(m_compiler.compile());
}

void WASMBinaryReaderJIT::OnFuncType(Index index, Index paramCount, Type* paramTypes, Index resultCount, Type* resultTypes)
{
    Walrus::ValueTypeVector* param = new Walrus::ValueTypeVector();
    param->reserve(paramCount);

    for (size_t i = 0; i < paramCount; i++) {
        param->push_back(toValueKind(paramTypes[i]));
    }

    Walrus::ValueTypeVector* result = new Walrus::ValueTypeVector();
    result->reserve(resultCount);

    for (size_t i = 0; i < resultCount; i++) {
        result->push_back(toValueKind(resultTypes[i]));
    }
    ASSERT(index == m_module->m_functionTypes.size());
    m_module->m_functionTypes.push_back(new Walrus::FunctionType(param, result));
}

void WASMBinaryReaderJIT::OnFunctionCount(Index count)
{
    m_module->m_functions.reserve(count);
}

void WASMBinaryReaderJIT::OnFunction(Index index, Index sigIndex)
{
    ASSERT(m_module->m_functions.size() == index);
    m_module->m_functions.push_back(new Walrus::ModuleFunction(m_module->functionType(sigIndex)));
    m_functionIsExported.push_back(false);
}

void WASMBinaryReaderJIT::OnExport(int kind, Index exportIndex, std::string name, Index itemIndex)
{
    ASSERT(m_module->m_exports.size() == exportIndex);

    Walrus::ExportType::Type exportKind = static_cast<Walrus::ExportType::Type>(kind);

    if (exportKind == Walrus::ExportType::Function && itemIndex >= m_functionImportEnd) {
        m_functionIsExported[itemIndex - m_functionImportEnd] = true;
    }

    m_module->m_exports.pushBack(new Walrus::ExportType(exportKind, name, itemIndex));
}

void WASMBinaryReaderJIT::BeginFunctionBody(Index index, Offset size)
{
    m_functionBodyIndex = index;

    Walrus::FunctionType* functionType = m_module->function(index)->functionType();

    m_compiler.pushLabel(Walrus::BlockOpcode, 0, functionType->result().size());

    for (auto it : functionType->param()) {
        m_compiler.locals().push_back(Walrus::LocationInfo::typeToValueInfo(it));
    }
}

void WASMBinaryReaderJIT::OnLocalDecl(Index declIndex, Index count, Type type)
{
    if (count <= 0) {
        return;
    }

    Walrus::ValueInfo valueInfo = Walrus::LocationInfo::typeToValueInfo(toValueKind(type));

    do {
        m_compiler.locals().push_back(valueInfo);
    } while (--count != 0);
}

void WASMBinaryReaderJIT::OnBinaryExpr(uint32_t opcode)
{
    Walrus::ValueInfo result = Walrus::LocationInfo::kFourByteSize;
    Walrus::Instruction::Group group = Walrus::Instruction::Binary;

    switch (opcode) {
    case Walrus::I64AddOpcode:
    case Walrus::I64SubOpcode:
    case Walrus::I64MulOpcode:
    case Walrus::I64DivSOpcode:
    case Walrus::I64DivUOpcode:
    case Walrus::I64RemSOpcode:
    case Walrus::I64RemUOpcode:
    case Walrus::I64RotlOpcode:
    case Walrus::I64RotrOpcode:
    case Walrus::I64AndOpcode:
    case Walrus::I64OrOpcode:
    case Walrus::I64XorOpcode:
    case Walrus::I64ShlOpcode:
    case Walrus::I64ShrSOpcode:
    case Walrus::I64ShrUOpcode:
        result = Walrus::LocationInfo::kEightByteSize;
        break;
    case Walrus::F32AddOpcode:
    case Walrus::F32SubOpcode:
    case Walrus::F32MulOpcode:
    case Walrus::F32DivOpcode:
    case Walrus::F32MaxOpcode:
    case Walrus::F32MinOpcode:
    case Walrus::F32CopysignOpcode:
        group = Walrus::Instruction::BinaryFloat;
        result = Walrus::LocationInfo::kFourByteSize | Walrus::LocationInfo::kFloat;
        break;
    case Walrus::F64AddOpcode:
    case Walrus::F64SubOpcode:
    case Walrus::F64MulOpcode:
    case Walrus::F64DivOpcode:
    case Walrus::F64MaxOpcode:
    case Walrus::F64MinOpcode:
    case Walrus::F64CopysignOpcode:
        group = Walrus::Instruction::BinaryFloat;
        result = Walrus::LocationInfo::kEightByteSize | Walrus::LocationInfo::kFloat;
        break;
    case Walrus::I32EqzOpcode:
    case Walrus::I64EqzOpcode:
    case Walrus::I32EqOpcode:
    case Walrus::I64EqOpcode:
    case Walrus::I32NeOpcode:
    case Walrus::I64NeOpcode:
    case Walrus::I32LtSOpcode:
    case Walrus::I64LtSOpcode:
    case Walrus::I32LtUOpcode:
    case Walrus::I64LtUOpcode:
    case Walrus::I32GtSOpcode:
    case Walrus::I64GtSOpcode:
    case Walrus::I32GtUOpcode:
    case Walrus::I64GtUOpcode:
    case Walrus::I32LeSOpcode:
    case Walrus::I64LeSOpcode:
    case Walrus::I32LeUOpcode:
    case Walrus::I64LeUOpcode:
    case Walrus::I32GeSOpcode:
    case Walrus::I64GeSOpcode:
    case Walrus::I32GeUOpcode:
    case Walrus::I64GeUOpcode:
        group = Walrus::Instruction::Compare;
        break;
    case Walrus::F32EqOpcode:
    case Walrus::F64EqOpcode:
    case Walrus::F32NeOpcode:
    case Walrus::F64NeOpcode:
    case Walrus::F32LtOpcode:
    case Walrus::F64LtOpcode:
    case Walrus::F32GtOpcode:
    case Walrus::F64GtOpcode:
    case Walrus::F32LeOpcode:
    case Walrus::F64LeOpcode:
    case Walrus::F32GeOpcode:
    case Walrus::F64GeOpcode:
        group = Walrus::Instruction::CompareFloat;
        break;
    default:
        break;
    }

    m_compiler.append(group, static_cast<Walrus::OpcodeKind>(opcode), 2, result);
}

void WASMBinaryReaderJIT::OnSelectExpr(Index resultCount, Type* resultTypes)
{
    /* The result type is unknown, set by buildParamDependencies(). */
    m_compiler.append(Walrus::Instruction::Any, Walrus::SelectOpcode, 3, Walrus::LocationInfo::kFourByteSize);
}

void WASMBinaryReaderJIT::OnUnaryExpr(uint32_t opcode)
{
    Walrus::Instruction::Group group = Walrus::Instruction::Unary;
    Walrus::ValueInfo result = Walrus::LocationInfo::kFourByteSize;

    switch (opcode) {
    case Walrus::I64ClzOpcode:
    case Walrus::I64CtzOpcode:
    case Walrus::I64PopcntOpcode:
    case Walrus::I64Extend8SOpcode:
    case Walrus::I64Extend16SOpcode:
    case Walrus::I64Extend32SOpcode:
        result = Walrus::LocationInfo::kEightByteSize;
        break;
    case Walrus::F32CeilOpcode:
    case Walrus::F32FloorOpcode:
    case Walrus::F32TruncOpcode:
    case Walrus::F32NearestOpcode:
    case Walrus::F32SqrtOpcode:
    case Walrus::F32AbsOpcode:
    case Walrus::F32NegOpcode:
        group = Walrus::Instruction::UnaryFloat;
        result = Walrus::LocationInfo::kFourByteSize | Walrus::LocationInfo::kFloat;
        break;
    case Walrus::F64CeilOpcode:
    case Walrus::F64FloorOpcode:
    case Walrus::F64TruncOpcode:
    case Walrus::F64NearestOpcode:
    case Walrus::F64SqrtOpcode:
    case Walrus::F64AbsOpcode:
    case Walrus::F64NegOpcode:
        group = Walrus::Instruction::UnaryFloat;
        result = Walrus::LocationInfo::kEightByteSize | Walrus::LocationInfo::kFloat;
        break;
    case Walrus::I32EqzOpcode:
    case Walrus::I64EqzOpcode:
        group = Walrus::Instruction::Compare;
        result = Walrus::LocationInfo::kFourByteSize;
        break;
    case Walrus::I32WrapI64Opcode:
        group = Walrus::Instruction::Convert;
        result = Walrus::LocationInfo::kFourByteSize;
        break;
    case Walrus::I64ExtendI32SOpcode:
    case Walrus::I64ExtendI32UOpcode:
        group = Walrus::Instruction::Convert;
        result = Walrus::LocationInfo::kEightByteSize;
        break;
    default:
        break;
    }

    m_compiler.append(group, static_cast<Walrus::OpcodeKind>(opcode), 1, result);
}

void WASMBinaryReaderJIT::OnBlockExpr(Type sigType)
{
    pushLabel(Walrus::BlockOpcode, sigType);
}

void WASMBinaryReaderJIT::OnBrExpr(Index depth)
{
    m_compiler.appendBranch(Walrus::BrOpcode, depth);
}

void WASMBinaryReaderJIT::OnBrIfExpr(Index depth)
{
    m_compiler.appendBranch(Walrus::BrIfOpcode, depth);
}

void WASMBinaryReaderJIT::OnBrTableExpr(Index numTargets, Index* targetDepths, Index defaultTargetDepth)
{
    m_compiler.appendBrTable(numTargets, targetDepths, defaultTargetDepth);
}

void WASMBinaryReaderJIT::OnCallExpr(Index index)
{
    Walrus::FunctionType* functionType = m_module->function(index)->functionType();

    Walrus::CallInstruction* callInstr = m_compiler.appendCall(Walrus::CallOpcode, functionType);

    if (callInstr != nullptr) {
        callInstr->value().funcIndex = index;
    }
}

void WASMBinaryReaderJIT::OnCallIndirectExpr(Index sigIndex, Index tableIndex)
{
}

void WASMBinaryReaderJIT::OnDropExpr()
{
    m_compiler.append(Walrus::Instruction::Any, Walrus::DropOpcode, 1);
}

void WASMBinaryReaderJIT::OnElseExpr()
{
    m_compiler.appendElseLabel();
}

void WASMBinaryReaderJIT::OnEndExpr()
{
    m_compiler.popLabel();
}

void WASMBinaryReaderJIT::OnF32ConstExpr(uint32_t value)
{
    Walrus::Instruction* instr = m_compiler.append(Walrus::Instruction::Immediate, Walrus::F32ConstOpcode, 0, Walrus::LocationInfo::kFourByteSize | Walrus::LocationInfo::kFloat);
    if (instr != nullptr) {
        instr->value().value32 = value;
    }
}

void WASMBinaryReaderJIT::OnF64ConstExpr(uint64_t value)
{
    Walrus::Instruction* instr = m_compiler.append(Walrus::Instruction::Immediate, Walrus::F32ConstOpcode, 0, Walrus::LocationInfo::kEightByteSize | Walrus::LocationInfo::kFloat);
    if (instr != nullptr) {
        instr->value().value64 = value;
    }
}

void WASMBinaryReaderJIT::OnI32ConstExpr(uint32_t value)
{
    Walrus::Instruction* instr = m_compiler.append(Walrus::Instruction::Immediate, Walrus::I32ConstOpcode, 0, Walrus::LocationInfo::kFourByteSize);
    if (instr != nullptr) {
        instr->value().value32 = value;
    }
}

void WASMBinaryReaderJIT::OnI64ConstExpr(uint64_t value)
{
    Walrus::Instruction* instr = m_compiler.append(Walrus::Instruction::Immediate, Walrus::I64ConstOpcode, 0, Walrus::LocationInfo::kEightByteSize);
    if (instr != nullptr) {
        instr->value().value64 = value;
    }
}

void WASMBinaryReaderJIT::OnIfExpr(Type sigType)
{
    pushLabel(Walrus::IfOpcode, sigType);
}

void WASMBinaryReaderJIT::OnLocalGetExpr(Index localIndex)
{
    Walrus::Instruction* instr = m_compiler.append(Walrus::Instruction::LocalMove, Walrus::LocalGetOpcode, 0, m_compiler.local(localIndex));
    if (instr != nullptr) {
        instr->value().localIndex = localIndex;
    }
}

void WASMBinaryReaderJIT::OnLocalSetExpr(Index localIndex)
{
    Walrus::Instruction* instr = m_compiler.append(Walrus::Instruction::LocalMove, Walrus::LocalSetOpcode, 1);
    if (instr != nullptr) {
        instr->value().localIndex = localIndex;
    }
}

void WASMBinaryReaderJIT::OnLocalTeeExpr(Index localIndex)
{
    Walrus::Instruction* instr = m_compiler.append(Walrus::Instruction::LocalMove, Walrus::LocalSetOpcode, 1);

    if (instr != nullptr) {
        instr->value().localIndex = localIndex;
        instr = m_compiler.append(Walrus::Instruction::LocalMove, Walrus::LocalGetOpcode, 0, m_compiler.local(localIndex));
        assert(instr != nullptr);
        instr->value().localIndex = localIndex;
    }
}

void WASMBinaryReaderJIT::OnLoopExpr(Type sigType)
{
    pushLabel(Walrus::LoopOpcode, sigType);
}

void WASMBinaryReaderJIT::OnUnreachableExpr()
{
    m_compiler.appendUnreachable();
}

void WASMBinaryReaderJIT::EndFunctionBody(Index index)
{
    Walrus::FunctionType* functionType = m_module->function(m_functionBodyIndex)->functionType();

    m_compiler.append(Walrus::Instruction::Return, Walrus::ReturnOpcode, functionType->result().size());

    if (m_compiler.verboseLevel() >= 1) {
        printf("[[[[[[[  Function %3d  ]]]]]]]\n", static_cast<int>(index));
    }

    m_compiler.checkLocals(functionType->param().size());
    m_compiler.buildParamDependencies();

    if (m_compiler.verboseLevel() >= 1) {
        m_compiler.dump(false);
    }

    m_compiler.reduceLocalAndConstantMoves();
    m_compiler.optimizeBlocks();

    Walrus::JITFunction* jitFunc = new Walrus::JITFunction();
    m_module->function(m_functionBodyIndex)->setJITFunction(jitFunc);
    m_compiler.computeOperandLocations(jitFunc, functionType->result());

    if (m_compiler.verboseLevel() >= 1) {
        printf("------------------------------\n");
        printf("Frame size: %d, Arguments size: %d (total: %d)\n", jitFunc->frameSize(), jitFunc->argsSize(), jitFunc->frameSize() + jitFunc->argsSize());
        m_compiler.dump(true);
        printf("\n");
    }

    m_compiler.appendFunction(jitFunc, m_functionIsExported[m_functionBodyIndex]);

    m_compiler.clear();
}

} // namespace wabt

namespace Walrus {

std::pair<Optional<Module*>, std::string> WASMParser::parseBinaryJIT(Store* store, const std::string& filename, const uint8_t* data, size_t len, int verboseLevel)
{
    Module* module = new Module(store);
    wabt::WASMBinaryReaderJIT delegate(module, verboseLevel);

    std::string error = ReadWasmBinary(filename, data, len, &delegate);
    if (error.length()) {
        // remove invalid module
        store->deleteModule(module);
        return std::make_pair(nullptr, error);
    }

    return std::make_pair(module, std::string());
}

} // namespace Walrus
