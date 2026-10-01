#pragma once
#include "runtime.hpp"
namespace fw {
struct Request{std::string method,route,host,origin,key,body;Params params;uint64_t size=0;};
bool response(int,int,const std::string&,const std::string& type="application/json; charset=utf-8",const std::string& extra="");
void fail(int,int,const std::string&);
bool validId(const std::string&,size_t n=32);
void uploadRoute(int,const Request&);
void recoverUploads();
}
