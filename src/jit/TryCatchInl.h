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

/* Only included by Backend.cpp */

static size_t trapListCountItems(std::vector<TrapBlock>& trapBlocks)
{
    sljit_uw lastAddress = 0;
    sljit_uw itemCount = 0;

    for (auto it : trapBlocks) {
        sljit_uw endAddress = sljit_get_label_addr(it.endLabel);

        ASSERT(lastAddress <= endAddress && endAddress != 0);

        if (endAddress != lastAddress) {
            itemCount += 2;
            lastAddress = endAddress;
        }
    }

    return itemCount;
}

InstanceConstData::InstanceConstData(std::vector<TrapBlock>& trapBlocks, std::vector<Walrus::TryBlock>& tryBlocks)
{
    sljit_uw lastAddress = 0;

    m_trapList.reserve(trapListCountItems(trapBlocks));

    for (auto it : trapBlocks) {
        sljit_uw endAddress = sljit_get_label_addr(it.endLabel);

        ASSERT(lastAddress <= endAddress && endAddress != 0);

        if (endAddress != lastAddress) {
            m_trapList.push_back(endAddress);
            m_trapList.push_back(sljit_get_label_addr(it.u.handlerLabel));
            lastAddress = endAddress;
        }
    }

    size_t catchStart = 0;
    m_tryBlocks.reserve(tryBlocks.size());

    for (auto it : tryBlocks) {
        size_t catchCount = it.catchBlocks.size();

        ASSERT(catchCount > 0);
        m_tryBlocks.push_back(TryBlock(catchStart, catchCount, it.parent, sljit_get_label_addr(it.returnToLabel)));

        for (auto catchIt : it.catchBlocks) {
            m_catchBlocks.push_back(CatchBlock(sljit_get_label_addr(catchIt.u.handlerLabel), catchIt.stackSizeToBe, catchIt.tagIndex));
        }

        catchStart += catchCount;
    }
}

void InstanceConstData::append(std::vector<TrapBlock>& trapBlocks, std::vector<Walrus::TryBlock>& tryBlocks)
{
    sljit_uw itemCount = trapListCountItems(trapBlocks);
    sljit_uw endAddress = sljit_get_label_addr(trapBlocks[0].endLabel);
    sljit_uw pos = 0;

    ASSERT(itemCount > 0);

    while (true) {
        if (pos >= m_trapList.size()) {
            m_trapList.resize(m_trapList.size() + itemCount);
            break;
        }

        if (endAddress < m_trapList[pos]) {
            m_trapList.insert(m_trapList.begin() + pos, itemCount, static_cast<sljit_uw>(0));
            break;
        }

        pos += 2;
    }

    sljit_uw lastAddress = 0;

    for (auto it : trapBlocks) {
        sljit_uw endAddress = sljit_get_label_addr(it.endLabel);

        ASSERT(lastAddress <= endAddress && endAddress != 0);

        if (endAddress != lastAddress) {
            m_trapList[pos] = endAddress;
            m_trapList[pos + 1] = sljit_get_label_addr(it.u.handlerLabel);
            lastAddress = endAddress;
            pos += 2;
        }
    }

    size_t catchStart = m_catchBlocks.size();
    size_t tryBlockOffset = m_tryBlocks.size();
    m_tryBlocks.reserve(tryBlockOffset + tryBlocks.size());

    for (auto it : tryBlocks) {
        size_t catchCount = it.catchBlocks.size();
        size_t parent = it.parent;

        ASSERT(catchCount > 0);

        if (parent != InstanceConstData::globalTryBlock) {
            parent += tryBlockOffset;
        }

        m_tryBlocks.push_back(TryBlock(catchStart, catchCount, parent, sljit_get_label_addr(it.returnToLabel)));

        for (auto catchIt : it.catchBlocks) {
            m_catchBlocks.push_back(CatchBlock(sljit_get_label_addr(catchIt.u.handlerLabel), catchIt.stackSizeToBe, catchIt.tagIndex));
        }

        catchStart += catchCount;
    }
}

static sljit_uw SLJIT_FUNC getTrapHandler(ExecutionContext* context, sljit_uw returnAddr)
{
    return context->currentInstanceConstData->find(returnAddr);
}

static void emitTry(CompileContext* context, Label* label)
{
    std::vector<TryBlock>& tryBlocks = context->compiler->tryBlocks();

    ASSERT(tryBlocks[context->nextTryBlock].start == label);
    context->trapBlocks.push_back(TrapBlock(label->label(), context->currentTryBlock));

    do {
        TryBlock& block = tryBlocks[context->nextTryBlock];
        block.parent = context->currentTryBlock;

        context->tryBlockStack.push_back(context->currentTryBlock);
        context->currentTryBlock = context->nextTryBlock++;
    } while (context->nextTryBlock < tryBlocks.size()
             && tryBlocks[context->nextTryBlock].start == label);
}

static sljit_sw findCatch(sljit_sw current, uint8_t* bp, ExecutionContext* context)
{
    std::vector<InstanceConstData::TryBlock>& tryBlocks = context->currentInstanceConstData->tryBlocks();

    ASSERT(context->error != ExecutionContext::NoError);

    if (context->error != ExecutionContext::CapturedException || !context->capturedException->isUserException()) {
        return tryBlocks[current].returnToAddr;
    }

    std::vector<InstanceConstData::CatchBlock>& catchBlocks = context->currentInstanceConstData->catchBlocks();
    Tag* tag = context->capturedException->tag().value();
    Instance* instance = context->instance;

    while (true) {
        size_t i = tryBlocks[current].catchStart;
        size_t end = i + tryBlocks[current].catchCount;

        while (i < end) {
            if (catchBlocks[i].tagIndex == std::numeric_limits<uint32_t>::max()) {
                context->error = ExecutionContext::NoError;
                context->clearException();
                return catchBlocks[i].handlerAddr;
            }

            if (instance->tag(catchBlocks[i].tagIndex) == tag) {
                context->error = ExecutionContext::NoError;
                memcpy(bp + catchBlocks[i].stackSizeToBe,
                       context->capturedException->userExceptionData().data(),
                       tag->functionType()->paramStackSize());
                context->clearException();
                return catchBlocks[i].handlerAddr;
            }

            i++;
        }

        size_t parent = tryBlocks[current].parent;

        if (parent == InstanceConstData::globalTryBlock) {
            return tryBlocks[current].returnToAddr;
        }

        current = parent;
    }
}

static void emitCatch(sljit_compiler* compiler, CompileContext* context)
{
    TryBlock& tryBlock = context->compiler->tryBlocks()[context->currentTryBlock];
    sljit_label* label = sljit_emit_label(compiler);

    tryBlock.findHandlerLabel = label;
    context->trapBlocks.push_back(TrapBlock(label, context->currentTryBlock));

    for (auto it : tryBlock.throwJumps) {
        sljit_set_label(it, label);
    }

    tryBlock.throwJumps.clear();

    sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R2, 0, SLJIT_MEM1(SLJIT_SP), kContextOffset);
    sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R0, 0, SLJIT_IMM, static_cast<sljit_sw>(context->compiler->tryBlockOffset() + context->currentTryBlock));
    sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R1, 0, kFrameReg, 0);
    sljit_emit_icall(compiler, SLJIT_CALL, SLJIT_ARGS3(W, W, W, W), SLJIT_IMM, GET_FUNC_ADDR(sljit_sw, findCatch));
    sljit_emit_ijump(compiler, SLJIT_JUMP, SLJIT_R0, 0);

    context->currentTryBlock = context->tryBlockStack.back();
    context->tryBlockStack.pop_back();
}

static void throwWithArgs(Throw* throwTag, uint8_t* bp, ExecutionContext* context)
{
    Tag* tag = context->instance->tag(throwTag->tagIndex());
    Vector<uint8_t> userExceptionData;
    size_t sz = tag->functionType()->paramStackSize();
    userExceptionData.resizeWithUninitializedValues(sz);

    uint8_t* ptr = userExceptionData.data();
    auto& param = tag->functionType()->param();
    for (size_t i = 0; i < param.size(); i++) {
        auto sz = valueStackAllocatedSize(param[i]);
        memcpy(ptr, bp + throwTag->dataOffsets()[i], sz);
        ptr += sz;
    }

    context->error = ExecutionContext::CapturedException;
    context->capturedException = Exception::create(context->state, tag, std::move(userExceptionData)).release();
}

static void emitThrow(sljit_compiler* compiler, Instruction* instr)
{
    CompileContext* context = CompileContext::get(compiler);
    Throw* throwTag = reinterpret_cast<Throw*>(instr->byteCode());
    TagType* tagType = context->compiler->module()->tagType(throwTag->tagIndex());
    const TypeVector& types = context->compiler->module()->functionType(tagType->sigIndex())->param();

    emitStoreOntoStack(compiler, instr->params(), throwTag->dataOffsets(), types, false);

    sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R2, 0, SLJIT_MEM1(SLJIT_SP), kContextOffset);
    sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R0, 0, SLJIT_IMM, reinterpret_cast<sljit_sw>(instr->byteCode()));
    sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R1, 0, kFrameReg, 0);
    sljit_emit_icall(compiler, SLJIT_CALL, SLJIT_ARGS3V(W, W, W), SLJIT_IMM, GET_FUNC_ADDR(sljit_sw, throwWithArgs));

    sljit_jump* jump = sljit_emit_jump(compiler, SLJIT_JUMP);

    if (context->currentTryBlock == InstanceConstData::globalTryBlock) {
        context->appendTrapJump(ExecutionContext::ReturnToLabel, jump);
        return;
    }

    std::vector<TryBlock>& tryBlocks = context->compiler->tryBlocks();
    tryBlocks[context->currentTryBlock].throwJumps.push_back(jump);
}
