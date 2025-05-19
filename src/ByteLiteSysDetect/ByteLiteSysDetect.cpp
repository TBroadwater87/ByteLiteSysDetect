#include "ByteLite/ByteLiteSysDetect.h"

#include <iostream>
#include <vector>
#include <chrono>
#include <thread>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#include <intrin.h>
#include <VersionHelpers.h>
#include <dxgi.h>
#pragma comment(lib, "dxgi.lib")
#endif

namespace ByteLite {

namespace {
#ifdef _WIN32
std::string GetCPUName() {
    int info[4] = {0};
    __cpuid(info, 0x80000002);
    char name[49] = {};
    memcpy(name, info, 16);
    __cpuid(info, 0x80000003);
    memcpy(name + 16, info, 16);
    __cpuid(info, 0x80000004);
    memcpy(name + 32, info, 16);
    return std::string(name);
}

void GetCoreCounts(uint32_t& physical, uint32_t& logical) {
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    logical = sysInfo.dwNumberOfProcessors;
    DWORD len = 0;
    GetLogicalProcessorInformation(nullptr, &len);
    std::vector<SYSTEM_LOGICAL_PROCESSOR_INFORMATION> buffer(len / sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION));
    if (GetLogicalProcessorInformation(buffer.data(), &len)) {
        for (auto& info : buffer) {
            if (info.Relationship == RelationProcessorCore)
                ++physical;
        }
    }
}

uint64_t GetRAMMB() {
    MEMORYSTATUSEX state{};
    state.dwLength = sizeof(state);
    GlobalMemoryStatusEx(&state);
    return static_cast<uint64_t>(state.ullTotalPhys / (1024 * 1024));
}

std::string GetOSName() {
    return "Windows";
}

std::string GetOSVersion() {
    OSVERSIONINFOEXW osvi{};
    osvi.dwOSVersionInfoSize = sizeof(osvi);
    if (GetVersionExW(reinterpret_cast<OSVERSIONINFOW*>(&osvi))) {
        return std::to_string(osvi.dwMajorVersion) + "." + std::to_string(osvi.dwMinorVersion);
    }
    return "Unknown";
}

void CheckSIMD(SystemProfile& profile) {
    int info[4];
    __cpuid(info, 1);
    profile.hasMMX = (info[3] & (1 << 23)) != 0;
    profile.hasSSE = (info[3] & (1 << 25)) != 0;
    profile.hasAVX = (info[2] & (1 << 28)) != 0;

    __cpuid(info, 7);
    profile.hasAVX2 = (info[1] & (1 << 5)) != 0;
    profile.hasAVX512 = (info[1] & (1 << 16)) != 0;
}

std::string GetGPUName() {
    IDXGIFactory* factory = nullptr;
    if (CreateDXGIFactory(__uuidof(IDXGIFactory), reinterpret_cast<void**>(&factory)) == S_OK) {
        IDXGIAdapter* adapter = nullptr;
        if (factory->EnumAdapters(0, &adapter) == S_OK) {
            DXGI_ADAPTER_DESC desc;
            if (adapter->GetDesc(&desc) == S_OK) {
                std::wstring ws(desc.Description);
                factory->Release();
                adapter->Release();
                return std::string(ws.begin(), ws.end());
            }
            adapter->Release();
        }
        factory->Release();
    }
    return "Unknown";
}

bool TestDriveWrite(char letter, double& mbps) {
    char path[] = "A:\\bytelite_test.dat";
    path[0] = letter;
    HANDLE file = CreateFileA(path, GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_DELETE_ON_CLOSE, nullptr);
    if (file == INVALID_HANDLE_VALUE)
        return false;
    const size_t size = 10 * 1024 * 1024;
    std::vector<char> buffer(size, '0');
    LARGE_INTEGER freq, start, end;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&start);
    DWORD written = 0;
    WriteFile(file, buffer.data(), static_cast<DWORD>(buffer.size()), &written, nullptr);
    FlushFileBuffers(file);
    QueryPerformanceCounter(&end);
    CloseHandle(file);
    if (written != buffer.size())
        return false;
    double seconds = static_cast<double>(end.QuadPart - start.QuadPart) / static_cast<double>(freq.QuadPart);
    mbps = (written / (1024.0 * 1024.0)) / seconds;
    return true;
}

void FindFastestDrive(char& letter, double& speed) {
    DWORD mask = GetLogicalDrives();
    for (char c = 'A'; c <= 'Z'; ++c) {
        if (!(mask & (1 << (c - 'A'))))
            continue;
        UINT type = GetDriveTypeA((std::string(1, c) + ":\\").c_str());
        if (type != DRIVE_FIXED && type != DRIVE_REMOVABLE)
            continue;
        double mbps = 0.0;
        if (TestDriveWrite(c, mbps)) {
            if (mbps > speed) {
                speed = mbps;
                letter = c;
            }
        }
    }
}
#endif // _WIN32
}

SystemProfile DetectSystem() {
    SystemProfile profile;
#ifdef _WIN32
    profile.cpuName = GetCPUName();
    GetCoreCounts(profile.physicalCores, profile.logicalThreads);
    profile.ramMB = GetRAMMB();
    profile.osName = GetOSName();
    profile.osVersion = GetOSVersion();
    CheckSIMD(profile);
    profile.gpuName = GetGPUName();
    profile.is64Bit = sizeof(void*) == 8;
    FindFastestDrive(profile.fastestDriveLetter, profile.fastestDriveWriteMBps);
#endif
    return profile;
}

void RunSystemDetectionTest() {
    SystemProfile p = DetectSystem();
    std::cout << "CPU: " << p.cpuName << '\n';
    std::cout << "Physical Cores: " << p.physicalCores << '\n';
    std::cout << "Logical Threads: " << p.logicalThreads << '\n';
    std::cout << "RAM MB: " << p.ramMB << '\n';
    std::cout << "OS: " << p.osName << " " << p.osVersion << '\n';
    std::cout << "MMX: " << (p.hasMMX ? "1" : "0") << '\n';
    std::cout << "SSE: " << (p.hasSSE ? "1" : "0") << '\n';
    std::cout << "AVX: " << (p.hasAVX ? "1" : "0") << '\n';
    std::cout << "AVX2: " << (p.hasAVX2 ? "1" : "0") << '\n';
    std::cout << "AVX512: " << (p.hasAVX512 ? "1" : "0") << '\n';
    if (p.fastestDriveLetter) {
        std::cout << "Fastest Drive: " << p.fastestDriveLetter << " ";
        std::cout << p.fastestDriveWriteMBps << " MB/s" << '\n';
    }
    std::cout << "GPU: " << p.gpuName << '\n';
    std::cout << "64-bit: " << (p.is64Bit ? "1" : "0") << '\n';
}

} // namespace ByteLite

