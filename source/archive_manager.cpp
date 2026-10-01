#include "archive_manager.hpp"
#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <set>
#include <sys/stat.h>
#include <unistd.h>
#include <utime.h>
#include <minizip/zip.h>
#include <minizip/unzip.h>
namespace fw {
    struct ZipItem {
        std::string source,entry;
        bool directory;
        uint64_t size;
        time_t modified;
    };
    static bool collect(const std::string& src,const std::string& relative,std::vector<ZipItem>& items,std::set<std::string>& names,std::string& error,unsigned depth,Progress* p){
        if(depth>64||items.size()>100000||(p&&p->cancel)){
            error="ZIP cancelado o limite de elementos alcanzado";
            return false;
        }
        struct stat st;
        if(lstat(src.c_str(),&st)!=0||(!S_ISREG(st.st_mode)&&!S_ISDIR(st.st_mode))){
            error="Origen ZIP no disponible";
            return false;
        }
        if(!names.insert(lower(relative)).second){
            error="La seleccion produce nombres ZIP duplicados: "+relative;
            return false;
        }
        items.push_back({
            src,relative,S_ISDIR(st.st_mode),static_cast<uint64_t>(std::max<off_t>(0,st.st_size)),st.st_mtime
        });
        if(S_ISDIR(st.st_mode)){
            std::vector<Entry> children;
            if(!list(src,children,error))return false;
            for(auto& c:children)if(!collect(c.path,relative+"/"+c.name,items,names,error,depth+1,p))return false;
        }
        return true;
    }
    bool makeZip(const std::vector<std::string>& raw,const std::string& output,const std::string& requested,std::string& error,Progress* p){
        std::string policy=requested;
        auto sources=deduplicate(raw);
        auto dest=path(output,true);
        if(dest.empty()||sources.empty()){
            error="Seleccion o destino ZIP invalido";
            return false;
        }
        for(auto& s:sources)if(lower(s)==lower(dest)||descendant(dest,s)){
            error="El ZIP no puede guardarse dentro de sus origenes";
            return false;
        }
        if(exists(dest)){
            if(policy=="ask"&&p&&p->resolve)policy=p->resolve(dest);
            if(policy=="skip"){
                if(p)++p->skipped;
                return true;
            }
            if(policy=="keep")dest=availableName(dest);
            else if(policy!="replace"){
                error="conflict";
                return false;
            }
        }
        if(dest.empty()||!makeDirs(parent(dest),error))return false;
        std::vector<ZipItem> items;
        std::set<std::string> names;
        for(auto& s:sources)if(!collect(s,name(s),items,names,error,0,p))return false;
        uint64_t total=0;
        for(auto& i:items)if(!i.directory){
            if(UINT64_MAX-total<i.size){
                error="ZIP demasiado grande";
                return false;
            }
            total+=i.size;
        }
        if(p)p->total=total;
        if(total>UINT64_MAX-items.size()*256||!freeSpace(total+items.size()*256,error))return false;
        auto id=randomToken(16);
        if(id.empty()){
            error="Aleatoriedad no disponible";
            return false;
        }
        auto temp=path(parent(dest)+"/.fw-zip-"+id,true);
        if(temp.empty()){
            error="Ruta demasiado larga para crear ZIP";
            return false;
        }
        int fd=open(temp.c_str(),O_CREAT|O_EXCL|O_WRONLY,0600);
        if(fd<0){
            error="No se pudo crear temporal ZIP";
            return false;
        }
        close(fd);
        zipFile zip=zipOpen64(temp.c_str(),APPEND_STATUS_CREATE);
        if(!zip){
            unlink(temp.c_str());
            error="No se pudo abrir ZIP";
            return false;
        }
        bool ok=true;
        for(auto& i:items){
            if(p&&p->cancel){
                error="Compresion cancelada";
                ok=false;
                break;
            }
            zip_fileinfo info={
            };
            tm t={
            };
            localtime_r(&i.modified,&t);
            info.tmz_date={
                t.tm_sec,t.tm_min,t.tm_hour,t.tm_mday,t.tm_mon,t.tm_year+1900
            };
            auto entry=i.entry+(i.directory?"/":"");
            if(zipOpenNewFileInZip4_64(zip,entry.c_str(),&info,nullptr,0,nullptr,0,nullptr,i.directory?0:Z_DEFLATED,Z_DEFAULT_COMPRESSION,0,-MAX_WBITS,8,Z_DEFAULT_STRATEGY,nullptr,0,0,1<<11,i.size>=0xffffffffULL)!=ZIP_OK){
                error="No se pudo escribir entrada ZIP";
                ok=false;
                break;
            }
            if(!i.directory){
                FILE* f=fopen(i.source.c_str(),"rb");
                if(!f){
                    error="No se pudo leer archivo para ZIP";
                    ok=false;
                }else{
                    char b[65536];
                    uint64_t read=0;
                    while(ok){
                        if(p&&p->cancel){
                            error="Compresion cancelada";
                            ok=false;
                            break;
                        }
                        size_t n=fread(b,1,sizeof(b),f);
                        if(!n)break;
                        if(zipWriteInFileInZip(zip,b,n)!=ZIP_OK){
                            error="Error escribiendo ZIP";
                            ok=false;
                            break;
                        }
                        read+=n;
                        if(p)p->bytes+=n;
                    }
                    if(ferror(f)||read!=i.size){
                        error="Error leyendo origen ZIP";
                        ok=false;
                    }
                    if(fclose(f)!=0)ok=false;
                }
            }
            if(zipCloseFileInZip(zip)!=ZIP_OK){
                error="Error cerrando entrada ZIP";
                ok=false;
            }
            if(!ok)break;
            if(p)++p->files;
        }
        if(zipClose(zip,nullptr)!=ZIP_OK){
            error="Error guardando ZIP";
            ok=false;
        }
        if(ok)ok=publish(temp,dest,policy=="replace",error);
        if(!ok){
            unlink(temp.c_str());
            if(error.empty())error="No se pudo finalizar ZIP";
        }
        return ok;
    }
    bool extractZip(const std::string& raw,const std::string& rawDest,const std::string& requested,std::string& error,Progress* p){
        std::string policy=requested;
        auto src=path(raw),dest=path(rawDest);
        if(src.empty()||dest.empty()||!directory(dest)){
            error="Origen o destino de extraccion invalido";
            return false;
        }
        unzFile zip=unzOpen64(src.c_str());
        if(!zip){
            error="ZIP invalido";
            return false;
        }
        unz_global_info64 global={
        };
        if(unzGetGlobalInfo64(zip,&global)!=UNZ_OK||global.number_entry>100000){
            unzClose(zip);
            error="ZIP demasiado grande o invalido";
            return false;
        }
        // Prevalidar todo el directorio central antes de escribir: Zip Slip, enlaces,
        // entradas ambiguas, rutas reservadas y tamano total.
        std::vector<std::string> targets;
        std::set<std::string> seen;
        uint64_t total=0;
        bool ok=true;
        int rc=unzGoToFirstFile(zip);
        for(uint64_t index=0;index<global.number_entry;++index){
            unz_file_info64 info={
            };
            char n[1025]={
            };
            if(rc!=UNZ_OK||unzGetCurrentFileInfo64(zip,&info,n,sizeof(n),nullptr,0,nullptr,0)!=UNZ_OK||info.size_filename==0||info.size_filename>=sizeof(n)||strlen(n)!=info.size_filename){
                error="Entrada ZIP invalida";
                ok=false;
                break;
            }
            std::string leaf=n;
            if(leaf.front()=='/'||leaf.find('\\')!=std::string::npos||((info.external_fa>>16)&0170000)==0120000||(info.flag&1)){
                error="ZIP con ruta absoluta, enlace o cifrado no admitido";
                ok=false;
                break;
            }
            auto target=path(dest+(dest.back()=='/'?"":"/")+leaf);
            if(target.empty()||!descendant(target,dest)||!seen.insert(lower(target)).second||lower(target)==lower(src)){
                error="Ruta peligrosa o duplicada en ZIP";
                ok=false;
                break;
            }
            if(UINT64_MAX-total<info.uncompressed_size){
                error="Tamano ZIP invalido";
                ok=false;
                break;
            }
            total+=info.uncompressed_size;
            targets.push_back(target);
            rc=unzGoToNextFile(zip);
        }
        if(ok)ok=freeSpace(total,error);
        if(p)p->total=total;
        if(!ok){
            unzClose(zip);
            return false;
        }
        rc=unzGoToFirstFile(zip);
        for(size_t index=0;index<targets.size();++index){
            if(p&&p->cancel){
                error="Extraccion cancelada";
                ok=false;
                break;
            }
            unz_file_info64 info={
            };
            char n[1025]={
            };
            if(rc!=UNZ_OK||unzGetCurrentFileInfo64(zip,&info,n,sizeof(n),nullptr,0,nullptr,0)!=UNZ_OK){
                error="ZIP cambiado o corrupto";
                ok=false;
                break;
            }
            auto target=targets[index];
            bool dir=std::string(n).back()=='/';
            if(dir){
                if(exists(target)&&!directory(target)){
                    error="conflict";
                    ok=false;
                    break;
                }
                if(!makeDirs(target,error)){
                    ok=false;
                    break;
                }
            } else{
                policy=requested;
                if(exists(target)){
                    if(policy=="ask"&&p&&p->resolve)policy=p->resolve(target);
                    if(policy=="skip"){
                        if(p)++p->skipped;
                        rc=unzGoToNextFile(zip);
                        continue;
                    }
                    if(policy=="keep")target=availableName(target);
                    else if(policy!="replace"&&policy!="merge"){
                        error="conflict";
                        ok=false;
                        break;
                    }
                }
                if(target.empty()||directory(target)||!makeDirs(parent(target),error)){
                    error="Destino incompatible";
                    ok=false;
                    break;
                }
                auto id=randomToken(16);
                if(id.empty()){
                    error="Aleatoriedad no disponible";
                    ok=false;
                    break;
                }
                auto temp=path(parent(target)+"/.fw-extract-"+id,true);
                if(temp.empty()){
                    error="Ruta demasiado larga para extraer de forma segura";
                    ok=false;
                    break;
                }
                int fd=open(temp.c_str(),O_CREAT|O_EXCL|O_WRONLY,0600);
                FILE* out=fd<0?nullptr:fdopen(fd,"wb");
                if(!out){
                    if(fd>=0)close(fd);
                    error="No se pudo crear temporal";
                    ok=false;
                    break;
                }
                if(unzOpenCurrentFile(zip)!=UNZ_OK){
                    fclose(out);
                    unlink(temp.c_str());
                    error="No se pudo abrir entrada ZIP";
                    ok=false;
                    break;
                }
                char b[65536];
                uint64_t written=0;
                for(;;){
                    if(p&&p->cancel){
                        error="Extraccion cancelada";
                        ok=false;
                        break;
                    }
                    int count=unzReadCurrentFile(zip,b,sizeof(b));
                    if(count<0){
                        error="ZIP corrupto";
                        ok=false;
                        break;
                    }
                    if(!count)break;
                    if(written+count>info.uncompressed_size||fwrite(b,1,count,out)!=(size_t)count){
                        error="Error escribiendo extraccion";
                        ok=false;
                        break;
                    }
                    written+=count;
                    if(p)p->bytes+=count;
                }
                if(unzCloseCurrentFile(zip)!=UNZ_OK||written!=info.uncompressed_size){
                    error="Integridad ZIP incorrecta";
                    ok=false;
                }
                if(fflush(out)!=0)ok=false;
                if(fsync(fileno(out))!=0)ok=false;
                if(fclose(out)!=0)ok=false;
                if(ok)ok=publish(temp,target,policy=="replace"||policy=="merge",error);
                if(!ok){
                    unlink(temp.c_str());
                    if(error.empty())error="No se pudo guardar extraccion";
                    break;
                }
                tm t={
                };
                t.tm_sec=info.tmu_date.tm_sec;
                t.tm_min=info.tmu_date.tm_min;
                t.tm_hour=info.tmu_date.tm_hour;
                t.tm_mday=info.tmu_date.tm_mday;
                t.tm_mon=info.tmu_date.tm_mon;
                t.tm_year=info.tmu_date.tm_year-1900;
                time_t date=mktime(&t);
                if(date!=-1){
                    utimbuf times={
                        date,date
                    };
                    utime(target.c_str(),&times);
                }
                if(p)++p->files;
            }
            rc=unzGoToNextFile(zip);
        }
        if(unzClose(zip)!=UNZ_OK){
            error="No se pudo cerrar ZIP";
            ok=false;
        }
        return ok;
    }
}
