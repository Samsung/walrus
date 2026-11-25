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

#if defined(WALRUS_PROFILER)

#include "Walrus.h"
#include "runtime/Profiler.h"
#include "interpreter/ByteCode.h"

#include <algorithm>
#include <cinttypes>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <new>

namespace Walrus {

Profiler* Profiler::s_instance = nullptr;

static uint64_t environmentValue(const char* name, uint64_t defaultValue)
{
    const char* value = getenv(name);
    if (value == nullptr || *value == '\0') {
        return defaultValue;
    }
    uint64_t result = strtoull(value, nullptr, 10);
    return result != 0 ? result : defaultValue;
}

template <typename F>
static void forEachJumpOffset(ByteCode* byteCode, F f)
{
    switch (byteCode->opcode()) {
    case ByteCode::JumpOpcode: {
        Jump* jump = reinterpret_cast<Jump*>(byteCode);
        jump->setOffset(f(jump->offset()));
        break;
    }
    case ByteCode::JumpIfTrueOpcode:
    case ByteCode::JumpIfFalseOpcode:
    case ByteCode::JumpIfNullOpcode:
    case ByteCode::JumpIfNonNullOpcode:
    case ByteCode::JumpIfCastGenericOpcode:
    case ByteCode::JumpIfCastDefinedOpcode: {
        ByteCodeOffsetValue* jump = reinterpret_cast<ByteCodeOffsetValue*>(byteCode);
        jump->setInt32Value(f(jump->int32Value()));
        break;
    }
    case ByteCode::BrTableOpcode: {
        BrTable* brTable = reinterpret_cast<BrTable*>(byteCode);
        brTable->setDefaultOffset(f(brTable->defaultOffset()));

        int32_t* jumpOffsets = brTable->jumpOffsets();
        int32_t* jumpOffsetsEnd = jumpOffsets + brTable->tableSize();

        while (jumpOffsets < jumpOffsetsEnd) {
            *jumpOffsets = f(*jumpOffsets);
            jumpOffsets++;
        }
        break;
    }
    default:
        break;
    }
}

Profiler& Profiler::create()
{
    s_instance = new Profiler();
    atexit([] { Profiler::instance().finish(); });
    return *s_instance;
}

Profiler::Profiler()
    : m_budget(0)
    , m_interval(0)
    , m_keyframeInterval(environmentValue("WALRUS_PROFILE_KEYFRAME", 16))
    , m_blockCount(0)
    , m_snapshotCount(0)
    , m_keyframeCount(0)
    , m_pendingExit(0)
    , m_output(nullptr)
{
    const uint64_t limit = std::min<uint64_t>(std::numeric_limits<uint32_t>::max(), std::numeric_limits<intptr_t>::max());
    uint64_t interval = environmentValue("WALRUS_PROFILE_INTERVAL", 10000000);

    if (interval > limit) {
        fprintf(stderr, "walrus profiler: WALRUS_PROFILE_INTERVAL must be in [1, %" PRIu64 "]\n", limit);
        abort();
    }

    m_budget = m_interval = static_cast<intptr_t>(interval);

    const char* path = getenv("WALRUS_PROFILE");
    if (path == nullptr || *path == '\0') {
        path = "walrus.profile";
    }

    m_output = fopen(path, "w");
    RELEASE_ASSERT(m_output != nullptr);

    fprintf(m_output, "walrus-profile 3\n");
    fprintf(m_output, "interval %" PRIdPTR "\n", m_interval);
    fprintf(m_output, "keyframe-interval %" PRIu32 "\n", m_keyframeInterval);
}

void Profiler::instrument(ModuleFunction* function, uint32_t functionIndex, Vector<uint8_t, std::allocator<uint8_t>>& byteCode,
                          Vector<ModuleFunction::CatchInfo, std::allocator<ModuleFunction::CatchInfo>>& catchInfo)
{
    const uint8_t* code = byteCode.data();
    size_t size = byteCode.size();

    std::vector<size_t> starts(1, 0);
    forEachBasicBlockStart(code, size, catchInfo, [&](size_t position) {
        starts.push_back(position);
    });
    std::sort(starts.begin(), starts.end());
    starts.erase(std::unique(starts.begin(), starts.end()), starts.end());
    ASSERT(starts.back() < size);

    std::vector<uint32_t> lengths(starts.size(), 0);
    size_t block = 0;
    for (size_t position = 0; position < size;) {
        while (block + 1 < starts.size() && starts[block + 1] <= position) {
            block++;
        }
        ASSERT(starts[block] <= position);
        lengths[block]++;
        position += reinterpret_cast<const ByteCode*>(code + position)->getSize();
    }

    m_functions.push_back({ functionIndex, m_blockCount, static_cast<uint32_t>(starts.size()), nullptr });
    m_pendingFunctions.push_back(function);
    m_pendingLengths.insert(m_pendingLengths.end(), lengths.begin(), lengths.end());
    m_blockCount += starts.size();

    auto before = [&](size_t position) {
        return position + sizeof(ProfileBlock) * (std::lower_bound(starts.begin(), starts.end(), position) - starts.begin());
    };
    auto after = [&](size_t position) {
        return position + sizeof(ProfileBlock) * (std::upper_bound(starts.begin(), starts.end(), position) - starts.begin());
    };

    std::vector<uint8_t> result(size + sizeof(ProfileBlock) * starts.size());
    for (size_t i = 0; i < starts.size(); i++) {
        size_t from = starts[i];
        size_t to = i + 1 < starts.size() ? starts[i + 1] : size;
        size_t destination = before(from);

        new (result.data() + destination) ProfileBlock(nullptr);
        memcpy(result.data() + destination + sizeof(ProfileBlock), code + from, to - from);
    }

    for (size_t position = 0; position < size;) {
        size_t moved = after(position);
        ByteCode* byteCode = reinterpret_cast<ByteCode*>(result.data() + moved);
        forEachJumpOffset(byteCode, [&](int32_t offset) {
            return static_cast<int32_t>(before(position + offset) - moved);
        });
        position += byteCode->getSize();
    }

    for (auto& item : catchInfo) {
        item.m_tryStart = before(item.m_tryStart);
        item.m_tryEnd = before(item.m_tryEnd);
        item.m_catchStartPosition = before(item.m_catchStartPosition);
    }

    byteCode.assign(result.data(), result.data() + result.size());
}

void Profiler::finishModule()
{
    if (m_pendingFunctions.empty()) {
        return;
    }

    uint32_t blockCount = m_pendingLengths.size();
    uint32_t firstSlot = m_blockCount - blockCount;
    ProfileCounter* counters = new ProfileCounter[blockCount];

    for (uint32_t i = 0; i < blockCount; i++) {
        counters[i] = { 0, m_pendingLengths[i] };
    }
    m_modules.push_back({ firstSlot, blockCount, counters });

    ProfileFunctionInfo* info = m_functions.data() + m_functions.size() - m_pendingFunctions.size();
    for (ModuleFunction* function : m_pendingFunctions) {
        ProfileCounter* counter = counters + (info->m_firstSlot - firstSlot);
        info->m_counters = counter;

        for (size_t position = 0; position < function->byteCodeSize();) {
            ByteCode* byteCode = function->getByteCode<ByteCode>(position);
            if (byteCode->opcode() == ByteCode::ProfileBlockOpcode) {
                reinterpret_cast<ProfileBlock*>(byteCode)->setCounter(counter++);
            }
            position += byteCode->getSize();
        }
        ASSERT(counter == info->m_counters + info->m_blockCount);
        info++;
    }

    m_pendingFunctions.clear();
    m_pendingLengths.clear();
}

void Profiler::discardModule()
{
    m_functions.resize(m_functions.size() - m_pendingFunctions.size());
    m_blockCount -= m_pendingLengths.size();
    m_pendingFunctions.clear();
    m_pendingLengths.clear();
}

uint32_t Profiler::slotOf(const ProfileCounter* counter) const
{
    for (const ProfileModuleInfo& module : m_modules) {
        if (counter >= module.m_counters && counter < module.m_counters + module.m_blockCount) {
            return module.m_firstSlot + static_cast<uint32_t>(counter - module.m_counters);
        }
    }
    RELEASE_ASSERT_NOT_REACHED();
}

void Profiler::recordExit(ModuleFunction* function, size_t position, bool isUserException)
{
    if (m_output == nullptr) {
        return;
    }

    const uint8_t* code = function->byteCode();
    const ProfileCounter* counter = nullptr;
    uint32_t executed = 0;

    for (size_t current = 0; current <= position;) {
        const ByteCode* byteCode = reinterpret_cast<const ByteCode*>(code + current);
        if (byteCode->opcode() == ByteCode::ProfileBlockOpcode) {
            counter = reinterpret_cast<const ProfileBlock*>(byteCode)->counter();
            executed = 0;
        } else {
            executed++;
        }
        current += byteCode->getSize();
    }

    if (counter == nullptr) {
        return;
    }
    m_exits.push_back({ m_snapshotCount, slotOf(counter), counter->m_length - executed, isUserException });
}

uintptr_t Profiler::addExitSite(const ProfileCounter* counter, uint32_t executed)
{
    m_exitSites.push_back({ slotOf(counter), counter->m_length - executed });
    return m_exitSites.size();
}

void Profiler::recordPendingExit(bool isUserException)
{
    if (m_pendingExit == 0) {
        return;
    }

    const ProfileExitSite& site = m_exitSites[m_pendingExit - 1];
    m_pendingExit = 0;

    if (m_output != nullptr) {
        m_exits.push_back({ m_snapshotCount, site.m_slot, site.m_skipped, isUserException });
    }
}

void Profiler::snapshotFromJIT()
{
    instance().snapshot();
}

void Profiler::snapshot()
{
    intptr_t instructions = m_interval - m_budget;
    m_budget = m_interval;

    if (m_output == nullptr) {
        return;
    }

    for (const ProfileModuleInfo& module : m_modules) {
        for (uint32_t i = 0; i < module.m_blockCount; i++) {
            ProfileCounter& counter = module.m_counters[i];
            if (counter.m_count != 0) {
                m_snapshotEntries.push_back({ module.m_firstSlot + i, counter.m_count });
                counter.m_count = 0;
            }
        }
    }
    m_snapshots.push_back({ static_cast<uint64_t>(instructions), m_snapshotEntries.size() });

    if (++m_snapshotCount % m_keyframeInterval == 0) {
        commitKeyframe();
    }
}

void Profiler::commitKeyframe()
{
    uint64_t id = m_snapshotCount - m_snapshots.size();
    fprintf(m_output, "keyframe %" PRIu64 " %" PRIu64 " %" PRIu64 "\n", m_keyframeCount++, id, m_snapshotCount - 1);

    size_t entry = 0;
    for (const ProfileSnapshot& snapshot : m_snapshots) {
        fprintf(m_output, "snapshot %" PRIu64 " %" PRIu64 " %zu\n", id++, snapshot.m_instructions, snapshot.m_entryEnd - entry);
        for (; entry < snapshot.m_entryEnd; entry++) {
            fprintf(m_output, "%" PRIu32 " %" PRIu32 "\n", m_snapshotEntries[entry].m_slot, m_snapshotEntries[entry].m_count);
        }
    }

    for (const ProfileExit& exit : m_exits) {
        fprintf(m_output, "exit %" PRIu64 " %s %" PRIu32 " %" PRIu32 "\n",
                exit.m_snapshotId, exit.m_isUserException ? "exception" : "trap", exit.m_slot, static_cast<uint32_t>(exit.m_skipped));
    }

    m_snapshots.clear();
    m_snapshotEntries.clear();
    m_exits.clear();
    fflush(m_output);
}

void Profiler::finish()
{
    if (m_output == nullptr) {
        return;
    }

    snapshot();
    if (!m_snapshots.empty()) {
        commitKeyframe();
    }

    fprintf(m_output, "layout %zu %" PRIu32 "\n", m_functions.size(), m_blockCount);
    for (const ProfileFunctionInfo& function : m_functions) {
        fprintf(m_output, "function %" PRIu32 " %" PRIu32 " %" PRIu32 "\n", function.m_functionIndex, function.m_firstSlot, function.m_blockCount);
    }
    for (const ProfileModuleInfo& module : m_modules) {
        for (uint32_t i = 0; i < module.m_blockCount; i++) {
            fprintf(m_output, "block %" PRIu32 " %" PRIu32 "\n", module.m_firstSlot + i, module.m_counters[i].m_length);
        }
    }
    fprintf(m_output, "end\n");
    fclose(m_output);
    m_output = nullptr;
}

} // namespace Walrus

#endif
