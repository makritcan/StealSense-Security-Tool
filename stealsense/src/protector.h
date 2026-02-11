#pragma once
#include <string>

namespace Protector {
    bool ProtectExecutable(const std::string& inputPath, const std::string& outputPath);
    void Render();
}
