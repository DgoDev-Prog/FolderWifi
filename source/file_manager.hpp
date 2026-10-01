#pragma once
#include <atomic>
#include <cstdint>
#include <functional>
#include <map>
#include <string>
#include <vector>

namespace fw {
constexpr const char* stateDir = "sdmc:/.folderwifi";
struct Entry { std::string name, path; bool directory=false; uint64_t size=0; int64_t modified=0; };
struct Progress { std::atomic<bool> cancel{false}; std::atomic<uint64_t> bytes{0}, total{0}, files{0}, skipped{0}, replaced{0}, dirs{0}; std::function<std::string(const std::string&)> resolve; };
using Params = std::multimap<std::string,std::string>;
std::string lower(std::string s);
std::string quote(const std::string& s);
std::string encode(const std::string& s);
bool decode(const std::string& s, std::string& out);
bool parseParams(const std::string& s, Params& out);
std::string value(const Params& p,const std::string& key,const std::string& fallback="");
std::vector<std::string> values(const Params& p,const std::string& key);
bool number(const std::string& s,uint64_t& out);
std::string randomToken(size_t count=32);
std::string path(const std::string& s, bool internal=false);
std::string parent(const std::string& s);
std::string name(const std::string& s);
std::string join(const std::string& dir,const std::string& leaf);
bool descendant(const std::string& child,const std::string& base);
bool exists(const std::string& p);
bool directory(const std::string& p);
bool makeDirs(const std::string& p,std::string& error);
bool list(const std::string& p,std::vector<Entry>& out,std::string& error);
bool inspect(const std::string& p,uint64_t& bytes,uint64_t& files,uint64_t& dirs,std::string& error,Progress* progress=nullptr,unsigned depth=0);
bool freeSpace(uint64_t required,std::string& error);
std::string availableName(const std::string& target);
bool publish(const std::string& temp,const std::string& dest,bool replace,std::string& error);
bool copy(const std::string& src,const std::string& dst,const std::string& policy,std::string& error,Progress* progress=nullptr,unsigned depth=0);
bool move(const std::string& src,const std::string& dst,const std::string& policy,std::string& error,Progress* progress=nullptr);
bool removeTree(const std::string& p,std::string& error,Progress* progress=nullptr,unsigned depth=0);
bool trash(const std::string& p,std::string& id,std::string& error);
bool restore(const std::string& id,const std::string& policy,std::string& error);
bool writeAtomic(const std::string& dest,const std::string& data,std::string& error);
std::string readSmall(const std::string& p,size_t limit=65536);
std::vector<std::string> deduplicate(const std::vector<std::string>& raw);
}
