#pragma once
#include <vector>
#include <string>
#include <windows.h>

struct ProcessInfo {
    DWORD pid;
    std::string name;
    SIZE_T workingSetSize;
};

namespace Debugger {
    void RefreshProcessList();
    const std::vector<ProcessInfo>& GetProcesses();
    void Render();
}
