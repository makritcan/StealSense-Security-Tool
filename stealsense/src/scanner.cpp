#include "scanner.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <vector>
#include <string>
#include <thread>
#include <future>
#include <atomic>
#include <iostream>
#include "../imgui/imgui.h"

#pragma comment(lib, "Ws2_32.lib")

namespace Scanner {

    struct Result {
        int port;
        bool isOpen;
    };

    std::vector<Result> scanResults;
    std::string targetIP = "127.0.0.1";
    char ipInput[64] = "127.0.0.1";
    int portStart = 1;
    int portEnd = 1000;
    
    std::atomic<bool> isScanning(false);
    std::atomic<int> currentPort(0);
    std::thread scanThread;
    
    bool CheckPort(const std::string& ip, int port) {
        SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (sock == INVALID_SOCKET) return false;

        u_long iMode = 1;
        ioctlsocket(sock, FIONBIO, &iMode);

        sockaddr_in service;
        service.sin_family = AF_INET;
        inet_pton(AF_INET, ip.c_str(), &service.sin_addr.s_addr);
        service.sin_port = htons(port);

        int res = connect(sock, (SOCKADDR*)&service, sizeof(service));
        
        fd_set myset;
        struct timeval tv;
        if (res != 0) {
            if (WSAGetLastError() != WSAEWOULDBLOCK) {
                closesocket(sock);
                return false;
            }
            FD_ZERO(&myset);
            FD_SET(sock, &myset);
            tv.tv_sec = 0;
            tv.tv_usec = 100000; 
            res = select(sock + 1, NULL, &myset, NULL, &tv);
            if (res > 0) {
                 int optError = 0;
                 int optLen = sizeof(optError);
                 getsockopt(sock, SOL_SOCKET, SO_ERROR, (char*)&optError, &optLen);
                 if (optError == 0) res = 1; else res = 0;
            } else {
                res = 0;
            }
        } else {
            res = 1; 
        }

        closesocket(sock);
        return (res == 1);
    }
    
    void Worker(std::string ip, int start, int end) {
        scanResults.clear();
        for (int p = start; p <= end; ++p) {
            if (!isScanning) break;
            currentPort = p;
            if (CheckPort(ip, p)) {
                scanResults.push_back({ p, true });
            }
            if (p % 50 == 0) std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        isScanning = false;
    }

    void Initialize() {
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);
    }

    void Render() {
        ImGui::Text("StealSense Port Scanner");
        ImGui::Separator();
        
        ImGui::InputText("Target IP", ipInput, IM_ARRAYSIZE(ipInput));
        ImGui::PushItemWidth(100);
        ImGui::InputInt("Start Port", &portStart);
        ImGui::SameLine();
        ImGui::InputInt("End Port", &portEnd);
        ImGui::PopItemWidth();

        ImGui::Spacing();

        if (isScanning) {
            if (ImGui::Button("STOP SCAN", ImVec2(-1, 40))) {
                isScanning = false;
                if (scanThread.joinable()) scanThread.join();
            }
            float progress = 0.0f;
            if (portEnd > portStart) 
                progress = (float)(currentPort - portStart) / (float)(portEnd - portStart);
            
            char buf[32];
            sprintf_s(buf, "Scanning %d...", (int)currentPort);
            ImGui::ProgressBar(progress, ImVec2(-1, 0), buf);
        } else {
            if (ImGui::Button("START SCAN", ImVec2(-1, 40))) {
                targetIP = std::string(ipInput);
                isScanning = true;
                if (scanThread.joinable()) scanThread.join();
                scanThread = std::thread(Worker, targetIP, portStart, portEnd);
            }
        }

        ImGui::Separator();
        ImGui::Text("Results (%d open ports found):", scanResults.size());
        
        if (ImGui::BeginTable("ScanTable", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_ScrollY, ImVec2(0, 200))) {
            ImGui::TableSetupColumn("Port");
            ImGui::TableSetupColumn("Status");
            ImGui::TableHeadersRow();

            for (const auto& res : scanResults) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("%d", res.port);
                ImGui::TableSetColumnIndex(1);
                ImGui::TextColored(ImVec4(0,1,0,1), "OPEN");
            }
            ImGui::EndTable();
        }
    }
}
