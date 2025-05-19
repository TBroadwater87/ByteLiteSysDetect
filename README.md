# ByteLiteSysDetect
Utility For Local System Detection

This static library provides simple CPU and memory capability detection.
It exposes a straightforward C++ API and a test function that prints
results to `std::cout`.

## Running Tests

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build
```

