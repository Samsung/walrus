/*
 * Copyright 2018 WebAssembly Community Group participants
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

#include "wabt/binary-reader-nop.h"
#include "wabt/binary-reader.h"
#include "wabt/binary.h"
#include "wabt/error-formatter.h"
#include "wabt/leb128.h"
#include "wabt/option-parser.h"
#include "wabt/stream.h"

using namespace wabt;

static std::string s_filename;
static std::string s_outfile;

static const char s_description[] =
    R"(  Remove sections of a WebAssembly binary file.

examples:
  # Remove all custom sections from test.wasm
  $ wasm-strip test.wasm
)";

static void ParseOptions(int argc, char** argv) {
  OptionParser parser("wasm-strip", s_description);

  parser.AddArgument("filename", OptionParser::ArgumentCount::One,
                     [](const char* argument) {
                       s_filename = argument;
                       ConvertBackslashToSlash(&s_filename);
                     });
  parser.AddOption('o', "output", "FILE", "output wasm binary file",
                   [](const char* argument) { s_outfile = argument; });
  parser.Parse(argc, argv);
}

class BinaryReaderStrip : public BinaryReaderNop {
 public:
  explicit BinaryReaderStrip(Errors* errors) : errors_(errors) {
    stream_.WriteU32(WABT_BINARY_MAGIC, "WASM_BINARY_MAGIC");
    stream_.WriteU32(WABT_BINARY_VERSION, "WASM_BINARY_VERSION");
  }

  bool OnError(const Error& error) override {
    errors_->push_back(error);
    return true;
  }

  Result BeginSection(Index section_index,
                      BinarySection section_type,
                      Offset size) override {
    if (section_type == BinarySection::Custom) {
      return Result::Ok;
    }
    stream_.WriteU8Enum(section_type, "section code");
    WriteU32Leb128(&stream_, size, "section size");
    stream_.WriteData(state->data + state->offset, size, "section data");
    return Result::Ok;
  }

  Result WriteToFile(std::string_view filename) {
    return stream_.WriteToFile(filename);
  }

 private:
  MemoryStream stream_;
  Errors* errors_;
};

int ProgramMain(int argc, char** argv) {
  Result result;

  InitStdio();
  ParseOptions(argc, argv);

  std::vector<uint8_t> file_data;
  result = ReadFile(s_filename.c_str(), &file_data);
  if (Failed(result)) {
    return Result::Error;
  }

  Errors errors;
  Features features;
  features.EnableAll();
  const bool kReadDebugNames = false;
  const bool kStopOnFirstError = true;
  const bool kFailOnCustomSectionError = false;
  ReadBinaryOptions options(features, nullptr, kReadDebugNames,
                            kStopOnFirstError, kFailOnCustomSectionError);

  BinaryReaderStrip reader(&errors);
  result = ReadBinary(file_data.data(), file_data.size(), &reader, options);
  FormatErrorsToFile(errors, Location::Type::Binary);
  if (Failed(result)) {
    return Result::Error;
  }

  if (s_outfile.empty()) {
    s_outfile = s_filename;
  }
  return reader.WriteToFile(s_outfile);
}

int main(int argc, char** argv) {
  WABT_TRY
  return ProgramMain(argc, argv);
  WABT_CATCH_BAD_ALLOC_AND_EXIT
}
