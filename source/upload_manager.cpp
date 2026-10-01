#include "upload_manager.hpp"
#include <array>
#include <cstdio>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <zlib.h>
#include <cerrno>
namespace fw {
    struct Upload {
        std::string dest,policy,fingerprint,phase="pending";
        uint64_t size=0,crc=0;
    };
    static std::string metaPath(const std::string& id){
        return std::string(stateDir)+"/uploads/"+id+".meta";
    }
    static std::string partPath(const std::string& id){
        return std::string(stateDir)+"/uploads/"+id+".part";
    }
    static bool loadUpload(const std::string& id,Upload& u){
        Params p;
        if(!validId(id)||!parseParams(readSmall(metaPath(id)),p))return false;
        u.dest=path(value(p,"destination"));
        u.policy=value(p,"policy");
        u.fingerprint=value(p,"fingerprint");
        u.phase=value(p,"phase","pending");
        return !u.dest.empty()&&number(value(p,"size"),u.size)&&number(value(p,"crc","0"),u.crc);
    }
    static bool saveUpload(const std::string& id,const Upload& u,std::string& error){
        return writeAtomic(metaPath(id),"destination="+encode(u.dest)+"&policy="+encode(u.policy)+"&size="+std::to_string(u.size)+"&fingerprint="+encode(u.fingerprint)+"&phase="+u.phase+"&crc="+std::to_string(u.crc),error);
    }
    static uint64_t fileSize(const std::string& p){
        struct stat s;
        return stat(p.c_str(),&s)==0&&s.st_size>=0?s.st_size:0;
    }
    static bool checksum(const std::string& p,uint64_t& crc){
        FILE* f=fopen(p.c_str(),"rb");
        if(!f)return false;
        std::array<char,65536>b;
        uLong c=crc32(0,nullptr,0);
        size_t n;
        while((n=fread(b.data(),1,b.size(),f))>0){
            c=crc32(c,reinterpret_cast<Bytef*>(b.data()),n);
            if(stopping){
                fclose(f);
                return false;
            }
        }
        bool ok=!ferror(f);
        if(fclose(f)!=0)ok=false;
        crc=c;
        return ok;
    }
    static std::string uploadJson(const std::string& id,const Upload& u){
        return "{\"id\":"+quote(id)+",\"offset\":"+std::to_string(u.phase=="done"?u.size:fileSize(partPath(id)))+",\"size\":"+std::to_string(u.size)+",\"phase\":"+quote(u.phase)+",\"crc\":"+std::to_string(u.crc)+",\"outcome\":"+quote(u.policy=="skip"?"skipped":"completed")+",\"destination\":"+quote(u.dest)+"}";
    }
    void uploadRoute(int fd,const Request& r){
        if(r.route=="/api/upload/forget"){
            auto ids=values(r.params,"id");
            if(ids.empty()||ids.size()>512){
                fail(fd,400,"Seleccion de transferencias invalida");
                return;
            }
            for(auto& id:ids)if(!validId(id)){
                fail(fd,400,"Identificador invalido");
                return;
            }
            for(auto& id:ids){
                Upload old;
                if(loadUpload(id,old)&&old.phase!="done"){
                    fail(fd,409,"No se puede olvidar una transferencia pendiente");
                    return;
                }
            }
            for(auto& id:ids){
                unlink(metaPath(id).c_str());
                unlink((metaPath(id)+".previous").c_str());
            }
            response(fd,200,"{}");
            return;
        }
        std::string id=value(r.params,"id"),error;
        if(!validId(id)){
            fail(fd,400,"Identificador de transferencia invalido");
            return;
        }
        Upload u;
        bool loaded=loadUpload(id,u);
        if(r.route=="/api/upload/directory"){
            auto dir=path(value(r.params,"destination")),relative=value(r.params,"relative");
            if(dir.empty()||relative.empty()||relative.front()=='/'){
                fail(fd,400,"Carpeta invalida");
                return;
            }
            auto dest=path(dir+(dir.back()=='/'?"":"/")+relative);
            if(dest.empty()||dest=="sdmc:/"){
                fail(fd,400,"Carpeta invalida");
                return;
            }
            if(loaded&&u.phase=="done"){
                response(fd,200,uploadJson(id,u));
                return;
            }
            auto policy=value(r.params,"policy","merge");
            if(exists(dest)){
                if(policy=="ask"){
                    response(fd,409,"{\"conflict\":"+quote(dest)+"}");
                    return;
                }
                if(policy=="keep")dest=availableName(dest);
                else if(policy=="skip"){
                    u.dest=dest;
                    u.phase="done";
                    u.policy="skip";
                    u.fingerprint="directory";
                    if(!saveUpload(id,u,error)){
                        fail(fd,500,error);
                        return;
                    }
                    response(fd,200,uploadJson(id,u));
                    return;
                } else if(!directory(dest)){
                    fail(fd,409,"No se puede combinar una carpeta con un archivo");
                    return;
                }
            }
            if(!makeDirs(dest,error)){
                fail(fd,409,error);
                return;
            }
            u.dest=dest;
            u.phase="done";
            u.size=0;
            u.policy="merge";
            u.fingerprint="directory";
            if(!saveUpload(id,u,error)){
                fail(fd,500,error);
                return;
            }
            response(fd,200,uploadJson(id,u));
            return;
        }
        if(r.route=="/api/upload/begin"){
            auto dir=path(value(r.params,"destination"));
            auto relative=value(r.params,"relative");
            uint64_t size=0;
            if(dir.empty()||relative.empty()||relative.front()=='/'||!number(value(r.params,"size"),size)||size>INT64_MAX){
                fail(fd,400,"Ruta o tamano invalido");
                return;
            }
            auto dest=path(dir+(dir.back()=='/'?"":"/")+relative);
            auto policy=value(r.params,"policy","ask"),fingerprint=value(r.params,"fingerprint");
            if(dest.empty()||dest=="sdmc:/"||fingerprint.empty()||(policy!="ask"&&policy!="keep"&&policy!="replace"&&policy!="skip")){
                fail(fd,400,"Transferencia invalida");
                return;
            }
            if(loaded){
                if(u.size!=size||u.fingerprint!=fingerprint){
                    fail(fd,409,"El identificador pertenece a otro archivo");
                    return;
                }
                response(fd,200,uploadJson(id,u));
                return;
            }
            if(exists(dest)){
                if(policy=="ask"){
                    response(fd,409,"{\"conflict\":"+quote(dest)+"}");
                    return;
                }
                if(policy=="skip"){
                    u.phase="done";
                    u.dest=dest;
                    u.size=size;
                    u.policy=policy;
                    u.fingerprint=fingerprint;
                    if(!saveUpload(id,u,error)){
                        fail(fd,500,error);
                        return;
                    }
                    response(fd,200,uploadJson(id,u));
                    return;
                }
                if(policy=="keep")dest=availableName(dest);
                else if(directory(dest)){
                    fail(fd,409,"Un archivo no puede reemplazar una carpeta");
                    return;
                }
            }
            u.dest=dest;
            u.size=size;
            u.fingerprint=fingerprint;
            u.policy=policy;
            if(dest.empty()||!freeSpace(size,error)||!makeDirs(parent(partPath(id)),error)){
                fail(fd,409,error);
                return;
            }
            int part=open(partPath(id).c_str(),O_CREAT|O_EXCL|O_WRONLY,0600);
            if(part<0){
                fail(fd,409,"Existe un temporal sin registro; usa un nuevo identificador");
                return;
            }
            close(part);
            if(!saveUpload(id,u,error)){
                unlink(partPath(id).c_str());
                fail(fd,500,error);
                return;
            }
            response(fd,200,uploadJson(id,u));
            return;
        }
        if(!loaded){
            fail(fd,404,"Transferencia no registrada");
            return;
        }
        if(r.route=="/api/upload/status"){
            response(fd,200,uploadJson(id,u));
            return;
        }
        if(r.route=="/api/upload/abort"){
            if(u.phase!="done")unlink(partPath(id).c_str());
            unlink(metaPath(id).c_str());
            response(fd,200,"{}");
            return;
        }
        if(u.phase=="done"){
            response(fd,200,uploadJson(id,u));
            return;
        }
        if(r.route=="/api/upload/chunk"){
            uint64_t offset=0,crc=0;
            if(!number(value(r.params,"offset"),offset)||!number(value(r.params,"crc"),crc)||crc>UINT32_MAX||crc32(0,reinterpret_cast<const Bytef*>(r.body.data()),r.body.size())!=crc){
                fail(fd,400,"Bloque corrupto o parametros invalidos");
                return;
            }
            auto size=fileSize(partPath(id));
            if(offset!=size||size>u.size||r.body.size()>u.size-size){
                response(fd,409,uploadJson(id,u));
                return;
            }
            int part=open(partPath(id).c_str(),O_WRONLY);
            bool ok=part>=0&&lseek(part,offset,SEEK_SET)==static_cast<off_t>(offset);
            size_t sent=0;
            while(ok&&sent<r.body.size()){
                ssize_t n=write(part,r.body.data()+sent,r.body.size()-sent);
                if(n<0&&errno==EINTR)continue;
                if(n<=0){
                    ok=false;
                    break;
                }
                sent+=n;
            }
            if(part>=0){
                if(fsync(part)!=0)ok=false;
                if(close(part)!=0)ok=false;
            }
            if(!ok){
                fail(fd,500,"No se pudo guardar el bloque; consulta el offset antes de reintentar");
                return;
            }
            response(fd,200,uploadJson(id,u));
            return;
        }
        if(r.route=="/api/upload/finish"){
            if(fileSize(partPath(id))!=u.size){
                fail(fd,409,"Transferencia incompleta");
                return;
            }
            if(exists(u.dest)&&u.policy!="replace"){
                auto policy=value(r.params,"policy");
                if(policy=="keep")u.dest=availableName(u.dest);
                else if(policy=="replace")u.policy=policy;
                else if(policy=="skip"){
                    unlink(partPath(id).c_str());
                    u.phase="done";
                    u.policy="skip";
                    if(!saveUpload(id,u,error)){
                        fail(fd,500,error);
                        return;
                    }
                    response(fd,200,uploadJson(id,u));
                    return;
                }else{
                    response(fd,409,"{\"conflict\":"+quote(u.dest)+"}");
                    return;
                }
            }
            if(!checksum(partPath(id),u.crc)){
                fail(fd,500,"No se pudo verificar la transferencia");
                return;
            }
            uint64_t expected=0;
            if(!number(value(r.params,"crc"),expected)||expected!=u.crc){
                fail(fd,409,"La integridad del archivo completo no coincide");
                return;
            }
            u.phase="publishing";
            if(!saveUpload(id,u,error)||!makeDirs(parent(u.dest),error)||!publish(partPath(id),u.dest,u.policy=="replace",error)){
                fail(fd,409,error);
                return;
            }
            u.phase="done";
            if(!saveUpload(id,u,error)){
                fail(fd,500,error);
                return;
            }
            log("Subida completada: "+u.dest);
            response(fd,200,uploadJson(id,u));
            return;
        }
        fail(fd,404,"Operacion de transferencia no admitida");
    }
    void recoverUploads(){
        std::string error;
        auto base=std::string(stateDir)+"/uploads";
        DIR* d=opendir(base.c_str());
        if(!d)return;
        while(auto* e=readdir(d)){
            std::string n=e->d_name;
            if(n.size()!=37||n.substr(32)!=".meta")continue;
            auto id=n.substr(0,32);
            Upload u;
            if(loadUpload(id,u)&&u.phase=="publishing"&&!exists(partPath(id))&&fileSize(u.dest)==u.size){
                uint64_t c;
                if(checksum(u.dest,c)&&c==u.crc){
                    u.phase="done";
                    saveUpload(id,u,error);
                }
            }
        }
        closedir(d);
    }
}
