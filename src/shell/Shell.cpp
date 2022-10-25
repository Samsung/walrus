#include <iostream>
#include <locale>
#include <sstream>
#include <iomanip>
#include <inttypes.h>

#include "Walrus.h"
#include "runtime/Engine.h"
#include "runtime/Store.h"
#include "runtime/Module.h"
#include "runtime/Function.h"
#include "runtime/Instance.h"
#include "runtime/Trap.h"
#include "parser/WASMParser.h"

#include "wabt/wast-lexer.h"
#include "wabt/wast-parser.h"
#include "wabt/binary-writer.h"

struct spectestseps : std::numpunct<char> {
    char do_thousands_sep() const { return '_'; }
    std::string do_grouping() const { return "\3"; }
};

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

static void executeWASM(Store* store, const std::vector<uint8_t>& src, Instance::InstanceVector& instances)
{
    auto module = WASMParser::parseBinary(store, src.data(), src.size());
    const auto& moduleImportData = module->moduleImport();

    ValueVector importValues;
    importValues.resize(moduleImportData.size());
    /*
        (module ;; spectest host module(https://github.com/WebAssembly/spec/tree/main/interpreter)
          (global (export "global_i32") i32) ;; TODO
          (global (export "global_i64") i64) ;; TODO
          (global (export "global_f32") f32) ;; TODO
          (global (export "global_f64") f64) ;; TODO

          (table (export "table") 10 20 funcref) ;; TODO

          (memory (export "memory") 1 2) ;; TODO

          (func (export "print"))
          (func (export "print_i32") (param i32))
          (func (export "print_i64") (param i64))
          (func (export "print_f32") (param f32))
          (func (export "print_f64") (param f64))
          (func (export "print_i32_f32") (param i32 f32))
          (func (export "print_f64_f64") (param f64 f64))
        )
    */

    for (size_t i = 0; i < moduleImportData.size(); i++) {
        auto import = moduleImportData[i];
        if (import->moduleName()->equals("spectest")) {
            if (import->fieldName()->equals("print_i32")) {
                auto ft = module->functionType(import->functionTypeIndex());
                ASSERT(ft->result().size() == 0 && ft->param().size() == 1 && ft->param()[0] == Value::Type::I32);
                importValues[i] = Value(new ImportedFunction(
                    store,
                    ft,
                    [](ExecutionState& state, const uint32_t argc, Value* argv, Value* result, void* data) {
                        printI32(argv[0].asI32());
                    },
                    nullptr));
            } else if (import->fieldName()->equals("print_i64")) {
                auto ft = module->functionType(import->functionTypeIndex());
                ASSERT(ft->result().size() == 0 && ft->param().size() == 1 && ft->param()[0] == Value::Type::I64);
                importValues[i] = Value(new ImportedFunction(
                    store,
                    ft,
                    [](ExecutionState& state, const uint32_t argc, Value* argv, Value* result, void* data) {
                        printI64(argv[0].asI64());
                    },
                    nullptr));
            } else if (import->fieldName()->equals("print_f32")) {
                auto ft = module->functionType(import->functionTypeIndex());
                ASSERT(ft->result().size() == 0 && ft->param().size() == 1 && ft->param()[0] == Value::Type::F32);
                importValues[i] = Value(new ImportedFunction(
                    store,
                    ft,
                    [](ExecutionState& state, const uint32_t argc, Value* argv, Value* result, void* data) {
                        printF32(argv[0].asF32());
                    },
                    nullptr));
            } else if (import->fieldName()->equals("print_f64")) {
                auto ft = module->functionType(import->functionTypeIndex());
                ASSERT(ft->result().size() == 0 && ft->param().size() == 1 && ft->param()[0] == Value::Type::F64);
                importValues[i] = Value(new ImportedFunction(
                    store,
                    ft,
                    [](ExecutionState& state, const uint32_t argc, Value* argv, Value* result, void* data) {
                        printF64(argv[0].asF64());
                    },
                    nullptr));
            } else if (import->fieldName()->equals("print_i32_f32")) {
                auto ft = module->functionType(import->functionTypeIndex());
                ASSERT(ft->result().size() == 0 && ft->param().size() == 2 && ft->param()[0] == Value::Type::I32 && ft->param()[1] == Value::Type::F32);
                importValues[i] = Value(new ImportedFunction(
                    store,
                    ft,
                    [](ExecutionState& state, const uint32_t argc, Value* argv, Value* result, void* data) {
                        printI32(argv[0].asI32());
                        printF32(argv[1].asF32());
                    },
                    nullptr));
            } else if (import->fieldName()->equals("print_f64_f64")) {
                auto ft = module->functionType(import->functionTypeIndex());
                ASSERT(ft->result().size() == 0 && ft->param().size() == 2 && ft->param()[0] == Value::Type::F64 && ft->param()[1] == Value::Type::F64);
                importValues[i] = Value(new ImportedFunction(
                    store,
                    ft,
                    [](ExecutionState& state, const uint32_t argc, Value* argv, Value* result, void* data) {
                        printF64(argv[0].asF64());
                        printF64(argv[1].asF64());
                    },
                    nullptr));
            }
        }
    }

    instances.pushBack(module->instantiate(importValues));
}

static bool endsWith(const std::string& str, const std::string& suffix)
{
    return str.size() >= suffix.size() && 0 == str.compare(str.size() - suffix.size(), suffix.size(), suffix);
}

static Walrus::Value toWalrusValue(wabt::Const& c)
{
    if (c.type() == wabt::Type::I32) {
        return Walrus::Value(static_cast<int32_t>(c.u32()));
    } else if (c.type() == wabt::Type::I64) {
        return Walrus::Value(static_cast<int64_t>(c.u64()));
    } else if (c.type() == wabt::Type::F32) {
        if (c.is_expected_nan(0)) {
            return Walrus::Value(std::numeric_limits<float>::quiet_NaN());
        }
        float s;
        auto bits = c.f32_bits();
        memcpy(&s, &bits, sizeof(float));
        return Walrus::Value(s);
    } else if (c.type() == wabt::Type::F64) {
        if (c.is_expected_nan(0)) {
            return Walrus::Value(std::numeric_limits<double>::quiet_NaN());
        }
        double s;
        auto bits = c.f64_bits();
        memcpy(&s, &bits, sizeof(double));
        return Walrus::Value(s);
    } else {
        RELEASE_ASSERT_NOT_REACHED();
    }
}

static void printConstVector(wabt::ConstVector& v)
{
    for (size_t i = 0; i < v.size(); i++) {
        auto c = v[i];
        if (c.type() == wabt::Type::I32) {
            printf("%" PRIu32, c.u32());
        } else if (c.type() == wabt::Type::I64) {
            printf("%" PRIu64, c.u64());
        } else if (c.type() == wabt::Type::F32) {
            if (c.is_expected_nan(0)) {
                printf("nan");
                return;
            }
            float s;
            auto bits = c.f32_bits();
            memcpy(&s, &bits, sizeof(float));
            printf("%f", s);
        } else if (c.type() == wabt::Type::F64) {
            if (c.is_expected_nan(0)) {
                printf("nan");
                return;
            }
            double s;
            auto bits = c.f64_bits();
            memcpy(&s, &bits, sizeof(double));
            printf("%lf", s);
        } else {
            RELEASE_ASSERT_NOT_REACHED();
        }
        if (i + 1 != v.size()) {
            printf(", ");
        }
    }
}

static void executeInvokeAction(wabt::InvokeAction* action, Walrus::Function* fn, wabt::ConstVector expectedResult, const char* expectedException)
{
    RELEASE_ASSERT(fn->functionType()->param().size() == action->args.size());
    Walrus::ValueVector args;
    for (auto& a : action->args) {
        args.pushBack(toWalrusValue(a));
    }

    struct RunData {
        Walrus::Function* fn;
        wabt::ConstVector& expectedResult;
        Walrus::ValueVector& args;
    } data = { fn, expectedResult, args };
    Walrus::Trap trap;
    auto trapResult = trap.run([](Walrus::ExecutionState& state, void* d) {
        RunData* data = reinterpret_cast<RunData*>(d);
        Walrus::ValueVector result;
        result.resize(data->expectedResult.size());
        data->fn->call(state, data->args.size(), data->args.data(), result.data());
        RELEASE_ASSERT(data->fn->functionType()->result().size() == data->expectedResult.size());
        // compare result
        for (size_t i = 0; i < result.size(); i++) {
            RELEASE_ASSERT(result[i] == toWalrusValue(data->expectedResult[i]));
        }
    },
                               &data);
    if (expectedResult.size()) {
        RELEASE_ASSERT(!trapResult.exception);
    }
    if (expectedException) {
        RELEASE_ASSERT(trapResult.exception);
        RELEASE_ASSERT(trapResult.exception->message()->equals(expectedException, strlen(expectedException)));
        printf("invoke %s(", action->name.data());
        printConstVector(action->args);
        printf("), expect exception: %s (line: %d) : OK\n", expectedException, action->loc.line);
    } else {
        printf("invoke %s(", action->name.data());
        printConstVector(action->args);
        printf(") expect value(");
        printConstVector(expectedResult);
        printf(") (line: %d) : OK\n", action->loc.line);
    }
}

static void executeWAST(Store* store, const std::vector<uint8_t>& src, Instance::InstanceVector& instances)
{
    auto lexer = wabt::WastLexer::CreateBufferLexer("test.wabt", src.data(), src.size());
    if (!lexer) {
        return;
    }

    wabt::Errors errors;
    std::unique_ptr<wabt::Script> script;
    wabt::Features features;
    features.EnableAll();
    wabt::WastParseOptions parse_wast_options(features);
    auto result = wabt::ParseWastScript(lexer.get(), &script, &errors, &parse_wast_options);
    if (!wabt::Succeeded(result)) {
        return;
    }

    for (const std::unique_ptr<wabt::Command>& command : script->commands) {
        if (auto* moduleCommand = dynamic_cast<wabt::ModuleCommand*>(command.get())) {
            auto module = &moduleCommand->module;
            wabt::MemoryStream stream;
            wabt::WriteBinaryOptions options;
            options.features = features;
            wabt::WriteBinaryModule(&stream, module, options);
            stream.Flush();
            auto buf = stream.ReleaseOutputBuffer();
            executeWASM(store, buf->data, instances);
        } else if (auto* assertReturn = dynamic_cast<wabt::AssertReturnCommand*>(command.get())) {
            auto value = instances[assertReturn->action->module_var.index()]->resolveExport(new Walrus::String(assertReturn->action->name));
            if (assertReturn->action->type() == wabt::ActionType::Invoke) {
                auto action = dynamic_cast<wabt::InvokeAction*>(assertReturn->action.get());
                RELEASE_ASSERT(value.type() == Walrus::Value::FuncRef);
                auto fn = instances[action->module_var.index()]->resolveExport(new Walrus::String(action->name)).asFunction();
                executeInvokeAction(action, fn, assertReturn->expected, nullptr);
            }
        } else if (auto* assertTrap = dynamic_cast<wabt::AssertTrapCommand*>(command.get())) {
            auto value = instances[assertTrap->action->module_var.index()]->resolveExport(new Walrus::String(assertTrap->action->name));
            if (assertTrap->action->type() == wabt::ActionType::Invoke) {
                auto action = dynamic_cast<wabt::InvokeAction*>(assertTrap->action.get());
                RELEASE_ASSERT(value.type() == Walrus::Value::FuncRef);
                auto fn = instances[action->module_var.index()]->resolveExport(new Walrus::String(action->name)).asFunction();
                executeInvokeAction(action, fn, wabt::ConstVector(), assertTrap->text.data());
            }
        }
    }
}

int main(int argc, char* argv[])
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

    GC_INIT();

    Engine* engine = new Engine;
    Store* store = new Store(engine);

    Instance::InstanceVector instances;

    for (int i = 1; i < argc; i++) {
        std::string filePath = argv[i];
        FILE* fp = fopen(filePath.data(), "r");
        if (fp) {
            fseek(fp, 0, SEEK_END);
            auto sz = ftell(fp);
            fseek(fp, 0, SEEK_SET);
            std::vector<uint8_t> buf;
            buf.resize(sz);
            fread(buf.data(), sz, 1, fp);
            fclose(fp);

            if (endsWith(filePath, "wasm")) {
                executeWASM(store, buf, instances);
            } else if (endsWith(filePath, "wat") || endsWith(filePath, "wast")) {
                executeWAST(store, buf, instances);
            }
        } else {
            printf("Cannot open file %s\n", argv[i]);
            return -1;
        }
    }

    return 0;
}
