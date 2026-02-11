#pragma once
#include <vector>
#include <string>

namespace Scanner {
    struct ScanResult {
        int port;
        bool isOpen;
        std::string service;
    };

    void Initialize();
    void ScanPort(const std::string& ip, int port);
    void ScanRange(const std::string& ip, int startPort, int endPort);
    
    void Render();
    const std::vector<ScanResult>& GetResults();
    bool IsScanning();
}
