#include "archive_manager.hpp"
#include "runtime.hpp"
#include "operations.hpp"
#include "upload_manager.hpp"
#include "notification_manager.hpp"
#include <algorithm>
#include <array>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <zlib.h>
namespace fw {
    static std::atomic<int> listener{
        -1
    };
    static Thread workers[3],jobThread;
    static int workerCount=0;
    static bool jobStarted=false;
    struct Job {
        std::string id,action,policy="ask",destination,error,term,status="queued",conflict,answer,all;
        std::vector<std::string> sources,remaining;
        Params decisions;
        std::vector<Entry> results;
        Progress progress;
    };
    static std::deque<std::shared_ptr<Job>> jobs;
    static Mutex jobMutex;
    static uint64_t pairNext=0;
    void allowWrites(bool enabled){
        writeEnabled=enabled;
        if(!enabled){
            Guard g(jobMutex);
            for(auto& j:jobs)if(j->action!="properties"&&j->action!="search")j->progress.cancel=true;
        }
        log(enabled?"Modificaciones habilitadas":"Modo de solo lectura activado");
    }
    static uint64_t millis(){
        const auto ticks=armGetSystemTick(),frequency=armGetSystemTickFreq();
        return ticks/frequency*1000+(ticks%frequency)*1000/frequency;
    }
    static bool ready(int fd,short events,uint64_t until){
        while(!stopping&&millis()<until){
            pollfd p{
                fd,events,0
            };
            int r=poll(&p,1,200);
            if(r>0)return (p.revents&events)!=0;
            if(r<0&&errno!=EINTR)return false;
        }
        return false;
    }
    static bool sendAll(int fd,const void* data,size_t length){
        auto p=static_cast<const char*>(data);
        size_t offset=0;
        auto until=millis()+15000;
        while(offset<length&&!stopping){
            if(!ready(fd,POLLOUT,until))return false;
            ssize_t n=send(fd,p+offset,length-offset,0);
            if(n>0){
                offset+=n;
                until=millis()+15000;
            }else if(n<0&&(errno==EINTR||errno==EAGAIN||errno==EWOULDBLOCK))continue;
            else return false;
        }
        return offset==length;
    }
    bool response(int fd,int code,const std::string& body,const std::string& type,const std::string& extra){
        const char* reason=code==200?"OK":code==202?"Accepted":code==400?"Bad Request":code==401?"Unauthorized":code==403?"Forbidden":code==404?"Not Found":code==409?"Conflict":code==413?"Payload Too Large":code==423?"Locked":code==429?"Too Many Requests":code==503?"Service Unavailable":code==500?"Internal Server Error":code==410?"Gone":"Error";
        std::string h="HTTP/1.1 "+std::to_string(code)+" "+reason+"\r\nContent-Type: "+type+"\r\nContent-Length: "+std::to_string(body.size())+"\r\nConnection: close\r\nCache-Control: no-store\r\nX-Content-Type-Options: nosniff\r\nReferrer-Policy: no-referrer\r\n"+extra+"\r\n";
        return sendAll(fd,h.data(),h.size())&&sendAll(fd,body.data(),body.size());
    }
    void fail(int fd,int code,const std::string& e){
        response(fd,code,"{\"error\":"+quote(e)+"}");
    }
    static bool receive(int fd,Request& r){
        std::string data;
        std::array<char,8192>b;
        auto until=millis()+10000;
        size_t end=std::string::npos;
        while((end=data.find("\r\n\r\n"))==std::string::npos){
            if(data.size()>16384){
                fail(fd,413,"Cabecera demasiado grande");
                return false;
            }
            if(!ready(fd,POLLIN,until))return false;
            ssize_t n=recv(fd,b.data(),b.size(),0);
            if(n>0){
                data.append(b.data(),n);
                until=millis()+10000;
            }else if(n<0&&(errno==EAGAIN||errno==EINTR))continue;
            else return false;
        }
        size_t line=data.find("\r\n"),a=data.find(' '),c=data.find(' ',a+1);
        if(line==std::string::npos||a>line||c>line||data.substr(c+1,line-c-1)!="HTTP/1.1"){
            fail(fd,400,"Solicitud HTTP invalida");
            return false;
        }
        if(end>16384){
            fail(fd,413,"Cabecera demasiado grande");
            return false;
        }
        r.method=data.substr(0,a);
        auto target=data.substr(a+1,c-a-1);
        size_t q=target.find('?');
        r.route=target.substr(0,q);
        if(r.route.empty()||r.route.front()!='/'){
            fail(fd,400,"Ruta HTTP invalida");
            return false;
        }
        if(q!=std::string::npos&&!parseParams(target.substr(q+1),r.params)){
            fail(fd,400,"Parametros invalidos");
            return false;
        }
        std::map<std::string,std::string> headers;
        size_t pos=line+2;
        while(pos<end){
            size_t stop=data.find("\r\n",pos),sep=data.find(':',pos);
            if(stop==std::string::npos||sep>=stop||data[pos]==' '||data[pos]=='\t'){
                fail(fd,400,"Cabecera invalida");
                return false;
            }
            auto k=lower(data.substr(pos,sep-pos));
            auto v=data.substr(sep+1,stop-sep-1);
            while(!v.empty()&&(v.front()==' '||v.front()=='\t'))v.erase(v.begin());
            if(headers.count(k)){
                fail(fd,400,"Cabecera duplicada");
                return false;
            }
            headers[k]=v;
            pos=stop+2;
        }
        if(headers.count("transfer-encoding")||headers.count("expect")||!headers.count("host")){
            fail(fd,400,"Solicitud no admitida");
            return false;
        }
        r.host=headers["host"];
        r.origin=headers["origin"];
        r.key=headers["x-folderwifi-key"];
        if(headers.count("content-length")&&!number(headers["content-length"],r.size)){
            fail(fd,400,"Longitud invalida");
            return false;
        }
        if(r.size>1024*1024){
            fail(fd,413,"Bloque demasiado grande");
            return false;
        }
        r.body=data.substr(end+4);
        if(r.body.size()>r.size){
            fail(fd,400,"Cuerpo HTTP invalido");
            return false;
        }
        until=millis()+15000;
        while(r.body.size()<r.size){
            if(!ready(fd,POLLIN,until))return false;
            ssize_t n=recv(fd,b.data(),std::min<uint64_t>(b.size(),r.size-r.body.size()),0);
            if(n>0){
                r.body.append(b.data(),n);
                until=millis()+15000;
            }else if(n<0&&(errno==EINTR||errno==EAGAIN))continue;
            else return false;
        }
        if(r.method=="POST"&&r.route!="/api/upload/chunk"&&!parseParams(r.body,r.params)){
            fail(fd,400,"Formulario invalido");
            return false;
        }
        return true;
    }
    bool validId(const std::string& id,size_t n){
        return id.size()==n&&id.find_first_not_of("0123456789abcdef")==std::string::npos;
    }
    static std::string pathsJson(const std::vector<std::string>& list){
        std::string out="[";
        for(auto& s:list){
            if(out.size()>1)out+=',';
            out+=quote(s);
        }
        return out+"]";
    }
    static std::string entriesJson(const std::vector<Entry>& list){
        std::string out="[";
        for(auto& e:list){
            if(out.size()>1)out+=',';
            out+="{\"name\":"+quote(e.name)+",\"path\":"+quote(e.path)+",\"directory\":"+(e.directory?"true":"false")+",\"size\":"+std::to_string(e.size)+",\"modified\":"+std::to_string(e.modified)+"}";
        }
        return out+"]";
    }
    static std::string resolve(const std::shared_ptr<Job>& j,const std::string& target){
        {
            Guard g(jobMutex);
            auto decision=j->decisions.find("decision:"+target);
            if(decision!=j->decisions.end())return decision->second;
            if(!j->all.empty())return j->all;
            j->conflict=target;
            j->status="conflict";
            j->answer.clear();
        }
        // La decision puede llegar desde otra red. No bloquear la SD mientras se
        // espera: el gestor Wi-Fi necesita poder guardar el perfil del punto local.
        mutexUnlock(&fsMutex);
        std::string answer;
        while(!stopping&&!j->progress.cancel){
            {
                Guard g(jobMutex);
                if(!j->answer.empty()){
                    answer=j->answer;
                    j->conflict.clear();
                    j->answer.clear();
                    j->status="running";
                }
            }
            if(!answer.empty())break;
            svcSleepThread(100000000);
        }
        mutexLock(&fsMutex);
        return answer.empty()?"cancel":answer;
    }
    static void jobWorker(void*){
        while(!stopping){
            std::shared_ptr<Job> j;
            {
                Guard g(jobMutex);
                for(auto& i:jobs)if(i->status=="queued"){
                    j=i;
                    j->status="running";
                    break;
                }
            }
            if(!j){
                svcSleepThread(100000000);
                continue;
            }
            std::string error;
            bool ok=true;
            j->progress.resolve=[j](const std::string& target){
                return resolve(j,target);
            };
            {
                Guard disk(fsMutex);
                if(j->progress.cancel){
                    ok=false;
                    error="Operacion cancelada";
                }
                uint64_t size=0,files=0,dirs=0;
                if(ok&&j->action!="restore"&&j->action!="purge"&&j->action!="search")for(auto& src:j->sources)if(!inspect(src,size,files,dirs,error,&j->progress)){
                    ok=false;
                    break;
                }
                j->progress.total=size;
                j->progress.dirs=dirs;
                if(ok&&j->action=="copy")ok=freeSpace(size,error);
                if(ok&&j->action=="search"){
                    std::vector<Entry> results;
                    ok=searchTree(j->sources[0],j->term,results,error,&j->progress);
                    Guard g(jobMutex);
                    j->results=std::move(results);
                } else if(ok&&j->action=="zip")ok=makeZip(j->sources,j->destination,j->policy,error,&j->progress);
                else if(ok)for(auto& src:j->sources){
                    if(j->progress.cancel){
                        error="Operacion cancelada; se conservan los elementos ya completados";
                        ok=false;
                        break;
                    }
                    std::string dest=j->sources.size()==1&&j->action=="rename"?j->destination:join(j->destination,name(src));
                    if(j->action=="copy")ok=copy(src,dest,j->policy,error,&j->progress);
                    else if(j->action=="move"||j->action=="rename")ok=move(src,dest,j->policy,error,&j->progress);
                    else if(j->action=="trash"){
                        std::string id;
                        ok=trash(src,id,error);
                        if(ok)++j->progress.files;
                    } else if(j->action=="delete")ok=removeTree(src,error,&j->progress);
                    else if(j->action=="extract")ok=extractZip(src,j->destination,j->policy,error,&j->progress);
                    else if(j->action=="restore")ok=restore(src,j->policy,error);
                    else if(j->action=="purge"){
                        if(!validId(src,24)){
                            ok=false;
                            error="Identificador de papelera invalido";
                        }else ok=removeTree(std::string(stateDir)+"/trash/"+src,error,&j->progress);
                    } else if(j->action=="properties"){
                        j->progress.bytes=size;
                        j->progress.files=files;
                        break;
                    } else {
                        ok=false;
                        error="Operacion no admitida";
                    }
                    if(!ok)break;
                }
                std::vector<std::string> remaining;
                for(auto& src:j->sources)if(exists(src))remaining.push_back(src);
                Guard g(jobMutex);
                j->remaining=std::move(remaining);
            }
            if(j->progress.cancel){
                ok=false;
                error="Operacion cancelada. Se conservan los elementos ya completados y se detiene lo pendiente";
            }
            {
                Guard g(jobMutex);
                j->error=error;
                j->status=ok?"done":j->progress.cancel?"cancelled":"error";
            }
            log(j->action+(ok?": completado":": "+error),ok?"success":"error");
            j->progress.resolve=nullptr;
        }
    }
    static bool enqueue(int fd,const Params& p){
        auto j=std::make_shared<Job>();
        j->id=value(p,"id");
        if(!validId(j->id)){
            fail(fd,400,"Identificador de operacion invalido");
            return false;
        }
        j->action=value(p,"action");
        j->sources=values(p,"source");
        j->destination=path(value(p,"destination"));
        j->policy=value(p,"policy","ask");
        j->term=value(p,"term");
        for(auto& entry:p)if(entry.first.rfind("decision:",0)==0){
            auto dest=path(entry.first.substr(9));
            if(!dest.empty()&&(entry.second=="replace"||entry.second=="keep"||entry.second=="skip"||entry.second=="merge"))j->decisions.emplace(entry.first,entry.second);
        }
        if(j->id.empty()||j->sources.empty()||j->sources.size()>512||(j->policy!="ask"&&j->policy!="replace"&&j->policy!="keep"&&j->policy!="skip"&&j->policy!="merge")){
            fail(fd,400,"Operacion invalida");
            return false;
        }
        const std::string allowed="|copy|move|rename|trash|delete|zip|extract|properties|restore|purge|search|";
        if(allowed.find("|"+j->action+"|")==std::string::npos){
            fail(fd,400,"Operacion no admitida");
            return false;
        }
        bool internal=j->action=="restore"||j->action=="purge";
        for(auto& src:j->sources){
            if(internal){
                if(!validId(src,24)){
                    fail(fd,400,"Registro invalido");
                    return false;
                }
            }else{
                src=path(src);
                if(src.empty()||(src=="sdmc:/"&&j->action!="search"&&j->action!="properties")){
                    fail(fd,400,"Origen invalido");
                    return false;
                }
            }
        }
        if(!internal&&j->action!="search"&&j->action!="properties")j->sources=deduplicate(j->sources);
        if((j->action=="copy"||j->action=="move"||j->action=="rename"||j->action=="zip"||j->action=="extract")&&j->destination.empty()){
            fail(fd,400,"Destino invalido");
            return false;
        }
        {
            Guard g(jobMutex);
            for(auto& existing:jobs)if(existing->id==j->id){
                if(existing->action!=j->action||existing->destination!=j->destination||existing->sources!=j->sources){
                    fail(fd,409,"Identificador usado por otra operacion");
                    return false;
                }
                response(fd,202,"{\"id\":"+quote(j->id)+"}");
                return true;
            }
            size_t pending=0;
            for(auto& i:jobs)if(i->status=="queued"||i->status=="running"||i->status=="conflict")++pending;
            if(pending>=32){
                fail(fd,429,"Demasiadas operaciones pendientes");
                return false;
            }
            while(jobs.size()>=64){
                if(jobs.front()->status=="queued"||jobs.front()->status=="running"||jobs.front()->status=="conflict")break;
                jobs.pop_front();
            }
            jobs.push_back(j);
        }
        response(fd,202,"{\"id\":"+quote(j->id)+"}");
        return true;
    }
    void recoverFiles(){
        std::string error;
        makeDirs(stateDir,error);
        recoverAtomicState(stateDir);
        DIR* d=opendir(stateDir);
        if(!d)return;
        while(auto* e=readdir(d)){
            std::string n=e->d_name;
            if(n.rfind("ticket-",0)==0){
                unlink((std::string(stateDir)+"/"+n).c_str());
                continue;
            }
            if(n.rfind("recovery-",0)!=0)continue;
            auto manifest=std::string(stateDir)+"/"+n,data=readSmall(manifest);
            size_t a=data.find('\n'),b=data.find('\n',a+1);
            if(a==std::string::npos||b==std::string::npos)continue;
            auto dest=path(data.substr(0,a),true),backup=path(data.substr(a+1,b-a-1),true),temp=path(data.substr(b+1),true);
            if(dest.empty()||backup.empty()||temp.empty()||parent(dest)!=parent(backup))continue;
            if(!exists(dest)&&exists(backup))rename(backup.c_str(),dest.c_str());
            if(exists(dest)){
                if(exists(backup)){
                    auto id=randomToken(24),base=std::string(stateDir)+"/trash/"+id;
                    if(!id.empty()&&makeDirs(base,error)&&writeAtomic(base+"/original",dest,error)&&rename(backup.c_str(),(base+"/item").c_str())==0){
                        log("Respaldo recuperado en papelera: "+dest,"warning");
                        unlink(manifest.c_str());
                    }
                }else unlink(manifest.c_str());
            }
        }
        closedir(d);
        recoverUploads();
    }
    static void route(int fd,Request& r){
        if(r.method!="GET"&&r.method!="POST"){
            fail(fd,400,"Metodo no admitido");
            return;
        }
        if(r.route=="/"&&r.method=="GET"){
            response(fd,200,webPage(),"text/html; charset=utf-8","Content-Security-Policy: default-src 'none'; script-src 'unsafe-inline'; style-src 'unsafe-inline'; connect-src 'self'; img-src 'self' blob: data:; frame-ancestors 'none'; base-uri 'none'; form-action 'self'\r\n");
            return;
        }
        if(r.origin.size()&&r.origin!="http://"+r.host){
            fail(fd,401,"Origen no autorizado");
            return;
        }
        if(r.route=="/api/pair"&&r.method=="POST"){
            Guard g(stateMutex);
            auto now=millis();
            if(now<pairNext){
                fail(fd,429,"Espera antes de volver a vincular");
                return;
            }
            pairNext=now+1000;
            if(value(r.params,"code")!=pairCode){
                fail(fd,401,"Codigo de vinculacion incorrecto");
                return;
            }
            response(fd,200,"{\"key\":"+quote(accessKey)+"}");
            return;
        }
        // El token nunca se envia a servicios externos, a registros ni mediante cookies.
        if(r.key!=accessKey||accessKey.empty()){
            fail(fd,401,"Vincula este dispositivo con el codigo mostrado en la Switch");
            return;
        }
        if(r.method=="POST"&&!writeEnabled&&((r.route=="/api/operation"&&value(r.params,"action")!="properties"&&value(r.params,"action")!="search")||r.route=="/api/mkdir"||r.route=="/api/upload/begin"||r.route=="/api/upload/chunk"||r.route=="/api/upload/finish"||r.route=="/api/upload/directory")){
            fail(fd,403,"Solo lectura. Pulsa A en la Switch para habilitar modificaciones");
            return;
        }
        if(r.route=="/api/status"&&r.method=="GET"){
            auto n=networkSnapshot();
            std::string out="{\"writable\":"+std::string(writeEnabled?"true":"false")+",\"online\":"+std::string(n.online?"true":"false")+",\"local\":"+(n.local?"true":"false")+",\"ip\":"+quote(n.ip)+",\"message\":"+quote(n.message)+",\"logs\":[";
            {
                Guard g(stateMutex);
                for(auto& s:logs){
                    if(out.back()!='[')out+=',';
                    out+=quote(s);
                }
            }
            response(fd,200,out+"]}");
            return;
        }
        if(r.route=="/api/job"&&r.method=="GET"){
            auto id=value(r.params,"id");
            Guard g(jobMutex);
            for(auto& j:jobs)if(j->id==id){
                response(fd,200,"{\"id\":"+quote(id)+",\"status\":"+quote(j->status)+",\"error\":"+quote(j->error)+",\"conflict\":"+quote(j->conflict)+",\"bytes\":"+std::to_string(j->progress.bytes)+",\"total\":"+std::to_string(j->progress.total)+",\"files\":"+std::to_string(j->progress.files)+",\"skipped\":"+std::to_string(j->progress.skipped)+",\"replaced\":"+std::to_string(j->progress.replaced)+",\"remaining\":"+pathsJson(j->remaining)+",\"dirs\":"+std::to_string(j->progress.dirs)+",\"results\":"+entriesJson(j->results)+"}");
                return;
            }
            fail(fd,404,"Operacion no disponible");
            return;
        }
        if(r.route=="/api/job/cancel"&&r.method=="POST"){
            Guard g(jobMutex);
            for(auto& j:jobs)if(j->id==value(r.params,"id")){
                j->progress.cancel=true;
                response(fd,200,"{}");
                return;
            }
            fail(fd,404,"Operacion no disponible");
            return;
        }
        if(r.route=="/api/job/decision"&&r.method=="POST"){
            auto choice=value(r.params,"choice");
            if(choice!="replace"&&choice!="keep"&&choice!="skip"&&choice!="merge"&&choice!="cancel"){
                fail(fd,400,"Decision invalida");
                return;
            }
            Guard g(jobMutex);
            for(auto& j:jobs)if(j->id==value(r.params,"id")&&j->status=="conflict"){
                if(choice=="cancel")j->progress.cancel=true;
                else{
                    j->answer=choice;
                    if(value(r.params,"all")=="1")j->all=choice;
                }
                response(fd,200,"{}");
                return;
            }
            fail(fd,409,"El conflicto ya no esta pendiente");
            return;
        }
        if(r.route=="/api/notifications"&&r.method=="GET"){
            uint64_t since=0;
            number(value(r.params,"since","0"),since);
            auto notices=noticesSince(since);
            std::string out="[";
            for(auto& n:notices){
                if(out.size()>1)out+=',';
                out+="{\"id\":"+std::to_string(n.id)+",\"type\":"+quote(n.type)+",\"timestamp\":"+std::to_string(n.timestamp)+",\"message\":"+quote(n.message)+"}";
            }
            response(fd,200,out+"]");
            return;
        }
        if(r.route=="/api/operation"&&r.method=="POST"){
            enqueue(fd,r.params);
            return;
        }
        // No esperar en un socket mientras otra operacion mantiene la SD ocupada.
        if(!mutexTryLock(&fsMutex)){
            fail(fd,423,"SD ocupada por una operacion. Reintenta al finalizar");
            return;
        }
        struct Unlock{
            ~Unlock(){
                mutexUnlock(&fsMutex);
            }
        }
        unlock;
        std::string error;
        if(r.route.rfind("/api/upload/",0)==0&&((r.route=="/api/upload/status"&&r.method=="GET")||(r.route!="/api/upload/status"&&r.method=="POST"))){
            uploadRoute(fd,r);
            return;
        }
        if(r.route=="/api/list"&&r.method=="GET"){
            auto dir=path(value(r.params,"path","sdmc:/"));
            std::vector<Entry> items;
            if(!list(dir,items,error)){
                fail(fd,404,error);
                return;
            }
            struct statvfs space{
            };
            statvfs("sdmc:/",&space);
            response(fd,200,"{\"path\":"+quote(dir)+",\"entries\":"+entriesJson(items)+",\"free\":"+std::to_string((uint64_t)space.f_bavail*space.f_frsize)+"}");
            return;
        }
        if(r.route=="/api/preflight"&&r.method=="POST"){
            auto sources=values(r.params,"source");
            auto dest=path(value(r.params,"destination")),action=value(r.params,"action");
            std::vector<std::string> found;
            bool truncated=false;
            for(auto& raw:sources){
                auto src=path(raw);
                auto target=action=="rename"||action=="zip"?dest:join(dest,name(src));
                if(src.empty()||src=="sdmc:/"||target.empty()||(lower(src)==lower(target)&&!(action=="rename"&&src!=target))||descendant(target,src)){
                    fail(fd,400,"Origen y destino coinciden o el destino esta dentro del origen");
                    return;
                }
                if(!conflicts(src,target,found,truncated,error)){
                    fail(fd,409,error);
                    return;
                }
            }
            std::string out="{\"conflicts\":[";
            for(auto& f:found){
                if(out.back()!='[')out+=',';
                out+=quote(f);
            }
            response(fd,200,out+"],\"truncated\":"+(truncated?"true":"false")+"}");
            return;
        }
        if(r.route=="/api/mkdir"&&r.method=="POST"){
            auto dest=path(value(r.params,"path"));
            if(dest.empty()||dest=="sdmc:/"){
                fail(fd,400,"Ruta invalida");
                return;
            }
            if(exists(dest)){
                fail(fd,409,"El destino ya existe");
                return;
            }
            if(!makeDirs(dest,error)){
                fail(fd,409,error);
                return;
            }
            log("Carpeta creada: "+dest);
            response(fd,200,"{}");
            return;
        }
        if(r.route=="/api/trash"&&r.method=="GET"){
            std::string out="[";
            auto base=std::string(stateDir)+"/trash";
            DIR* d=opendir(base.c_str());
            if(d){
                while(auto* e=readdir(d)){
                    std::string id=e->d_name;
                    if(!validId(id,24)||!exists(base+"/"+id+"/item"))continue;
                    auto original=path(readSmall(base+"/"+id+"/original"));
                    if(original.empty())continue;
                    if(out.size()>1)out+=',';
                    out+="{\"id\":"+quote(id)+",\"path\":"+quote(original)+"}";
                }
                closedir(d);
            }
            response(fd,200,out+"]");
            return;
        }
        if(r.route=="/api/download/ticket"&&r.method=="POST"){
            auto p=path(value(r.params,"path"));
            if(p.empty()||directory(p)||!exists(p)){
                fail(fd,404,"Archivo no disponible");
                return;
            }
            auto ticket=randomToken(32);
            if(ticket.empty()||!writeAtomic(std::string(stateDir)+"/ticket-"+ticket,std::to_string(millis()+60000)+"\n"+p,error)){
                fail(fd,500,"No se pudo preparar descarga");
                return;
            }
            response(fd,200,"{\"url\":"+quote("/download?ticket="+ticket)+"}");
            return;
        }
        fail(fd,404,"Ruta no disponible");
    }
    // Un solo uso y un minuto de validez. Permite al navegador descargar sin exponer
    // la clave de acceso permanente en la URL, y sin cargar el archivo en memoria.
    static void download(int fd,const Request& r){
        auto id=value(r.params,"ticket");
        if(!validId(id)){
            fail(fd,401,"Descarga no autorizada");
            return;
        }
        if(!mutexTryLock(&fsMutex)){
            fail(fd,423,"SD ocupada");
            return;
        }
        auto meta=std::string(stateDir)+"/ticket-"+id,data=readSmall(meta);
        auto split=data.find('\n');
        uint64_t expiry=0;
        auto p=split==std::string::npos?"":path(data.substr(split+1));
        if(split==std::string::npos||!number(data.substr(0,split),expiry)||expiry<millis()||p.empty()){
            mutexUnlock(&fsMutex);
            fail(fd,401,"Descarga caducada");
            return;
        }
        unlink(meta.c_str());
        FILE* f=fopen(p.c_str(),"rb");
        struct stat st{
        };
        bool valid=f&&fstat(fileno(f),&st)==0&&S_ISREG(st.st_mode);
        if(!valid){
            mutexUnlock(&fsMutex);
        }
        if(!valid){
            if(f)fclose(f);
            fail(fd,404,"Archivo no disponible");
            return;
        }
        std::string h="HTTP/1.1 200 OK\r\nContent-Type: application/octet-stream\r\nContent-Disposition: attachment; filename*=UTF-8''"+encode(name(p))+"\r\nContent-Length: "+std::to_string(st.st_size)+"\r\nConnection: close\r\nCache-Control: no-store\r\nX-Content-Type-Options: nosniff\r\n\r\n";
        bool ok=sendAll(fd,h.data(),h.size());
        std::array<char,65536>b;
        uint64_t sent=0;
        while(ok&&sent<(uint64_t)st.st_size){
            size_t n=fread(b.data(),1,std::min<uint64_t>(b.size(),st.st_size-sent),f);
            if(!n){
                ok=false;
                break;
            }
            ok=sendAll(fd,b.data(),n);
            sent+=n;
        }
        if(ferror(f))ok=false;
        fclose(f);
        mutexUnlock(&fsMutex);
        if(!ok)log("Descarga interrumpida: "+p);
    }
    static void httpWorker(void*){
        while(!stopping){
            if(!ready(listener,POLLIN,millis()+500))continue;
            int client=accept(listener,nullptr,nullptr);
            if(client<0)continue;
            fcntl(client,F_SETFL,fcntl(client,F_GETFL,0)|O_NONBLOCK);
            Request r;
            if(receive(client,r)){
                if(r.route=="/download"&&r.method=="GET")download(client,r);
                else route(client,r);
            }
            shutdown(client,SHUT_RDWR);
            close(client);
        }
    }
    bool startServer(std::string& error){
        accessKey=readSmall(std::string(stateDir)+"/access");
        if(accessKey.size()!=64){
            accessKey=randomToken(64);
            if(accessKey.empty()||!writeAtomic(std::string(stateDir)+"/access",accessKey,error))return false;
        }
        pairCode=randomToken(12);
        if(pairCode.empty()){
            error="No hay aleatoriedad segura";
            return false;
        }
        listener=socket(AF_INET,SOCK_STREAM,0);
        if(listener<0){
            error="No se pudo crear socket HTTP";
            return false;
        }
        int yes=1;
        setsockopt(listener,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(yes));
        sockaddr_in addr{
        };
        addr.sin_family=AF_INET;
        addr.sin_port=htons(8080);
        addr.sin_addr.s_addr=INADDR_ANY;
        if(::bind(listener.load(),(sockaddr*)&addr,sizeof(addr))<0||listen(listener,8)<0){
            error="No se pudo abrir el puerto 8080";
            close(listener);
            listener=-1;
            return false;
        }
        fcntl(listener,F_SETFL,fcntl(listener,F_GETFL,0)|O_NONBLOCK);
        for(int i=0;i<3;++i){
            if(R_FAILED(threadCreate(&workers[i],httpWorker,nullptr,nullptr,0x20000,0x2C,-2))){
                error="No se pudo crear trabajador HTTP";
                return false;
            }
            if(R_FAILED(threadStart(&workers[i]))){
                threadClose(&workers[i]);
                error="No se pudo iniciar trabajador HTTP";
                return false;
            }
            ++workerCount;
        }
        if(R_FAILED(threadCreate(&jobThread,jobWorker,nullptr,nullptr,0x60000,0x2C,-2))){
            error="No se pudo crear trabajador de operaciones";
            return false;
        }
        if(R_FAILED(threadStart(&jobThread))){
            threadClose(&jobThread);
            error="No se pudo iniciar operaciones";
            return false;
        }
        jobStarted=true;
        serverReady=true;
        log("Servidor HTTP iniciado en puerto 8080");
        return true;
    }
    void stopServer(){
        serverReady=false;
        stopping=true;
        {
            Guard g(jobMutex);
            for(auto& j:jobs)j->progress.cancel=true;
        }
        if(listener>=0){
            shutdown(listener,SHUT_RDWR);
            close(listener);
            listener=-1;
        }
        for(int i=0;i<workerCount;++i){
            threadWaitForExit(&workers[i]);
            threadClose(&workers[i]);
        }
        workerCount=0;
        if(jobStarted){
            threadWaitForExit(&jobThread);
            threadClose(&jobThread);
            jobStarted=false;
        }
    }
}
