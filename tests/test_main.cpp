#include "ByteLiteSysDetect.h"
#include <cassert>
#include <iostream>

int main() {
    ByteLiteSysDetect::SystemInfo info = ByteLiteSysDetect::DetectSystem();
    assert(!info.cpuName.empty());
    assert(info.coreCount > 0);
    assert(info.totalRAMBytes > 0);
    std::cout << "CPU=" << info.cpuName << '\n';
    std::cout << "Cores=" << info.coreCount << '\n';
    std::cout << "SSE2=" << (info.hasSSE2 ? "1" : "0") << '\n';
    std::cout << "AVX2=" << (info.hasAVX2 ? "1" : "0") << '\n';
    std::cout << "RAM=" << info.totalRAMBytes << '\n';
    return 0;
}
