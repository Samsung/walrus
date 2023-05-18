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

#ifndef __WalrusGlobal__
#define __WalrusGlobal__

#include "runtime/Store.h"
#include "runtime/Object.h"

namespace Walrus {

class Global : public Extern {
    friend class JITFieldAccessor;

public:
    static Global* createGlobal(Store* store, const Value& value)
    {
        Global* glob = new Global(value);
        store->appendExtern(glob);
        return glob;
    }

    virtual Object::Kind kind() const override
    {
        return Object::GlobalKind;
    }

    virtual bool isGlobal() const override
    {
        return true;
    }

    Value& value()
    {
        return m_value;
    }

    Value value() const
    {
        return m_value;
    }

    void setValue(const Value& value)
    {
        ASSERT(value.type() == m_value.type());
        m_value = value;
    }

private:
    Global(const Value& value)
        : m_value(value)
    {
    }

    Value m_value;
};

} // namespace Walrus

#endif // __WalrusGlobal__
