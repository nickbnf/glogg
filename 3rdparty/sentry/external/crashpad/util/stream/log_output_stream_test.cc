// Copyright 2019 The Crashpad Authors. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "util/stream/log_output_stream.h"

#include <algorithm>
#include <memory>
#include <string>

#include "base/macros.h"
#include "gtest/gtest.h"
#include "util/stream/test_output_stream.h"

namespace crashpad {
namespace test {
namespace {

constexpr size_t kOutputCap = 128 * 1024;
constexpr size_t kLineBufferSize = 512;
const char* kBeginGuard = "-----BEGIN CRASHPAD MINIDUMP-----";
const char* kEndGuard = "-----END CRASHPAD MINIDUMP-----";
const char* kAbortGuard = "-----ABORT CRASHPAD MINIDUMP-----";

std::string ConvertToString(const std::vector<uint8_t>& src) {
  return std::string(reinterpret_cast<const char*>(src.data()), src.size());
}

class LogOutputStreamTest : public testing::Test {
 public:
  LogOutputStreamTest() : test_output_stream_(nullptr) {}

 protected:
  void SetUp() override {
    std::unique_ptr<TestOutputStream> output_stream =
        std::make_unique<TestOutputStream>();
    test_output_stream_ = output_stream.get();
    log_stream_ = std::make_unique<LogOutputStream>();
    log_stream_->SetOutputStreamForTesting(std::move(output_stream));
  }

  const uint8_t* BuildDeterministicInput(size_t size) {
    deterministic_input_ = std::make_unique<uint8_t[]>(size);
    uint8_t* deterministic_input_base = deterministic_input_.get();
    while (size-- > 0)
      deterministic_input_base[size] = static_cast<uint8_t>('a');
    return deterministic_input_base;
  }

  TestOutputStream* test_output_stream() const { return test_output_stream_; }

  LogOutputStream* log_stream() const { return log_stream_.get(); }

 private:
  std::unique_ptr<LogOutputStream> log_stream_;
  TestOutputStream* test_output_stream_;
  std::unique_ptr<uint8_t[]> deterministic_input_;

  DISALLOW_COPY_AND_ASSIGN(LogOutputStreamTest);
};

TEST_F(LogOutputStreamTest, VerifyGuards) {
  log_stream()->Flush();
  // Verify OutputStream wrote 2 guards.
  EXPECT_EQ(test_output_stream()->write_count(), 2u);
  EXPECT_EQ(test_output_stream()->flush_count(), 1u);
  EXPECT_FALSE(test_output_stream()->all_data().empty());
  EXPECT_EQ(ConvertToString(test_output_stream()->all_data()),
            std::string(kBeginGuard).append(kEndGuard));
}

TEST_F(LogOutputStreamTest, WriteShortLog) {
  const uint8_t* input = BuildDeterministicInput(2);
  EXPECT_TRUE(log_stream()->Write(input, 2));
  EXPECT_TRUE(log_stream()->Flush());
  // Verify OutputStream wrote 2 guards and data.
  EXPECT_EQ(test_output_stream()->write_count(), 3u);
  EXPECT_EQ(test_output_stream()->flush_count(), 1u);
  EXPECT_FALSE(test_output_stream()->all_data().empty());
  EXPECT_EQ(ConvertToString(test_output_stream()->all_data()),
            std::string(kBeginGuard).append("aa").append(kEndGuard));
}

TEST_F(LogOutputStreamTest, WriteLongLog) {
  size_t input_length = kLineBufferSize + kLineBufferSize / 2;
  const uint8_t* input = BuildDeterministicInput(input_length);
  // Verify OutputStream wrote 2 guards and data.
  EXPECT_TRUE(log_stream()->Write(input, input_length));
  EXPECT_TRUE(log_stream()->Flush());
  EXPECT_EQ(test_output_stream()->write_count(),
            2 + input_length / kLineBufferSize + 1);
  EXPECT_EQ(test_output_stream()->flush_count(), 1u);
  EXPECT_EQ(test_output_stream()->all_data().size(),
            strlen(kBeginGuard) + strlen(kEndGuard) + input_length);
}

TEST_F(LogOutputStreamTest, WriteAbort) {
  size_t input_length = kOutputCap + kLineBufferSize;
  const uint8_t* input = BuildDeterministicInput(input_length);
  EXPECT_FALSE(log_stream()->Write(input, input_length));
  std::string data(ConvertToString(test_output_stream()->all_data()));
  EXPECT_EQ(data.substr(data.size() - strlen(kAbortGuard)), kAbortGuard);
}

TEST_F(LogOutputStreamTest, FlushAbort) {
  size_t input_length = kOutputCap + kLineBufferSize / 2;
  const uint8_t* input = BuildDeterministicInput(input_length);
  EXPECT_TRUE(log_stream()->Write(input, input_length));
  EXPECT_FALSE(log_stream()->Flush());
  std::string data(ConvertToString(test_output_stream()->all_data()));
  EXPECT_EQ(data.substr(data.size() - strlen(kAbortGuard)), kAbortGuard);
}

}  // namespace
}  // namespace test
}  // namespace crashpad
