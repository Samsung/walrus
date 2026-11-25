/*
 * Copyright (c) 2026-present Samsung Electronics Co., Ltd
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

/* Only included by Backend.cpp */

#if (defined SLJIT_SEPARATE_VECTOR_REGISTERS && SLJIT_SEPARATE_VECTOR_REGISTERS)
constexpr sljit_sw kProfileFloatSaveSize = sizeof(sljit_f64);
#ifdef HAS_SIMD
constexpr sljit_sw kProfileVectorSaveCount = SLJIT_NUMBER_OF_SCRATCH_VECTOR_REGISTERS;
#else /* !HAS_SIMD */
constexpr sljit_sw kProfileVectorSaveCount = 0;
#endif /* HAS_SIMD */
#else /* !SLJIT_SEPARATE_VECTOR_REGISTERS */
#ifdef HAS_SIMD
constexpr sljit_sw kProfileFloatSaveSize = 16;
#else /* !HAS_SIMD */
constexpr sljit_sw kProfileFloatSaveSize = sizeof(sljit_f64);
#endif /* HAS_SIMD */
constexpr sljit_sw kProfileVectorSaveCount = 0;
#endif /* SLJIT_SEPARATE_VECTOR_REGISTERS */

constexpr sljit_sw kProfileSaveSize = (1 + SLJIT_NUMBER_OF_SCRATCH_REGISTERS) * sizeof(sljit_sw)
    + SLJIT_NUMBER_OF_SCRATCH_FLOAT_REGISTERS * kProfileFloatSaveSize + kProfileVectorSaveCount * 16;

static void emitProfileBlock(sljit_compiler* compiler, Instruction* instr)
{
    ProfileCounter* counter = reinterpret_cast<ProfileBlock*>(instr->byteCode())->counter();
    sljit_sw count = reinterpret_cast<sljit_sw>(&counter->m_count);
    sljit_sw budget = reinterpret_cast<sljit_sw>(Profiler::instance().budgetAddress());

    sljit_emit_op2(compiler, SLJIT_ADD32, SLJIT_MEM0(), count, SLJIT_MEM0(), count, SLJIT_IMM, 1);
    sljit_emit_op2(compiler, SLJIT_SUB | SLJIT_SET_SIG_LESS_EQUAL, SLJIT_MEM0(), budget, SLJIT_MEM0(), budget, SLJIT_IMM, counter->m_length);

    sljit_jump* skip = sljit_emit_jump(compiler, SLJIT_SIG_GREATER);
    CompileContext::get(compiler)->profileSnapshotCalls.push_back(sljit_emit_jump(compiler, SLJIT_FAST_CALL));
    sljit_set_label(skip, sljit_emit_label(compiler));
}

static bool emitProfilePage(sljit_compiler* compiler, sljit_s32 reg, sljit_sw targetBufferOffset, sljit_s32 addressReg, sljit_uw address, uint32_t access)
{
    if (access == 0) {
        return false;
    }

    sljit_s32 shift = static_cast<sljit_s32>(Profiler::instance().pageShift());
    const sljit_sw tables[] = {
        targetBufferOffset + static_cast<sljit_sw>(offsetof(Memory::TargetBuffer, readCounters)),
        targetBufferOffset + static_cast<sljit_sw>(offsetof(Memory::TargetBuffer, writeCounters)),
    };

    for (uint32_t i = 0; i < 2; i++) {
        if (!(access & (Memory::ReadAccess << i))) {
            continue;
        }

        sljit_emit_op1(compiler, SLJIT_MOV_P, reg, 0, SLJIT_MEM1(kInstanceReg), tables[i]);

        if (addressReg == 0) {
            sljit_sw offset = static_cast<sljit_sw>((address >> shift) * sizeof(uint32_t));
            sljit_emit_op2(compiler, SLJIT_ADD32, SLJIT_MEM1(reg), offset, SLJIT_MEM1(reg), offset, SLJIT_IMM, 1);
            continue;
        }

#if defined(SLJIT_TMP_OPT_REG) && ((defined SLJIT_CONFIG_X86_64 && SLJIT_CONFIG_X86_64) || (defined SLJIT_CONFIG_ARM_64 && SLJIT_CONFIG_ARM_64))
        sljit_emit_op2(compiler, SLJIT_LSHR, SLJIT_TMP_OPT_REG, 0, addressReg, 0, SLJIT_IMM, shift);
        sljit_emit_op2(compiler, SLJIT_ADD32, SLJIT_MEM2(reg, SLJIT_TMP_OPT_REG), 2, SLJIT_MEM2(reg, SLJIT_TMP_OPT_REG), 2, SLJIT_IMM, 1);
#else
        sljit_emit_op2(compiler, SLJIT_LSHR, SLJIT_TMP_DEST_REG, 0, addressReg, 0, SLJIT_IMM, shift);
        sljit_emit_op2(compiler, SLJIT_SHL, SLJIT_TMP_DEST_REG, 0, SLJIT_TMP_DEST_REG, 0, SLJIT_IMM, 2);
        sljit_emit_op2(compiler, SLJIT_ADD, reg, 0, reg, 0, SLJIT_TMP_DEST_REG, 0);
        sljit_emit_op2(compiler, SLJIT_ADD32, SLJIT_MEM1(reg), 0, SLJIT_MEM1(reg), 0, SLJIT_IMM, 1);
#endif
    }
    return true;
}

static void emitProfileScratchTransfer(sljit_compiler* compiler, sljit_sw base, bool load)
{
    sljit_sw offset = base + sizeof(sljit_sw);

    for (sljit_s32 i = 0; i < SLJIT_NUMBER_OF_SCRATCH_REGISTERS; i++) {
        if (load) {
            sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R(i), 0, SLJIT_MEM1(SLJIT_SP), offset);
        } else {
            sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_MEM1(SLJIT_SP), offset, SLJIT_R(i), 0);
        }
        offset += sizeof(sljit_sw);
    }

    for (sljit_s32 i = 0; i < SLJIT_NUMBER_OF_SCRATCH_FLOAT_REGISTERS; i++) {
#if defined(HAS_SIMD) && !(defined SLJIT_SEPARATE_VECTOR_REGISTERS && SLJIT_SEPARATE_VECTOR_REGISTERS)
        sljit_emit_simd_mov(compiler, (load ? SLJIT_SIMD_LOAD : SLJIT_SIMD_STORE) | SLJIT_SIMD_REG_128, SLJIT_FR(i), SLJIT_MEM1(SLJIT_SP), offset);
#else /* !HAS_SIMD || SLJIT_SEPARATE_VECTOR_REGISTERS */
        if (load) {
            sljit_emit_fop1(compiler, SLJIT_MOV_F64, SLJIT_FR(i), 0, SLJIT_MEM1(SLJIT_SP), offset);
        } else {
            sljit_emit_fop1(compiler, SLJIT_MOV_F64, SLJIT_MEM1(SLJIT_SP), offset, SLJIT_FR(i), 0);
        }
#endif /* HAS_SIMD && !SLJIT_SEPARATE_VECTOR_REGISTERS */
        offset += kProfileFloatSaveSize;
    }

    for (sljit_s32 i = 0; i < kProfileVectorSaveCount; i++) {
#if defined(HAS_SIMD) && (defined SLJIT_SEPARATE_VECTOR_REGISTERS && SLJIT_SEPARATE_VECTOR_REGISTERS)
        sljit_emit_simd_mov(compiler, (load ? SLJIT_SIMD_LOAD : SLJIT_SIMD_STORE) | SLJIT_SIMD_REG_128, SLJIT_VR(i), SLJIT_MEM1(SLJIT_SP), offset);
#endif /* HAS_SIMD && SLJIT_SEPARATE_VECTOR_REGISTERS */
        offset += 16;
    }
}

static void emitProfileSnapshotStub(sljit_compiler* compiler, sljit_sw base)
{
    std::vector<sljit_jump*>& calls = CompileContext::get(compiler)->profileSnapshotCalls;

    if (calls.empty()) {
        return;
    }

    sljit_label* label = sljit_emit_label(compiler);
    for (auto it : calls) {
        sljit_set_label(it, label);
    }
    calls.clear();

    sljit_emit_op_dst(compiler, SLJIT_FAST_ENTER, SLJIT_MEM1(SLJIT_SP), base);
    emitProfileScratchTransfer(compiler, base, false);
    sljit_emit_icall(compiler, SLJIT_CALL, SLJIT_ARGS0V(), SLJIT_IMM, GET_FUNC_ADDR(sljit_sw, Profiler::snapshotFromJIT));
    emitProfileScratchTransfer(compiler, base, true);
    sljit_emit_op_src(compiler, SLJIT_FAST_RETURN, SLJIT_MEM1(SLJIT_SP), base);
}

struct ProfileInstructionInfo {
    size_t offset;
    const ProfileCounter* counter;
    uint32_t executed;
};

class ProfileExitSites {
public:
    ProfileExitSites(ModuleFunction* function)
        : m_function(function)
    {
    }

    uintptr_t find(Instruction* instr)
    {
        if (instr == nullptr) {
            return 0;
        }

        size_t offset = reinterpret_cast<const uint8_t*>(instr->byteCode()) - m_function->byteCode();
        if (offset >= m_function->byteCodeSize()) {
            return 0;
        }

        if (m_instructions.empty()) {
            const ProfileCounter* counter = nullptr;
            uint32_t executed = 0;

            for (size_t position = 0; position < m_function->byteCodeSize();) {
                ByteCode* byteCode = m_function->getByteCode<ByteCode>(position);
                if (byteCode->opcode() == ByteCode::ProfileBlockOpcode) {
                    counter = reinterpret_cast<ProfileBlock*>(byteCode)->counter();
                    executed = 0;
                } else {
                    executed++;
                }
                m_instructions.push_back({ position, counter, executed });
                position += byteCode->getSize();
            }
        }

        auto it = std::lower_bound(m_instructions.begin(), m_instructions.end(), offset,
                                   [](const ProfileInstructionInfo& info, size_t offset) { return info.offset < offset; });
        if (it == m_instructions.end() || it->offset != offset || it->counter == nullptr) {
            return 0;
        }
        return Profiler::instance().addExitSite(it->counter, it->executed);
    }

private:
    ModuleFunction* m_function;
    std::vector<ProfileInstructionInfo> m_instructions;
};

static sljit_jump* emitProfileExitStub(sljit_compiler* compiler, ProfileExitSites& sites, sljit_jump* jump, Instruction* instr)
{
    uintptr_t site = sites.find(instr);

    if (site == 0) {
        return jump;
    }

    sljit_set_label(jump, sljit_emit_label(compiler));
    sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_MEM0(), reinterpret_cast<sljit_sw>(Profiler::instance().pendingExitAddress()), SLJIT_IMM, static_cast<sljit_sw>(site));
    return sljit_emit_jump(compiler, SLJIT_JUMP);
}

static void emitProfileExitStubs(sljit_compiler* compiler, size_t tryBlockStart)
{
    CompileContext* context = CompileContext::get(compiler);
    ProfileExitSites sites(context->compiler->moduleFunction());

    for (TrapJump& it : context->trapJumps) {
        it.jump = emitProfileExitStub(compiler, sites, it.jump, it.instr);
    }

    std::vector<TryBlock>& tryBlocks = context->compiler->tryBlocks();
    for (size_t i = tryBlockStart; i < tryBlocks.size(); i++) {
        TryBlock& tryBlock = tryBlocks[i];
        for (size_t j = 0; j < tryBlock.throwJumps.size(); j++) {
            tryBlock.throwJumps[j] = emitProfileExitStub(compiler, sites, tryBlock.throwJumps[j], tryBlock.throwInstrs[j]);
        }
        tryBlock.throwInstrs.clear();
    }
}
