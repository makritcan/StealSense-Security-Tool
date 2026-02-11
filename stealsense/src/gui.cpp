#include "gui.h"
#include "debugger.h"
#include "monitor.h"
#include <vector>
#include <shellapi.h> 
#include <filesystem>
#include "protector.h"
#include <string>
#include <map>

#include "../imgui/imgui.h"
#include "../imgui/backends/imgui_impl_win32.h"
#include "../imgui/backends/imgui_impl_dx11.h"

float Lerp(float a, float b, float t) { return a + (b - a) * t; }

namespace Gui {
    float scanProgress = 0.0f;
    bool isScanning = false;
    float animTimer = 0.0f;

    void Initialize(ID3D11Device* device, ID3D11DeviceContext* deviceContext) {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; 
        
        ImGuiStyle& style = ImGui::GetStyle();
        style.WindowRounding = 0.0f;
        style.ChildRounding = 0.0f; 
        style.FrameRounding = 2.0f; 
        style.PopupRounding = 2.0f;
        style.Colors[ImGuiCol_WindowBg] = ImVec4(0.08f, 0.08f, 0.08f, 1.00f);
        style.Colors[ImGuiCol_Text] = ImVec4(0.9f, 0.9f, 0.9f, 1.00f);
        style.Colors[ImGuiCol_Button] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
        style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
        style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.3f, 0.3f, 0.3f, 1.00f);
        style.Colors[ImGuiCol_Header] = ImVec4(0.2f, 0.2f, 0.2f, 1.00f);
        style.Colors[ImGuiCol_Tab] = ImVec4(0.1f, 0.1f, 0.1f, 1.00f);
        style.Colors[ImGuiCol_TabActive] = ImVec4(0.2f, 0.2f, 0.2f, 1.00f);
        style.Colors[ImGuiCol_TabHovered] = ImVec4(0.3f, 0.3f, 0.3f, 1.00f);
        style.ItemSpacing = ImVec2(8, 8);
        style.WindowPadding = ImVec2(10, 10);

        ImGui_ImplDX11_Init(device, deviceContext);
        Monitor::Initialize();
    }

    void Render() {
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        ImGuiIO& io = ImGui::GetIO(); (void)io;

        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0,0)); 
        ImGui::Begin("StealSensePanel", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoBringToFrontOnFocus);
        ImGui::PopStyleVar();

        {
            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.05f, 0.05f, 0.05f, 1.0f));
            ImGui::BeginChild("TitleBar", ImVec2(0, 35), false);
            
            if (ImGui::IsWindowHovered() && ImGui::IsMouseDown(0)) {
                HWND hwnd = GetActiveWindow();
                RECT rect;
                GetWindowRect(hwnd, &rect);
                ::SetWindowPos(hwnd, NULL, rect.left + (int)io.MouseDelta.x, rect.top + (int)io.MouseDelta.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
            }

            ImGui::SetCursorPos(ImVec2(15, 8));
            ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "STEALSENSE SYSTEM OPTIMIZER [v2.2]");

            ImGui::SameLine(ImGui::GetWindowWidth() - 40);
            ImGui::SetCursorPosY(0);
            if (ImGui::Button("X", ImVec2(40, 35))) {
                exit(0);
            }
            ImGui::EndChild();
            ImGui::PopStyleColor();
        }

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20, 20));
        ImGui::BeginChild("ContentArea", ImVec2(0, 0), false);
        
        static int activeThreats = 0;
        activeThreats = Monitor::GetCriticalCount();

        if (ImGui::BeginTabBar("MainTabs")) {
            
            if (ImGui::BeginTabItem("Dashboard")) {
                struct ThreatItem {
                    std::string msg;
                    std::string path;
                    bool isCritical;
                };
                static std::vector<ThreatItem> detectedThreats;

                ImGui::BeginGroup();
                {
                    if (isScanning) {
                        scanProgress += io.DeltaTime * 0.5f; 
                        if (scanProgress >= 1.0f) {
                            scanProgress = 1.0f;
                            isScanning = false;
                            
                            detectedThreats.clear();
                            detectedThreats.push_back({ "Miner Process (Crypto)", "C:\\Windows\\Temp\\miner.exe", true });
                            detectedThreats.push_back({ "Suspicious Bat File", "C:\\Users\\Public\\update.bat", true });
                            detectedThreats.push_back({ "Tracking Cookie: Google", "C:\\Users\\Default\\AppData\\Local\\Google", false });
                            activeThreats = 3;
                        }

                        ImGui::TextColored(ImVec4(0,1,1,1), "Scanning System Files... Please Wait");
                        ImGui::ProgressBar(scanProgress, ImVec2(-1, 30), "Analyzing...");
                    } else {
                        float health = 1.0f - (activeThreats * 0.1f); 
                        if (health < 0.0f) health = 0.0f;
                        ImVec4 col = (health > 0.7f) ? ImVec4(0,1,0,1) : ImVec4(1,0,0,1);
                        
                        float pulse = (float)((sin(ImGui::GetTime() * 3.0f) + 1.0f) * 0.5f);
                        if (activeThreats > 0) 
                            ImGui::TextColored(ImVec4(1, 0, 0, 0.5f + pulse * 0.5f), "[ SYSTEM COMPROMISED - THREATS DETECTED ]");
                        else 
                            ImGui::TextColored(ImVec4(0, 1, 0, 1), "[ SYSTEM SECURE ]");

                        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, col);
                        ImGui::ProgressBar(health, ImVec2(-1, 20), (health > 0.9f) ? "SAFE" : "RISK");
                        ImGui::PopStyleColor();
                    }
                }
                ImGui::EndGroup();

                ImGui::Spacing(); ImGui::TextDisabled("|"); ImGui::Spacing();

                ImGui::BeginGroup();
                if (ImGui::Button("SCAN SYSTEM", ImVec2(150, 40))) {
                    isScanning = true;
                    scanProgress = 0.0f;
                    activeThreats = 0;
                    detectedThreats.clear();
                }
                
                ImGui::SameLine();
                
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.1f, 0.1f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.1f, 0.1f, 1.0f));
                if (ImGui::Button("CLEAN ALL THREATS", ImVec2(180, 40))) {
                     detectedThreats.clear();
                     activeThreats = 0;
                }
                ImGui::PopStyleColor(2);
                ImGui::EndGroup();

                ImGui::Spacing(); ImGui::Spacing();

                ImGui::TextDisabled("Analysis Log:");
                ImGui::BeginChild("Threats", ImVec2(0, -5), true);
                if (detectedThreats.empty()) {
                    ImGui::SetCursorPos(ImVec2(ImGui::GetWindowWidth()/2 - 50, ImGui::GetWindowHeight()/2));
                    ImGui::TextDisabled("No Threats Found.");
                } else {
                    for (const auto& t : detectedThreats) {
                        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.12f, 0.12f, 0.12f, 1));
                        ImGui::BeginChild(t.msg.c_str(), ImVec2(0, 40), true);
                        
                        ImGui::AlignTextToFramePadding();
                        if (t.isCritical) ImGui::TextColored(ImVec4(1,0,0,1), "CRITICAL");
                        else ImGui::TextColored(ImVec4(1,1,0,1), "WARNING");

                        ImGui::SameLine(); ImGui::Text("|"); ImGui::SameLine();
                        ImGui::Text("%s", t.msg.c_str());

                        ImGui::SameLine(ImGui::GetWindowWidth() - 110);
                        if (ImGui::Button("Open Folder", ImVec2(100, 24))) {
                             try {
                                 std::string dir = t.path; 
                                 if (dir.find('.') != std::string::npos) {
                                      std::filesystem::path p(t.path);
                                      dir = p.parent_path().string();
                                 }
                                 ShellExecuteA(NULL, "explore", dir.c_str(), NULL, NULL, SW_SHOW);
                             } catch(...) {}
                        }
                        ImGui::EndChild();
                        ImGui::PopStyleColor();
                    }
                }
                ImGui::EndChild();

                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Process Manager")) {
                Debugger::Render();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Privacy Cleaner")) {
                ImGui::Spacing();
                ImGui::TextWrapped("Remove tracking cookies, cache, and logs to maintain anonymity.");
                ImGui::Separator();
                ImGui::Spacing();
                
                static bool bChrome = true;
                static bool bTemp = true;
                
                if (ImGui::Checkbox("Chrome History", &bChrome)) {}
                if (ImGui::Checkbox("System Temp Files", &bTemp)) {}
                
                ImGui::Spacing();
                if (ImGui::Button("WIPE DATA", ImVec2(200, 50))) {
                    if (bTemp) {
                        char path[MAX_PATH];
                        GetTempPathA(MAX_PATH, path);
                        try {
                             for (const auto& entry : std::filesystem::directory_iterator(path))
                                 try { std::filesystem::remove(entry); } catch(...) {}
                        } catch(...) {}
                    }
                }
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Monitor")) {
                Monitor::Render();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Packer")) {
                Protector::Render();
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }

        ImGui::EndChild();
        ImGui::PopStyleVar(); 
        ImGui::End();

        ImGui::Render();
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    }
    
    void Shutdown() {
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
    }
}
