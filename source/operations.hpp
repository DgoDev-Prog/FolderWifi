#pragma once
#include "file_manager.hpp"
namespace fw {
// El plan no modifica la SD. Los conflictos se verifican otra vez al ejecutar.
bool conflicts(const std::string& source,const std::string& destination,std::vector<std::string>& out,bool& truncated,std::string& error,unsigned depth=0);
bool searchTree(const std::string& directory,const std::string& term,std::vector<Entry>& out,std::string& error,Progress* progress,unsigned depth=0);
void recoverAtomicState(const std::string& directory,unsigned depth=0);
}
