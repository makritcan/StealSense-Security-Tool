#include "debugger.h"
#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <vector>
#include <iostream>
#include "../imgui/imgui.h"

#pragma comment(lib, "Psapi.lib")

namespace Debugger {

    std::vector<ProcessInfo> processList;
    char searchBuffer[128] = "";

    void RefreshProcessList() {
        processList.clear();

        HANDLE hProcessSnap;
        PROCESSENTRY32 pe32;

        hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hProcessSnap == INVALID_HANDLE_VALUE) {
            return;
        }

        pe32.dwSize = sizeof(PROCESSENTRY32);

        if (!Process32First(hProcessSnap, &pe32)) {
            CloseHandle(hProcessSnap);
            return;
        }

        do {
            ProcessInfo info;
            info.pid = pe32.th32ProcessID;
            
            #ifdef UNICODE
                char mbName[MAX_PATH];
                WideCharToMultiByte(CP_ACP, 0, pe32.szExeFile, -1, mbName, MAX_PATH, NULL, NULL);
                info.name = std::string(mbName);
            #else
                info.name = std::string(pe32.szExeFile);
            #endif
            
            info.workingSetSize = 0;

            HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pe32.th32ProcessID);
            if (hProcess) {
                PROCESS_MEMORY_COUNTERS pmc;
                if (GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc))) {
                    info.workingSetSize = pmc.WorkingSetSize;
                }
                CloseHandle(hProcess);
            }

            processList.push_back(info);

        } while (Process32Next(hProcessSnap, &pe32));

        CloseHandle(hProcessSnap);
    }

    const std::vector<ProcessInfo>& GetProcesses() {
        return processList;
    }

    void Render() {
        if (ImGui::Button("Refresh Process List")) {
            RefreshProcessList();
        }
        ImGui::SameLine();
        ImGui::InputText("Search", searchBuffer, sizeof(searchBuffer));

        if (processList.empty()) {
             static bool firstRun = true;
             if (firstRun) { RefreshProcessList(); firstRun = false; }
        }

        if (ImGui::BeginTable("ProcessTable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY)) {
            
            ImGui::TableSetupColumn("PID", ImGuiTableColumnFlags_WidthFixed, 50.0f);
            ImGui::TableSetupColumn("Name");
            ImGui::TableSetupColumn("Memory (KB)");
            ImGui::TableHeadersRow();

            for (const auto& proc : processList) {
                if (searchBuffer[0] != '\0' && proc.name.find(searchBuffer) == std::string::npos) {
                    continue;
                }

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("%lu", proc.pid);
                
                ImGui::TableSetColumnIndex(1);
                std::string uniqueLabel = proc.name + "##" + std::to_string(proc.pid);
                if (ImGui::Selectable(uniqueLabel.c_str(), false, ImGuiSelectableFlags_SpanAllColumns)) {
                }
 
                if (ImGui::BeginPopupContextItem()) {
                    if (ImGui::MenuItem("Terminate Process")) {
                        HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, proc.pid);
                        if (hProcess) {
                            TerminateProcess(hProcess, 1);
                            CloseHandle(hProcess);
                        }
                    }
                    ImGui::EndPopup();
                }

                ImGui::TableSetColumnIndex(2);
                ImGui::Text("%.2f KB", proc.workingSetSize / 1024.0f);
            }
            ImGui::EndTable();
        }
    }
}
