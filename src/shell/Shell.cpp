#include <iostream>
#include <locale>
#include <sstream>
#include <iomanip>

#include "Walrus.h"
#include "runtime/Module.h"
#include "runtime/Function.h"
#include "parser/WASMParser.h"

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

    for (int i = 1; i < argc; i++) {
        FILE* fp = fopen(argv[i], "r");
        if (fp) {
            fseek(fp, 0, SEEK_END);
            auto sz = ftell(fp);
            fseek(fp, 0, SEEK_SET);
            uint8_t* buf = new uint8_t[sz];
            fread(buf, sz, 1, fp);
            fclose(fp);
            auto module = WASMParser::parseBinary(buf, sz);
            delete[] buf;
            const auto& moduleImportData = module->import();

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
                            ft,
                            [](ExecutionState& state, const uint32_t argc, Value* argv, Value* result, void* data) {
                                printI32(argv[0].asI32());
                            },
                            nullptr));
                    } else if (import->fieldName()->equals("print_i64")) {
                        auto ft = module->functionType(import->functionTypeIndex());
                        ASSERT(ft->result().size() == 0 && ft->param().size() == 1 && ft->param()[0] == Value::Type::I64);
                        importValues[i] = Value(new ImportedFunction(
                            ft,
                            [](ExecutionState& state, const uint32_t argc, Value* argv, Value* result, void* data) {
                                printI64(argv[0].asI64());
                            },
                            nullptr));
                    } else if (import->fieldName()->equals("print_f32")) {
                        auto ft = module->functionType(import->functionTypeIndex());
                        ASSERT(ft->result().size() == 0 && ft->param().size() == 1 && ft->param()[0] == Value::Type::F32);
                        importValues[i] = Value(new ImportedFunction(
                            ft,
                            [](ExecutionState& state, const uint32_t argc, Value* argv, Value* result, void* data) {
                                printF32(argv[0].asF32());
                            },
                            nullptr));
                    } else if (import->fieldName()->equals("print_f64")) {
                        auto ft = module->functionType(import->functionTypeIndex());
                        ASSERT(ft->result().size() == 0 && ft->param().size() == 1 && ft->param()[0] == Value::Type::F64);
                        importValues[i] = Value(new ImportedFunction(
                            ft,
                            [](ExecutionState& state, const uint32_t argc, Value* argv, Value* result, void* data) {
                                printF64(argv[0].asF64());
                            },
                            nullptr));
                    } else if (import->fieldName()->equals("print_i32_f32")) {
                        auto ft = module->functionType(import->functionTypeIndex());
                        ASSERT(ft->result().size() == 0 && ft->param().size() == 2 && ft->param()[0] == Value::Type::I32 && ft->param()[1] == Value::Type::F32);
                        importValues[i] = Value(new ImportedFunction(
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
                            ft,
                            [](ExecutionState& state, const uint32_t argc, Value* argv, Value* result, void* data) {
                                printF64(argv[0].asF64());
                                printF64(argv[1].asF64());
                            },
                            nullptr));
                    }
                }
            }

            module->instantiate(importValues);
        } else {
            printf("Cannot open file %s\n", argv[i]);
            return -1;
        }
    }

    return 0;
}
