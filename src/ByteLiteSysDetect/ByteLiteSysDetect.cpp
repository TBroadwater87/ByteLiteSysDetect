#include "ByteLiteSysDetect.h"

#include <iostream>
#include <thread>
#include <vector>
#include <cstring>
#include <chrono>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#include <intrin.h>
#include <VersionHelpers.h>
#include <dxgi1_4.h>
#pragma comment(lib, "dxgi.lib")
#else
#include <cpuid.h>
#include <sys/utsname.h>
#include <unistd.h>
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

int GetLogicalThreads() {
#ifdef _WIN32
    return static_cast<int>(GetActiveProcessorCount(ALL_PROCESSOR_GROUPS));
#else
    return static_cast<int>(std::thread::hardware_concurrency());
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
        if (count > 0) {
            return count;
        }
    }
    return GetLogicalThreads();
#else
    return static_cast<int>(std::thread::hardware_concurrency());
#endif
}

std::uint64_t GetRAMMB() {
#ifdef _WIN32
    MEMORYSTATUSEX status{};
    status.dwLength = sizeof(status);
    if (GlobalMemoryStatusEx(&status)) {
        return static_cast<std::uint64_t>(status.ullTotalPhys / (1024ull * 1024ull));
    }
    return 0;
#else
    long pages = sysconf(_SC_PHYS_PAGES);
    long page_size = sysconf(_SC_PAGE_SIZE);
    if (pages > 0 && page_size > 0) {
        return static_cast<std::uint64_t>(pages) * static_cast<std::uint64_t>(page_size) / (1024ull * 1024ull);
    }
    return 0;
#endif
}

void GetOSInfo(std::string& name, std::string& version) {
#ifdef _WIN32
    OSVERSIONINFOEXA info{};
    info.dwOSVersionInfoSize = sizeof(info);
    if (GetVersionExA(reinterpret_cast<OSVERSIONINFOA*>(&info))) {
        name = "Windows";
        version = std::to_string(info.dwMajorVersion) + "." + std::to_string(info.dwMinorVersion);
    }
#else
    struct utsname uts{};
    if (uname(&uts) == 0) {
        name = uts.sysname;
        version = uts.release;
    }
#endif
}

void GetSIMDFlags(bool& mmx, bool& sse, bool& avx, bool& avx2, bool& avx512) {
#ifdef _WIN32
    int regs[4] = {0};
    __cpuid(regs, 1);
    mmx = (regs[3] & (1 << 23)) != 0;
    sse = (regs[3] & (1 << 25)) != 0;
    avx = (regs[2] & (1 << 28)) != 0;
    __cpuidex(regs, 7, 0);
    avx2 = (regs[1] & (1 << 5)) != 0;
    avx512 = (regs[1] & (1 << 16)) != 0;
#else
    unsigned int eax, ebx, ecx, edx;
    __cpuid(1, eax, ebx, ecx, edx);
    mmx = (edx & (1u << 23)) != 0;
    sse = (edx & (1u << 25)) != 0;
    avx = (ecx & (1u << 28)) != 0;
    __cpuid_count(7, 0, eax, ebx, ecx, edx);
    avx2 = (ebx & (1u << 5)) != 0;
    avx512 = (ebx & (1u << 16)) != 0;
#endif
}

std::string GetGPUName() {
#ifdef _WIN32
    std::string name = "";
    IDXGIFactory1* factory = nullptr;
    if (SUCCEEDED(CreateDXGIFactory1(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(&factory)))) {
        IDXGIAdapter1* adapter = nullptr;
        if (factory->EnumAdapters1(0, &adapter) != DXGI_ERROR_NOT_FOUND) {
            DXGI_ADAPTER_DESC1 desc;
            if (SUCCEEDED(adapter->GetDesc1(&desc))) {
                char buffer[128];
                std::wcstombs(buffer, desc.Description, sizeof(buffer));
                name = buffer;
            }
            adapter->Release();
        }
        factory->Release();
    }
    return name;
#else
    return "";
#endif
}

std::string GetFastestDrive(double& mbps) {
    mbps = 0.0;
#ifdef _WIN32
    char drives[256] = {0};
    DWORD len = GetLogicalDriveStringsA(sizeof(drives) - 1, drives);
    std::string fastest;
    LARGE_INTEGER freq{};
    QueryPerformanceFrequency(&freq);
    for (DWORD i = 0; i < len; ) {
        char* drive = &drives[i];
        if (GetDriveTypeA(drive) == DRIVE_FIXED) {
            std::string file = std::string(drive) + "bytelite_test.tmp";
            HANDLE h = CreateFileA(file.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE, nullptr);
            if (h != INVALID_HANDLE_VALUE) {
                const size_t bytes = 64 * 1024;
                std::vector<char> buf(bytes, 0);
                LARGE_INTEGER s{}, e{};
                QueryPerformanceCounter(&s);
                DWORD written = 0;
                WriteFile(h, buf.data(), static_cast<DWORD>(buf.size()), &written, nullptr);
                FlushFileBuffers(h);
                QueryPerformanceCounter(&e);
                CloseHandle(h);
                double elapsed = static_cast<double>(e.QuadPart - s.QuadPart) / freq.QuadPart;
                double speed = (written / (1024.0 * 1024.0)) / (elapsed > 0 ? elapsed : 1e-6);
                if (speed > mbps) {
                    mbps = speed;
                    fastest = std::string(1, drive[0]);
                }
            }
        }
        i += static_cast<DWORD>(std::strlen(drive)) + 1;
    }
    return fastest;
#else
    return "";
#endif
}

bool Is64Bit() {
    return sizeof(void*) == 8;
}

} // namespace

SystemInfo DetectSystem() {
    SystemInfo profile{};
    profile.cpuName = GetCPUName();
    profile.logicalThreads = GetLogicalThreads();
    profile.coreCount = profile.logicalThreads;
    profile.physicalCores = GetPhysicalCores();
    profile.totalRAMBytes = GetRAMMB() * 1024 * 1024; // convert MB to bytes
    GetOSInfo(profile.osName, profile.osVersion);
    GetSIMDFlags(profile.hasMMX, profile.hasSSE, profile.hasAVX, profile.hasAVX2, profile.hasAVX512);
    profile.fastestDrive = GetFastestDrive(profile.driveWriteMBps).empty() ? 'C' : GetFastestDrive(profile.driveWriteMBps)[0];
    profile.gpuName = GetGPUName();
    profile.is64Bit = Is64Bit();
    return profile;
}

void RunByteLiteSysDetectTest() {
    SystemInfo p = DetectSystem();
    std::cout << "CPU: " << p.cpuName << '\n';
    std::cout << "Physical Cores: " << p.physicalCores << '\n';
    std::cout << "Logical Threads: " << p.logicalThreads << '\n';
    std::cout << "RAM (Bytes): " << p.totalRAMBytes << '\n';
    std::cout << "OS: " << p.osName << ' ' << p.osVersion << '\n';
    std::cout << "MMX: " << (p.hasMMX ? "1" : "0") << '\n';
    std::cout << "SSE: " << (p.hasSSE ? "1" : "0") << '\n';
    std::cout << "AVX: " << (p.hasAVX ? "1" : "0") << '\n';
    std::cout << "AVX2: " << (p.hasAVX2 ? "1" : "0") << '\n';
    std::cout << "AVX-512: " << (p.hasAVX512 ? "1" : "0") << '\n';
    std::cout << "Fastest Drive: " << p.fastestDrive << " (" << p.driveWriteMBps << " MB/s)" << '\n';
    std::cout << "GPU: " << p.gpuName << '\n';
    std::cout << "64-bit: " << (p.is64Bit ? "1" : "0") << '\n';
}

} // namespace ByteLiteSysDetect
