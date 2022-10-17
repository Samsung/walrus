/*
 * Copyright 2017 WebAssembly Community Group participants
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

#include <cassert>
#include <cinttypes>
#include <cstdio>
#include <cstdlib>

#include "wabt/apply-names.h"
#include "wabt/binary-reader-ir.h"
#include "wabt/binary-reader.h"
#include "wabt/error-formatter.h"
#include "wabt/feature.h"
#include "wabt/filenames.h"
#include "wabt/generate-names.h"
#include "wabt/ir.h"
#include "wabt/option-parser.h"
#include "wabt/stream.h"
#include "wabt/validator.h"
#include "wabt/wast-lexer.h"

#include "wabt/c-writer.h"

using namespace wabt;

static int s_verbose;
static std::string s_infile;
static std::string s_outfile;
static Features s_features;
static WriteCOptions s_write_c_options;
static bool s_read_debug_names = true;
static std::unique_ptr<FileStream> s_log_stream;

static const char s_description[] =
    R"(  Read a file in the WebAssembly binary format, and convert it to
  a C source file and header.

examples:
  # parse binary file test.wasm and write test.c and test.h
  $ wasm2c test.wasm -o test.c

  # parse test.wasm, write test.c and test.h, but ignore the debug names, if any
  $ wasm2c test.wasm --no-debug-names -o test.c
)";

static const std::string supported_features[] = {
    "multi-memory", "multi-value", "sign-extend", "saturating-float-to-int",
    "exceptions"};

static bool IsFeatureSupported(const std::string& feature) {
  return std::find(std::begin(supported_features), std::end(supported_features),
                   feature) != std::end(supported_features);
};

static void ParseOptions(int argc, char** argv) {
  OptionParser parser("wasm2c", s_description);

  parser.AddOption('v', "verbose", "Use multiple times for more info", []() {
    s_verbose++;
    s_log_stream = FileStream::CreateStderr();
  });
  parser.AddOption(
      'o', "output", "FILENAME",
      "Output file for the generated C source file, by default use stdout",
      [](const char* argument) {
        s_outfile = argument;
        ConvertBackslashToSlash(&s_outfile);
      });
  parser.AddOption(
      'n', "module-name", "MODNAME",
      "Unique name for the module being generated. This name is prefixed to\n"
      "each of the generaed C symbols. By default, the module name from the\n"
      "names section is used. If that is not present the name of the input\n"
      "file is used as the default.\n",
      [](const char* argument) { s_write_c_options.module_name = argument; });
  s_features.AddOptions(&parser);
  parser.AddOption("no-debug-names", "Ignore debug names in the binary file",
                   []() { s_read_debug_names = false; });
  parser.AddArgument("filename", OptionParser::ArgumentCount::One,
                     [](const char* argument) {
                       s_infile = argument;
                       ConvertBackslashToSlash(&s_infile);
                     });
  parser.Parse(argc, argv);

  bool any_non_supported_feature = false;
#define WABT_FEATURE(variable, flag, default_, help)   \
  any_non_supported_feature |=                         \
      (s_features.variable##_enabled() != default_) && \
      !IsFeatureSupported(flag);
#include "wabt/feature.def"
#undef WABT_FEATURE

  if (any_non_supported_feature) {
    fprintf(stderr,
            "wasm2c currently only supports a limited set of features.\n");
    exit(1);
  }
}

// TODO(binji): copied from binary-writer-spec.cc, probably should share.
static std::string_view strip_extension(std::string_view s) {
  std::string_view ext = s.substr(s.find_last_of('.'));
  std::string_view result = s;

  if (ext == ".c")
    result.remove_suffix(ext.length());
  return result;
}

int ProgramMain(int argc, char** argv) {
  Result result;

  InitStdio();
  ParseOptions(argc, argv);

  std::vector<uint8_t> file_data;
  result = ReadFile(s_infile.c_str(), &file_data);
  if (Succeeded(result)) {
    Errors errors;
    Module module;
    const bool kStopOnFirstError = true;
    const bool kFailOnCustomSectionError = true;
    ReadBinaryOptions options(s_features, s_log_stream.get(),
                              s_read_debug_names, kStopOnFirstError,
                              kFailOnCustomSectionError);
    result = ReadBinaryIr(s_infile.c_str(), file_data.data(), file_data.size(),
                          options, &errors, &module);
    if (Succeeded(result)) {
      if (Succeeded(result)) {
        ValidateOptions options(s_features);
        result = ValidateModule(&module, &errors, options);
        result |= GenerateNames(&module);
      }

      if (Succeeded(result)) {
        /* TODO(binji): This shouldn't fail; if a name can't be applied
         * (because the index is invalid, say) it should just be skipped. */
        Result dummy_result = ApplyNames(&module);
        WABT_USE(dummy_result);
      }

      if (Succeeded(result)) {
        if (!s_outfile.empty()) {
          std::string header_name_full =
              std::string(strip_extension(s_outfile)) + ".h";
          FileStream c_stream(s_outfile.c_str());
          FileStream h_stream(header_name_full);
          std::string_view header_name = GetBasename(header_name_full);
          if (s_write_c_options.module_name.empty()) {
            s_write_c_options.module_name = module.name;
            if (s_write_c_options.module_name.empty()) {
              // In the absence of module name in the names section use the
              // filename.
              s_write_c_options.module_name =
                  StripExtension(GetBasename(s_infile));
            }
          }
          result =
              WriteC(&c_stream, &h_stream, std::string(header_name).c_str(),
                     &module, s_write_c_options);
        } else {
          FileStream stream(stdout);
          result =
              WriteC(&stream, &stream, "wasm.h", &module, s_write_c_options);
        }
      }
    }
    FormatErrorsToFile(errors, Location::Type::Binary);
  }
  return result != Result::Ok;
}

int main(int argc, char** argv) {
  WABT_TRY
  return ProgramMain(argc, argv);
  WABT_CATCH_BAD_ALLOC_AND_EXIT
}
