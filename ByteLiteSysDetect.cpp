#include "ByteLiteSysDetect.h"

#include <iostream>
#include <thread>
#include <cstring>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#include <intrin.h>
#include <versionhelpers.h>
#include <dxgi1_2.h>
#pragma comment(lib, "dxgi.lib")
#else
#include <cpuid.h>
#include <sys/sysinfo.h>
#include <unistd.h>
#include <sys/utsname.h>
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

int GetPhysicalCores() {
#ifdef _WIN32
    DWORD len = 0;
    GetLogicalProcessorInformation(nullptr, &len);
    std::vector<SYSTEM_LOGICAL_PROCESSOR_INFORMATION> buffer(len / sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION));
    if (GetLogicalProcessorInformation(buffer.data(), &len)) {
        int count = 0;
        for (const auto& info : buffer) {
            if (info.Relationship == RelationProcessorCore) {
                ++count;
            }
        }
        return count;
    }
    return 0;
#else
    long cores = sysconf(_SC_NPROCESSORS_ONLN);
    return cores > 0 ? static_cast<int>(cores) : 0;
#endif
}

std::string GetOSName() {
#ifdef _WIN32
    return "Windows";
#else
    struct utsname uts{};
    if (uname(&uts) == 0) {
        return std::string(uts.sysname);
    }
    return "Unknown";
#endif
}

std::string GetOSVersion() {
#ifdef _WIN32
    OSVERSIONINFOEXA osvi{};
    osvi.dwOSVersionInfoSize = sizeof(osvi);
    if (GetVersionExA(reinterpret_cast<OSVERSIONINFOA*>(&osvi))) {
        return std::to_string(osvi.dwMajorVersion) + "." + std::to_string(osvi.dwMinorVersion);
    }
    return "0";
#else
    struct utsname uts{};
    if (uname(&uts) == 0) {
        return std::string(uts.release);
    }
    return "0";
#endif
}

bool CheckMMX() {
#ifdef _WIN32
    int regs[4] = {0};
    __cpuid(regs, 1);
    return (regs[3] & (1 << 23)) != 0;
#else
    unsigned int eax, ebx, ecx, edx;
    __cpuid(1, eax, ebx, ecx, edx);
    return (edx & (1u << 23)) != 0;
#endif
}

bool CheckSSE() {
#ifdef _WIN32
    int regs[4] = {0};
    __cpuid(regs, 1);
    return (regs[3] & (1 << 25)) != 0;
#else
    unsigned int eax, ebx, ecx, edx;
    __cpuid(1, eax, ebx, ecx, edx);
    return (edx & (1u << 25)) != 0;
#endif
}

bool CheckAVX() {
#ifdef _WIN32
    int regs[4];
    __cpuid(regs, 1);
    bool osUsesXSAVE_XRSTORE = regs[2] & (1 << 27);
    bool avxSupported = regs[2] & (1 << 28);
    if (osUsesXSAVE_XRSTORE && avxSupported) {
        unsigned long long xcrFeatureMask = _xgetbv(0);
        return (xcrFeatureMask & 0x6) == 0x6;
    }
    return false;
#else
    unsigned int eax, ebx, ecx, edx;
    __cpuid(1, eax, ebx, ecx, edx);
    bool osUsesXSAVE_XRSTORE = (ecx & (1u << 27)) != 0;
    bool avxSupported = (ecx & (1u << 28)) != 0;
    if (osUsesXSAVE_XRSTORE && avxSupported) {
        unsigned int a, d;
        __asm__ __volatile__("xgetbv" : "=a"(a), "=d"(d) : "c"(0));
        return (a & 0x6) == 0x6;
    }
    return false;
#endif
}

bool CheckAVX512() {
#ifdef _WIN32
    int regs[4] = {0};
    __cpuidex(regs, 7, 0);
    return (regs[1] & (1 << 16)) != 0;
#else
    unsigned int eax, ebx, ecx, edx;
    __cpuid_count(7, 0, eax, ebx, ecx, edx);
    return (ebx & (1u << 16)) != 0;
#endif
}

char GetFastestDrive() {
#ifdef _WIN32
    for (char drive = 'C'; drive <= 'Z'; ++drive) {
        char root[] = {drive, ':', '\\', 0};
        if (GetDriveTypeA(root) == DRIVE_FIXED) {
            return drive;
        }
    }
    return 'C';
#else
    return '/';
#endif
}

double MeasureDriveWriteMBps(char drive) {
#ifdef _WIN32
    char path[] = {drive, ':', '\\', 'b', 'l', 't', 0};
    HANDLE h = CreateFileA(path, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY, nullptr);
    if (h == INVALID_HANDLE_VALUE) {
        return 0.0;
    }
    const size_t size = 1 << 20; // 1MB
    std::vector<char> buffer(size, 0);
    LARGE_INTEGER start{}, end{}, freq{};
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&start);
    DWORD written = 0;
    for (int i = 0; i < 64; ++i) {
        WriteFile(h, buffer.data(), static_cast<DWORD>(buffer.size()), &written, nullptr);
    }
    QueryPerformanceCounter(&end);
    CloseHandle(h);
    DeleteFileA(path);
    double seconds = static_cast<double>(end.QuadPart - start.QuadPart) / static_cast<double>(freq.QuadPart);
    return 64.0 / seconds;
#else
    return 0.0;
#endif
}

std::string GetGPUName() {
#ifdef _WIN32
    IDXGIFactory1* factory = nullptr;
    if (CreateDXGIFactory1(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(&factory)) != S_OK) {
        return "Unknown";
    }
    IDXGIAdapter1* adapter = nullptr;
    std::string name = "Unknown";
    if (factory->EnumAdapters1(0, &adapter) == S_OK) {
        DXGI_ADAPTER_DESC1 desc{};
        adapter->GetDesc1(&desc);
        char buf[128] = {0};
        wcstombs(buf, desc.Description, 127);
        name = buf;
        adapter->Release();
    }
    factory->Release();
    return name;
#else
    return "Unknown";
#endif
}

bool Is64Bit() {
    return sizeof(void*) == 8;
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
    info.physicalCores = GetPhysicalCores();
    info.logicalThreads = static_cast<int>(std::thread::hardware_concurrency());
    info.osName = GetOSName();
    info.osVersion = GetOSVersion();
    info.hasMMX = CheckMMX();
    info.hasSSE = CheckSSE();
    info.hasAVX = CheckAVX();
    info.hasAVX512 = CheckAVX512();
    info.fastestDrive = GetFastestDrive();
    info.driveWriteMBps = MeasureDriveWriteMBps(info.fastestDrive);
    info.gpuName = GetGPUName();
    info.is64Bit = Is64Bit();
    return info;
}

void RunByteLiteSysDetectTest() {
    SystemInfo info = DetectSystem();
    std::cout << "CPU: " << info.cpuName << '\n';
    std::cout << "LogicalThreads: " << info.logicalThreads << '\n';
    std::cout << "PhysicalCores: " << info.physicalCores << '\n';
    std::cout << "SSE2: " << (info.hasSSE2 ? "1" : "0") << '\n';
    std::cout << "AVX2: " << (info.hasAVX2 ? "1" : "0") << '\n';
    std::cout << "MMX: " << (info.hasMMX ? "1" : "0") << '\n';
    std::cout << "SSE: " << (info.hasSSE ? "1" : "0") << '\n';
    std::cout << "AVX: " << (info.hasAVX ? "1" : "0") << '\n';
    std::cout << "AVX512: " << (info.hasAVX512 ? "1" : "0") << '\n';
    std::cout << "RAM Bytes: " << info.totalRAMBytes << '\n';
    std::cout << "OS: " << info.osName << ' ' << info.osVersion << '\n';
    std::cout << "Drive: " << info.fastestDrive << ' ' << info.driveWriteMBps << " MBps\n";
    std::cout << "GPU: " << info.gpuName << '\n';
    std::cout << "64Bit: " << (info.is64Bit ? "1" : "0") << '\n';
}

} // namespace ByteLiteSysDetect
