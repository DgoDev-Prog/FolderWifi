#include "operations.hpp"
#include <dirent.h>
#include <cstdio>
namespace fw {
    bool conflicts(const std::string& src,const std::string& dst,std::vector<std::string>& out,bool& truncated,std::string& error,unsigned depth){
        if(depth>64){
            error="Demasiados niveles";
            return false;
        }
        if(out.size()>=50){
            truncated=true;
            return true;
        }
        if(exists(dst))out.push_back(dst);
        if(directory(src)&&directory(dst)){
            std::vector<Entry> items;
            if(!list(src,items,error))return false;
            for(auto& e:items)if(!conflicts(e.path,join(dst,e.name),out,truncated,error,depth+1))return false;
        }
        return true;
    }
    bool searchTree(const std::string& dir,const std::string& term,std::vector<Entry>& out,std::string& error,Progress* p,unsigned depth){
        if(depth>64||(p&&p->cancel)){
            error="Busqueda cancelada o demasiados niveles";
            return false;
        }
        std::vector<Entry> items;
        if(!list(dir,items,error))return false;
        for(auto& e:items){
            if(p&&++p->files>100000){
                error="Limite de busqueda alcanzado";
                return false;
            }
            if(lower(e.name).find(lower(term))!=std::string::npos){
                out.push_back(e);
                if(out.size()>5000){
                    error="Mas de 5000 resultados; usa una busqueda mas especifica";
                    return false;
                }
            }
            if(e.directory&&!searchTree(e.path,term,out,error,p,depth+1))return false;
        }
        return true;
    }
    void recoverAtomicState(const std::string& dir,unsigned depth){
        if(depth>4)return;
        DIR* d=opendir(dir.c_str());
        if(!d)return;
        while(auto* e=readdir(d)){
            std::string n=e->d_name;
            if(n=="."||n=="..")continue;
            auto full=dir+"/"+n;
            if(directory(full))recoverAtomicState(full,depth+1);
            else if(n.size()>9&&n.substr(n.size()-9)==".previous"){
                auto dest=full.substr(0,full.size()-9);
                if(!exists(dest))rename(full.c_str(),dest.c_str());
            }
        }
        closedir(d);
    }
}
