#include "file_manager.hpp"
#include "unicode_case.hpp"
#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <set>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <unistd.h>
#include <utime.h>
#include <minizip/zip.h>
#include <minizip/unzip.h>
#ifdef __SWITCH__
#include <switch.h>
#else
#include <random>
#endif
namespace fw {
    static bool validUtf8(const std::string& text){
        for(size_t i=0;i<text.size();){
            unsigned char first=text[i++];
            if(first<128)continue;
            int count=first>=0xC2&&first<=0xDF?1:first>=0xE0&&first<=0xEF?2:first>=0xF0&&first<=0xF4?3:0;
            if(!count||i+count>text.size())return false;
            unsigned code=first&(count==1?31:count==2?15:7);
            for(int j=0;j<count;++j){
                unsigned char c=text[i++];
                if((c&0xC0)!=0x80)return false;
                code=(code<<6)|(c&63);
            }
            if((count==1&&code<128)||(count==2&&code<2048)||(count==3&&code<65536)||code>0x10FFFF||(code>=0xD800&&code<=0xDFFF)||(code>=0x80&&code<=0x9F))return false;
        }
        return true;
    }
    static std::string ioError(const char* action) {
        return std::string(action)+": "+strerror(errno);
    }
    std::string lower(std::string s) {
        std::string out;
        for(size_t i=0;i<s.size();){
            unsigned char first=s[i++];
            if(first<128){
                out+=first>='A'&&first<='Z'?char(first+32):char(first);
                continue;
            }
            int count=first>=0xC2&&first<=0xDF?1:first>=0xE0&&first<=0xEF?2:first>=0xF0&&first<=0xF4?3:0;
            if(!count||i+count>s.size()){
                out+=char(first);
                continue;
            }
            unsigned code=first&(count==1?31:count==2?15:7);
            for(int j=0;j<count;++j)code=(code<<6)|(static_cast<unsigned char>(s[i++])&63);
            size_t lo=0,hi=sizeof(unicodeCase)/sizeof(unicodeCase[0]);
            while(lo<hi){
                size_t mid=lo+(hi-lo)/2;
                if(unicodeCase[mid][0]<code)lo=mid+1;
                else hi=mid;
            }
            if(lo<sizeof(unicodeCase)/sizeof(unicodeCase[0])&&unicodeCase[lo][0]==code)code=unicodeCase[lo][1];
            if(code<128)out+=char(code);
            else if(code<2048){
                out+=char(0xC0|(code>>6));
                out+=char(0x80|(code&63));
            } else if(code<65536){
                out+=char(0xE0|(code>>12));
                out+=char(0x80|((code>>6)&63));
                out+=char(0x80|(code&63));
            } else{
                out+=char(0xF0|(code>>18));
                out+=char(0x80|((code>>12)&63));
                out+=char(0x80|((code>>6)&63));
                out+=char(0x80|(code&63));
            }
        }
        return out;
    }
    std::string quote(const std::string& s) {
        std::string out="\"";
        const char* hex="0123456789abcdef";
        for(unsigned char c:s) {
            if(c=='"'||c=='\\') {
                out+='\\';
                out+=char(c);
            } else if(c<32) {
                out+="\\u00";
                out+=hex[c>>4];
                out+=hex[c&15];
            } else out+=char(c);
        }
        return out+'"';
    }
    std::string encode(const std::string& s) {
        std::string out;
        const char* hex="0123456789ABCDEF";
        for(unsigned char c:s) {
            if((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='-'||c=='_'||c=='.'||c=='~') out+=char(c);
            else {
                out+='%';
                out+=hex[c>>4];
                out+=hex[c&15];
            }
        }
        return out;
    }
    bool decode(const std::string& s,std::string& out) {
        out.clear();
        auto hex=[](char c)->int {
            if(c>='0'&&c<='9')return c-'0';
            if(c>='a'&&c<='f')return c-'a'+10;
            if(c>='A'&&c<='F')return c-'A'+10;
            return -1;
        };
        for(size_t i=0;i<s.size();++i) {
            unsigned char c=s[i];
            if(c=='%'){
                if(i+2>=s.size())return false;
                int a=hex(s[i+1]),b=hex(s[i+2]);
                if(a<0||b<0)return false;
                c=(a<<4)|b;
                i+=2;
            }else if(c=='+')c=' ';
            if(c==0||c<32||c==127)return false;
            out+=char(c);
        }
        return validUtf8(out);
    }
    bool parseParams(const std::string& s,Params& out) {
        if(s.size()>1024*1024)return false;
        size_t pos=0;
        while(pos<s.size()){
            size_t end=s.find('&',pos);
            if(end==std::string::npos)end=s.size();
            auto part=s.substr(pos,end-pos);
            size_t eq=part.find('=');
            std::string k,v;
            if(eq==std::string::npos||!decode(part.substr(0,eq),k)||!decode(part.substr(eq+1),v)||k.empty())return false;
            out.emplace(k,v);
            if(out.size()>1024)return false;
            pos=end+1;
        }
        return true;
    }
    std::string value(const Params& p,const std::string& key,const std::string& fallback) {
        auto r=p.equal_range(key);
        if(r.first==r.second)return fallback;
        auto i=r.first;
        if(++i!=r.second)return "";
        return r.first->second;
    }
    std::vector<std::string> values(const Params& p,const std::string& key){
        std::vector<std::string> a;
        auto r=p.equal_range(key);
        for(auto i=r.first;i!=r.second;++i)a.push_back(i->second);
        return a;
    }
    bool number(const std::string& s,uint64_t& out){
        out=0;
        if(s.empty())return false;
        for(char c:s){
            if(c<'0'||c>'9'||out>(UINT64_MAX-(c-'0'))/10)return false;
            out=out*10+c-'0';
        }
        return true;
    }
    std::string randomToken(size_t count) {
        // Hexadecimal evita sesgo de modulo; nunca usar un PRNG predecible en Switch.
        std::vector<unsigned char> bytes((count+1)/2);
        #ifdef __SWITCH__
        if(R_FAILED(csrngGetRandomBytes(bytes.data(),bytes.size())))return "";
        #else
        std::random_device r;
        for(auto& b:bytes)b=static_cast<unsigned char>(r());
        #endif
        const char* hex="0123456789abcdef";
        std::string out;
        for(auto b:bytes){
            out+=hex[b>>4];
            out+=hex[b&15];
        }
        out.resize(count);
        return out;
    }
    std::string path(const std::string& s,bool internal) {
        if(s.rfind("sdmc:/",0)!=0||s.size()>768||!validUtf8(s))return "";
        std::string out="sdmc:/";
        size_t pos=6;
        while(pos<=s.size()){
            size_t end=s.find('/',pos);
            if(end==std::string::npos)end=s.size();
            auto part=s.substr(pos,end-pos);
            if(part==".."||part.size()>255||(!internal&&part.rfind(".fw-",0)==0))return "";
            for(unsigned char c:part)if(c<32||c==127||c==':'||c=='\\'||c=='*'||c=='?'||c=='"'||c=='<'||c=='>'||c=='|')return "";
            if(!part.empty()&&part!="."){
                if(part.back()==' '||part.back()=='.')return "";
                if(out.size()>6)out+='/';
                out+=part;
            }
            if(end==s.size())break;
            pos=end+1;
        }
        if(!internal&&(lower(out)==stateDir||descendant(out,stateDir)))return "";
        return out;
    }
    std::string parent(const std::string& s){
        auto i=s.find_last_of('/');
        return i<=5?"sdmc:/":s.substr(0,i);
    }
    std::string name(const std::string& s){
        return s.substr(s.find_last_of('/')+1);
    }
    std::string join(const std::string& d,const std::string& leaf){
        if(d.empty()||leaf.empty()||leaf.find('/')!=std::string::npos||leaf.find('\\')!=std::string::npos)return "";
        return path(d+(d.back()=='/'?"":"/")+leaf);
    }
    bool descendant(const std::string& child,const std::string& base){
        if(base.empty())return false;
        std::string b=lower(base);
        if(b.back()!='/')b+='/';
        return lower(child).rfind(b,0)==0;
    }
    bool exists(const std::string& p){
        struct stat st;
        return !p.empty()&&lstat(p.c_str(),&st)==0;
    }
    bool directory(const std::string& p){
        struct stat st;
        return !p.empty()&&lstat(p.c_str(),&st)==0&&S_ISDIR(st.st_mode);
    }
    static bool cancelled(Progress* p,std::string& error){
        if(p&&p->cancel){
            error="Operacion cancelada";
            return true;
        }
        return false;
    }
    bool makeDirs(const std::string& raw,std::string& error){
        auto p=path(raw,true);
        if(p.empty()){
            error="Ruta invalida";
            return false;
        }
        if(directory(p))return true;
        if(p=="sdmc:/"){
            error="La SD no esta disponible";
            return false;
        }
        if(!makeDirs(parent(p),error))return false;
        if(mkdir(p.c_str(),0777)!=0){
            error=ioError("No se pudo crear carpeta");
            return false;
        }
        return true;
    }
    bool list(const std::string& raw,std::vector<Entry>& out,std::string& error) {
        auto p=path(raw);
        if(p.empty()){
            error="Ruta invalida";
            return false;
        }
        DIR* d=opendir(p.c_str());
        if(!d){
            error=ioError("No se pudo abrir carpeta");
            return false;
        }
        bool ok=true;
        for(;;){
            errno=0;
            auto* e=readdir(d);
            if(!e){
                if(errno){
                    error=ioError("Error leyendo carpeta");
                    ok=false;
                }
                break;
            }
            std::string n=e->d_name;
            if(n=="."||n==".."||lower(n)==".folderwifi"||n.rfind(".fw-",0)==0)continue;
            auto full=join(p,n);
            struct stat st;
            if(full.empty()||lstat(full.c_str(),&st)!=0){
                error=ioError("No se pudo consultar elemento");
                ok=false;
                break;
            }
            if(S_ISLNK(st.st_mode))continue;
            out.push_back({
                n,full,S_ISDIR(st.st_mode),static_cast<uint64_t>(std::max<off_t>(0,st.st_size)),st.st_mtime
            });
            if(out.size()>50000){
                error="Carpeta demasiado grande (limite 50000 elementos)";
                ok=false;
                break;
            }
        }
        if(closedir(d)!=0&&ok){
            error=ioError("Error cerrando carpeta");
            ok=false;
        }
        return ok;
    }
    bool inspect(const std::string& raw,uint64_t& bytes,uint64_t& files,uint64_t& dirs,std::string& error,Progress* progress,unsigned depth){
        if(depth>64||files+dirs>100000){
            error="Arbol demasiado grande";
            return false;
        }
        if(cancelled(progress,error))return false;
        auto p=path(raw);
        struct stat st;
        if(p.empty()||lstat(p.c_str(),&st)!=0){
            error="Elemento no disponible";
            return false;
        }
        if(S_ISDIR(st.st_mode)){
            ++dirs;
            std::vector<Entry> entries;
            if(!list(p,entries,error))return false;
            for(auto& e:entries)if(!inspect(e.path,bytes,files,dirs,error,progress,depth+1))return false;
        }else if(S_ISREG(st.st_mode)){
            if(st.st_size<0||UINT64_MAX-bytes<(uint64_t)st.st_size){
                error="Tamano invalido";
                return false;
            }
            bytes+=st.st_size;
            ++files;
        }else{
            error="Tipo de archivo no permitido";
            return false;
        }
        return true;
    }
    bool freeSpace(uint64_t required,std::string& error){
        struct statvfs st;
        if(statvfs("sdmc:/",&st)!=0){
            error=ioError("No se pudo consultar espacio libre");
            return false;
        }
        uint64_t available=static_cast<uint64_t>(st.f_bavail)*st.f_frsize;
        if(required>available||available-required<1024*1024){
            error="Espacio insuficiente en la SD";
            return false;
        }
        return true;
    }
    std::string availableName(const std::string& target){
        if(!exists(target))return target;
        std::string dir=parent(target),n=name(target),stem=n,ext;
        auto dot=n.find_last_of('.');
        if(!directory(target)&&dot!=std::string::npos&&dot>0){
            stem=n.substr(0,dot);
            ext=n.substr(dot);
        }
        for(int i=1;i<10000;++i){
            auto p=join(dir,stem+" ("+std::to_string(i)+")"+ext);
            if(!p.empty()&&!exists(p))return p;
        }
        return "";
    }
    bool publish(const std::string& temp,const std::string& dest,bool replace,std::string& error){
        if(!exists(dest)){
            if(rename(temp.c_str(),dest.c_str())==0)return true;
            error=ioError("No se pudo publicar archivo");
            return false;
        }
        if(!replace||directory(dest)){
            error="El destino ya existe o es una carpeta";
            return false;
        }
        auto token=randomToken(16);
        if(token.empty()){
            error="No se pudo generar respaldo seguro";
            return false;
        }
        auto backup=path(parent(dest)+"/.fw-backup-"+token,true);
        if(backup.empty()){
            error="Ruta demasiado larga para un reemplazo seguro";
            return false;
        }
        // El respaldo queda disponible para recuperar ante un cierre abrupto.
        std::string manifest=std::string(stateDir)+"/recovery-"+token;
        if(!writeAtomic(manifest,dest+"\n"+backup+"\n"+temp,error))return false;
        if(rename(dest.c_str(),backup.c_str())!=0){
            unlink(manifest.c_str());
            error=ioError("No se pudo proteger destino anterior");
            return false;
        }
        if(rename(temp.c_str(),dest.c_str())!=0){
            error=ioError("No se pudo reemplazar");
            if(rename(backup.c_str(),dest.c_str())!=0)error+="; respaldo conservado para recuperacion";
            else unlink(manifest.c_str());
            return false;
        }
        if(unlink(backup.c_str())==0)unlink(manifest.c_str());
        return true;
    }
    bool writeAtomic(const std::string& dest,const std::string& data,std::string& error){
        if(!makeDirs(parent(dest),error))return false;
        auto id=randomToken(16);
        if(id.empty()){
            error="Aleatoriedad no disponible";
            return false;
        }
        auto temp=path(parent(dest)+"/.fw-write-"+id,true);
        if(temp.empty()){
            error="Ruta demasiado larga para guardar de forma segura";
            return false;
        }
        int fd=open(temp.c_str(),O_CREAT|O_EXCL|O_WRONLY,0600);
        if(fd<0){
            error=ioError("No se pudo crear temporal");
            return false;
        }
        size_t pos=0;
        bool ok=true;
        while(pos<data.size()){
            ssize_t n=write(fd,data.data()+pos,data.size()-pos);
            if(n<0&&errno==EINTR)continue;
            if(n<=0){
                ok=false;
                break;
            }
            pos+=n;
        }
        if(fsync(fd)!=0)ok=false;
        if(close(fd)!=0)ok=false;
        if(ok){
            auto backup=dest+".previous";
            if(exists(backup)&&!exists(dest))rename(backup.c_str(),dest.c_str());
            if(exists(dest)){
                unlink(backup.c_str());
                ok=rename(dest.c_str(),backup.c_str())==0;
            }
            if(ok&&rename(temp.c_str(),dest.c_str())==0){
                unlink(backup.c_str());
                return true;
            }
            if(!exists(dest)&&exists(backup))rename(backup.c_str(),dest.c_str());
        }
        error=ioError("No se pudo guardar configuracion");
        unlink(temp.c_str());
        return false;
    }
    std::string readSmall(const std::string& p,size_t limit){
        FILE* f=fopen(p.c_str(),"rb");
        if(!f)return "";
        std::string out;
        char b[1024];
        size_t n;
        while((n=fread(b,1,sizeof(b),f))>0){
            if(out.size()+n>limit){
                out.clear();
                break;
            }
            out.append(b,n);
        }
        if(ferror(f))out.clear();
        fclose(f);
        return out;
    }
    bool copy(const std::string& rawSrc,const std::string& rawDst,const std::string& requested,std::string& error,Progress* progress,unsigned depth){
        std::string policy=requested;
        auto src=path(rawSrc),dst=path(rawDst);
        if(policy=="keep"&&!src.empty()&&lower(src)==lower(dst))dst=availableName(dst);
        if(src.empty()||dst.empty()||src=="sdmc:/"||dst=="sdmc:/"||lower(src)==lower(dst)||descendant(dst,src)){
            error="Origen/destino invalido o dentro del origen";
            return false;
        }
        bool replacing=exists(dst);
        if(depth>64||cancelled(progress,error)){
            if(error.empty())error="Demasiados niveles";
            return false;
        }
        struct stat st;
        if(lstat(src.c_str(),&st)!=0){
            error=ioError("Origen no disponible");
            return false;
        }
        if(S_ISLNK(st.st_mode)){
            error="No se admiten enlaces simbolicos";
            return false;
        }
        if(exists(dst)){
            if(policy=="ask"&&progress&&progress->resolve)policy=progress->resolve(dst);
            if(policy=="skip"){
                if(progress)++progress->skipped;
                return true;
            }
            if(policy=="keep")dst=availableName(dst);
            else if(policy!="replace"&&policy!="merge"){
                error="conflict";
                return false;
            }
        }
        if(dst.empty()){
            error="No hay un nombre libre";
            return false;
        }
        if(S_ISDIR(st.st_mode)){
            if(exists(dst)&&!directory(dst)){
                error="conflict";
                return false;
            }
            if(!makeDirs(dst,error))return false;
            std::vector<Entry> entries;
            if(!list(src,entries,error))return false;
            for(auto& e:entries){
                auto child=join(dst,e.name);
                if(!copy(e.path,child,requested,error,progress,depth+1))return false;
            }
            return true;
        }
        if(!S_ISREG(st.st_mode)||directory(dst)){
            error="Tipo de destino incompatible";
            return false;
        }
        if(!freeSpace(st.st_size,error))return false;
        if(!makeDirs(parent(dst),error))return false;
        auto id=randomToken(16);
        if(id.empty()){
            error="Aleatoriedad no disponible";
            return false;
        }
        auto temp=path(parent(dst)+"/.fw-copy-"+id,true);
        if(temp.empty()){
            error="Ruta demasiado larga para una copia segura";
            return false;
        }
        FILE* in=fopen(src.c_str(),"rb");
        if(!in){
            error=ioError("No se pudo leer origen");
            return false;
        }
        int fd=open(temp.c_str(),O_CREAT|O_EXCL|O_WRONLY,0600);
        FILE* out=fd<0?nullptr:fdopen(fd,"wb");
        if(!out){
            if(fd>=0)close(fd);
            fclose(in);
            unlink(temp.c_str());
            error=ioError("No se pudo crear copia");
            return false;
        }
        std::vector<char> buffer(65536);
        bool ok=true;
        uint64_t written=0;
        for(;;){
            if(cancelled(progress,error)){
                ok=false;
                break;
            }
            size_t n=fread(buffer.data(),1,buffer.size(),in);
            if(!n)break;
            if(fwrite(buffer.data(),1,n,out)!=n){
                error="Error escribiendo copia";
                ok=false;
                break;
            }
            written+=n;
            if(progress)progress->bytes+=n;
        }
        if(ferror(in)||ferror(out)||written!=(uint64_t)st.st_size){
            error="Error leyendo/escribiendo copia";
            ok=false;
        }
        if(fclose(in)!=0)ok=false;
        if(fflush(out)!=0)ok=false;
        if(fsync(fileno(out))!=0)ok=false;
        if(fclose(out)!=0)ok=false;
        if(ok&&!cancelled(progress,error))ok=publish(temp,dst,policy=="replace"||policy=="merge",error);
        else ok=false;
        if(!ok){
            unlink(temp.c_str());
            if(error.empty())error="No se pudo completar la copia";
        }else{
            struct utimbuf times={
                st.st_atime,st.st_mtime
            };
            utime(dst.c_str(),&times);
            if(progress){
                ++progress->files;
                if(replacing)++progress->replaced;
            }
        }
        return ok;
    }
    bool removeTree(const std::string& raw,std::string& error,Progress* progress,unsigned depth){
        auto p=path(raw,true);
        if(p.empty()||p=="sdmc:/"||depth>64){
            error="Ruta de borrado invalida";
            return false;
        }
        if(cancelled(progress,error))return false;
        struct stat st;
        if(lstat(p.c_str(),&st)!=0){
            error=ioError("No se pudo consultar elemento");
            return false;
        }
        if(S_ISDIR(st.st_mode)){
            DIR* d=opendir(p.c_str());
            if(!d){
                error=ioError("No se pudo abrir carpeta");
                return false;
            }
            bool ok=true;
            for(;;){
                errno=0;
                auto* e=readdir(d);
                if(!e){
                    if(errno)ok=false;
                    break;
                }
                std::string n=e->d_name;
                if(n=="."||n=="..")continue;
                if(!removeTree(p+"/"+n,error,progress,depth+1)){
                    ok=false;
                    break;
                }
            }
            if(closedir(d)!=0)ok=false;
            if(!ok){
                if(error.empty())error="Error leyendo carpeta";
                return false;
            }
            if(rmdir(p.c_str())!=0){
                error=ioError("No se pudo eliminar carpeta");
                return false;
            }
        }else{
            if(unlink(p.c_str())!=0){
                error=ioError("No se pudo eliminar archivo");
                return false;
            }
            if(progress){
                progress->bytes+=std::max<off_t>(0,st.st_size);
                ++progress->files;
            }
        }
        return true;
    }
    bool move(const std::string& a,const std::string& b,const std::string& requested,std::string& error,Progress* progress){
        std::string policy=requested;
        auto src=path(a),dst=path(b);
        if(src.empty()||dst.empty()||src=="sdmc:/"||src==dst||descendant(dst,src)){
            error="Movimiento invalido";
            return false;
        }
        if(cancelled(progress,error))return false;
        if(lower(src)==lower(dst)&&src!=dst){
            auto id=randomToken(16);
            if(id.empty()){
                error="Aleatoriedad no disponible";
                return false;
            }
            auto temp=path(parent(src)+"/.fw-rename-"+id,true);
            if(temp.empty()){
                error="Ruta demasiado larga para renombrar de forma segura";
                return false;
            }
            auto manifest=std::string(stateDir)+"/recovery-"+id;
            if(!writeAtomic(manifest,dst+"\n"+temp+"\n"+temp,error))return false;
            if(rename(src.c_str(),temp.c_str())!=0){
                unlink(manifest.c_str());
                error=ioError("No se pudo renombrar");
                return false;
            }
            if(rename(temp.c_str(),dst.c_str())!=0){
                rename(temp.c_str(),src.c_str());
                error=ioError("No se pudo renombrar");
                return false;
            }
            unlink(manifest.c_str());
            if(progress)++progress->files;
            return true;
        }
        if(exists(dst)){
            if(policy=="ask"&&progress&&progress->resolve)policy=progress->resolve(dst);
            if(policy=="skip"){
                if(progress)++progress->skipped;
                return true;
            }
            if(policy=="keep")dst=availableName(dst);
            else if(directory(src)&&directory(dst)&&(policy=="merge"||policy=="replace")){
                std::vector<Entry> entries;
                if(!list(src,entries,error))return false;
                for(auto& e:entries)if(!move(e.path,join(dst,e.name),requested,error,progress))return false;
                std::vector<Entry> remaining;
                if(!list(src,remaining,error))return false;
                if(!remaining.empty())return true;
                if(rmdir(src.c_str())!=0){
                    error=ioError("No se pudo cerrar movimiento de carpeta");
                    return false;
                }
                return true;
            }else if(policy=="replace"&&!directory(src)&&!directory(dst)){
                bool ok=publish(src,dst,true,error);
                if(ok&&progress){
                    ++progress->files;
                    ++progress->replaced;
                }
                return ok;
            }else{
                error="conflict";
                return false;
            }
        }
        if(dst.empty()||!exists(src)){
            error="Origen o destino no disponible";
            return false;
        }
        if(!makeDirs(parent(dst),error))return false;
        if(rename(src.c_str(),dst.c_str())!=0){
            error=ioError("No se pudo mover");
            return false;
        }
        if(progress)++progress->files;
        return true;
    }
    bool trash(const std::string& raw,std::string& id,std::string& error){
        auto p=path(raw);
        if(p.empty()||p=="sdmc:/"){
            error="Ruta invalida";
            return false;
        }
        id=randomToken(24);
        if(id.empty()){
            error="Aleatoriedad no disponible";
            return false;
        }
        auto base=std::string(stateDir)+"/trash/"+id;
        if(!makeDirs(base,error)||!writeAtomic(base+"/original",p,error))return false;
        if(rename(p.c_str(),(base+"/item").c_str())!=0){
            error=ioError("No se pudo enviar a papelera");
            return false;
        }
        return true;
    }
    bool restore(const std::string& id,const std::string& policy,std::string& error){
        if(id.size()!=24||id.find_first_not_of("0123456789abcdef")!=std::string::npos){
            error="Identificador invalido";
            return false;
        }
        auto base=std::string(stateDir)+"/trash/"+id;
        auto dest=path(readSmall(base+"/original"));
        if(dest.empty()){
            error="Registro de papelera invalido";
            return false;
        }
        if(exists(dest)){
            if(policy=="keep")dest=availableName(dest);
            else{
                error="El destino ya existe; elige conservar ambos";
                return false;
            }
        }
        if(dest.empty()||!makeDirs(parent(dest),error))return false;
        if(rename((base+"/item").c_str(),dest.c_str())!=0){
            error=ioError("No se pudo restaurar");
            return false;
        }
        unlink((base+"/original").c_str());
        rmdir(base.c_str());
        return true;
    }
    std::vector<std::string> deduplicate(const std::vector<std::string>& raw){
        std::vector<std::string> out;
        for(auto& a:raw){
            auto p=path(a);
            if(p.empty()||p=="sdmc:/")continue;
            bool covered=false;
            for(auto& q:raw){
                auto other=path(q);
                if(!other.empty()&&lower(p)!=lower(other)&&descendant(p,other)){
                    covered=true;
                    break;
                }
            }
            if(!covered&&std::none_of(out.begin(),out.end(),[&](const std::string& b){
                return lower(b)==lower(p);
            }))out.push_back(p);
        }
        return out;
    }
}
