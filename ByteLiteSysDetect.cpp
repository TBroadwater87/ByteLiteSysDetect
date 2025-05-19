#include "ByteLiteSysDetect.h"

#include <iostream>
#include <thread>
#include <cstring>
#ifdef _WIN32
#include <windows.h>
#include <intrin.h>
#else
#include <cpuid.h>
#include <sys/sysinfo.h>
#endif

namespace ByteLiteSysDetect {

namespace {

std::string GetCPUName() {
    char brand[0x40] = {0};
#ifdef _WIN32
    int regs[4] = {0};
    __cpuid(regs, 0x80000000);
    unsigned int maxExt = regs[0];
    if (maxExt >= 0x80000002) {
        __cpuid(reinterpret_cast<int*>(regs), 0x80000002);
        memcpy(brand, regs, sizeof(regs));
        __cpuid(reinterpret_cast<int*>(regs), 0x80000003);
        memcpy(brand + 16, regs, sizeof(regs));
        __cpuid(reinterpret_cast<int*>(regs), 0x80000004);
        memcpy(brand + 32, regs, sizeof(regs));
    }
#else
    unsigned int regs[4] = {0};
    __cpuid(0x80000000, regs[0], regs[1], regs[2], regs[3]);
    unsigned int maxExt = regs[0];
    if (maxExt >= 0x80000002) {
        __cpuid(0x80000002, regs[0], regs[1], regs[2], regs[3]);
        memcpy(brand, regs, sizeof(regs));
        __cpuid(0x80000003, regs[0], regs[1], regs[2], regs[3]);
        memcpy(brand + 16, regs, sizeof(regs));
        __cpuid(0x80000004, regs[0], regs[1], regs[2], regs[3]);
        memcpy(brand + 32, regs, sizeof(regs));
    }
#endif
    return std::string(brand);
}

bool CheckSSE2() {
#ifdef _WIN32
    int regs[4] = {0};
    __cpuid(regs, 1);
    return (regs[3] & (1 << 26)) != 0;
#else
    unsigned int eax, ebx, ecx, edx;
    __cpuid(1, eax, ebx, ecx, edx);
    return (edx & (1u << 26)) != 0;
#endif
}

bool CheckAVX2() {
#ifdef _WIN32
    int regs[4] = {0};
    __cpuidex(regs, 7, 0);
    return (regs[1] & (1 << 5)) != 0;
#else
    unsigned int eax, ebx, ecx, edx;
    __cpuid_count(7, 0, eax, ebx, ecx, edx);
    return (ebx & (1u << 5)) != 0;
#endif
}

std::uint64_t GetTotalRAM() {
#ifdef _WIN32
    MEMORYSTATUSEX status{};
    status.dwLength = sizeof(status);
    if (GlobalMemoryStatusEx(&status)) {
        return static_cast<std::uint64_t>(status.ullTotalPhys);
    }
    return 0;
#else
    struct sysinfo info{};
    if (sysinfo(&info) == 0) {
        return static_cast<std::uint64_t>(info.totalram) * static_cast<std::uint64_t>(info.mem_unit);
    }
    return 0;
#endif
}

} // namespace

SystemInfo DetectSystem() {
    SystemInfo info{};
    info.cpuName = GetCPUName();
    info.coreCount = static_cast<int>(std::thread::hardware_concurrency());
    info.hasSSE2 = CheckSSE2();
    info.hasAVX2 = CheckAVX2();
    info.totalRAMBytes = GetTotalRAM();
    return info;
}

void RunByteLiteSysDetectTest() {
    SystemInfo info = DetectSystem();
    std::cout << "CPU: " << info.cpuName << '\n';
    std::cout << "Cores: " << info.coreCount << '\n';
    std::cout << "SSE2: " << (info.hasSSE2 ? "1" : "0") << '\n';
    std::cout << "AVX2: " << (info.hasAVX2 ? "1" : "0") << '\n';
    std::cout << "RAM Bytes: " << info.totalRAMBytes << '\n';
}

} // namespace ByteLiteSysDetect

