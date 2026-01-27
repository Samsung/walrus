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

#ifdef ENABLE_WASI_NN
#ifdef WASINN_NNTRAINER

#include "wasi/WasiNN.h"

#include "runtime/Value.h"
#include "runtime/Memory.h"
#include "runtime/Instance.h"

#include <fstream>
#include <iostream>

namespace Walrus {

static void* get_memory_pointer(Memory* memory, Value& value, size_t size)
{
    uint32_t offset = value.asI32();

    if (memory->sizeInByte() < size || memory->sizeInByte() - size < offset) {
        return nullptr;
    }

    return memory->buffer() + offset;
}

TensorType nntrainerTensorToWasiNNTensor(ml::train::TensorDim::DataType type, uint32_t& bytesPerType)
{
    switch (type) {
    case ml::train::TensorDim::DataType::QINT4:
    case ml::train::TensorDim::DataType::QINT8:
    case ml::train::TensorDim::DataType::Q4_K:
    case ml::train::TensorDim::DataType::Q6_K:
    case ml::train::TensorDim::DataType::Q4_0:
    case ml::train::TensorDim::DataType::QS4CX:
    case ml::train::TensorDim::DataType::UINT4:
    case ml::train::TensorDim::DataType::UINT8:
    case ml::train::TensorDim::DataType::BCQ: {
        return TensorType::U8;
    }
    case ml::train::TensorDim::DataType::FP16:
    case ml::train::TensorDim::DataType::UINT16:
    case ml::train::TensorDim::DataType::QINT16: {
        bytesPerType = 2;
        return TensorType::FP16;
    }
    case ml::train::TensorDim::DataType::UINT32: {
        bytesPerType = 4;
        return TensorType::I32;
    }
    case ml::train::TensorDim::DataType::FP32: {
        bytesPerType = 4;
        return TensorType::FP32;
    }
    case ml::train::TensorDim::DataType::NONE:
    default: {
        return TensorType::NONE;
    }
    }
}

void WasiNN::Load(ExecutionState& state, Value* argv, Value* result, ComponentInstance* instance, CanonOptions* options)
{
    uint8_t* buffer = options->memory()->buffer() + argv[0].asI32();
    uint32_t builderLen = argv[1].asI32();
    uint32_t encoding = argv[2].asI32();
    uint32_t executionTarget = argv[3].asI32();
    uint32_t offset = argv[4].asI32();

    // nntrainer api does not have a way to load models from arbitiary bytes
    options->memory()->buffer()[offset] = resultError;
    options->memory()->store(state, offset, 4, WasiNN::ErrNo::unsupported_operation);
}

void WasiNN::LoadByName(ExecutionState& state, Value* argv, Value* result, ComponentInstance* instance, CanonOptions* options)
{
    uint32_t length = argv[1].asI32();
    std::string path = std::string(reinterpret_cast<char*>(get_memory_pointer(options->memory(), argv[0], length)), length);
    uint32_t offset = argv[2].asI32();

    ml::train::ModelFormat format = ml::train::ModelFormat::MODEL_FORMAT_BIN;
    if (path.find(".bin") != std::string::npos) {
        format = ml::train::ModelFormat::MODEL_FORMAT_BIN;
    }
    if (path.find(".ini") != std::string::npos) {
        format = ml::train::ModelFormat::MODEL_FORMAT_INI;
    }
    if (path.find(".onnx") != std::string::npos) {
        format = ml::train::ModelFormat::MODEL_FORMAT_ONNX;
    }
    if (path.find(".qnn") != std::string::npos) {
        format = ml::train::ModelFormat::MODEL_FORMAT_QNN;
    }
    if (path.find(".safetensors") != std::string::npos) {
        format = ml::train::ModelFormat::MODEL_FORMAT_SAFETENSORS;
    }
    if (path.find(".flatbuffer") != std::string::npos) {
        format = ml::train::ModelFormat::MODEL_FORMAT_FLATBUFFER;
    }

    ComponentResourceWasiNNGraph* graph = new ComponentResourceWasiNNGraph(instance->type()->getType(0)->asTypeResource());
    try {
        graph->model()->load(path, format);
        graph->model()->compile();
        graph->model()->initialize();
    } catch (const std::exception& e) {
        options->memory()->store(state, offset, 4, WasiNN::runtime_error);
        options->memory()->buffer()[offset] = resultError;
    }

    // When debbugging a model this could come in handy
    // graph->model()->summarize(std::cout, ML_TRAIN_SUMMARY_MODEL);

    uint32_t resource = options->instance()->appendHandle(state, graph);
    options->memory()->store(state, offset, 4, resource);
    options->memory()->buffer()[offset] = resultOk;
}

void WasiNN::TensorConstructor(ExecutionState& state, Value* argv, Value* result, ComponentInstance* instance, CanonOptions* options)
{
    uint32_t* dims = reinterpret_cast<uint32_t*>(get_memory_pointer(options->memory(), argv[0], sizeof(uint32_t)));
    uint32_t dimsLen = argv[1].asI32();
    TensorType tensorType = static_cast<TensorType>(argv[2].asI32());
    uint8_t* data = reinterpret_cast<uint8_t*>(get_memory_pointer(options->memory(), argv[3], sizeof(uint8_t)));
    uint32_t dataLen = argv[4].asI32();

    ComponentResource* tensor = new ComponentResourceWasiNNTensor(instance->type()->getType(3)->asTypeResource(), tensorType, dims, dimsLen, data, dataLen);

    uint32_t resource = options->instance()->appendHandle(state, tensor);
    result[0] = Value(static_cast<int32_t>(resource));
}

void WasiNN::InitExecutionContext(ExecutionState& state, Value* argv, Value* result, ComponentInstance* instance, CanonOptions* options)
{
    uint32_t graph = argv[0].asI32();
    uint32_t offset = argv[1].asI32();

    ComponentHandle* handle = options->instance()->getHandle(state, graph);
    if (handle->kind() != ComponentHandle::ResourceWasiNNGraph) {
        ComponentInstance::throwInvalidHandle(state, graph);
    }

    ComponentResource* context = new ComponentResourceWasiNNGraphExecutionContext(
        instance->type()->getType(0)->asTypeResource(), asGraph(handle));

    uint32_t resource = options->instance()->appendHandle(state, context);
    options->memory()->store(state, offset, 4, resource);
    options->memory()->buffer()[offset] = resultOk;
}

void WasiNN::Compute(ExecutionState& state, Value* argv, Value* result, ComponentInstance* instance, CanonOptions* options)
{
    uint32_t executionContextIdx = argv[0].asI32();
    uint32_t* inputs = reinterpret_cast<uint32_t*>(get_memory_pointer(options->memory(), argv[1], sizeof(uint32_t)));
    uint32_t inputLen = argv[2].asI32();
    uint32_t offset = argv[3].asI32();

    ComponentHandle* handle = options->instance()->getHandle(state, executionContextIdx);
    if (handle->kind() != ComponentHandle::ResourceWasiNNGraphExecContext) {
        ComponentInstance::throwInvalidHandle(state, executionContextIdx);
    }

    std::vector<float*> inputVec;
    std::vector<float*> labels;
    uint32_t inputOffset = 0;
    for (uint32_t i = 0; i < inputLen; i++) {
        uint32_t nameOffset = *reinterpret_cast<uint32_t*>(inputs + inputOffset++);
        char* name = reinterpret_cast<char*>(options->memory()->buffer() + nameOffset);
        uint32_t nameLen = *(inputs + inputOffset++);
        uint32_t tensorIdx = *(inputs + inputOffset++);

        ComponentHandle* tensorHandle = options->instance()->getHandle(state, tensorIdx);
        if (tensorHandle->kind() != ComponentHandle::ResourceWasiNNTensor) {
            ComponentInstance::throwInvalidHandle(state, tensorIdx);
        }
        ComponentResourceWasiNNTensor* tensor = asTensor(tensorHandle);

        uint32_t tensorLen = 1;
        for (auto elem : tensor->tensorDim()) {
            tensorLen *= elem;
        }

        inputVec.push_back(new float[tensorLen]);
        for (uint32_t j = 0; j < tensorLen; j++) {
            inputVec.back()[j] = *reinterpret_cast<float*>(tensor->tensorData().data() + j * sizeof(float));
        }
    }

    ComponentResourceWasiNNGraph* graph = asGraph(asContext(handle)->graph());

    std::vector<float*> ret = graph->model()->inference(1, inputVec, labels);

    for (auto& elem : inputVec) {
        delete elem;
    }


    std::vector<uint32_t> outputTensorList;
    uint32_t outputLen = 0;
    auto outputDim = graph->model()->getOutputDimension();
    for (uint32_t i = 0; i < outputDim.size(); i++) {
        uint32_t bytesPerType = 1;
        TensorType type = nntrainerTensorToWasiNNTensor(outputDim[i].getDataType(), bytesPerType);
        if (type == TensorType::NONE) {
            options->memory()->store(state, offset, 0, resultError);
            options->memory()->store(state, offset, 4, WasiNN::ErrNo::invalid_encoding);
            return;
        }

        uint32_t len = outputDim[i].getFeatureLen() * bytesPerType;
        ComponentResourceWasiNNTensor* output = new ComponentResourceWasiNNTensor(
            instance->type()->getType(3)->asTypeResource(),
            type,
            std::vector<size_t>{ outputDim[i].batch(), outputDim[i].channel(), outputDim[i].height(), outputDim[i].width() },
            std::vector<uint8_t>(len));

        for (uint32_t j = 0; j < len; j++) {
            output->tensorData().data()[j] = reinterpret_cast<uint8_t*>(ret[i])[j];
        }

        uint32_t resource = options->instance()->appendHandle(state, reinterpret_cast<ComponentResource*>(output));
        outputTensorList.push_back(resource);
    }

    uint32_t listStart = options->memoryMalloc32(state, 4, outputTensorList.size() * 3);
    uint32_t* argBuffer = reinterpret_cast<uint32_t*>(options->memory()->buffer() + listStart);

    options->memory()->buffer()[offset] = resultOk;
    uint32_t* list = reinterpret_cast<uint32_t*>(options->memory()->buffer() + offset);
    list[1] = listStart;
    list[2] = outputTensorList.size();

    for (uint32_t i = 0; i < outputTensorList.size(); i++) {
        std::string tmp = "output" + std::to_string(i);
        uint32_t length = tmp.length();
        CanonOptions::UtfData utfData;
        utfData.init(CanonOptions::UtfData::Utf8, reinterpret_cast<const uint8_t*>(tmp.data()), length);
        *argBuffer++ = static_cast<uint32_t>(options->storeString(state, utfData, &length));
        *argBuffer++ = length;
        *argBuffer++ = outputTensorList[i];
    }
}

void WasiNN::TensorData(ExecutionState& state, Value* argv, Value* result, ComponentInstance* instance, CanonOptions* options)
{
    uint32_t tensorIdx = argv[0].asI32();
    uint32_t offset = argv[1].asI32();

    ComponentHandle* handle = options->instance()->getHandle(state, tensorIdx);
    if (handle->kind() != ComponentHandle::ResourceWasiNNTensor) {
        ComponentInstance::throwInvalidHandle(state, tensorIdx);
    }
    ComponentResourceWasiNNTensor* tensor = asTensor(handle);

    std::vector<uint8_t> data = tensor->tensorData();
    uint32_t start = options->memoryMalloc32(state, 4, data.size());
    memcpy(options->memory()->buffer() + start, data.data(), data.size());

    uint32_t* list = reinterpret_cast<uint32_t*>(options->memory()->buffer() + offset);
    list[0] = start;
    list[1] = data.size();
}

} // namespace Walrus

#endif // _WASINN_NNTRAINER_
#endif // _ENABLE_WASI_NN
