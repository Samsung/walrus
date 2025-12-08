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
        I32I32I32I32I64I64I32_RI32,
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

        FunctionType* functionType;
        TypeVector* param;
        TypeVector* result;

        {
            // NONE
            functionType = new FunctionType(0, 0, 0, 0);
            functionType->initDone();
            m_vector[index++] = functionType;
        }
        {
            // I32R
            functionType = new FunctionType(1, 0, 0, 0);
            param = functionType->initParam();
            param->setType(0, Value::Type::I32);
            functionType->initDone();
            m_vector[index++] = functionType;
        }
        {
            // I32_RI32
            functionType = new FunctionType(1, 0, 1, 0);
            param = functionType->initParam();
            result = functionType->initResult();
            param->setType(0, Value::Type::I32);
            result->setType(0, Value::Type::I32);
            functionType->initDone();
            m_vector[index++] = functionType;
        }
        {
            // I32I32_RI32
            functionType = new FunctionType(2, 0, 1, 0);
            param = functionType->initParam();
            result = functionType->initResult();
            param->setType(0, Value::Type::I32);
            param->setType(1, Value::Type::I32);
            result->setType(0, Value::Type::I32);
            functionType->initDone();
            m_vector[index++] = functionType;
        }
        {
            // I32I64I32_RI32
            functionType = new FunctionType(3, 0, 1, 0);
            param = functionType->initParam();
            result = functionType->initResult();
            param->setType(0, Value::Type::I32);
            param->setType(1, Value::Type::I64);
            param->setType(2, Value::Type::I32);
            result->setType(0, Value::Type::I32);
            functionType->initDone();
            m_vector[index++] = functionType;
        }
        {
            // I32I32I32_RI32
            functionType = new FunctionType(3, 0, 1, 0);
            param = functionType->initParam();
            result = functionType->initResult();
            param->setType(0, Value::Type::I32);
            param->setType(1, Value::Type::I32);
            param->setType(2, Value::Type::I32);
            result->setType(0, Value::Type::I32);
            functionType->initDone();
            m_vector[index++] = functionType;
        }
        {
            // I32I32I32I32_RI32
            functionType = new FunctionType(4, 0, 1, 0);
            param = functionType->initParam();
            result = functionType->initResult();
            param->setType(0, Value::Type::I32);
            param->setType(1, Value::Type::I32);
            param->setType(2, Value::Type::I32);
            param->setType(3, Value::Type::I32);
            result->setType(0, Value::Type::I32);
            functionType->initDone();
            m_vector[index++] = functionType;
        }
        {
            // I32I64I32I32_RI32
            functionType = new FunctionType(4, 0, 1, 0);
            param = functionType->initParam();
            result = functionType->initResult();
            param->setType(0, Value::Type::I32);
            param->setType(1, Value::Type::I64);
            param->setType(2, Value::Type::I32);
            param->setType(3, Value::Type::I32);
            result->setType(0, Value::Type::I32);
            functionType->initDone();
            m_vector[index++] = functionType;
        }
        {
            // I32I64I64I32_RI32
            functionType = new FunctionType(4, 0, 1, 0);
            param = functionType->initParam();
            result = functionType->initResult();
            param->setType(0, Value::Type::I32);
            param->setType(1, Value::Type::I64);
            param->setType(2, Value::Type::I64);
            param->setType(3, Value::Type::I32);
            result->setType(0, Value::Type::I32);
            functionType->initDone();
            m_vector[index++] = functionType;
        }
        {
            // I32I32I32I32I32_RI32
            functionType = new FunctionType(5, 0, 1, 0);
            param = functionType->initParam();
            result = functionType->initResult();
            param->setType(0, Value::Type::I32);
            param->setType(1, Value::Type::I32);
            param->setType(2, Value::Type::I32);
            param->setType(3, Value::Type::I32);
            param->setType(4, Value::Type::I32);
            result->setType(0, Value::Type::I32);
            functionType->initDone();
            m_vector[index++] = functionType;
        }
        {
            // I32I32I32I64I32_RI32
            functionType = new FunctionType(5, 0, 1, 0);
            param = functionType->initParam();
            result = functionType->initResult();
            param->setType(0, Value::Type::I32);
            param->setType(1, Value::Type::I32);
            param->setType(2, Value::Type::I32);
            param->setType(3, Value::Type::I64);
            param->setType(4, Value::Type::I32);
            result->setType(0, Value::Type::I32);
            functionType->initDone();
            m_vector[index++] = functionType;
        }
        {
            // I32I32I32I32I32I32_RI32
            functionType = new FunctionType(6, 0, 1, 0);
            param = functionType->initParam();
            result = functionType->initResult();
            param->setType(0, Value::Type::I32);
            param->setType(1, Value::Type::I32);
            param->setType(2, Value::Type::I32);
            param->setType(3, Value::Type::I32);
            param->setType(4, Value::Type::I32);
            param->setType(5, Value::Type::I32);
            result->setType(0, Value::Type::I32);
            functionType->initDone();
            m_vector[index++] = functionType;
        }
        {
            // I32I32I32I32I32I64I64I32_RI32
            functionType = new FunctionType(7, 0, 1, 0);
            param = functionType->initParam();
            result = functionType->initResult();
            param->setType(0, Value::Type::I32);
            param->setType(1, Value::Type::I32);
            param->setType(2, Value::Type::I32);
            param->setType(3, Value::Type::I32);
            param->setType(4, Value::Type::I64);
            param->setType(5, Value::Type::I64);
            param->setType(6, Value::Type::I32);
            result->setType(0, Value::Type::I32);
            functionType->initDone();
            m_vector[index++] = functionType;
        }
        {
            // I32I32I32I32I32I64I64I32I32_RI32
            functionType = new FunctionType(9, 0, 1, 0);
            param = functionType->initParam();
            result = functionType->initResult();
            param->setType(0, Value::Type::I32);
            param->setType(1, Value::Type::I32);
            param->setType(2, Value::Type::I32);
            param->setType(3, Value::Type::I32);
            param->setType(4, Value::Type::I32);
            param->setType(5, Value::Type::I64);
            param->setType(6, Value::Type::I64);
            param->setType(7, Value::Type::I32);
            param->setType(8, Value::Type::I32);
            result->setType(0, Value::Type::I32);
            functionType->initDone();
            m_vector[index++] = functionType;
        }
        {
            // RI32
            functionType = new FunctionType(0, 0, 1, 0);
            result = functionType->initResult();
            result->setType(0, Value::Type::I32);
            functionType->initDone();
            m_vector[index++] = functionType;
        }
        {
            // I64
            functionType = new FunctionType(1, 0, 0, 0);
            param = functionType->initParam();
            param->setType(0, Value::Type::I64);
            functionType->initDone();
            m_vector[index++] = functionType;
        }
        {
            // F32
            functionType = new FunctionType(1, 0, 0, 0);
            param = functionType->initParam();
            param->setType(0, Value::Type::F32);
            functionType->initDone();
            m_vector[index++] = functionType;
        }
        {
            // F64
            functionType = new FunctionType(1, 0, 0, 0);
            param = functionType->initParam();
            param->setType(0, Value::Type::F64);
            functionType->initDone();
            m_vector[index++] = functionType;
        }
        {
            // I32F32
            functionType = new FunctionType(2, 0, 0, 0);
            param = functionType->initParam();
            param->setType(0, Value::Type::I32);
            param->setType(1, Value::Type::F32);
            functionType->initDone();
            m_vector[index++] = functionType;
        }
        {
            // F64F64
            functionType = new FunctionType(2, 0, 0, 0);
            param = functionType->initParam();
            param->setType(0, Value::Type::F64);
            param->setType(1, Value::Type::F64);
            functionType->initDone();
            m_vector[index++] = functionType;
        }
        {
            // INVALID
            functionType = new FunctionType(1, 0, 0, 0);
            param = functionType->initParam();
            // Temporary types cannot be used as params
            param->setType(0, Value::Type::Void);
            functionType->initDone();
            m_vector[index++] = functionType;
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
        return m_vector[idx]->asFunction();
    }

private:
    CompositeTypeVector m_vector;
};

} // namespace Walrus

#endif //__WalrusDefinedFunctionTypes__
