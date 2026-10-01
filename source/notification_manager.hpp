#pragma once
#include <string>
#include <vector>
#include <cstdint>
namespace fw {
struct Notice{uint64_t id;int64_t timestamp;std::string type,message;};
void notify(const std::string& type,const std::string& message);
std::vector<Notice> noticesSince(uint64_t id);
}
