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
};

SystemInfo DetectSystem();

void RunByteLiteSysDetectTest();

} // namespace ByteLiteSysDetect

#endif // BYTELITESYSDETECT_H
