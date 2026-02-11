#include "monitor.h"
#include <windows.h>
#include <vector>
#include <string>
#include <chrono>
#include <thread>
#include <atomic>
#include <mutex>
#include <iostream>
#include "../imgui/imgui.h"

namespace Monitor {

    struct LogEntry {
        std::string message;
        std::string time;
        int type;
    };

    std::vector<LogEntry> eventLog;
    std::mutex logMutex;
    DWORD lastClipboardSeq = 0;
    
    std::thread watcherThread;
    std::atomic<bool> isRunning(false);

    std::string GetTimeStr() {
        auto now = std::chrono::system_clock::now();
        std::time_t now_c = std::chrono::system_clock::to_time_t(now);
        char buf[100];
        ctime_s(buf, sizeof(buf), &now_c);
        std::string s(buf);
        if (!s.empty() && s.back() == '\n') s.pop_back();
        std::string timeOnly(s.substr(11, 8));
        return timeOnly;
    }

    void LogEvent(const std::string& msg, int type) {
        std::lock_guard<std::mutex> lock(logMutex);
        eventLog.push_back({ msg, GetTimeStr(), type });
        if (eventLog.size() > 1000) {
            eventLog.erase(eventLog.begin());
        }
    }

    void WatchDirectory(std::string path) {
        HANDLE hDir = CreateFileA(
            path.c_str(),
            FILE_LIST_DIRECTORY,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            NULL,
            OPEN_EXISTING,
            FILE_FLAG_BACKUP_SEMANTICS,
            NULL
        );

        if (hDir == INVALID_HANDLE_VALUE) {
            return;
        }

        char buffer[2048];
        DWORD bytesReturned;

        while (isRunning) {
            if (ReadDirectoryChangesW(
                hDir,
                &buffer,
                sizeof(buffer),
                TRUE,
                FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE,
                &bytesReturned,
                NULL,
                NULL
            )) {
                FILE_NOTIFY_INFORMATION* pNotify = (FILE_NOTIFY_INFORMATION*)buffer;
                do {
                    std::wstring wFileName(pNotify->FileName, pNotify->FileNameLength / sizeof(WCHAR));
                    std::string fileName(wFileName.begin(), wFileName.end()); 
                    
                    std::string action = "Unknown";
                    int type = 0;
                    switch (pNotify->Action) {
                        case FILE_ACTION_ADDED: action = "Created: "; type = 1; break;
                        case FILE_ACTION_REMOVED: action = "Deleted: "; type = 1; break;
                        case FILE_ACTION_MODIFIED: action = "Modified: "; type = 0; break;
                        case FILE_ACTION_RENAMED_OLD_NAME: action = "Renamed From: "; type = 0; break;
                        case FILE_ACTION_RENAMED_NEW_NAME: action = "Renamed To: "; type = 0; break;
                    }
                    
                    LogEvent(action + fileName, type);

                    if (pNotify->NextEntryOffset == 0) break;
                    pNotify = (FILE_NOTIFY_INFORMATION*)((char*)pNotify + pNotify->NextEntryOffset);
                } while(true);
            } else {
                 std::this_thread::sleep_for(std::chrono::milliseconds(500));
            }
        }
        CloseHandle(hDir);
    }

    void Initialize() {
        lastClipboardSeq = GetClipboardSequenceNumber();
        LogEvent("Security Monitor Initialized.", 0);
        
        char tempPath[MAX_PATH];
        GetTempPathA(MAX_PATH, tempPath);
        std::string watchPath(tempPath);

        LogEvent("Watching: " + watchPath, 0);

        isRunning = true;
        watcherThread = std::thread(WatchDirectory, watchPath);
    }
    
    void Shutdown() {
        isRunning = false;
        if (watcherThread.joinable()) watcherThread.detach(); 
    }

    void CheckForClipboardChanges() {
        DWORD currentSeq = GetClipboardSequenceNumber();
        if (currentSeq != lastClipboardSeq) {
            lastClipboardSeq = currentSeq;
            
            std::string content = "Clipboard Modified";
            if (OpenClipboard(NULL)) {
                HANDLE hData = GetClipboardData(CF_TEXT);
                if (hData) {
                    char* pszText = static_cast<char*>(GlobalLock(hData));
                    if (pszText) {
                        std::string text(pszText);
                        if (text.length() > 50) text = text.substr(0, 47) + "...";
                        content += " (" + text + ")";
                        GlobalUnlock(hData);
                    }
                }
                CloseClipboard();
            }
            LogEvent(content, 2); 
        }
    }

    int GetCriticalCount() {
        int count = 0;
        std::lock_guard<std::mutex> lock(logMutex);
        for (const auto& ev : eventLog) {
            if (ev.type >= 1) count++; 
        }
        return count;
    }

    void Render() {
        CheckForClipboardChanges();

        if (ImGui::Button("Clear Logs")) {
            std::lock_guard<std::mutex> lock(logMutex);
            eventLog.clear();
        }
        ImGui::SameLine();
        ImGui::TextDisabled("(Watching Temp Folder & Clipboard)");

        ImGui::Separator();

        float footer_height_to_reserve = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing(); 
        ImGui::BeginChild("LogRegion", ImVec2(0, -footer_height_to_reserve), true, ImGuiWindowFlags_HorizontalScrollbar);
        
        {
            std::lock_guard<std::mutex> lock(logMutex);
            for (const auto& ev : eventLog) {
                ImVec4 color;
                if (ev.type == 0) color = ImVec4(0.7f, 0.7f, 0.7f, 1.0f); 
                else if (ev.type == 1) color = ImVec4(1.0f, 0.8f, 0.4f, 1.0f); 
                else color = ImVec4(1.0f, 0.4f, 0.4f, 1.0f); 

                ImGui::TextColored(color, "[%s] %s", ev.time.c_str(), ev.message.c_str());
            }
            
            if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
                ImGui::SetScrollHereY(1.0f);
        }
            
        ImGui::EndChild();
    }
}
