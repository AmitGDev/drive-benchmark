// SPDX-License-Identifier: MIT
// Copyright (c) 2023-2026, Amit Gefen

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <print>

#include "DriveBenchmark.hpp"

// * Must run as Admin *
int main() {
  try {
    const auto bytes_per_second =
        DriveBenchmark().Check(L"C:", std::chrono::seconds(10));

    std::println(" Volume : C:");
    std::println(" Read (MB/s) : {}",
                 bytes_per_second / DriveBenchmark::kMegaByte);

    return EXIT_SUCCESS;
  } catch (const std::exception& e) {
    // fputs instead of std::println: the error path must not throw again.
    std::fputs(e.what(), stderr);
    std::fputs("\n", stderr);
    return EXIT_FAILURE;
  }
}