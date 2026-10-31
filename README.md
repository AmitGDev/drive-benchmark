# Drive Benchmark

A small C++23 demo that measures the sequential read throughput of a Windows volume.

## What it shows

| Part | Role | Mechanism |
|---|---|---|
| `Check` | Runs the benchmark and returns the average read throughput in bytes per second | `std::async` + `std::this_thread::sleep_for` |
| `OpenVolume` | Opens the raw volume (for example `\\.\C:`) for reading | `CreateFileW` |
| `ReadVolumeUntilStop` | Reads 32 MB blocks in a loop until told to stop, then reports bytes read and elapsed time | `ReadFile` + `std::atomic_bool` stop flag |

The demo benchmarks the `C:` volume for 10 seconds and prints the result in MB/s.

## Expected output

```text
 Volume : C:
 Read (MB/s) : 3200
```

The throughput value depends on the drive, so the number above is only an example.

## Notes

- The program must run as Administrator. Opening a raw volume handle fails otherwise, and `Check` returns 0.
- The volume is opened as a raw device handle. Microsoft file systems open volume handles as non-cached, so the numbers reflect the drive and not the OS file cache.
- Direct volume access only allows reads in multiples of the sector size. The 32 MB buffer is a multiple of every common sector size.
- The reader runs on its own thread via `std::async`. The calling thread sleeps for the requested duration, then sets an atomic flag that the reader checks between reads.
- The reader finishes the `ReadFile` call that is in flight before it sees the flag, so the elapsed time is measured by the reader itself and not assumed to equal the requested duration.
- Elapsed time comes from `GetTickCount64`, truncated to whole seconds. Very short durations are therefore coarse. A zero elapsed time is treated as one second to avoid division by zero.
- On a read failure the reader returns 0 bytes in 1 second, so `Check` reports 0 instead of throwing.
- `DriveBenchmark` shares one stop flag per instance, so call `Check` from one thread at a time on the same object.
- Time complexity is O(d), where d is the benchmark duration. Memory use is O(1) apart from the single 32 MB read buffer.

## Requirements

Windows 10 or later and a C++23-compatible compiler (MSVC from Visual Studio 2022 or later). Administrator rights are required at run time.

## Build and Run

- [Build on Windows](https://github.com/AmitGDev/cpp-ci-template#build-on-windows)

## Code Style

- [C++](https://github.com/AmitGDev/cpp-coding-standard)
- [Python](https://github.com/AmitGDev/cpp-ci-template#python)
- [CMake](https://github.com/AmitGDev/cpp-ci-template#cmake)

## License

MIT License. Copyright © 2023-2026 Amit Gefen.

---

**Last Updated:** 2026-10-31  
**Maintainer:** AmitGDev
