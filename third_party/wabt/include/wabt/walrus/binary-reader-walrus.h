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

#ifndef WABT_BINARY_READER_WALRUS_H_
#define WABT_BINARY_READER_WALRUS_H_

#define WABT_COMMON_H_

#include <cstddef>
#include <cstdarg>

#include "wabt/base-types.h"
#include "wabt/type.h"

namespace wabt {

class WASMBinaryReaderDelegate {
public:
    WASMBinaryReaderDelegate()
        : m_shouldContinueToGenerateByteCode(true)
        , m_resumeGenerateByteCodeAfterNBlockEnd(0)
        , m_skipValidationUntil(0)
    {
    }
    virtual ~WASMBinaryReaderDelegate() { }
    virtual void OnSetOffsetAddress(size_t* o) = 0;
    /* Module */
    virtual void BeginModule(uint32_t version) = 0;
    virtual void EndModule() = 0;

    virtual void OnTypeCount(Index count) = 0;
    virtual void OnFuncType(Index index, Index paramCount, Type *paramTypes, Index resultCount, Type *resultTypes) = 0;

    virtual void OnImportCount(Index count) = 0;
    virtual void OnImportFunc(Index importIndex, std::string moduleName, std::string fieldName, Index funcIndex, Index sigIndex) = 0;
    virtual void OnImportGlobal(Index importIndex, std::string moduleName, std::string fieldName, Index globalIndex, Type type, bool mutable_) = 0;
    virtual void OnImportTable(Index importIndex, std::string moduleName, std::string fieldName, Index tableIndex, Type type, size_t initialSize, size_t maximumSize) = 0;
    virtual void OnImportMemory(Index importIndex, std::string moduleName, std::string fieldName, Index memoryIndex, size_t initialSize, size_t maximumSize) = 0;
    virtual void OnImportTag(Index importIndex, std::string moduleName, std::string fieldName, Index tagIndex, Index sigIndex) = 0;

    virtual void OnExportCount(Index count) = 0;
    virtual void OnExport(int kind, Index exportIndex, std::string name, Index itemIndex) = 0;

    virtual void OnMemoryCount(Index count) = 0;
    virtual void OnMemory(Index index, size_t initialSize, size_t maximumSize) = 0;

    virtual void OnDataSegmentCount(Index count) = 0;
    virtual void BeginDataSegment(Index index, Index memoryIndex, uint8_t flags) = 0;
    virtual void BeginDataSegmentInitExpr(Index index) = 0;
    virtual void EndDataSegmentInitExpr(Index index) = 0;
    virtual void OnDataSegmentData(Index index, const void *data, Address size) = 0;
    virtual void EndDataSegment(Index index) = 0;

    virtual void OnTableCount(Index count) = 0;
    virtual void OnTable(Index index, Type type, size_t initialSize, size_t maximumSize) = 0;

    virtual void OnElemSegmentCount(Index count) = 0;
    virtual void BeginElemSegment(Index index, Index tableIndex, uint8_t flags) = 0;
    virtual void BeginElemSegmentInitExpr(Index index) = 0;
    virtual void EndElemSegmentInitExpr(Index index) = 0;
    virtual void OnElemSegmentElemType(Index index, Type elemType) = 0;
    virtual void OnElemSegmentElemExprCount(Index index, Index count) = 0;
    virtual void OnElemSegmentElemExpr_RefNull(Index segmentIndex, Type type) = 0;
    virtual void OnElemSegmentElemExpr_RefFunc(Index segmentIndex, Index funcIndex) = 0;
    virtual void EndElemSegment(Index index) = 0;

    virtual void OnFunctionCount(Index count) = 0;
    virtual void OnFunction(Index index, Index sigIndex) = 0;

    virtual void OnGlobalCount(Index count) = 0;
    virtual void BeginGlobal(Index index, Type type, bool mutable_) = 0;
    virtual void BeginGlobalInitExpr(Index index) = 0;
    virtual void EndGlobalInitExpr(Index index) = 0;
    virtual void EndGlobal(Index index) = 0;
    virtual void EndGlobalSection() = 0;

    virtual void OnTagCount(Index count) = 0;
    virtual void OnTagType(Index index, Index sigIndex) = 0;

    virtual void OnStartFunction(Index funcIndex) = 0;

    virtual void BeginFunctionBody(Index index, Offset size) = 0;

    virtual void OnLocalDeclCount(Index count) = 0;
    virtual void OnLocalDecl(Index decl_index, Index count, Type type) = 0;

    virtual void OnStartReadInstructions() = 0;

    virtual void OnOpcode(uint32_t opcode) = 0;

    virtual void OnCallExpr(Index index) = 0;
    virtual void OnCallIndirectExpr(Index sigIndex, Index tableIndex) = 0;
    virtual void OnI32ConstExpr(uint32_t value) = 0;
    virtual void OnI64ConstExpr(uint64_t value) = 0;
    virtual void OnF32ConstExpr(uint32_t value) = 0;
    virtual void OnF64ConstExpr(uint64_t value) = 0;
    virtual void OnLocalGetExpr(Index localIndex) = 0;
    virtual void OnLocalSetExpr(Index localIndex) = 0;
    virtual void OnLocalTeeExpr(Index localIndex) = 0;
    virtual void OnGlobalGetExpr(Index globalIndex) = 0;
    virtual void OnGlobalSetExpr(Index globalIndex) = 0;
    virtual void OnDropExpr() = 0;
    virtual void OnBinaryExpr(uint32_t opcode) = 0;
    virtual void OnUnaryExpr(uint32_t opcode) = 0;
    virtual void OnIfExpr(Type sigType) = 0;
    virtual void OnElseExpr() = 0;
    virtual void OnLoopExpr(Type sigType) = 0;
    virtual void OnBlockExpr(Type sigType) = 0;
    virtual void OnBrExpr(Index depth) = 0;
    virtual void OnBrIfExpr(Index depth) = 0;
    virtual void OnBrTableExpr(Index numTargets, Index *targetDepths, Index defaultTargetDepth) = 0;
    virtual void OnSelectExpr(Index resultCount, Type *resultTypes) = 0;
    virtual void OnThrowExpr(Index tagIndex) = 0;
    virtual void OnTryExpr(Type sigType) = 0;
    virtual void OnCatchExpr(Index tagIndex) = 0;
    virtual void OnCatchAllExpr() = 0;
    virtual void OnMemoryGrowExpr(Index memidx) = 0;
    virtual void OnMemoryInitExpr(Index segmentIndex, Index memidx) = 0;
    virtual void OnMemoryCopyExpr(Index srcMemIndex, Index dstMemIndex) = 0;
    virtual void OnMemoryFillExpr(Index memidx) = 0;
    virtual void OnDataDropExpr(Index segmentIndex) = 0;
    virtual void OnMemorySizeExpr(Index memidx) = 0;
    virtual void OnTableGetExpr(Index table_index) = 0;
    virtual void OnTableSetExpr(Index table_index) = 0;
    virtual void OnTableGrowExpr(Index table_index) = 0;
    virtual void OnTableSizeExpr(Index table_index) = 0;
    virtual void OnTableCopyExpr(Index dst_index, Index src_index) = 0;
    virtual void OnTableFillExpr(Index table_index) = 0;
    virtual void OnElemDropExpr(Index segmentIndex) = 0;
    virtual void OnTableInitExpr(Index segmentIndex, Index tableIndex) = 0;
    virtual void OnLoadExpr(int opcode, Index memidx, Address alignmentLog2, Address offset) = 0;
    virtual void OnAtomicLoadExpr(int opcode, Index memidx, Address alignmentLog2, Address offset) = 0;
    virtual void OnAtomicStoreExpr(int opcode, Index memidx, Address alignmentLog2, Address offset) = 0;
    virtual void OnAtomicRmwExpr(int opcode, Index memidx, Address alignmentLog2, Address offset) = 0;
    virtual void OnAtomicCmpxchgExpr(int opcode, Index memidx, Address alignmentLog2, Address offset) = 0;
    virtual void OnStoreExpr(int opcode, Index memidx, Address alignmentLog2, Address offset) = 0;
    virtual void OnReturnExpr() = 0;
    virtual void OnRefFuncExpr(Index func_index) = 0;
    virtual void OnRefNullExpr(Type type) = 0;
    virtual void OnRefIsNullExpr() = 0;
    virtual void OnNopExpr() = 0;
    virtual void OnEndExpr() = 0;
    virtual void OnUnreachableExpr() = 0;
    virtual void EndFunctionBody(Index index) = 0;

    bool shouldContinueToGenerateByteCode() const
    {
        return m_shouldContinueToGenerateByteCode;
    }

    void setShouldContinueToGenerateByteCode(bool b)
    {
        m_shouldContinueToGenerateByteCode = b;
    }

    size_t resumeGenerateByteCodeAfterNBlockEnd() const
    {
        return m_resumeGenerateByteCodeAfterNBlockEnd;
    }

    void setResumeGenerateByteCodeAfterNBlockEnd(size_t s)
    {
        m_resumeGenerateByteCodeAfterNBlockEnd = s;
    }

    size_t skipValidationUntil() const
    {
        return m_skipValidationUntil;
    }

protected:
    bool m_shouldContinueToGenerateByteCode;
    size_t m_resumeGenerateByteCodeAfterNBlockEnd;
    size_t m_skipValidationUntil;
};

std::string ReadWasmBinary(const std::string& filename, const uint8_t *data, size_t size, WASMBinaryReaderDelegate* delegate);

}  // namespace wabt

#endif /* WABT_BINARY_READER_WALRUS_H_ */
