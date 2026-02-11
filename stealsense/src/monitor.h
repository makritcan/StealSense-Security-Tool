#pragma once
#include <vector>
#include <string>

namespace Monitor {
    struct Event {
        std::string message;
        std::string time;
        int type; 
    };

    void Initialize();
    void Shutdown(); 
    void CheckForChanges();
    const std::vector<Event>& GetEvents();
    int GetCriticalCount(); 
    void Render();
}
