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

#include "runtime/ComponentInstance.h"
#include "runtime/Instance.h"
#include "runtime/Global.h"
#include "runtime/Memory.h"
#include "runtime/Table.h"
#include "runtime/Tag.h"
#include "runtime/Store.h"
#include "wasi/WASI02.h"

namespace Walrus {

DEFINE_GLOBAL_TYPE_INFO(componentInstanceTypeInfo, ComponentInstanceKind);

static void throwException(ExecutionState& state, const char* message)
{
    std::string stringMessage = message;
    Trap::throwException(state, stringMessage);
}

enum Utf8Consts : uint8_t {
    Utf8Len2 = 0xc0,
    Utf8Len2Mask = 0x1f,
    Utf8Len3 = 0xe0,
    Utf8Len3Mask = 0x0f,
    Utf8Len4 = 0xf0,
    Utf8Len4Mask = 0x07,
    Utf8Cont = 0x80,
    Utf8ContMask = 0x3f,
    Utf8ContShift1 = 6,
    Utf8ContShift2 = 12,
    Utf8ContShift3 = 18,

    // The F and S postfix represents a two byte sequence, where the first byte is fixed.
    Utf8Len2Start = 0xc2,
    Utf8Latin1End = 0xc4,
    Utf8Len3StartF = 0xe0,
    Utf8Len3StartS = 0xa0,
    Utf8SurrogateStartF = 0xed,
    Utf8SurrogateStartS = 0xa0,
    Utf8Len4StartF = 0xf0,
    Utf8Len4StartS = 0xc0,
    Utf8Len4EndF = 0xf4,
    Utf8Len4EndS = 0x8f,
};

enum Utf16Consts : uint16_t {
    Utf16HighSurrogate = 0xd800,
    Utf16LowSurrogate = 0xdc00,
    Utf16SurrogateEnd = 0xdfff,
    Utf16SurrogateMask = 0x3ff,
    Utf16ContShift = 10,
};

enum UtfLimits : uint32_t {
    UtfChar1Limit = 0x80,
    UtfCharL1Limit = 0x100,
    UtfChar2Limit = 0x800,
    UtfChar3Limit = 0x10000,
    UtfChar4Limit = 0x110000,
};

bool CanonOptions::UtfData::validateUtfString()
{
    ASSERT(m_type != UtfData::Latin1);

    if (UNLIKELY(m_type == UtfData::Utf16)) {
        const uint16_t* buffer = reinterpret_cast<const uint16_t*>(m_buffer);
        const uint16_t* end = buffer + m_length;
        uint32_t charL1 = 0;
        uint32_t char2 = 0;
        uint32_t char3 = 0;
        uint32_t char4 = 0;

        while (buffer < end) {
            uint16_t chr = *buffer;

            if (chr < Utf16HighSurrogate || chr > Utf16SurrogateEnd) {
                if (chr < UtfChar2Limit) {
                    if (chr >= UtfChar1Limit) {
                        charL1++;
                    }
                } else if (chr < 0x800) {
                    char2++;
                } else {
                    char3++;
                }
                buffer++;
            } else {
                if (chr >= Utf16LowSurrogate
                    || buffer >= end
                    || buffer[1] < Utf16LowSurrogate
                    || buffer[1] > Utf16SurrogateEnd) {
                    return false;
                }

                char4++;
                buffer += 2;
            }
        }

        m_charL1 = charL1;
        m_char2 = char2;
        m_char3 = char3;
        m_char4 = char4;
        return true;
    }

    const uint8_t* buffer = m_buffer;
    const uint8_t* end = buffer + m_length;
    uint32_t charL1 = 0;
    uint32_t char2 = 0;
    uint32_t char3 = 0;
    uint32_t char4 = 0;

    while (buffer < end) {
        uint8_t chr = *buffer;
        if (chr <= Utf8Cont) {
            buffer++;
        } else if (chr < Utf8Len3) {
            if (chr < Utf8Len2 // Continuation byte.
                || end - buffer < 2
                || (buffer[1] & ~Utf8ContMask) != Utf8Cont
                || chr < Utf8Len2Start) {
                return false;
            }

            if (chr < Utf8Latin1End) {
                charL1++;
            } else {
                char2++;
            }
            buffer += 2;
        } else if (chr < Utf8Len4) {
            if (end - buffer < 3
                || (buffer[1] & ~Utf8ContMask) != Utf8Cont
                || (buffer[2] & ~Utf8ContMask) != Utf8Cont
                || (chr == Utf8Len3StartF && buffer[1] < Utf8Len3StartS)
                || (chr == Utf8SurrogateStartF && buffer[1] >= Utf8SurrogateStartS)) {
                return false;
            }

            buffer += 3;
            char3++;
        } else {
            if (chr > Utf8Len4EndF
                || end - buffer < 4
                || (buffer[1] & ~Utf8ContMask) != Utf8Cont
                || (buffer[2] & ~Utf8ContMask) != Utf8Cont
                || (buffer[3] & ~Utf8ContMask) != Utf8Cont
                || (chr == Utf8Len4StartF && buffer[1] < Utf8Len4StartS)
                || (chr == Utf8Len4EndF && buffer[1] > Utf8Len4EndS)) {
                return false;
            }

            buffer += 4;
            char4++;
        }
    }

    m_charL1 = charL1;
    m_char2 = char2;
    m_char3 = char3;
    m_char4 = char4;
    return true;
}

void CanonOptions::UtfData::utf8ComputeL1()
{
    ASSERT(m_type == UtfData::Latin1);
    if (m_char2 == 1) {
        return;
    }

    const uint8_t* buffer = m_buffer;
    const uint8_t* end = buffer + m_length;
    uint32_t charL1 = 0;

    while (buffer < end) {
        if (*buffer++ >= UtfChar1Limit) {
            charL1++;
        }
    }
    m_charL1 = charL1;
    m_char2 = 1;
}

void CanonOptions::UtfData::toUtf8String(uint8_t* dstBuffer)
{
#if !defined(NDEBUG)
    uint8_t* start = dstBuffer;
#endif
    ASSERT(m_type != UtfData::Utf8);

    if (m_type == UtfData::Latin1) {
        const uint8_t* buffer = m_buffer;
        const uint8_t* end = buffer + m_length;

        while (buffer < end) {
            uint8_t chr = *buffer++;
            if (chr < Utf8Cont) {
                *dstBuffer++ = chr;
            } else {
                dstBuffer[0] = static_cast<uint8_t>(Utf8Len2 | (chr >> Utf8ContShift1));
                dstBuffer[1] = static_cast<uint8_t>(Utf8Cont | (chr & Utf8ContMask));
                dstBuffer += 2;
            }
        }
        ASSERT(static_cast<size_t>(dstBuffer - start) == utf8Length());
        return;
    }

    const uint16_t* buffer = reinterpret_cast<const uint16_t*>(m_buffer);
    const uint16_t* end = buffer + m_length;

    while (buffer < end) {
        uint16_t chr = *buffer;

        if (chr < Utf16HighSurrogate || chr > Utf16SurrogateEnd) {
            if (chr < UtfChar1Limit) {
                *dstBuffer++ = static_cast<uint8_t>(chr);
            } else if (chr < UtfChar2Limit) {
                dstBuffer[0] = static_cast<uint8_t>(Utf8Len2 | (chr >> Utf8ContShift1));
                dstBuffer[1] = static_cast<uint8_t>(Utf8Cont | (chr & Utf8ContMask));
                dstBuffer += 2;
            } else {
                dstBuffer[0] = static_cast<uint8_t>(Utf8Len3 | (chr >> Utf8ContShift2));
                dstBuffer[1] = static_cast<uint8_t>(Utf8Cont | ((chr >> Utf8ContShift1) & Utf8ContMask));
                dstBuffer[2] = static_cast<uint8_t>(Utf8Cont | (chr & Utf8ContMask));
                dstBuffer += 3;
            }
            buffer++;
        } else {
            uint32_t chr32 = (static_cast<uint32_t>(chr & Utf16SurrogateMask) << Utf16ContShift) + static_cast<uint32_t>(buffer[1] & Utf16SurrogateMask) + UtfChar3Limit;
            dstBuffer[0] = static_cast<uint8_t>(Utf8Len4 | (chr >> Utf8ContShift3));
            dstBuffer[1] = static_cast<uint8_t>(Utf8Cont | ((chr >> Utf8ContShift2) & Utf8ContMask));
            dstBuffer[2] = static_cast<uint8_t>(Utf8Cont | ((chr >> Utf8ContShift1) & Utf8ContMask));
            dstBuffer[3] = static_cast<uint8_t>(Utf8Cont | (chr & Utf8ContMask));
            dstBuffer += 4;
            buffer += 2;
        }
    }
    ASSERT(static_cast<size_t>(dstBuffer - start) == utf8Length());
}

void CanonOptions::UtfData::toUtf16String(uint16_t* dstBuffer)
{
#if !defined(NDEBUG)
    uint16_t* start = dstBuffer;
#endif
    const uint8_t* buffer = m_buffer;
    const uint8_t* end = buffer + m_length;
    ASSERT(m_type != UtfData::Utf16);

    if (m_type == UtfData::Latin1) {
        while (buffer < end) {
            *dstBuffer++ = *buffer++;
        }
        ASSERT(static_cast<size_t>(dstBuffer - start) == utf8Length());
        return;
    }

    while (buffer < end) {
        uint16_t chr;
        uint8_t byte = *buffer;

        if (byte < UtfChar1Limit) {
            chr = byte;
            buffer++;
        } else if (byte < Utf8Len2) {
            chr = static_cast<uint16_t>((byte & Utf8Len2Mask) << Utf8ContShift1);
            chr |= static_cast<uint16_t>(buffer[1] & Utf8ContMask);
            buffer += 2;
        } else if (byte < Utf8Len3) {
            chr = static_cast<uint16_t>((byte & Utf8Len3Mask) << Utf8ContShift2);
            chr |= static_cast<uint16_t>((buffer[1] & Utf8ContMask) << Utf8ContShift1);
            chr |= static_cast<uint16_t>(buffer[2] & Utf8ContMask);
            buffer += 3;
        } else {
            uint32_t chr32 = static_cast<uint32_t>((byte & Utf8Len4Mask) << Utf8ContShift3);
            chr32 |= static_cast<uint32_t>((buffer[1] & Utf8ContMask) << Utf8ContShift2);
            chr32 |= static_cast<uint32_t>((buffer[2] & Utf8ContMask) << Utf8ContShift1);
            chr32 |= static_cast<uint32_t>(buffer[3] & Utf8ContMask);
            chr = static_cast<uint16_t>(Utf16LowSurrogate | (chr32 & Utf16SurrogateMask));
            *dstBuffer++ = static_cast<uint16_t>(Utf16HighSurrogate | (chr32 >> Utf16ContShift));
        }

        *dstBuffer++ = chr;
    }
}

void CanonOptions::UtfData::toLatin1String(uint8_t* dstBuffer)
{
#if !defined(NDEBUG)
    uint8_t* start = dstBuffer;
#endif
    const uint8_t* buffer = m_buffer;
    ASSERT(m_type != UtfData::Latin1);

    if (m_type == UtfData::Utf16) {
        const uint8_t* end = buffer + (m_length << 1);

        while (buffer < end) {
            *dstBuffer++ = *buffer;
            buffer += 2;
        }
        ASSERT(static_cast<size_t>(dstBuffer - start) == latin1Length());
        return;
    }

    const uint8_t* end = buffer + m_length;

    while (buffer < end) {
        uint8_t chr;
        uint8_t byte = *buffer;
        if (byte < UtfChar1Limit) {
            chr = byte;
            buffer++;
        } else {
            chr = static_cast<uint8_t>((byte << Utf8ContShift1) | (buffer[1] & Utf8ContMask));
            buffer += 2;
        }
        *dstBuffer++ = chr;
    }
    ASSERT(static_cast<size_t>(dstBuffer - start) == latin1Length());
}

void CanonOptions::memoryCheckRange32(ExecutionState& state, uint32_t align, uint32_t start, uint32_t size)
{
    ASSERT(!memory()->is64() && align <= 8 && (align & (align - 1)) == 0);

    align = start & (align - 1);
    if (align == 0 && memory()->sizeInByte() >= size && memory()->sizeInByte() - size >= start) {
        return;
    }

    throwException(state, align != 0 ? "incorrectly aligned memory area" : "out of bounds memory area");
}

void CanonOptions::memoryCheckRange64(ExecutionState& state, uint64_t align, uint64_t start, uint64_t size)
{
    ASSERT(memory()->is64() && align <= 8 && (align & (align - 1)) == 0);

    align = start & (align - 1);
    if (align == 0 && memory()->sizeInByte() >= size && memory()->sizeInByte() - size >= start) {
        return;
    }

    throwException(state, align != 0 ? "incorrectly aligned memory area" : "out of bounds memory area");
}

uint32_t CanonOptions::memoryMalloc32(ExecutionState& state, uint32_t align, uint32_t size)
{
    ASSERT(!memory()->is64() && realloc() != nullptr && align <= 8 && (align & (align - 1)) == 0);

    Value argv[4];
    Value result;
    argv[0] = Value(static_cast<int32_t>(0));
    argv[1] = Value(static_cast<int32_t>(0));
    argv[2] = Value(static_cast<int32_t>(align));
    argv[3] = Value(static_cast<int32_t>(size));

    // Should trap on an error (unreachable).
    realloc()->call(state, argv, &result);
    uint32_t start = static_cast<uint32_t>(result.asI32());
    memoryCheckRange32(state, align, start, size);
    return start;
}

uint64_t CanonOptions::memoryMalloc64(ExecutionState& state, uint64_t align, uint64_t size)
{
    ASSERT(memory()->is64() && realloc() != nullptr && align <= 8 && (align & (align - 1)) == 0);

    Value argv[4];
    Value result;
    argv[0] = Value(static_cast<int64_t>(0));
    argv[1] = Value(static_cast<int64_t>(0));
    argv[2] = Value(static_cast<int64_t>(align));
    argv[3] = Value(static_cast<int64_t>(size));

    // Should trap on an error (unreachable).
    realloc()->call(state, argv, &result);
    uint64_t start = static_cast<uint64_t>(result.asI64());
    memoryCheckRange64(state, align, start, size);
    return start;
}

void CanonOptions::validateString(ExecutionState& state, uint64_t start, uint64_t length, UtfData* utfData)
{
    uint8_t* buffer = memory()->buffer();
    uint32_t align = encoding() == ComponentCanonOptions::Utf8 ? 1 : 2;
    UtfData::Type type = UtfData::Utf8;
    uint32_t length32;

    if (memory()->is64()) {
        uint32_t shift = 0;

        if (encoding() == ComponentCanonOptions::Utf16) {
            shift = 1;
            type = UtfData::Utf16;
        } else if (encoding() == ComponentCanonOptions::Latin1Utf16) {
            if ((length & Utf16Tag64) != 0) {
                length ^= Utf16Tag64;
                shift = 1;
                type = UtfData::Utf16;
            } else {
                type = UtfData::Latin1;
            }
        }

        if (length > (Component::MaxStringByteLength >> shift)) {
            throwException(state, "Input string too large");
        }
        memoryCheckRange64(state, align, start, length);
        length32 = static_cast<uint32_t>(length);
        buffer += start;
    } else {
        uint32_t start32 = static_cast<uint32_t>(start);
        length32 = static_cast<uint32_t>(length);
        uint32_t shift = 0;

        if (encoding() == ComponentCanonOptions::Utf16) {
            shift = 1;
            type = UtfData::Utf16;
        } else if (encoding() == ComponentCanonOptions::Latin1Utf16) {
            if ((length32 & Utf16Tag32) != 0) {
                length32 ^= Utf16Tag32;
                shift = 1;
                type = UtfData::Utf16;
            } else {
                type = UtfData::Latin1;
            }
        }

        if (length32 > (Component::MaxStringByteLength >> shift)) {
            throwException(state, "Input string too large");
        }
        memoryCheckRange32(state, align, start32, length32);
        buffer += start32;
    }
    utfData->init(type, buffer, length32);

    // Latin1 is not validated by default.
    if (type != UtfData::Latin1 && !utfData->validateUtfString()) {
        throwException(state, type == UtfData::Utf8 ? "Invalid UTF8 string" : "Invalid UTF16 string");
    }
}

uint64_t CanonOptions::storeString(ExecutionState& state, UtfData& utfData, uint32_t* length)
{
    uint32_t byteLength;
    bool copy = false;
    bool isLatin1 = false;

    switch (encoding()) {
    case ComponentCanonOptions::Utf8:
        if (utfData.type() == UtfData::Utf8) {
            byteLength = utfData.length();
            copy = true;
            break;
        }
        if (utfData.type() == UtfData::Latin1) {
            utfData.utf8ComputeL1();
        }
        byteLength = utfData.utf8Length();
        break;
    case ComponentCanonOptions::Utf16:
        if (utfData.type() == UtfData::Utf16) {
            byteLength = utfData.length() << 1;
            copy = true;
            break;
        }
        byteLength = utfData.utf16Length() << 1;
        break;
    default:
        ASSERT(encoding() == ComponentCanonOptions::Latin1Utf16);
        if (utfData.type() == UtfData::Latin1) {
            byteLength = utfData.length();
            copy = true;
            break;
        }
        if (utfData.isLatin1()) {
            byteLength = utfData.latin1Length();
            isLatin1 = true;
            break;
        }
        if (utfData.type() == UtfData::Utf16) {
            byteLength = utfData.length() << 1;
            copy = true;
            break;
        }
        byteLength = utfData.utf16Length() << 1;
        break;
    }

    if (byteLength > Component::MaxStringByteLength) {
        throwException(state, "result string too large");
    }

    uint32_t align = encoding() == ComponentCanonOptions::Utf8 ? 1 : 2;
    uint64_t start;
    uint8_t* ptr;
    if (memory()->is64()) {
        start = memoryMalloc64(state, align, byteLength);
        ptr = reinterpret_cast<uint8_t*>(memory()->buffer() + start);
    } else {
        uint32_t start32 = memoryMalloc32(state, align, byteLength);
        ptr = reinterpret_cast<uint8_t*>(memory()->buffer() + start32);
        start = start32;
    }

    if (copy) {
        *length = utfData.length();
        memcpy(ptr, utfData.buffer(), byteLength);
        return start;
    }

    switch (encoding()) {
    case ComponentCanonOptions::Utf8:
        ASSERT(!isLatin1);
        utfData.toUtf8String(ptr);
        *length = byteLength;
        break;
    case ComponentCanonOptions::Utf16:
        ASSERT(!isLatin1);
        utfData.toUtf16String(reinterpret_cast<uint16_t*>(ptr));
        *length = byteLength >> 1;
        break;
    default:
        ASSERT(encoding() == ComponentCanonOptions::Latin1Utf16);
        if (isLatin1) {
            utfData.toLatin1String(ptr);
            *length = byteLength;
        } else {
            utfData.toUtf16String(reinterpret_cast<uint16_t*>(ptr));
            *length = (byteLength >> 1) | Utf16Tag32;
        }
        break;
    }
    return start;
}

uint64_t CanonOptions::storeLatin1String(ExecutionState& state, const uint8_t* src, uint32_t* length)
{
    uint32_t codeUnitLength = *length;
    uint32_t byteLength = codeUnitLength;
    uint32_t align = 2;
    const uint8_t* end = src + codeUnitLength;

    switch (encoding()) {
    case ComponentCanonOptions::Utf8:
        align = 1;
        for (const uint8_t* ptr = src; ptr < end; ptr++) {
            if (*ptr >= UtfChar1Limit) {
                byteLength++;
            }
        }
        *length = byteLength;
        break;
    case ComponentCanonOptions::Utf16:
        byteLength <<= 1;
        break;
    default:
        ASSERT(encoding() == ComponentCanonOptions::Latin1Utf16);
        break;
    }

    if (byteLength > Component::MaxStringByteLength) {
        throwException(state, "result string too large");
    }

    uint64_t start;
    uint8_t* ptr;
    if (memory()->is64()) {
        start = memoryMalloc64(state, align, byteLength);
        ptr = reinterpret_cast<uint8_t*>(memory()->buffer() + start);
    } else {
        uint32_t start32 = memoryMalloc32(state, align, byteLength);
        ptr = reinterpret_cast<uint8_t*>(memory()->buffer() + start32);
        start = start32;
    }

    switch (encoding()) {
    case ComponentCanonOptions::Utf8:
        if (byteLength == codeUnitLength) {
            break;
        }

        while (src < end) {
            if (*src < UtfChar1Limit) {
                *ptr++ = *src;
            } else {
                ptr[0] = static_cast<uint8_t>(Utf8Len2 | (*src >> Utf8ContShift1));
                ptr[1] = static_cast<uint8_t>(Utf8Cont | (*src & Utf8ContMask));
                ptr += 2;
            }
            src++;
        }
        return start;
    case ComponentCanonOptions::Utf16:
        while (src < end) {
            ptr[0] = *src++;
            ptr[1] = 0;
            ptr += 2;
        }
        return start;
    default:
        ASSERT(byteLength == codeUnitLength);
        break;
    }
    memcpy(ptr, src, byteLength);
    return start;
}

LoweredFunction* LoweredFunction::createLoweredFunction(const FunctionType* functionType, LiftedFunction* liftedFunction, CanonOptions* options)
{
    LoweredFunction* func = new LoweredFunction(functionType, liftedFunction, options);
    options->instance()->store()->appendExtern(func);
    return func;
}

void LoweredFunction::call(ExecutionState& state, Value* argv, Value* result)
{
#ifdef ENABLE_WASI
    if (m_liftedFunction->kind() == LiftedFunction::WasiFunctionKind) {
        callWasiFunction(state, argv, result, m_liftedFunction->asLiftedWasiFunction(), m_options);
        return;
    }
#endif

    ASSERT(0);
}

CanonFunction* CanonFunction::createCanonFunction(Store* store, const FunctionType* functionType, Type type)
{
    CanonFunction* func = new CanonFunction(functionType, type, store);
    store->appendExtern(func);
    return func;
}

void CanonFunction::call(ExecutionState& state, Value* argv, Value* result)
{
    ComponentInstance* instance = store()->context()->instance();

    switch (type()) {
    case ResourceDrop: {
        uint32_t index = argv[0].asI32();
        ComponentHandle* handle = instance->getHandle(state, index);
#ifdef ENABLE_WASI
        if (dropWasiResource(state, handle)) {
            delete handle;
            instance->removeHandle(index);
            break;
        }
#endif /* ENABLE_WASI */

        if (handle->kind() != ComponentHandle::ResourceRepKind) {
            ComponentInstance::throwInvalidHandle(state, index);
        }

        delete handle;
        instance->removeHandle(index);
        break;
    }
    default:
        ASSERT(0);
        break;
    }
}

ComponentInstance::ComponentInstance(Store* store, ComponentType* type)
    : Object(GET_GLOBAL_TYPE_INFO(componentInstanceTypeInfo))
    , m_type(type)
    , m_store(store)
    , m_freeResourceHandle(LastHandle)
{
    type->addRef();
}

ComponentInstance::~ComponentInstance()
{
    for (auto it : m_handles) {
        if ((it & (UnusedSlotMask | BorrowedHandleMask)) == 0) {
            delete reinterpret_cast<ComponentHandle*>(it);
        }
    }
    for (auto it : m_funcs) {
        it->releaseRef();
    }
    for (auto it : m_canonOptions) {
        delete it;
    }
    m_type->releaseRef();
}

uint32_t ComponentInstance::appendHandle(ExecutionState& state, ComponentHandle* handle)
{
    uintptr_t handleValue = reinterpret_cast<uintptr_t>(handle);
    ASSERT((handleValue & (UnusedSlotMask | BorrowedHandleMask)) == 0);

    if (m_freeResourceHandle != LastHandle) {
        uint32_t result = static_cast<uint32_t>(m_freeResourceHandle);
        ASSERT((reinterpret_cast<uintptr_t>(m_handles[result]) & UnusedSlotMask) != 0);
        m_freeResourceHandle = m_handles[result];
        if (m_freeResourceHandle != LastHandle) {
            m_freeResourceHandle >>= UnusedSlotShift;
        }
        m_handles[result] = handleValue;
        return result + FirstHandleIndex;
    }

    if (m_handles.size() >= ~static_cast<uint32_t>(0) - FirstHandleIndex) {
        std::string message = "too many handles allocated";
        Trap::throwException(state, message);
        return 0;
    }

    uint32_t result = static_cast<uint32_t>(m_handles.size());
    m_handles.push_back(handleValue);
    return result + FirstHandleIndex;
}

ComponentHandle* ComponentInstance::getHandle(ExecutionState& state, uint32_t index)
{
    if (index >= FirstHandleIndex && (index - FirstHandleIndex) < m_handles.size()) {
        uintptr_t handleValue = m_handles[index - FirstHandleIndex];
        if ((handleValue & UnusedSlotMask) == 0) {
            return reinterpret_cast<ComponentHandle*>(handleValue & ~BorrowedHandleMask);
        }
    }

    throwInvalidHandle(state, index);
    RELEASE_ASSERT_NOT_REACHED();
    return nullptr;
}

void ComponentInstance::removeHandle(uint32_t index)
{
    ASSERT(index >= FirstHandleIndex && (index - FirstHandleIndex) < m_handles.size()
           && (m_handles[index - FirstHandleIndex] & UnusedSlotMask) == 0);
    index -= FirstHandleIndex;
    m_handles[index] = (m_freeResourceHandle << UnusedSlotShift) | UnusedSlotMask;
    m_freeResourceHandle = index;
}

void ComponentInstance::throwInvalidHandle(ExecutionState& state, uint32_t index)
{
    std::string message = "invalid resource handle: ";
    message.append(std::to_string(index));
    Trap::throwException(state, message);
}

ComponentInstance* ComponentInstance::createInstance(Store* store, ComponentType* type)
{
    ComponentInstance* instance = new ComponentInstance(store, type);
    store->appendComponentInstance(instance);
    return instance;
}

bool ComponentInstance::compareTypes(ComponentRefCounted* expected, ComponentRefCounted* provided, std::vector<ComponentRefCounted*>& resources, std::string& componentName)
{
    switch (expected->kind()) {
    case ComponentRefCounted::InstanceTypeKind: {
        if (!provided->isComponentType()) {
            return false;
        }

        ComponentType* expectedType = expected->asComponentType();
        ComponentType* providedType = provided->asComponentType();

        for (auto& left : expectedType->exports()) {
            ComponentType::External* found = nullptr;

            for (auto& right : providedType->exports()) {
                if (left.name == right.name) {
                    found = &right;
                    break;
                }
            }

            componentName = left.name;
            if (found == nullptr || left.sort != found->sort) {
                return false;
            }

            if (left.type->kind() == ComponentRefCounted::SubResourceKind) {
                if (found->type->kind() != ComponentRefCounted::SubResourceKind && !found->type->isTypeResource()) {
                    return false;
                }

                uint32_t index = left.type->asTypeSubResource()->index();
                if (resources[index] == nullptr) {
                    resources[index] = found->type;
                } else if (resources[index] != found->type) {
                    return false;
                }
            }
        }
        break;
    }
    default:
        break;
    }
    return true;
}

void ComponentInstance::coreInstantiate(ExecutionState& state, Component* component, ComponentCoreInstantiate* instantiate)
{
    ExternVector imports;
    Module* module = component->modules()[instantiate->moduleIndex()];

    for (auto import : module->imports()) {
        Instance* instance = nullptr;
        uint32_t inlineIndex = 0;

        for (auto argument : instantiate->arguments()) {
            if (import->moduleName() == argument.name) {
                if (argument.index < m_coreInstances.size()) {
                    instance = m_coreInstances[argument.index];
                } else {
                    inlineIndex = argument.index;
                    ASSERT(inlineIndex > 0);
                }
                break;
            }
        }

        Extern* value = nullptr;

        if (instance != nullptr) {
            ExportType* exportType = nullptr;
            for (auto field : instance->module()->exports()) {
                if (import->fieldName() == field->name()) {
                    exportType = field;
                    break;
                }
            }

            if (exportType == nullptr) {
                std::string message = "cannot import field \"";
                message.append(import->fieldName());
                message.append("\" from module: ");
                message.append(import->moduleName());
                Trap::throwException(state, message);
            }

            switch (import->importType()) {
            case ImportType::Function:
                if (exportType->exportType() == ExportType::Function) {
                    Function* func = instance->function(exportType->itemIndex());
                    if (func->functionType()->equals(import->functionType())) {
                        value = func;
                    }
                }
                break;
            case ImportType::Table:
                if (exportType->exportType() == ExportType::Table) {
                    Table* table = instance->table(exportType->itemIndex());
                    if (table->type() == import->tableType()->type()) {
                        value = table;
                    }
                }
                break;
            case ImportType::Memory:
                if (exportType->exportType() == ExportType::Memory) {
                    Memory* memory = instance->memory(exportType->itemIndex());
                    if (import->memoryType()->initialSize() < memory->sizeInPageSize()
                        && import->memoryType()->maximumSize() >= memory->maximumSizeInPageSize()
                        && import->memoryType()->isShared() == memory->isShared()
                        && import->memoryType()->is64() == memory->is64()) {
                        value = memory;
                    }
                }
                break;
            case ImportType::Global:
                if (exportType->exportType() == ExportType::Global) {
                    Global* global = instance->global(exportType->itemIndex());
                    if (global->type() == import->globalType()->type()) {
                        value = global;
                    }
                }
                break;
            case ImportType::Tag:
                if (exportType->exportType() == ExportType::Tag) {
                    Tag* tag = instance->tag(exportType->itemIndex());
                    if (tag->functionType()->equals(import->tagType()->functionType())) {
                        value = tag;
                    }
                }
                break;
            default:
                RELEASE_ASSERT_NOT_REACHED();
                break;
            }
        } else {
            if (inlineIndex == 0) {
                std::string message = "cannot import module: ";
                message.append(import->moduleName());
                Trap::throwException(state, message);
            }

            size_t inlineStart = component->coreInlineExportsStart(~inlineIndex);
            size_t inlineEnd = component->coreInlineExportsEnd(~inlineIndex);
            std::vector<Walrus::Component::InlineExport>& exports = component->coreInlineExports();
            ComponentSort sort = ComponentSort::Invalid;
            uint32_t exportIndex = 0;

            for (size_t i = inlineStart; i < inlineEnd; i++) {
                if (exports[i].name == import->fieldName()) {
                    sort = exports[i].sort;
                    exportIndex = exports[i].index;
                    break;
                }
            }

            if (sort == ComponentSort::Invalid) {
                std::string message = "cannot import field \"";
                message.append(import->fieldName());
                message.append("\" from module: ");
                message.append(import->moduleName());
                Trap::throwException(state, message);
            }

            switch (import->importType()) {
            case ImportType::Function:
                if (sort == ComponentSort::CoreFunc) {
                    Function* func = m_coreFuncs[exportIndex];
                    if (func->functionType()->equals(import->functionType())) {
                        value = func;
                    }
                }
                break;
            case ImportType::Table:
                if (sort == ComponentSort::CoreTable) {
                    Table* table = m_coreTables[exportIndex];
                    if (table->type() == import->tableType()->type()) {
                        value = table;
                    }
                }
                break;
            case ImportType::Memory:
                if (sort == ComponentSort::CoreMemory) {
                    Memory* memory = m_coreMemories[exportIndex];
                    if (import->memoryType()->initialSize() < memory->sizeInPageSize()
                        && import->memoryType()->maximumSize() >= memory->maximumSizeInPageSize()
                        && import->memoryType()->isShared() == memory->isShared()
                        && import->memoryType()->is64() == memory->is64()) {
                        value = memory;
                    }
                }
                break;
            case ImportType::Global:
                if (sort == ComponentSort::CoreGlobal) {
                    Global* global = m_coreGlobals[exportIndex];
                    if (global->type() == import->globalType()->type()) {
                        value = global;
                    }
                }
                break;
            case ImportType::Tag:
                if (sort == ComponentSort::CoreTag) {
                    Tag* tag = m_coreTags[exportIndex];
                    if (tag->functionType()->equals(import->tagType()->functionType())) {
                        value = tag;
                    }
                }
                break;
            default:
                RELEASE_ASSERT_NOT_REACHED();
                break;
            }
        }

        if (value == nullptr) {
            std::string message = "type mismatch for field \"";
            message.append(import->fieldName());
            message.append("\" from module: ");
            message.append(import->moduleName());
            Trap::throwException(state, message);
        }

        imports.push_back(value);
    }

    m_coreInstances.push_back(module->instantiate(state, imports));
}

void ComponentInstance::aliasExport(ComponentAliasExport* alias)
{
    ComponentInstance* instance = m_instances[alias->instanceIndex()];

    for (auto it : instance->type()->exports()) {
        if (alias->name() == it.name) {
            ASSERT(alias->sort() == it.sort);
            switch (it.sort) {
            case ComponentSort::Func: {
                LiftedFunction* func = instance->m_funcs[it.exportIndex];
                m_funcs.push_back(func);
                func->addRef();
                break;
            }
            case ComponentSort::Instance:
                m_instances.push_back(instance->m_instances[it.exportIndex]);
                break;
            default:
                RELEASE_ASSERT_NOT_REACHED();
                break;
            }
            return;
        }
    }
    ASSERT(0);
}

void ComponentInstance::aliasCoreExport(ComponentAliasExport* alias)
{
    Instance* instance = m_coreInstances[alias->instanceIndex()];

    for (auto it : instance->module()->exports()) {
        if (alias->name() == it->name()) {
            switch (it->exportType()) {
            case ExportType::Function:
                ASSERT(alias->sort() == ComponentSort::CoreFunc);
                m_coreFuncs.push_back(instance->function(it->itemIndex()));
                break;
            case ExportType::Table:
                ASSERT(alias->sort() == ComponentSort::CoreTable);
                m_coreTables.push_back(instance->table(it->itemIndex()));
                break;
            case ExportType::Memory:
                ASSERT(alias->sort() == ComponentSort::CoreMemory);
                m_coreMemories.push_back(instance->memory(it->itemIndex()));
                break;
            case ExportType::Global:
                ASSERT(alias->sort() == ComponentSort::CoreGlobal);
                m_coreGlobals.push_back(instance->global(it->itemIndex()));
                break;
            case ExportType::Tag:
                ASSERT(alias->sort() == ComponentSort::CoreTag);
                m_coreTags.push_back(instance->tag(it->itemIndex()));
                break;
            default:
                RELEASE_ASSERT_NOT_REACHED();
                break;
            }
            return;
        }
    }
    ASSERT(0);
}

void ComponentInstance::aliasInline(ComponentAliasInline* alias)
{
    switch (alias->sort()) {
    case ComponentSort::CoreFunc:
        m_coreFuncs.push_back(m_coreFuncs[alias->exportIndex()]);
        break;
    case ComponentSort::CoreTable:
        m_coreTables.push_back(m_coreTables[alias->exportIndex()]);
        break;
    case ComponentSort::CoreMemory:
        m_coreMemories.push_back(m_coreMemories[alias->exportIndex()]);
        break;
    case ComponentSort::CoreGlobal:
        m_coreGlobals.push_back(m_coreGlobals[alias->exportIndex()]);
        break;
    case ComponentSort::CoreTag:
        m_coreTags.push_back(m_coreTags[alias->exportIndex()]);
        break;
    default:
        RELEASE_ASSERT_NOT_REACHED();
        break;
    }
}

void ComponentInstance::liftFunction(std::vector<CanonOptions*>& canonOptions, ComponentCanonLift* lift)
{
    CanonOptions* options = canonOptions[lift->options()];
    m_funcs.push_back(new LiftedCoreFunction(m_coreFuncs[lift->coreFuncIndex()], options));
}

void ComponentInstance::lowerFunction(std::vector<CanonOptions*>& canonOptions, ComponentCanonLower* lower)
{
    LiftedFunction* func = m_funcs[lower->funcIndex()];
    CanonOptions* options = canonOptions[lower->options()];

#ifdef ENABLE_WASI
    if (func->kind() == LiftedFunction::WasiFunctionKind) {
        m_coreFuncs.push_back(LoweredFunction::createLoweredFunction(getWasiFunctionType(func->asLiftedWasiFunction()), func, options));
        return;
    }
#endif /* ENABLE_WASI */
    m_coreFuncs.push_back(LoweredFunction::createLoweredFunction(func->asLiftedCoreFunction()->function()->functionType(), func, options));
}

ComponentInstance* ComponentInstance::InstantiateContext::instantiate(Component* component, ComponentInstance* parent, ComponentInstantiate* arg)
{
    ComponentInstance* instance = createInstance(m_store, component->type());
    std::vector<ComponentRefCounted*> resources;
    resources.resize(component->resourceCount());

    for (auto it : component->declarations()) {
        switch (it->kind()) {
        case ComponentDeclaration::CoreInstantiateKind: {
            instance->coreInstantiate(m_state, component, it->asCoreInstantiate());
            break;
        }
        case ComponentDeclaration::InstantiateKind: {
            Component* source = component->components()[it->asInstantiate()->componentIndex()];
            instance->m_instances.push_back(instantiate(source, instance, it->asInstantiate()));
            break;
        }
        case ComponentDeclaration::AliasExportKind: {
            instance->aliasExport(it->asAliasExport());
            break;
        }
        case ComponentDeclaration::AliasCoreExportKind: {
            instance->aliasCoreExport(it->asAliasExport());
            break;
        }
        case ComponentDeclaration::AliasInlineKind: {
            instance->aliasInline(it->asAliasInline());
            break;
        }
        case ComponentDeclaration::CanonOptionsKind: {
            ComponentCanonOptions* options = it->asCanonOptions();
            Memory* memory = options->memoryIndex() == ComponentCanonOptions::NotDefined ? nullptr : instance->m_coreMemories[options->memoryIndex()];
            Function* realloc = options->reallocIndex() == ComponentCanonOptions::NotDefined ? nullptr : instance->m_coreFuncs[options->reallocIndex()];
            Function* postReturn = options->postReturnIndex() == ComponentCanonOptions::NotDefined ? nullptr : instance->m_coreFuncs[options->postReturnIndex()];
            Function* callback = options->callbackIndex() == ComponentCanonOptions::NotDefined ? nullptr : instance->m_coreFuncs[options->callbackIndex()];

            if (realloc != nullptr) {
                ASSERT(memory != nullptr);
                Value::Type type = memory->is64() ? Value::I64 : Value::I32;

                const TypeVector::Types& param = realloc->functionType()->param().types();
                const TypeVector::Types& result = realloc->functionType()->result().types();
                if (param.size() != 4 || param[0] != type || param[1] != type || param[2] != type || param[3] != type
                    || result.size() != 1 || result[0] != type) {
                    std::string message = "invalid realloc function";
                    Trap::throwException(m_state, message);
                }
            }

            instance->m_canonOptions.push_back(new CanonOptions(instance, options->encoding(), options->isAsync(), memory, realloc, postReturn, callback));
            break;
        }
        case ComponentDeclaration::CanonLiftKind: {
            instance->liftFunction(instance->m_canonOptions, it->asCanonLift());
            break;
        }
        case ComponentDeclaration::CanonLowerKind: {
            instance->lowerFunction(instance->m_canonOptions, it->asCanonLower());
            break;
        }
        case ComponentDeclaration::CanonResourceDrop: {
            instance->m_coreFuncs.push_back(CanonFunction::createCanonFunction(m_store, m_store->getDefinedFunctionType(Store::I32R), CanonFunction::ResourceDrop));
            break;
        }
        case ComponentDeclaration::ImportKind: {
            uint32_t index = it->asComponentImport()->importIndex();
            ComponentType::External& external = component->type()->imports()[index];
            bool success = false;

            if (parent != nullptr) {
                for (auto& it : arg->arguments()) {
                    if (it.sort == external.sort && it.name == external.name) {
                        switch (it.sort) {
                        case ComponentSort::Func: {
                            LiftedFunction* func = parent->m_funcs[it.index];
                            instance->m_funcs.push_back(func);
                            func->addRef();
                            break;
                        }
                        case ComponentSort::Instance:
                            instance->m_instances.push_back(parent->m_instances[it.index]);
                            break;
                        default:
                            RELEASE_ASSERT_NOT_REACHED();
                            break;
                        }
                        success = true;
                        break;
                    }
                }
            }

#ifdef ENABLE_WASI
            if (!success && external.sort == ComponentSort::Instance) {
                ComponentInstance* importedInstance = wasi02LoadInstance(m_store, external.name);
                if (importedInstance != nullptr) {
                    instance->m_instances.push_back(importedInstance);
                    success = true;
                }
            }
#endif /* ENABLE_WASI */

            if (!success) {
                std::string message = "cannot import: ";
                message.append(external.name);
                Trap::throwException(m_state, message);
            }

            std::string componentName;
            if (external.sort == ComponentSort::Instance) {
                success = compareTypes(external.type, instance->m_instances.back()->type(), resources, componentName);
            }

            if (!success) {
                std::string message = "import type mismatch in: ";
                message += external.name;
                if (!componentName.empty()) {
                    message += " at: " + componentName;
                }
                Trap::throwException(m_state, message);
            }
            break;
        }
        default:
            RELEASE_ASSERT_NOT_REACHED();
            return instance;
        }
    }

    return instance;
}

ComponentInstance* ComponentInstance::instantiate(ExecutionState& state, Store* store, Component* component)
{
    InstantiateContext context(state, store);
    return context.instantiate(component, nullptr, nullptr);
}

} // namespace Walrus
