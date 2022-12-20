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

#ifndef __WalrusTag__
#define __WalrusTag__

#include "runtime/Object.h"

namespace Walrus {

class FunctionType;

class Tag : public Object {
public:
    Tag(FunctionType* functionType)
        : m_functionType(functionType)
    {
    }

    virtual Object::Kind kind() const override
    {
        return Object::TagKind;
    }

    virtual bool isTag() const override
    {
        return true;
    }

    const FunctionType* functionType() const
    {
        return m_functionType;
    }

private:
    const FunctionType* m_functionType;
};

} // namespace Walrus

#endif // __WalrusTag__
