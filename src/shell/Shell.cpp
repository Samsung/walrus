#include <iostream>
#include <locale>
#include <sstream>
#include <iomanip>
#include <inttypes.h>

#if defined(WALRUS_GOOGLE_PERF)
#include <gperftools/profiler.h>
#endif

#include "Walrus.h"
#include "runtime/Engine.h"
#include "runtime/Store.h"
#include "runtime/Module.h"
#include "runtime/Instance.h"
#include "runtime/Function.h"
#include "runtime/Table.h"
#include "runtime/Memory.h"
#include "runtime/Global.h"
#include "runtime/Tag.h"
#include "runtime/Trap.h"
#include "runtime/DefinedFunctionTypes.h"
#include "parser/WASMParser.h"

#include "wabt/wast-lexer.h"
#include "wabt/wast-parser.h"
#include "wabt/binary-writer.h"
#include "string-view-lite/string_view.h"

#ifdef ENABLE_WASI
#include "wasi/WASI.h"
#endif

struct spectestseps : std::numpunct<char> {
    char do_thousands_sep() const { return '_'; }
    std::string do_grouping() const { return "\3"; }
};

struct ParseOptions {
    std::string exportToRun;
    std::vector<std::string> fileNames;

    // WASI options
    std::vector<std::string> wasi_envs;
    std::vector<std::pair<std::string, std::string>> wasi_dirs;
    int argsIndex = -1;
};

static uint32_t s_JITFlags = 0;

using namespace Walrus;

static void printI32(int32_t v)
{
    std::stringstream ss;
    std::locale slocale(std::locale(), new spectestseps);
    ss.imbue(slocale);
    ss << v;
    printf("%s : i32\n", ss.str().c_str());
}

static void printI64(int64_t v)
{
    std::stringstream ss;
    std::locale slocale(std::locale(), new spectestseps);
    ss.imbue(slocale);
    ss << v;
    printf("%s : i64\n", ss.str().c_str());
}

static std::string formatDecmialString(std::string s)
{
    while (s.find('.') != std::string::npos && s[s.length() - 1] == '0') {
        s.resize(s.length() - 1);
    }

    if (s.length() && s[s.length() - 1] == '.') {
        s.resize(s.length() - 1);
    }

    auto pos = s.find('.');
    if (pos != std::string::npos) {
        std::string out = s.substr(0, pos);
        out += ".";

        size_t cnt = 0;
        for (size_t i = pos + 1; i < s.length(); i++) {
            out += s[i];
            cnt++;
            if (cnt % 3 == 0 && i != s.length() - 1) {
                out += "_";
            }
        }

        s = out;
    }

    return s;
}

static void printF32(float v)
{
    std::stringstream ss;
    ss.imbue(std::locale(std::locale(), new spectestseps));
    ss.setf(std::ios_base::fixed);
    ss << std::setprecision(std::numeric_limits<float>::max_digits10);
    ss << v;
    printf("%s : f32\n", formatDecmialString(ss.str()).c_str());
}

static void printF64(double v)
{
    std::stringstream ss;
    ss.imbue(std::locale(std::locale(), new spectestseps));
    ss.setf(std::ios_base::fixed);
    ss << std::setprecision(std::numeric_limits<double>::max_digits10 - 1);
    ss << v;
    printf("%s : f64\n", formatDecmialString(ss.str()).c_str());
}

static Trap::TrapResult executeWASM(Store* store, const std::string& filename, const std::vector<uint8_t>& src, DefinedFunctionTypes& functionTypes,
                                    std::map<std::string, Instance*>* registeredInstanceMap = nullptr)
{
    auto parseResult = WASMParser::parseBinary(store, filename, src.data(), src.size(), s_JITFlags);
    if (!parseResult.second.empty()) {
        Trap::TrapResult tr;
        tr.exception = Exception::create(parseResult.second);
        return tr;
    }

    auto module = parseResult.first;
    const auto& importTypes = module->imports();

    ExternVector importValues;
    importValues.reserve(importTypes.size());
    /*
        (module ;; spectest host module(https://github.com/WebAssembly/spec/tree/main/interpreter)
          (global (export "global_i32") i32)
          (global (export "global_i64") i64)
          (global (export "global_f32") f32)
          (global (export "global_f64") f64)

          (table (export "table") 10 20 funcref)

          (memory (export "memory") 1 2)

          (func (export "print"))
          (func (export "print_i32") (param i32))
          (func (export "print_i64") (param i64))
          (func (export "print_f32") (param f32))
          (func (export "print_f64") (param f64))
          (func (export "print_i32_f32") (param i32 f32))
          (func (export "print_f64_f64") (param f64 f64))
        )
    */
    bool hasWasiImport = false;

    for (size_t i = 0; i < importTypes.size(); i++) {
        auto import = importTypes[i];
        if (import->moduleName() == "spectest") {
            if (import->fieldName() == "print") {
                auto ft = functionTypes[DefinedFunctionTypes::NONE];
                importValues.push_back(ImportedFunction::createImportedFunction(
                    store,
                    ft,
                    [](ExecutionState& state, Value* argv, Value* result, void* data) {
                    },
                    nullptr));
            } else if (import->fieldName() == "print_i32") {
                auto ft = functionTypes[DefinedFunctionTypes::I32R];
                importValues.push_back(ImportedFunction::createImportedFunction(
                    store,
                    ft,
                    [](ExecutionState& state, Value* argv, Value* result, void* data) {
                        printI32(argv[0].asI32());
                    },
                    nullptr));
            } else if (import->fieldName() == "print_i64") {
                auto ft = functionTypes[DefinedFunctionTypes::I64R];
                importValues.push_back(ImportedFunction::createImportedFunction(
                    store,
                    ft,
                    [](ExecutionState& state, Value* argv, Value* result, void* data) {
                        printI64(argv[0].asI64());
                    },
                    nullptr));
            } else if (import->fieldName() == "print_f32") {
                auto ft = functionTypes[DefinedFunctionTypes::F32R];
                importValues.push_back(ImportedFunction::createImportedFunction(
                    store,
                    ft,
                    [](ExecutionState& state, Value* argv, Value* result, void* data) {
                        printF32(argv[0].asF32());
                    },
                    nullptr));
            } else if (import->fieldName() == "print_f64") {
                auto ft = functionTypes[DefinedFunctionTypes::F64R];
                importValues.push_back(ImportedFunction::createImportedFunction(
                    store,
                    ft,
                    [](ExecutionState& state, Value* argv, Value* result, void* data) {
                        printF64(argv[0].asF64());
                    },
                    nullptr));
            } else if (import->fieldName() == "print_i32_f32") {
                auto ft = functionTypes[DefinedFunctionTypes::I32F32R];
                importValues.push_back(ImportedFunction::createImportedFunction(
                    store,
                    ft,
                    [](ExecutionState& state, Value* argv, Value* result, void* data) {
                        printI32(argv[0].asI32());
                        printF32(argv[1].asF32());
                    },
                    nullptr));
            } else if (import->fieldName() == "print_f64_f64") {
                auto ft = functionTypes[DefinedFunctionTypes::F64F64R];
                importValues.push_back(ImportedFunction::createImportedFunction(
                    store,
                    ft,
                    [](ExecutionState& state, Value* argv, Value* result, void* data) {
                        printF64(argv[0].asF64());
                        printF64(argv[1].asF64());
                    },
                    nullptr));
            } else if (import->fieldName() == "global_i32") {
                importValues.push_back(Global::createGlobal(store, Value(int32_t(666)), false));
            } else if (import->fieldName() == "global_i64") {
                importValues.push_back(Global::createGlobal(store, Value(int64_t(666)), false));
            } else if (import->fieldName() == "global_f32") {
                importValues.push_back(Global::createGlobal(store, Value(float(0x44268000)), false));
            } else if (import->fieldName() == "global_f64") {
                importValues.push_back(Global::createGlobal(store, Value(double(0x4084d00000000000)), false));
            } else if (import->fieldName() == "table") {
                importValues.push_back(Table::createTable(store, Value::Type::FuncRef, 10, 20));
            } else if (import->fieldName() == "memory") {
                importValues.push_back(Memory::createMemory(store, 1 * Memory::s_memoryPageSize, 2 * Memory::s_memoryPageSize, false));
            } else {
                // import wrong value for test
                auto ft = functionTypes[DefinedFunctionTypes::INVALID];
                importValues.push_back(ImportedFunction::createImportedFunction(
                    store,
                    ft,
                    [](ExecutionState& state, Value* argv, Value* result, void* data) {
                    },
                    nullptr));
            }
#ifdef ENABLE_WASI
        } else if (import->moduleName() == "wasi_snapshot_preview1") {
            WASI::WasiFuncInfo* wasiImportFunc = WASI::find(import->fieldName());
            if (wasiImportFunc) {
                FunctionType* ft = functionTypes[wasiImportFunc->functionType];
                if (ft->equals(import->functionType())) {
                    importValues.push_back(WasiFunction::createWasiFunction(
                        store,
                        ft,
                        wasiImportFunc->ptr));
                }
                hasWasiImport = true;
            }
#endif
        } else if (registeredInstanceMap) {
            auto iter = registeredInstanceMap->find(import->moduleName());
            if (iter != registeredInstanceMap->end()) {
                Instance* instance = iter->second;
                auto e = instance->resolveExportType(import->fieldName());
                if (e == nullptr) {
                    printf("Error: %s:%s module has not been found.\n", import->fieldName().c_str(), import->moduleName().c_str());
                    RELEASE_ASSERT_NOT_REACHED();
                }
                switch (e->exportType()) {
                case ExportType::Function:
                    importValues.push_back(instance->resolveExportFunction(import->fieldName()));
                    break;
                case ExportType::Tag:
                    importValues.push_back(instance->resolveExportTag(import->fieldName()));
                    break;
                case ExportType::Table:
                    importValues.push_back(instance->resolveExportTable(import->fieldName()));
                    break;
                case ExportType::Memory:
                    importValues.push_back(instance->resolveExportMemory(import->fieldName()));
                    break;
                case ExportType::Global:
                    importValues.push_back(instance->resolveExportGlobal(import->fieldName()));
                    break;
                default:
                    printf("Error: unsupported export type: %s\n", import->moduleName().c_str());
                    RELEASE_ASSERT_NOT_REACHED();
                    break;
                }
            }
        }
    }

    struct RunData {
        Module* module;
        ExternVector& importValues;
        bool hasWasiImport;
    } data = { module.value(), importValues, hasWasiImport };
    Walrus::Trap trap;
    return trap.run([](ExecutionState& state, void* d) {
        RunData* data = reinterpret_cast<RunData*>(d);
        Instance* instance = data->module->instantiate(state, data->importValues);

#ifdef ENABLE_WASI
        if (data->hasWasiImport) {
            for (auto&& exp : data->module->exports()) {
                if (exp->exportType() == ExportType::Function) {
                    if ("_start" != exp->name()) {
                        continue;
                    }

                    auto fn = instance->function(exp->itemIndex());
                    FunctionType* fnType = fn->asDefinedFunction()->moduleFunction()->functionType();

                    if (!fnType->param().empty()) {
                        printf("warning: function %s has params, but params are not supported\n", exp->name().c_str());
                        return;
                    }

                    if (!fnType->result().empty()) {
                        printf("warning: function %s has results, but results are not supported\n", exp->name().c_str());
                        return;
                    }


                    fn->call(state, nullptr, nullptr);
                }
            }
        }
#endif
    },
                    &data);
}

static bool endsWith(const std::string& str, const std::string& suffix)
{
    return str.size() >= suffix.size() && str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

static Walrus::Value toWalrusValue(wabt::Const& c)
{
    switch (c.type()) {
    case wabt::Type::I32:
        return Walrus::Value(static_cast<int32_t>(c.u32()));
    case wabt::Type::I64:
        return Walrus::Value(static_cast<int64_t>(c.u64()));
    case wabt::Type::F32: {
        if (c.is_expected_nan(0)) {
            return Walrus::Value(std::numeric_limits<float>::quiet_NaN());
        }
        float s;
        auto bits = c.f32_bits();
        memcpy(&s, &bits, sizeof(float));
        return Walrus::Value(s);
    }
    case wabt::Type::F64: {
        if (c.is_expected_nan(0)) {
            return Walrus::Value(std::numeric_limits<double>::quiet_NaN());
        }
        double s;
        auto bits = c.f64_bits();
        memcpy(&s, &bits, sizeof(double));
        return Walrus::Value(s);
    }
    case wabt::Type::V128: {
        Walrus::Vec128 v;
        v128 bits = c.vec128();
        memcpy((void*)&v, (void*)&bits, sizeof(v128));
        return Walrus::Value(v);
    }
    case wabt::Type::FuncRef: {
        if (c.ref_bits() == wabt::Const::kRefNullBits) {
            return Walrus::Value(Walrus::Value::FuncRef, Walrus::Value::Null);
        }
        // Add one similar to wabt interpreter.
        return Walrus::Value(Walrus::Value::FuncRef, c.ref_bits() + 1, Walrus::Value::Force);
    }
    case wabt::Type::ExternRef: {
        if (c.ref_bits() == wabt::Const::kRefNullBits) {
            return Walrus::Value(Walrus::Value::ExternRef, Walrus::Value::Null);
        }
        // Add one similar to wabt interpreter.
        return Walrus::Value(Walrus::Value::ExternRef, c.ref_bits() + 1, Walrus::Value::Force);
    }
    default:
        printf("Error: unknown value type during converting wabt::Const to wabt::Value\n");
        RELEASE_ASSERT_NOT_REACHED();
        return Walrus::Value();
    }
}

static bool isCanonicalNan(float val)
{
    uint32_t s;
    memcpy(&s, &val, sizeof(float));
    return s == 0x7fc00000U || s == 0xffc00000U;
}

static bool isCanonicalNan(double val)
{
    uint64_t s;
    memcpy(&s, &val, sizeof(double));
    return s == 0x7ff8000000000000ULL || s == 0xfff8000000000000ULL;
}

static bool isArithmeticNan(float val)
{
    uint32_t s;
    memcpy(&s, &val, sizeof(float));
    return (s & 0x7fc00000U) == 0x7fc00000U;
}

static bool isArithmeticNan(double val)
{
    uint64_t s;
    memcpy(&s, &val, sizeof(double));
    return (s & 0x7ff8000000000000ULL) == 0x7ff8000000000000ULL;
}

static bool equals(Walrus::Value& v, wabt::Const& c)
{
    if (c.type() == wabt::Type::I32 && v.type() == Walrus::Value::I32) {
        return v.asI32() == static_cast<int32_t>(c.u32());
    } else if (c.type() == wabt::Type::I64 && v.type() == Walrus::Value::I64) {
        return v.asI64() == static_cast<int64_t>(c.u64());
    } else if (c.type() == wabt::Type::F32 && v.type() == Walrus::Value::F32) {
        if (c.is_expected_nan(0)) {
            if (c.expected_nan() == wabt::ExpectedNan::Arithmetic) {
                return isArithmeticNan(v.asF32());
            } else {
                return isCanonicalNan(v.asF32());
            }
        }
        return c.f32_bits() == v.asF32Bits();
    } else if (c.type() == wabt::Type::F64 && v.type() == Walrus::Value::F64) {
        if (c.is_expected_nan(0)) {
            if (c.expected_nan() == wabt::ExpectedNan::Arithmetic) {
                return isArithmeticNan(v.asF64());
            } else {
                return isCanonicalNan(v.asF64());
            }
        }
        return c.f64_bits() == v.asF64Bits();
    } else if (c.type() == wabt::Type::V128 && v.type() == Walrus::Value::V128) {
        switch (c.lane_type()) {
        case wabt::Type::I8:
        case wabt::Type::I16:
        case wabt::Type::I32:
        case wabt::Type::I64:
            return memcmp(v.asV128Addr(), c.vec128().v, 16) == 0;
        case wabt::Type::F32: {
            bool result = true;
            for (int lane = 0; lane < c.lane_count(); ++lane) {
                if (c.is_expected_nan(lane)) {
                    float value = v.asV128().asF32(lane);
                    if (c.expected_nan(lane) == wabt::ExpectedNan::Arithmetic) {
                        result &= isArithmeticNan(value);
                    } else {
                        result &= isCanonicalNan(value);
                    }
                } else {
                    result &= (v.asV128().asF32Bits(lane) == c.v128_lane<uint32_t>(lane));
                }
            }
            return result;
        }
        case wabt::Type::F64: {
            bool result = true;
            for (int lane = 0; lane < c.lane_count(); ++lane) {
                if (c.is_expected_nan(lane)) {
                    double value = v.asV128().asF64(lane);
                    if (c.expected_nan(lane) == wabt::ExpectedNan::Arithmetic) {
                        result &= isArithmeticNan(value);
                    } else {
                        result &= isCanonicalNan(value);
                    }
                } else {
                    result &= (v.asV128().asF64Bits(lane) == c.v128_lane<uint64_t>(lane));
                }
            }
            return result;
        }
        default:
            return false;
        }

    } else if (c.type() == wabt::Type::ExternRef && v.type() == Walrus::Value::ExternRef) {
        // FIXME value of c.ref_bits() for RefNull
        wabt::Const constNull;
        constNull.set_null(c.type());
        if (c.ref_bits() == constNull.ref_bits()) {
            // check RefNull
            return v.isNull();
        }
        // Add one similar to wabt interpreter.
        return (c.ref_bits() + 1) == reinterpret_cast<uintptr_t>(v.asExternal());
    } else if (c.type() == wabt::Type::FuncRef && v.type() == Walrus::Value::FuncRef) {
        // FIXME value of c.ref_bits() for RefNull
        wabt::Const constNull;
        constNull.set_null(c.type());
        if (c.ref_bits() == constNull.ref_bits()) {
            // check RefNull
            return v.isNull();
        }
        // Add one similar to wabt interpreter.
        return (c.ref_bits() + 1) == reinterpret_cast<uintptr_t>(v.asFunction());
    }

    return false;
}

static void printConstVector(wabt::ConstVector& v)
{
    for (size_t i = 0; i < v.size(); i++) {
        auto c = v[i];
        switch (c.type()) {
        case wabt::Type::I32: {
            printf("%" PRIu32, c.u32());
            break;
        }
        case wabt::Type::I64: {
            printf("%" PRIu64, c.u64());
            break;
        }
        case wabt::Type::F32: {
            if (c.is_expected_nan(0)) {
                printf("nan");
                return;
            }
            float s;
            auto bits = c.f32_bits();
            memcpy(&s, &bits, sizeof(float));
            printf("%f", s);
            break;
        }
        case wabt::Type::F64: {
            if (c.is_expected_nan(0)) {
                printf("nan");
                return;
            }
            double s;
            auto bits = c.f64_bits();
            memcpy(&s, &bits, sizeof(double));
            printf("%lf", s);
            break;
        }
        case wabt::Type::V128: {
            char result[16 * 3];
            char* ptr = result;
            for (int i = 0; i < 16; i++) {
                uint8_t left = (c.vec128().u8(i) & 0xf0) >> 4;
                uint8_t right = c.vec128().u8(i) & 0x0f;
                ptr[0] = (left < 10) ? ('0' + left) : ('a' + (left - 10));
                ptr[1] = (right < 10) ? ('0' + right) : ('a' + (right - 10));
                ptr[2] = ':';
                ptr += 3;
            }
            ptr[-1] = '\0';
            printf("%s", result);
            break;
        }
        case wabt::Type::ExternRef: {
            // FIXME value of c.ref_bits() for RefNull
            wabt::Const constNull;
            constNull.set_null(c.type());
            if (c.ref_bits() == constNull.ref_bits()) {
                printf("ref.null");
                return;
            }
            break;
        }
        case wabt::Type::FuncRef: {
            // FIXME value of c.ref_bits() for RefNull
            wabt::Const constNull;
            constNull.set_null(c.type());
            if (c.ref_bits() == constNull.ref_bits()) {
                printf("ref.null");
                return;
            }
            break;
        }
        default: {
            printf("Error: unkown wabt::Const type\n");
            RELEASE_ASSERT_NOT_REACHED();
            break;
        }
        }
        if (i + 1 != v.size()) {
            printf(", ");
        }
    }
}

static void executeInvokeAction(wabt::InvokeAction* action, Walrus::Function* fn, wabt::ConstVector expectedResult,
                                const char* expectedException, bool expectUserException = false, bool either = false)
{
    if (fn->functionType()->param().size() != action->args.size()) {
        printf("Error: expected %zu parameter(s) but got %zu.\n", fn->functionType()->param().size(), action->args.size());
        RELEASE_ASSERT_NOT_REACHED();
    }
    Walrus::ValueVector args;
    for (auto& a : action->args) {
        args.push_back(toWalrusValue(a));
    }

    struct RunData {
        Walrus::Function* fn;
        wabt::ConstVector& expectedResult;
        Walrus::ValueVector& args;
        wabt::InvokeAction* action;
        bool either;
    } data = { fn, expectedResult, args, action, either };
    Walrus::Trap trap;
    auto trapResult = trap.run([](Walrus::ExecutionState& state, void* d) {
        RunData* data = reinterpret_cast<RunData*>(d);
        Walrus::ValueVector result;
        result.resize(data->fn->functionType()->result().size());
        data->fn->call(state, data->args.data(), result.data());
        if (data->expectedResult.size()) {
            int errorIndex = -1;

            if (data->either) {
                if (data->fn->functionType()->result().size() != 1) {
                    printf("Error: %s returned with %zu parameter(s) but expected 1", data->action->name.data(), data->fn->functionType()->result().size());
                    RELEASE_ASSERT_NOT_REACHED();
                }

                // compare result
                for (size_t i = 0; i < data->expectedResult.size(); i++) {
                    if (equals(result[0], data->expectedResult[i])) {
                        return;
                    }
                }

                errorIndex = 0;
            } else {
                if (data->fn->functionType()->result().size() != data->expectedResult.size()) {
                    printf("Error: %s returned with %zu parameter(s) but expected %zu", data->action->name.data(), data->fn->functionType()->result().size(), data->expectedResult.size());
                    RELEASE_ASSERT_NOT_REACHED();
                }

                // compare result
                for (size_t i = 0; i < result.size(); i++) {
                    if (!equals(result[i], data->expectedResult[i])) {
                        errorIndex = i;
                        break;
                    }
                }

                if (errorIndex == -1) {
                    return;
                }
            }

            printf("Assertion failed at %d: ", data->action->loc.line);
            printf("%s(", data->action->name.data());
            printConstVector(data->action->args);
            printf(") %sexpected ", data->either ? "any " : "");
            printConstVector(data->expectedResult);
            printf(", but got %s\n", ((std::string)result[errorIndex]).c_str());
            RELEASE_ASSERT_NOT_REACHED();
        }
    },
                               &data);
    if (expectedResult.size()) {
        if (trapResult.exception != nullptr) {
            printf("Error: %s\n", trapResult.exception->message().c_str());
            RELEASE_ASSERT_NOT_REACHED();
        }
    }
    if (expectedException) {
        if (trapResult.exception == nullptr) {
            printf("Missing exception: %s\n", expectedException);
            RELEASE_ASSERT_NOT_REACHED();
        }
        std::string& s = trapResult.exception->message();
        if (s.find(expectedException) != 0) {
            printf("Error: different error message than expected!\n");
            printf("Expected: %s\n", expectedException);
            printf("But got: %s\n", s.c_str());
            RELEASE_ASSERT_NOT_REACHED();
        }
        printf("invoke %s(", action->name.data());
        printConstVector(action->args);
        printf("), expect exception: %s (line: %d) : OK\n", expectedException, action->loc.line);
    } else if (expectUserException) {
        if (trapResult.exception->tag() == nullptr) {
            printf("Missing user exception: %s\n", action->name.data());
            RELEASE_ASSERT_NOT_REACHED();
        }
        printf("invoke %s(", action->name.data());
        printConstVector(action->args);
        printf(") expect user exception() (line: %d) : OK\n", action->loc.line);
    } else if (expectedResult.size()) {
        printf("invoke %s(", action->name.data());
        printConstVector(action->args);
        printf(") expect %svalue(", either ? "either " : "");
        printConstVector(expectedResult);
        printf(") (line: %d) : OK\n", action->loc.line);
    }
}

static std::unique_ptr<wabt::OutputBuffer> readModuleData(wabt::Module* module)
{
    wabt::MemoryStream stream;
    wabt::WriteBinaryOptions options;
    wabt::Features features;
    features.EnableAll();
    options.features = features;
    wabt::WriteBinaryModule(&stream, module, options);
    stream.Flush();
    return stream.ReleaseOutputBuffer();
}

static Instance* fetchInstance(wabt::Var& moduleVar, std::map<size_t, Instance*>& instanceMap,
                               std::map<std::string, Instance*>& registeredInstanceMap)
{
    if (moduleVar.is_index()) {
        return instanceMap[moduleVar.index()];
    }
    return registeredInstanceMap[moduleVar.name()];
}

static void executeWAST(Store* store, const std::string& filename, const std::vector<uint8_t>& src, DefinedFunctionTypes& functionTypes)
{
    wabt::Errors errors;
    auto lexer = wabt::WastLexer::CreateBufferLexer("test.wabt", src.data(), src.size(), &errors);
    ASSERT(lexer);

    std::unique_ptr<wabt::Script> script;
    wabt::Features features;
    features.EnableAll();
    wabt::WastParseOptions parse_wast_options(features);
    auto result = wabt::ParseWastScript(lexer.get(), &script, &errors, &parse_wast_options);
    if (!wabt::Succeeded(result)) {
        printf("Syntax error(s):\n");
        for (auto& e : errors) {
            printf("  %s\n", e.message.c_str());
        }
        printf("\n");
        RELEASE_ASSERT_NOT_REACHED();
    }

    std::map<size_t, Instance*> instanceMap;
    std::map<std::string, Instance*> registeredInstanceMap;
    size_t commandCount = 0;
    for (const std::unique_ptr<wabt::Command>& command : script->commands) {
        switch (command->type) {
        case wabt::CommandType::Module:
        case wabt::CommandType::ScriptModule: {
            auto* moduleCommand = static_cast<wabt::ModuleCommand*>(command.get());
            auto buf = readModuleData(&moduleCommand->module);
            auto trapResult = executeWASM(store, filename, buf->data, functionTypes, &registeredInstanceMap);
            if (trapResult.exception) {
                std::string& errorMessage = trapResult.exception->message();
                printf("Error: %s\n", errorMessage.c_str());
                RELEASE_ASSERT_NOT_REACHED();
            }
            instanceMap[commandCount] = store->getLastInstance();
            if (moduleCommand->module.name.size()) {
                registeredInstanceMap[moduleCommand->module.name] = store->getLastInstance();
            }
            break;
        }
        case wabt::CommandType::AssertReturn: {
            auto* assertReturn = static_cast<wabt::AssertReturnCommand*>(command.get());
            auto value = fetchInstance(assertReturn->action->module_var, instanceMap, registeredInstanceMap)->resolveExportType(assertReturn->action->name);
            if (value == nullptr) {
                printf("Undefined function: %s\n", assertReturn->action->name.c_str());
                RELEASE_ASSERT_NOT_REACHED();
            }
            if (assertReturn->action->type() == wabt::ActionType::Invoke) {
                auto action = static_cast<wabt::InvokeAction*>(assertReturn->action.get());
                auto fn = fetchInstance(action->module_var, instanceMap, registeredInstanceMap)->resolveExportFunction(action->name);
                executeInvokeAction(action, fn, assertReturn->expected->expected, nullptr, false, assertReturn->expected->type() == wabt::ExpectationType::Either);
            } else if (assertReturn->action->type() == wabt::ActionType::Get) {
                auto action = static_cast<wabt::GetAction*>(assertReturn->action.get());
                auto v = fetchInstance(action->module_var, instanceMap, registeredInstanceMap)->resolveExportGlobal(action->name)->value();
                if (!equals(v, assertReturn->expected->expected[0])) {
                    printf("Assert failed.\n");
                    RELEASE_ASSERT_NOT_REACHED();
                }
                printf("get %s", action->name.data());
                printf(" expect value(");
                printConstVector(assertReturn->expected->expected);
                printf(") (line: %d) : OK\n", action->loc.line);
            } else {
                printf("Not supported action type.\n");
                RELEASE_ASSERT_NOT_REACHED();
            }
            break;
        }
        case wabt::CommandType::AssertTrap: {
            auto* assertTrap = static_cast<wabt::AssertTrapCommand*>(command.get());
            auto value = fetchInstance(assertTrap->action->module_var, instanceMap, registeredInstanceMap)->resolveExportFunction(assertTrap->action->name);
            if (value == nullptr) {
                printf("Error: fetchInstance returned with nullptr.\n");
                RELEASE_ASSERT_NOT_REACHED();
            }
            if (assertTrap->action->type() == wabt::ActionType::Invoke) {
                auto action = static_cast<wabt::InvokeAction*>(assertTrap->action.get());
                auto fn = fetchInstance(action->module_var, instanceMap, registeredInstanceMap)->resolveExportFunction(action->name);
                executeInvokeAction(action, fn, wabt::ConstVector(), assertTrap->text.data());
            } else {
                ASSERT_NOT_REACHED();
            }
            break;
        }
        case wabt::CommandType::AssertException: {
            auto* assertException = static_cast<wabt::AssertExceptionCommand*>(command.get());
            auto value = fetchInstance(assertException->action->module_var, instanceMap, registeredInstanceMap)->resolveExportFunction(assertException->action->name);
            if (value == nullptr) {
                printf("Fetching instance failed (at wabt::CommandType::AssertException case)\n");
                RELEASE_ASSERT_NOT_REACHED();
            }
            if (assertException->action->type() == wabt::ActionType::Invoke) {
                auto action = static_cast<wabt::InvokeAction*>(assertException->action.get());
                auto fn = fetchInstance(action->module_var, instanceMap, registeredInstanceMap)->resolveExportFunction(action->name);
                executeInvokeAction(action, fn, wabt::ConstVector(), nullptr, true);
            } else {
                ASSERT_NOT_REACHED();
            }
            break;
        }
        case wabt::CommandType::AssertUninstantiable: {
            auto* assertModuleUninstantiable = static_cast<wabt::AssertModuleCommand<wabt::CommandType::AssertUninstantiable>*>(command.get());
            auto m = assertModuleUninstantiable->module.get();
            auto tsm = dynamic_cast<wabt::TextScriptModule*>(m);
            if (tsm == nullptr) {
                printf("Error at casting to wabt::TextScriptModule*.\n");
                RELEASE_ASSERT_NOT_REACHED();
            }
            auto buf = readModuleData(&tsm->module);
            auto trapResult = executeWASM(store, filename, buf->data, functionTypes, &registeredInstanceMap);
            RELEASE_ASSERT(trapResult.exception);
            std::string& s = trapResult.exception->message();
            if (s.find(assertModuleUninstantiable->text) != 0) {
                printf("Error: different error message than expected!\n");
                printf("Expected: %s\n", assertModuleUninstantiable->text.c_str());
                printf("But got: %s\n", s.c_str());
                RELEASE_ASSERT_NOT_REACHED();
            }
            printf("assertModuleUninstantiable (expect exception: %s(line: %d)) : OK\n", assertModuleUninstantiable->text.data(), assertModuleUninstantiable->module->location().line);
            break;
        }
        case wabt::CommandType::Register: {
            auto* registerCommand = static_cast<wabt::RegisterCommand*>(command.get());
            registeredInstanceMap[registerCommand->module_name] = fetchInstance(registerCommand->var, instanceMap, registeredInstanceMap);
            break;
        }
        case wabt::CommandType::Action: {
            auto* actionCommand = static_cast<wabt::ActionCommand*>(command.get());
            auto value = fetchInstance(actionCommand->action->module_var, instanceMap, registeredInstanceMap)->resolveExportFunction(actionCommand->action->name);
            if (value == nullptr) {
                printf("Fetching instance failed (at wabt::CommandType::Action case)");
                RELEASE_ASSERT_NOT_REACHED();
            }
            if (actionCommand->action->type() == wabt::ActionType::Invoke) {
                auto action = static_cast<wabt::InvokeAction*>(actionCommand->action.get());
                auto fn = fetchInstance(action->module_var, instanceMap, registeredInstanceMap)->resolveExportFunction(action->name);
                executeInvokeAction(action, fn, wabt::ConstVector(), nullptr);
            } else {
                ASSERT_NOT_REACHED();
            }
            break;
        }
        case wabt::CommandType::AssertInvalid: {
            auto* assertModuleInvalid = static_cast<wabt::AssertModuleCommand<wabt::CommandType::AssertInvalid>*>(command.get());
            auto m = assertModuleInvalid->module.get();
            auto tsm = dynamic_cast<wabt::TextScriptModule*>(m);
            auto dsm = dynamic_cast<wabt::BinaryScriptModule*>(m);
            if (!tsm && !dsm) {
                printf("Module is neither TextScriptModule nor BinaryScriptModule.\n");
                RELEASE_ASSERT_NOT_REACHED();
            }
            std::vector<uint8_t> buf;
            if (tsm) {
                buf = readModuleData(&tsm->module)->data;
            } else {
                buf = dsm->data;
            }
            auto trapResult = executeWASM(store, filename, buf, functionTypes);
            if (trapResult.exception == nullptr) {
                printf("Execute WASM returned nullptr (in wabt::CommandType::AssertInvalid case)\n");
                printf("Expected exception:%s\n", assertModuleInvalid->text.data());
                RELEASE_ASSERT_NOT_REACHED();
            }
            std::string& actual = trapResult.exception->message();
            printf("assertModuleInvalid (expect compile error: '%s', actual '%s'(line: %d)) : OK\n", assertModuleInvalid->text.data(), actual.data(), assertModuleInvalid->module->location().line);
            break;
        }
        case wabt::CommandType::AssertMalformed: {
            // we don't need to run invalid wat
            auto* assertMalformed = static_cast<wabt::AssertModuleCommand<wabt::CommandType::AssertMalformed>*>(command.get());
            break;
        }
        case wabt::CommandType::AssertUnlinkable: {
            auto* assertUnlinkable = static_cast<wabt::AssertUnlinkableCommand*>(command.get());
            auto m = assertUnlinkable->module.get();
            auto tsm = dynamic_cast<wabt::TextScriptModule*>(m);
            auto dsm = dynamic_cast<wabt::BinaryScriptModule*>(m);
            if (!tsm && !dsm) {
                printf("Both TextScriptModule* and BinaryScriptModule* castings failed (in wabt::CommandType::AssertUnlinkable case)\n");
                RELEASE_ASSERT_NOT_REACHED();
            }

            std::vector<uint8_t> buf;
            if (tsm) {
                buf = readModuleData(&tsm->module)->data;
            } else {
                buf = dsm->data;
            }
            auto trapResult = executeWASM(store, filename, buf, functionTypes);
            if (trapResult.exception == nullptr) {
                printf("Execute WASM returned nullptr (in wabt::CommandType::AssertUnlinkable case)\n");
                printf("Expected exception:%s\n", assertUnlinkable->text.data());
                RELEASE_ASSERT_NOT_REACHED();
            }
            std::string& actual = trapResult.exception->message();
            printf("assertUnlinkable (expect compile error: '%s', actual '%s'(line: %d)) : OK\n", assertUnlinkable->text.data(), actual.data(), assertUnlinkable->module->location().line);
            break;
        }
        case wabt::CommandType::AssertExhaustion: {
            auto* assertExhaustion = static_cast<wabt::AssertExhaustionCommand*>(command.get());
            auto value = fetchInstance(assertExhaustion->action->module_var, instanceMap, registeredInstanceMap)->resolveExportFunction(assertExhaustion->action->name);
            if (value == nullptr) {
                printf("Fetching instance failed (at wabt::CommandType::AssertExhaustion case)\n");
                RELEASE_ASSERT_NOT_REACHED();
            }
            if (assertExhaustion->action->type() == wabt::ActionType::Invoke) {
                auto action = static_cast<wabt::InvokeAction*>(assertExhaustion->action.get());
                auto fn = fetchInstance(action->module_var, instanceMap, registeredInstanceMap)->resolveExportFunction(action->name);
                executeInvokeAction(action, fn, wabt::ConstVector(), assertExhaustion->text.data());
            } else {
                ASSERT_NOT_REACHED();
            }
            break;
        }
        default: {
            RELEASE_ASSERT_NOT_REACHED();
            break;
        }
        }

        commandCount++;
    }
}

static void runExports(Store* store, const std::string& filename, const std::vector<uint8_t>& src, std::string& exportToRun, DefinedFunctionTypes& functionTypes)
{
    auto parseResult = WASMParser::parseBinary(store, filename, src.data(), src.size(), s_JITFlags);
    if (!parseResult.second.empty()) {
        fprintf(stderr, "parse error: %s\n", parseResult.second.c_str());
        return;
    }

    auto module = parseResult.first;
    const auto& importTypes = module->imports();
    ExternVector importValues;
    importValues.reserve(importTypes.size());

    for (size_t i = 0; i < importTypes.size(); i++) {
#ifdef ENABLE_WASI
        auto import = importTypes[i];
        if (import->moduleName() == "wasi_snapshot_preview1") {
            Walrus::WASI::WasiFuncInfo* wasiImportFunc = WASI::find(import->fieldName());
            if (wasiImportFunc != nullptr) {
                FunctionType* ft = functionTypes[wasiImportFunc->functionType];
                if (ft->equals(import->functionType())) {
                    importValues.push_back(WasiFunction::createWasiFunction(
                        store,
                        ft,
                        wasiImportFunc->ptr));
                }
            }
        } else {
            fprintf(stderr, "error: module has imports, but imports are not supported\n");
            return;
        }
#else
        fprintf(stderr, "error: module has imports, but imports are not supported\n");
        return;
#endif
    }

    struct RunData {
        Module* module;
        ExternVector& importValues;
        std::string* exportToRun;
    } data = { module.value(), importValues, &exportToRun };
    Walrus::Trap trap;

    trap.run([](ExecutionState& state, void* d) {
        auto data = reinterpret_cast<RunData*>(d);
        Instance* instance = data->module->instantiate(state, data->importValues);

        for (auto&& exp : data->module->exports()) {
            if (exp->exportType() == ExportType::Function) {
                if (*data->exportToRun != exp->name() && *data->exportToRun != "*") {
                    continue;
                }

                auto fn = instance->function(exp->itemIndex());
                FunctionType* fnType = fn->asDefinedFunction()->moduleFunction()->functionType();

                if (!fnType->param().empty()) {
                    printf("warning: function %s has params, but params are not supported\n", exp->name().c_str());
                    return;
                }

                Walrus::ValueVector result;
                result.resize(fnType->result().size());
                fn->call(state, nullptr, result.data());

                for (auto&& r : result) {
                    switch (r.type()) {
                    case Value::I32: {
                        printf("%d\n", r.asI32());
                        break;
                    }
                    case Value::I64: {
                        printf("%" PRId64 "\n", r.asI64());
                        break;
                    }
                    case Value::F32: {
                        printf("%.8f\n", r.asF32());
                        break;
                    }
                    case Value::F64: {
                        printf("%.8lf\n", r.asF64());
                        break;
                    }
                    default:
                        printf("(unknown)\n");
                        break;
                    }
                }
            }
        }
    },
             &data);
}

static void parseArguments(int argc, const char* argv[], ParseOptions& options)
{
    for (int i = 1; i < argc; i++) {
        if (strlen(argv[i]) >= 2 && argv[i][0] == '-') { // parse command line option
            if (argv[i][1] == '-') { // `--option` case
                if (strcmp(argv[i], "--run-export") == 0) {
                    if (i + 1 == argc || argv[i + 1][0] == '-') {
                        fprintf(stderr, "error: --run-export requires an argument\n");
                        exit(1);
                    }
                    ++i;
                    options.exportToRun = argv[i];
                    continue;
#if defined(WALRUS_ENABLE_JIT)
                } else if (strcmp(argv[i], "--jit") == 0) {
                    s_JITFlags |= JITFlagValue::useJIT;
                    continue;
                } else if (strcmp(argv[i], "--jit-verbose") == 0) {
                    s_JITFlags |= JITFlagValue::JITVerbose;
                    continue;
                } else if (strcmp(argv[i], "--jit-verbose-color") == 0) {
                    s_JITFlags |= JITFlagValue::JITVerbose | JITFlagValue::JITVerboseColor;
                    continue;
                } else if (strcmp(argv[i], "--jit-no-reg-alloc") == 0) {
                    s_JITFlags |= JITFlagValue::disableRegAlloc;
                    continue;
#endif
                } else if (strcmp(argv[i], "--env") == 0) {
                    if (i + 1 == argc || argv[i + 1][0] == '-') {
                        fprintf(stderr, "error: --env requires an argument\n");
                        exit(1);
                    }
                    ++i;
                    options.wasi_envs.emplace_back(argv[i]);
                    continue;
                } else if (strcmp(argv[i], "--mapdirs") == 0) {
                    if (i + 2 >= argc || argv[i + 1][0] == '-' || argv[i + 2][0] == '-') {
                        fprintf(stderr, "error: --mapdirs requires two arguments\n");
                        exit(1);
                    }
                    // pair of (mapped path, real_path)
                    options.wasi_dirs.push_back(std::make_pair(argv[i + 2], argv[i + 1]));
                    i += 2;
                    continue;
                } else if (strcmp(argv[i], "--args") == 0) {
                    if (i + 1 == argc || argv[i + 1][0] == '-') {
                        fprintf(stderr, "error: --args requires one or more arguments\n");
                        exit(1);
                    }
                    ++i;
                    options.fileNames.emplace_back(argv[i]);
                    options.argsIndex = i;
                    break;
                } else if (strcmp(argv[i], "--help") == 0) {
                    fprintf(stdout, "Usage: walrus [OPTIONS] <INPUT>\n\n");
                    fprintf(stdout, "OPTIONS:\n");
                    fprintf(stdout, "\t--help\n\t\tShow this message then exit.\n\n");
                    fprintf(stdout, "\t--jit\n\t\tEnable just-in-time interpretation.\n\n");
                    fprintf(stdout, "\t--jit-verbose\n\t\tEnable verbose output for just-in-time interpretation.\n\n");
                    fprintf(stdout, "\t--jit-verbose-color\n\t\tEnable colored verbose output for just-in-time interpretation.\n\n");
#ifdef WALRUS_JITPERF
                    fprintf(stdout, "\t--perf\n\t\tEnable JITDump for Perf tool.\n\n");
#endif
                    fprintf(stdout, "\t--mapdirs <HOST_DIR> <VIRTUAL_DIR>\n\t\tMap real directories to virtual ones for WASI functions to use.\n\t\tExample: ./walrus test.wasm --mapdirs this/real/directory/ this/virtual/directory\n\n");
                    fprintf(stdout, "\t--env\n\t\tShare host environment to walrus WASI.\n\n");
                    fprintf(stdout, "\t--args <MODULE_FILE_NAME> [<ARG1> <ARG2> ... <ARGN>]\n\t\tRun Webassembly module with arguments: must be followed by the name of the Webassembly module file, then optionally following arguments which are passed on to the module\n\t\tExample: ./walrus --args test.wasm 'hello' 'world' 42\n\n");
                    exit(0);
                }
            }
            fprintf(stderr, "error: unknown argument: %s\n", argv[i]);
            exit(1);
        } else {
            std::string fileName = argv[i];
            if (endsWith(fileName, "wat") || endsWith(fileName, "wast") || endsWith(fileName, "wasm")) {
                options.fileNames.emplace_back(argv[i]);
            } else {
                fprintf(stderr, "error: unknown argument: %s\n", argv[i]);
                exit(1);
            }
        }
    }

    if (options.fileNames.empty()) {
        fprintf(stderr, "error: no input files\n");
        exit(1);
    }
}

int main(int argc, const char* argv[])
{
#ifndef NDEBUG
    setbuf(stdout, NULL);
    setbuf(stderr, NULL);
#endif

#ifdef M_MMAP_THRESHOLD
    mallopt(M_MMAP_THRESHOLD, 2048);
#endif
#ifdef M_MMAP_MAX
    mallopt(M_MMAP_MAX, 1024 * 1024);
#endif

#if defined(WALRUS_GOOGLE_PERF)
    ProfilerStart("gperf_result");
#endif

    Engine* engine = new Engine();
    Store* store = new Store(engine);

    DefinedFunctionTypes functionTypes;
    ParseOptions options;

    parseArguments(argc, argv, options);


#ifdef ENABLE_WASI
    // initialize WASI
    uvwasi_t uvwasi;

    std::vector<const char*> envp;
    for (auto& s : options.wasi_envs) {
        envp.push_back(s.c_str());
    }
    envp.push_back(nullptr);

    std::vector<uvwasi_preopen_t> dirs;
    for (auto& dir : options.wasi_dirs) {
        dirs.push_back({ dir.first.c_str(), dir.second.c_str() });
    }

    uvwasi_options_t init_options;
    init_options.in = 0;
    init_options.out = 1;
    init_options.err = 2;
    init_options.fd_table_size = 3;
    init_options.argc = (options.argsIndex == -1 ? 0 : argc - options.argsIndex);
    init_options.argv = (options.argsIndex == -1 ? nullptr : argv + options.argsIndex);
    init_options.envp = envp.data();
    init_options.preopenc = dirs.size();
    init_options.preopens = dirs.data();
    init_options.preopen_socketc = 0;
    init_options.allocator = nullptr;

    uvwasi_errno_t err = uvwasi_init(&uvwasi, &init_options);
    assert(err == UVWASI_ESUCCESS);

    WASI::initialize(&uvwasi);
#endif

    for (const auto& filePath : options.fileNames) {
        FILE* fp = fopen(filePath.data(), "r");
        if (fp) {
            fseek(fp, 0, SEEK_END);
            size_t sz = ftell(fp);
            fseek(fp, 0, SEEK_SET);
            std::vector<uint8_t> buf;
            buf.resize(sz);
            int ret = fread(buf.data(), 1, sz, fp);
            fclose(fp);
            // read again with binary mode if result is not success
            // this one needs for windows
            if (UNLIKELY((size_t)ret != sz)) {
                FILE* fp = fopen(filePath.data(), "rb");
                fread(buf.data(), sz, 1, fp);
                fclose(fp);
            }
            if (endsWith(filePath, "wasm")) {
                if (!options.exportToRun.empty()) {
                    runExports(store, filePath, buf, options.exportToRun, functionTypes);
                } else {
                    auto trapResult = executeWASM(store, filePath, buf, functionTypes);
                    if (trapResult.exception) {
                        fprintf(stderr, "Uncaught Exception: %s\n", trapResult.exception->message().data());
                        return -1;
                    }
                }
            } else if (endsWith(filePath, "wat") || endsWith(filePath, "wast")) {
                executeWAST(store, filePath, buf, functionTypes);
            }
        } else {
            printf("Cannot open file %s\n", filePath.data());
            return -1;
        }
    }

    // finalize
    delete store;
    delete engine;

#if defined(WALRUS_GOOGLE_PERF)
    ProfilerStop();
#endif

    return 0;
}
