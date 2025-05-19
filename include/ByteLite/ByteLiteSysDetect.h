#ifndef BYTELITE_SYS_DETECT_H
#define BYTELITE_SYS_DETECT_H

#include <string>
#include <cstdint>

namespace ByteLite {

struct SystemProfile {
    std::string cpuName;
    uint32_t physicalCores = 0;
    uint32_t logicalThreads = 0;
    uint64_t ramMB = 0;
    std::string osName;
    std::string osVersion;
    bool hasMMX = false;
    bool hasSSE = false;
    bool hasAVX = false;
    bool hasAVX2 = false;
    bool hasAVX512 = false;
    char fastestDriveLetter = '\0';
    double fastestDriveWriteMBps = 0.0;
    std::string gpuName;
    bool is64Bit = false;
};

SystemProfile DetectSystem();
void RunSystemDetectionTest();

} // namespace ByteLite

#endif // BYTELITE_SYS_DETECT_H
