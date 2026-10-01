#include "runtime.hpp"
#include "notification_manager.hpp"
#include <ctime>
namespace fw {
    static Mutex noticeMutex;
    static std::vector<Notice> notices;
    static uint64_t nextNotice=1;
    void notify(const std::string& type,const std::string& message){
        Guard g(noticeMutex);
        notices.push_back({
            nextNotice++,time(nullptr),type,message
        });
        if(notices.size()>100)notices.erase(notices.begin());
    }
    std::vector<Notice> noticesSince(uint64_t id){
        Guard g(noticeMutex);
        std::vector<Notice> out;
        for(auto& n:notices)if(n.id>id)out.push_back(n);
        return out;
    }
}
