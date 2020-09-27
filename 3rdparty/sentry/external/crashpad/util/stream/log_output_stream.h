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

#ifndef CRASHPAD_UTIL_STREAM_LOG_OUTPUT_STREAM_H_
#define CRASHPAD_UTIL_STREAM_LOG_OUTPUT_STREAM_H_

#include <stddef.h>
#include <stdint.h>

#include <memory>
#include <string>

#include "base/macros.h"
#include "util/stream/output_stream_interface.h"

namespace crashpad {

//! \brief This class output the received data to Android log, NOP in other
//! platform.
//!
//! To avoid overflowing Android log, total 128k log data is allowed, after
//! that cap, the output is aborted.
class LogOutputStream : public OutputStreamInterface {
 public:
  LogOutputStream();
  ~LogOutputStream() override;

  // OutputStreamInterface:
  bool Write(const uint8_t* data, size_t size) override;
  bool Flush() override;

  void SetOutputStreamForTesting(std::unique_ptr<OutputStreamInterface> stream);

 private:
  // Flush the |buffer_|, return false if kOutputCap meet.
  bool WriteBuffer();
  bool WriteToLog(const char* buf);

  std::string buffer_;
  size_t output_count_;
  bool flush_needed_;
  bool flushed_;
  std::unique_ptr<OutputStreamInterface> output_stream_for_testing_;

  DISALLOW_COPY_AND_ASSIGN(LogOutputStream);
};

}  // namespace crashpad

#endif  // CRASHPAD_UTIL_STREAM_LOG_OUTPUT_STREAM_H_
