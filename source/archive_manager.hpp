#pragma once
#include "file_manager.hpp"
namespace fw {
bool makeZip(const std::vector<std::string>& sources,const std::string& dest,const std::string& policy,std::string& error,Progress* progress=nullptr);
bool extractZip(const std::string& source,const std::string& dest,const std::string& policy,std::string& error,Progress* progress=nullptr);
}
