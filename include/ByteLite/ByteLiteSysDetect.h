#ifndef BYTELITESYSDETECT_H
#define BYTELITESYSDETECT_H

#include <cstdint>
#include <string>

namespace ByteLiteSysDetect {

struct SystemProfile {
    std::string cpuName;
    int physicalCores;
    int logicalThreads;
    std::uint64_t ramMB;
    std::string osName;
    std::string osVersion;
    bool hasMMX;
    bool hasSSE;
    bool hasAVX;
    bool hasAVX2;
    bool hasAVX512;
    std::string fastestDrive;
    double driveWriteMBps;
    std::string gpuName;
    bool is64bit;
};

SystemProfile DetectSystem();

void RunSystemDetectionTest();

} // namespace ByteLiteSysDetect

#endif // BYTELITESYSDETECT_H
