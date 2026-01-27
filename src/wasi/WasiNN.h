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

#ifndef __WalrusWASI_NN__
#define __WalrusWASI_NN__

#ifdef ENABLE_WASI_NN

#include "Walrus.h"
#include "runtime/Function.h"
#include "runtime/ObjectType.h"
#include "runtime/ComponentInstance.h"

#include "wasi/WASI02.h"
#include "wasi/WASI02Impl.h"

#ifdef WASINN_NNTRAINER
// hack to make the nntrainer c++ api work
namespace std {

template <bool B, class T = void>
using enable_if_t = typename enable_if<B, T>::type;

template <typename T, typename... Args>
std::unique_ptr<T> make_unique(Args&&... args)
{
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}
#endif

} // namespace std

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-qualifiers"
// hack because nntrainer has a const on primitive type return and walrus complains
#include <layer.h>
#pragma GCC diagnostic pop

#include <model.h>
#include <nntrainer-api-common.h>
#include <tensor_api.h>

class CanonOptions;

namespace Walrus {

enum TensorType : uint8_t {
    FP16,
    FP32,
    FP64,
    BF16,
    U8,
    I32,
    I64,
    NONE
};

class ComponentResourceWasiNNTensor : public ComponentResource {
public:
    ComponentResourceWasiNNTensor(ComponentTypeResource* type, TensorType ty, uint32_t* tensorDim, uint32_t tensorDimLen, uint8_t* tensorData, uint32_t tensorDataLen)
        : ComponentResource(ResourceWasiNNTensor, type)
        , m_type(ty)
    {
        m_tensorDim = std::vector<size_t>(tensorDim, tensorDim + tensorDimLen);
        m_tensorData = std::vector<uint8_t>(tensorData, tensorData + tensorDataLen);
    }

    ComponentResourceWasiNNTensor(ComponentTypeResource* type, TensorType ty, std::vector<size_t> tensorDim, std::vector<uint8_t> tensorData)
        : ComponentResource(ResourceWasiNNTensor, type)
        , m_type(ty)
        , m_tensorDim(tensorDim)
        , m_tensorData(tensorData)
    {
    }

    std::vector<size_t>& tensorDim()
    {
        return m_tensorDim;
    }

    std::vector<uint8_t>& tensorData()
    {
        return m_tensorData;
    }

    TensorType type()
    {
        return m_type;
    }

private:
    TensorType m_type;
    std::vector<size_t> m_tensorDim;
    std::vector<uint8_t> m_tensorData;
};

class ComponentResourceWasiNNGraph : public ComponentResource {
public:
    ComponentResourceWasiNNGraph(ComponentTypeResource* type)
        : ComponentResource(ResourceWasiNNGraph, type)
    {
        m_model = ml::train::createModel(ml::train::ModelType::NEURAL_NET);
    };

    ~ComponentResourceWasiNNGraph()
    {
        m_model.reset();
    }

    ml::train::Model* model()
    {
        return m_model.get();
    }

private:
    std::unique_ptr<ml::train::Model> m_model;
};


class ComponentResourceWasiNNGraphExecutionContext : public ComponentResource {
public:
    ComponentResourceWasiNNGraphExecutionContext(ComponentTypeResource* type, ComponentResourceWasiNNGraph* graph)
        : ComponentResource(ResourceWasiNNGraphExecContext, type)
        , m_graph(graph)
    {
    }

    ComponentResource* graph()
    {
        return m_graph;
    }

private:
    ComponentResourceWasiNNGraph* m_graph;
};

class WasiNN {
public:
    // Definitions following wasi-nn proposal.
    // https://github.com/WebAssembly/wasi-nn/tree/main

#define FOR_EACH_WASINN_FUNCTION(F) \
    F(Load)                         \
    F(LoadByName)                   \
    F(InitExecutionContext)         \
    F(TensorConstructor)            \
    F(Compute)                      \
    F(TensorData)

#define WASI_NN_ERRORS(ERR)                                                                   \
    ERR(success, "No error.")                                                                 \
    ERR(invalid_argument, "Caller module passed an invalid argument.")                        \
    ERR(invalid_encoding, "Invalid encoding.")                                                \
    ERR(timeout, "The operation timed out.")                                                  \
    ERR(runtime_error, "Runtime error.")                                                      \
    ERR(unsupported_operation, "Unsupported operation.")                                      \
    ERR(too_large, "Graph is too large.")                                                     \
    ERR(not_found, "Graph not found.")                                                        \
    ERR(security, "The operation is insecure or has insufficient privilage to be performed.") \
    ERR(unknown, "The operation failed for an unspecified reason.")

    enum ErrNo : uint8_t {
#define TO_ENUM(ERR, MSG) ERR,
        WASI_NN_ERRORS(TO_ENUM)
#undef TO_ENUM
    };

    static inline ComponentResourceWasiNNGraph* asGraph(ComponentHandle* handle)
    {
        return reinterpret_cast<ComponentResourceWasiNNGraph*>(handle);
    }

    static inline ComponentResourceWasiNNGraphExecutionContext* asContext(ComponentHandle* handle)
    {
        return reinterpret_cast<ComponentResourceWasiNNGraphExecutionContext*>(handle);
    }

    static inline ComponentResourceWasiNNTensor* asTensor(ComponentHandle* handle)
    {
        return reinterpret_cast<ComponentResourceWasiNNTensor*>(handle);
    }

#define DECLARE_FUNCTION(NAME) static void NAME(ExecutionState& state, Value* argv, Value* result, ComponentInstance* instance, CanonOptions* options);
    FOR_EACH_WASINN_FUNCTION(DECLARE_FUNCTION)
#undef DECLARE_FUNCTION
};

} // namespace Walrus

#endif // _ENABLE_WASI_NN_
#endif // __WalrusWASI_NN__
