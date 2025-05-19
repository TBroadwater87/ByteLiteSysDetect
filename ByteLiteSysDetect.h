#ifndef BYTELITESYSDETECT_H
#define BYTELITESYSDETECT_H

#include <cstdint>
#include <string>

namespace ByteLiteSysDetect {

struct SystemInfo {
    std::string cpuName;
    int coreCount;
    bool hasSSE2;
    bool hasAVX2;
    std::uint64_t totalRAMBytes;
    int physicalCores;
    int logicalThreads;
    std::string osName;
    std::string osVersion;
    bool hasMMX;
    bool hasSSE;
    bool hasAVX;
    bool hasAVX512;
    char fastestDrive;
    double driveWriteMBps;
    std::string gpuName;
    bool is64Bit;
};

SystemInfo DetectSystem();

void RunByteLiteSysDetectTest();

} // namespace ByteLiteSysDetect

#endif // BYTELITESYSDETECT_H
