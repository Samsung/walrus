/*
 * Copyright (c) 2023-present Samsung Electronics Co., Ltd
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

#ifndef __WalrusDefinedFunctionTypes__
#define __WalrusDefinedFunctionTypes__

#include "Walrus.h"

namespace Walrus {

class DefinedFunctionTypes {
    MAKE_STACK_ALLOCATED();

public:
    enum Index : uint8_t {
        // The R is meant to represent the results, after R are the result types.
        NONE = 0,
        I32R,
        I32_RI32,
        I32I32_RI32,
        I32I64I32_RI32,
        I32I32I32_RI32,
        I32I32I32I32_RI32,
        I32I64I32I32_RI32,
        I32I64I64I32_RI32,
        I32I32I32I32I32_RI32,
        I32I32I32I64I32_RI32,
        I32I32I32I32I32I32_RI32,
        I32I32I32I32I32I64I64I32I32_RI32,
        RI32,
        I64R,
        F32R,
        F64R,
        I32F32R,
        F64F64R,
        INVALID,
        INDEX_NUM,
    };

    DefinedFunctionTypes()
    {
        m_vector.reserve(INDEX_NUM);
        size_t index = 0;

        ValueTypeVector* param;
        ValueTypeVector* result;

        {
            // NONE
            param = new ValueTypeVector();
            result = new ValueTypeVector();
            m_vector[index++] = new FunctionType(param, result);
        }
        {
            // I32R
            param = new ValueTypeVector();
            result = new ValueTypeVector();
            param->push_back(Value::Type::I32);
            m_vector[index++] = new FunctionType(param, result);
        }
        {
            // I32_RI32
            param = new ValueTypeVector();
            result = new ValueTypeVector();
            param->push_back(Value::Type::I32);
            result->push_back(Value::Type::I32);
            m_vector[index++] = new FunctionType(param, result);
        }
        {
            // I32I32_RI32
            param = new ValueTypeVector();
            result = new ValueTypeVector();
            param->push_back(Value::Type::I32);
            param->push_back(Value::Type::I32);
            result->push_back(Value::Type::I32);
            m_vector[index++] = new FunctionType(param, result);
        }
        {
            // I32I64I32_RI32
            param = new ValueTypeVector();
            result = new ValueTypeVector();
            param->push_back(Value::Type::I32);
            param->push_back(Value::Type::I64);
            param->push_back(Value::Type::I32);
            result->push_back(Value::Type::I32);
            m_vector[index++] = new FunctionType(param, result);
        }
        {
            // I32I32I32_RI32
            param = new ValueTypeVector();
            result = new ValueTypeVector();
            param->push_back(Value::Type::I32);
            param->push_back(Value::Type::I32);
            param->push_back(Value::Type::I32);
            result->push_back(Value::Type::I32);
            m_vector[index++] = new FunctionType(param, result);
        }
        {
            // I32I32I32I32_RI32
            param = new ValueTypeVector();
            result = new ValueTypeVector();
            param->push_back(Value::Type::I32);
            param->push_back(Value::Type::I32);
            param->push_back(Value::Type::I32);
            param->push_back(Value::Type::I32);
            result->push_back(Value::Type::I32);
            m_vector[index++] = new FunctionType(param, result);
        }
        {
            // I32I64I32I32_RI32
            param = new ValueTypeVector();
            result = new ValueTypeVector();
            param->push_back(Value::Type::I32);
            param->push_back(Value::Type::I64);
            param->push_back(Value::Type::I32);
            param->push_back(Value::Type::I32);
            result->push_back(Value::Type::I32);
            m_vector[index++] = new FunctionType(param, result);
        }
        {
            // I32I64I64I32_RI32
            param = new ValueTypeVector();
            result = new ValueTypeVector();
            param->push_back(Value::Type::I32);
            param->push_back(Value::Type::I64);
            param->push_back(Value::Type::I64);
            param->push_back(Value::Type::I32);
            result->push_back(Value::Type::I32);
            m_vector[index++] = new FunctionType(param, result);
        }
        {
            // I32I32I32I32I32_RI32
            param = new ValueTypeVector();
            result = new ValueTypeVector();
            param->push_back(Value::Type::I32);
            param->push_back(Value::Type::I32);
            param->push_back(Value::Type::I32);
            param->push_back(Value::Type::I32);
            param->push_back(Value::Type::I32);
            result->push_back(Value::Type::I32);
            m_vector[index++] = new FunctionType(param, result);
        }
        {
            // I32I32I32I64I32_RI32
            param = new ValueTypeVector();
            result = new ValueTypeVector();
            param->push_back(Value::Type::I32);
            param->push_back(Value::Type::I32);
            param->push_back(Value::Type::I32);
            param->push_back(Value::Type::I64);
            param->push_back(Value::Type::I32);
            result->push_back(Value::Type::I32);
            m_vector[index++] = new FunctionType(param, result);
        }
        {
            // I32I32I32I32I32I32_RI32
            param = new ValueTypeVector();
            result = new ValueTypeVector();
            param->push_back(Value::Type::I32);
            param->push_back(Value::Type::I32);
            param->push_back(Value::Type::I32);
            param->push_back(Value::Type::I32);
            param->push_back(Value::Type::I32);
            param->push_back(Value::Type::I32);
            result->push_back(Value::Type::I32);
            m_vector[index++] = new FunctionType(param, result);
        }
        {
            // I32I32I32I32I32I64I64I32I32_RI32
            param = new ValueTypeVector();
            result = new ValueTypeVector();
            param->push_back(Value::Type::I32);
            param->push_back(Value::Type::I32);
            param->push_back(Value::Type::I32);
            param->push_back(Value::Type::I32);
            param->push_back(Value::Type::I32);
            param->push_back(Value::Type::I64);
            param->push_back(Value::Type::I64);
            param->push_back(Value::Type::I32);
            param->push_back(Value::Type::I32);
            result->push_back(Value::Type::I32);
            m_vector[index++] = new FunctionType(param, result);
        }
        {
            // RI32
            param = new ValueTypeVector();
            result = new ValueTypeVector();
            result->push_back(Value::Type::I32);
            m_vector[index++] = new FunctionType(param, result);
        }
        {
            // I64
            param = new ValueTypeVector();
            result = new ValueTypeVector();
            param->push_back(Value::Type::I64);
            m_vector[index++] = new FunctionType(param, result);
        }
        {
            // F32
            param = new ValueTypeVector();
            result = new ValueTypeVector();
            param->push_back(Value::Type::F32);
            m_vector[index++] = new FunctionType(param, result);
        }
        {
            // F64
            param = new ValueTypeVector();
            result = new ValueTypeVector();
            param->push_back(Value::Type::F64);
            m_vector[index++] = new FunctionType(param, result);
        }
        {
            // I32F32
            param = new ValueTypeVector();
            result = new ValueTypeVector();
            param->push_back(Value::Type::I32);
            param->push_back(Value::Type::F32);
            m_vector[index++] = new FunctionType(param, result);
        }
        {
            // F64F64
            param = new ValueTypeVector();
            result = new ValueTypeVector();
            param->push_back(Value::Type::F64);
            param->push_back(Value::Type::F64);
            m_vector[index++] = new FunctionType(param, result);
        }
        {
            // INVALID
            param = new ValueTypeVector();
            result = new ValueTypeVector();
            param->push_back(Value::Type::Void);
            m_vector[index++] = new FunctionType(param, result);
        }

        ASSERT(index == INDEX_NUM);
    }

    ~DefinedFunctionTypes()
    {
        for (size_t i = 0; i < m_vector.size(); i++) {
            delete m_vector[i];
        }
    }

    FunctionType* operator[](const size_t idx)
    {
        ASSERT(idx < m_vector.size());
        return m_vector[idx];
    }

private:
    FunctionTypeVector m_vector;
};

} // namespace Walrus

#endif //__WalrusDefinedFunctionTypes__
