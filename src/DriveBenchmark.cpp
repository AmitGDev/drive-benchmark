/*
    DriveBenchmark.cpp
    Copyright (c) 2023-2026, Amit Gefen

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to
    deal in the Software without restriction, including without limitation the
    rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
    sell copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in
    all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
    IN THE SOFTWARE.
*/

#include "DriveBenchmark.hpp"

// NOLINTNEXTLINE(misc-include-cleaner, llvm-include-order)
#include <Windows.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <future>
#include <memory>
#include <string>
#include <thread>
#include <utility>

// Windows.h is the only supported entry point to the Win32 API. The headers
// that actually define these symbols (fileapi.h, handleapi.h, ...) are not
// meant to be included directly, so misc-include-cleaner is silenced for the
// Win32 usages instead of bending the includes to satisfy it.
// NOLINTBEGIN(misc-include-cleaner)

namespace {

// Ties the volume handle's lifetime to a scope so every normal exit path
// closes it. Only wrap handles already checked against INVALID_HANDLE_VALUE:
// unique_ptr treats that value (-1) as a valid, non-null pointer.
struct HandleCloser {
  void operator()(HANDLE handle) const noexcept { CloseHandle(handle); }
};

using UniqueHandle = std::unique_ptr<void, HandleCloser>;

}  // namespace

// Returns the average size in bytes of read operations per second on the
// volume.
std::uint64_t DriveBenchmark::Check(const std::wstring& volume,
                                    const std::chrono::milliseconds duration) {
  uint64_t average_read_throughput_of_the_volume{0};

  // 1) Open Volume & Read async. until stop:
  if (HANDLE raw = OpenVolume(volume); raw != INVALID_HANDLE_VALUE) {
    // Declared before the future on purpose: locals are destroyed in reverse
    // order, so the future joins the reader thread before the handle closes.
    const UniqueHandle handle{raw};

    auto outcome = std::async(std::launch::async, ReadVolumeUntilStop,
                              handle.get(), 32 * kMegaByte, std::ref(stop_));

    // 2) Wait for duration & stop reading:
    std::this_thread::sleep_for(duration);
    stop_ = true;

    // 3) Get the outcome of the async ReadVolumeUntilStop run:
    const auto& [total_bytes_read, elapsed_time_in_seconds] = outcome.get();
    average_read_throughput_of_the_volume =
        total_bytes_read / (elapsed_time_in_seconds > 0
                                ? elapsed_time_in_seconds
                                : 1);  // 1 prevents divide by zero.
    stop_ = false;
  }

  return average_read_throughput_of_the_volume;
}

// It is important to us to read non-cached bytes!
// A volume contains one or more mounted file systems. Volume handles can be
// opened as non-cached at the discretion of the particular file system, even
// when the non-cached option is not specified in CreateFile. You should assume
// that all Microsoft file systems open volume handles as non-cached. For
// example: String "\\.\C:" means: Opens the C: volume.
HANDLE
DriveBenchmark::OpenVolume(const std::wstring& volume_letter) {
  // Opens the device representing the volume. This call fails if the user
  // doesn't have sufficient rights (i.e. not running as Admin).
  HANDLE handle = CreateFileW((L"\\\\.\\" + volume_letter).c_str(),
                              GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                              nullptr, OPEN_EXISTING, 0, nullptr);

  return handle;
}

// It is important to us to read non-cached bytes!
// *For direct access of a disk, you can only read and write multiples of the
// sector size!* The function returns a pair of: total_bytes_read,
// actual_elapsed_time_in_seconds. In case of failure, the function returns a
// pair of: 0, 1 (i.e. 0 bytes in 1 second).
std::pair<std::uint64_t, std::uint64_t>
DriveBenchmark::ReadVolumeUntilStop(HANDLE handle,
                                    const std::uint32_t buffer_size,
                                    const std::atomic_bool& stop) {
  uint64_t total_bytes_read{0};
  const auto buffer = std::make_unique<unsigned char[]>(buffer_size);
  DWORD bytes_read{0};

  const auto start_time_tag = GetTickCount64();

  while (!stop.load()) {
    if (!ReadFile(handle, buffer.get(), buffer_size, &bytes_read, nullptr)) {
      // Error:
      [[maybe_unused]] const DWORD err = GetLastError();  // Just for debug.
      return {0, 1};                                      // 0 bytes per second.
    }

    total_bytes_read += bytes_read;
  }

  return {total_bytes_read, (GetTickCount64() - start_time_tag) / 1000};
}

// NOLINTEND(misc-include-cleaner)
