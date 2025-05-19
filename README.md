# ByteLiteSysDetect

ByteLiteSysDetect is a static library for advanced local system detection across platforms.

It gathers extended system information, including:

- CPU name and core/thread counts
- RAM total (in MB)
- Operating System version and architecture
- SIMD feature flags (e.g., AVX2, SSE4.2, etc.)
- Drive performance (read/write speed detection)
- GPU name and bitness

This library exposes:

- A `SystemProfile` struct containing all collected metrics
- A `RunSystemDetectionTest()` diagnostic function that prints a clean, ASCII-only report for logging/debugging

### Build Info

- Builds as a `.lib` on Windows and `.a` on other platforms
- Written in C++20
