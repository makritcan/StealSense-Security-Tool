#pragma once
#include <string>

namespace Obfuscator {
    std::string ObfuscateString(const std::string& input, const std::string& varName);
    std::string GenerateJunkCode(int lines);
    void Render();
}
