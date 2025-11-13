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

#include "Type.h"
#include "runtime/ObjectType.h"

namespace Walrus {

static Value::Type toGenericType(const CompositeType* ref)
{
    switch (ref->kind()) {
    case ObjectType::FunctionKind:
        return Value::FuncRef;
    case ObjectType::StructKind:
        return Value::StructRef;
    default:
        ASSERT(ref->kind() == ObjectType::ArrayKind);
        return Value::ArrayRef;
    }
}

bool Type::isSubType(const Type& expected) const
{
    Value::Type actualType = type();
    Value::Type expectedType = expected.type();

    if (!Value::isRefType(expectedType)) {
        return expectedType == actualType;
    } else if (!Value::isRefType(actualType)) {
        return false;
    }

    actualType = Value::toNonNullableRefType(actualType);
    expectedType = Value::toNonNullableRefType(expectedType);

    if (expectedType == Value::DefinedRef) {
        if (actualType == Value::DefinedRef) {
            return expected.ref()->isSubTypeOf(ref()) && (!Value::isNullableRefType(expected.type()) || Value::isNullableRefType(type()));
        }
        expectedType = toGenericType(expected.ref());
    }

    if (actualType == Value::DefinedRef) {
        actualType = toGenericType(ref());
    }

    switch (expectedType) {
    case Value::AnyRef:
        if (actualType == Value::AnyRef) {
            break;
        }
        return false;
    case Value::NoAnyRef:
        if (actualType == Value::AnyRef || actualType == Value::NoAnyRef || actualType == Value::EqRef || actualType == Value::I31Ref || actualType == Value::StructRef || actualType == Value::ArrayRef) {
            break;
        }
        return false;
    case Value::EqRef:
        if (actualType == Value::NoAnyRef || actualType == Value::EqRef || actualType == Value::I31Ref || actualType == Value::StructRef || actualType == Value::ArrayRef) {
            break;
        }
        return false;
    case Value::I31Ref:
        if (actualType == Value::NoAnyRef || actualType == Value::I31Ref) {
            break;
        }
        return false;
    case Value::StructRef:
        if (actualType == Value::NoAnyRef || actualType == Value::StructRef) {
            break;
        }
        return false;
    case Value::ArrayRef:
        if (actualType == Value::NoAnyRef || actualType == Value::ArrayRef) {
            break;
        }
        return false;
    case Value::ExternRef:
        if (actualType == Value::ExternRef) {
            break;
        }
        return false;
    case Value::NoExternRef:
        if (actualType == Value::ExternRef || actualType == Value::NoExternRef) {
            break;
        }
        return false;
    case Value::FuncRef:
        if (actualType == Value::FuncRef) {
            break;
        }
        return false;
    default:
        ASSERT(expectedType == Value::NoFuncRef);
        if (actualType == Value::FuncRef || actualType == Value::NoFuncRef) {
            break;
        }
        return false;
    }

    return !Value::isNullableRefType(expected.type()) || Value::isNullableRefType(type());
}

} // namespace Walrus
