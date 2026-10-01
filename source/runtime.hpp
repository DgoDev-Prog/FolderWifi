#pragma once
#include <atomic>
#include "file_manager.hpp"
#include <switch.h>
#include <deque>
#include <memory>
#include <vector>
namespace fw {
struct Guard { Mutex* m; explicit Guard(Mutex& x):m(&x){mutexLock(m);} ~Guard(){mutexUnlock(m);} };
struct ConnectedDevice {
    std::string ip,mac;
};
struct Network {
    bool online=false,local=false,offer=false,hasProfile=false;
    int attempt=0;
    std::string ip,message,ssid,password,localError;
    bool devicesKnown=false;
    std::string devicesError;
    std::vector<ConnectedDevice> devices;
};
extern Mutex stateMutex,fsMutex;
extern std::atomic<bool> stopping;
extern std::atomic<bool> serverReady;
extern std::atomic<bool> writeEnabled;
extern std::atomic<int> networkCommand;
extern Network network;
extern std::string accessKey,pairCode;
extern std::deque<std::string> logs;
void log(const std::string& s,const std::string& type="info");
Network networkSnapshot();
bool startNetwork();
void stopNetwork();
bool startServer(std::string& error);
void stopServer();
void allowWrites(bool enabled);
const char* webPage();
void recoverFiles();
}
