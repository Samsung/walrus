#include "wat.h"

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <string>

#include "wabt/wast-lexer.h"
#include "wabt/wast-parser.h"
#include "wabt/binary-writer.h"

// This assembler is linked only into C API test executables.
extern "C" void wasm_test_wat(wasm_byte_vec_t* bytes, const char* name)
{
    std::string path = std::string(WALRUS_API_WAT_DIR) + "/" + name + ".wat";
    std::ifstream input(path);
    if (!input) {
        std::fprintf(stderr, "Cannot open %s\n", path.c_str());
        std::abort();
    }
    std::string text((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
    wabt::Errors errors;
    wabt::Features features;
    features.EnableAll();
    wabt::WastParseOptions options(features);
    std::unique_ptr<wabt::Module> module;
    auto lexer = wabt::WastLexer::CreateBufferLexer(path.c_str(), text.data(), text.size(), &errors);
    if (wabt::ParseWatModule(lexer.get(), &module, &errors, &options) != wabt::Result::Ok) {
        for (const auto& error : errors) {
            std::fprintf(stderr, "%s: %s\n", path.c_str(), error.message.c_str());
        }
        std::abort();
    }
    wabt::MemoryStream stream;
    wabt::WriteBinaryOptions binary;
    binary.features = features;
    if (wabt::WriteBinaryModule(&stream, module.get(), binary) != wabt::Result::Ok) {
        std::fprintf(stderr, "Cannot assemble %s\n", path.c_str());
        std::abort();
    }
    stream.Flush();
    auto output = stream.ReleaseOutputBuffer();
    wasm_byte_vec_new(bytes, output->data.size(), reinterpret_cast<const char*>(output->data.data()));
}
