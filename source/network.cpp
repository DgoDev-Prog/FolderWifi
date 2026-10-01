#include "runtime.hpp"
#include "notification_manager.hpp"
#include <arpa/inet.h>
#include <sys/socket.h>
#include <cstring>
#include <cstdio>
namespace fw {
    Mutex stateMutex,fsMutex;
    std::atomic<bool> stopping{
        false
    };
    std::atomic<bool> serverReady{
        false
    };
    std::atomic<bool> writeEnabled{
        false
    };
    std::atomic<int> networkCommand{
        0
    };
    Network network;
    std::string accessKey,pairCode;
    std::deque<std::string> logs;
    static Thread worker;
    static bool started=false;
    void log(const std::string& s,const std::string& type){
        notify(type,s);
        Guard g(stateMutex);
        logs.push_back(s);
        while(logs.size()>500)logs.pop_front();
    }
    Network networkSnapshot(){
        Guard g(stateMutex);
        return network;
    }
    static void setNetwork(const Network& n){
        Guard g(stateMutex);
        network=n;
    }
    static std::string address(u32 ip){
        char b[INET_ADDRSTRLEN];
        in_addr a{
            ip
        };
        return inet_ntop(AF_INET,&a,b,sizeof(b))?b:"";
    }
    static std::string securePassword(){
        u8 bytes[20];
        if(R_FAILED(csrngGetRandomBytes(bytes,sizeof(bytes))))return "";
        constexpr char alphabet[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
        std::string out;
        for(auto b:bytes)out+=alphabet[b&63];
        return out;
    }
    static void refreshDevices(Network& n){
        Lp2pNodeInfo members[8]{};
        s32 count=0;
        Result r=lp2pGetMembers(members,8,&count);
        n.devices.clear();
        n.devicesKnown=false;
        n.devicesError.clear();
        if(R_FAILED(r)||count<0||count>8){
            char b[100];
            if(R_FAILED(r))snprintf(b,sizeof(b),"Consultar dispositivos: %08X (%04u-%04u)",static_cast<unsigned>(r),2000+static_cast<unsigned>(R_MODULE(r)),static_cast<unsigned>(R_DESCRIPTION(r)));
            else snprintf(b,sizeof(b),"El servicio devolvio una cantidad de dispositivos invalida");
            n.devicesError=b;
            return;
        }
        // Excluir la propia consola si el servicio la incluye entre los miembros.
        Lp2pNodeInfo owner{};
        const bool ownerKnown=R_SUCCEEDED(lp2pGetGroupOwner(&owner));
        for(s32 i=0;i<count;++i){
            if(ownerKnown&&memcmp(members[i].mac_addr.addr,owner.mac_addr.addr,sizeof(owner.mac_addr.addr))==0)continue;
            sockaddr_in sa{};
            memcpy(&sa,members[i].ip_addr,sizeof(sa));
            ConnectedDevice device;
            if(sa.sin_family==AF_INET)device.ip=address(sa.sin_addr.s_addr);
            if(!n.ip.empty()&&n.ip==device.ip)continue;
            if(device.ip=="0.0.0.0")device.ip.clear();
            char mac[18];
            const auto* bytes=members[i].mac_addr.addr;
            snprintf(mac,sizeof(mac),"%02X:%02X:%02X:%02X:%02X:%02X",static_cast<unsigned>(bytes[0]),static_cast<unsigned>(bytes[1]),static_cast<unsigned>(bytes[2]),static_cast<unsigned>(bytes[3]),static_cast<unsigned>(bytes[4]),static_cast<unsigned>(bytes[5]));
            device.mac=mac;
            n.devices.push_back(device);
        }
        n.devicesKnown=true;
    }
    static void run(void*){
        Network n;
        std::string error;
        bool lp=false,ap=false;
        NifmRequest request{
        };
        bool req=false;
        int attempts=0,apFailures=0;
        bool needsGroupInfo=false;
        uint64_t next=0,deadline=0;
        bool submitted=false,declined=false,pendingSave=false,hasCachedGroup=false;
        Lp2pGroupInfo cachedGroup{
        };
        uint64_t nextSave=0;
        uint64_t nextDevices=0;
        // Perfil persistente por consola. No se publica en el registro ni en el HTTP.
        {
            Guard g(fsMutex);
            makeDirs(stateDir,error);
            auto config=readSmall(std::string(stateDir)+"/wifi");
            auto split=config.find('\n');
            if(split!=std::string::npos){
                n.ssid=config.substr(0,split);
                n.password=config.substr(split+1);
            }
            if(n.ssid.empty()||n.password.size()<8){
                n.ssid="FolderWifi-"+randomToken(6);
                n.password=securePassword();
                if(n.password.empty()||!writeAtomic(std::string(stateDir)+"/wifi",n.ssid+"\n"+n.password,error)){
                    n.message="No se pudo guardar la configuracion Wi-Fi: "+error;
                    setNetwork(n);
                    return;
                }
            }
        }
        {
            Guard g(fsMutex);
            auto saved=readSmall(std::string(stateDir)+"/wifi-group");
            if(saved.size()==sizeof(cachedGroup)){
                memcpy(&cachedGroup,saved.data(),sizeof(cachedGroup));
                cachedGroup.service_name[sizeof(cachedGroup.service_name)-1]=0;
                hasCachedGroup=true;
                n.hasProfile=true;
                n.ssid=cachedGroup.service_name;
            }
        }
        Result r=nifmCreateRequest(&request,false);
        req=R_SUCCEEDED(r);
        u32 initialIp=0;
        n.offer=R_FAILED(nifmGetCurrentIpAddress(&initialIp))||initialIp==0;
        n.message=n.offer?"Sin red local. Puedes crear una red local":"Buscando conexion local";
        setNetwork(n);
        while(!stopping){
            int command=networkCommand.exchange(0);
            uint64_t now=armGetSystemTick()/armGetSystemTickFreq();
            if(command==1||command==3||command==4){
                n.devices.clear();
                n.devicesKnown=false;
                n.devicesError.clear();
                nextDevices=0;
            }
            if(command==3){
                n.localError.clear();
                declined=false;
                if(ap){
                    lp2pDestroyGroup();
                    ap=false;
                    n.local=false;
                }
                if(lp){
                    lp2pExit();
                    lp=false;
                }
                n.online=false;
                n.ip.clear();
                attempts=0;
                next=0;
                n.offer=false;
                submitted=false;
                log("Buscando nuevamente una red conocida");
            }
            if(command==4){
                if(!mutexTryLock(&fsMutex)){
                    n.message="Esperando la SD para cambiar la clave Wi-Fi";
                    setNetwork(n);
                    networkCommand=4;
                    svcSleepThread(250000000);
                    continue;
                }
                auto password=securePassword();
                bool saved=!password.empty()&&writeAtomic(std::string(stateDir)+"/wifi",n.ssid+"\n"+password,error);
                mutexUnlock(&fsMutex);
                if(!saved){
                    n.message="No se pudo cambiar la clave: "+error;
                    log(n.message,"error");
                    setNetwork(n);
                    svcSleepThread(250000000);
                    continue;
                }
                n.password=password;
                if(ap)lp2pDestroyGroup();
                ap=false;
                n.online=false;
                n.local=false;
                n.ip.clear();
                command=1;
            }
            if(command==2){
                declined=true;
                n.offer=false;
            }
            if(command==1){
                n.localError.clear();
                if(ap){
                    lp2pDestroyGroup();
                    ap=false;
                    n.local=false;
                }
                n.offer=false;
                if(req)nifmRequestCancel(&request);
                submitted=false;
                n.online=false;
                n.ip.clear();
                if(!hosversionAtLeast(11,0,0)){
                    n.message="Wi-Fi local WPA2 requiere sistema 11.0 o posterior";
                    n.localError=n.message;
                    log(n.message);
                    deadline=UINT64_MAX;
                    attempts=5;
                }else{
                    const char* failedStep="iniciar servicio";
                    if(!lp){
                        r=lp2pInitialize(Lp2pServiceType_App);
                        lp=R_SUCCEEDED(r);
                    }else r=0;
                    if(R_SUCCEEDED(r)){
                        Lp2pGroupInfo info;
                        lp2pCreateGroupInfo(&info);
                        if(hasCachedGroup)info=cachedGroup;
                        else lp2pGroupInfoSetServiceName(&info,n.ssid.substr(0,n.ssid.find('_')).c_str());
                        memset(&info.group_id,0,sizeof(info.group_id));
                        // El identificador permitido depende del anfitrion de este arranque.
                        info.local_communication_id=0;
                        s8 flag=0;
                        lp2pGroupInfoSetFlags(&info,&flag,1);
                        info.security_type=3;
                        // Un cliente: valor conservador para comprobar compatibilidad real.
                        lp2pGroupInfoSetMemberCountMax(&info,1);
                        lp2pGroupInfoSetFrequencyChannel(&info,24,0);
                        failedStep="configurar clave";
                        r=lp2pGroupInfoSetPassphrase(&info,n.password.c_str());
                        if(R_SUCCEEDED(r)){
                            failedStep="crear red";
                            log("LP2P: crear red, limite="+std::to_string(static_cast<int>(info.member_count_max))+", tipo de inicio="+std::to_string(static_cast<int>(appletGetAppletType())));
                            r=lp2pCreateGroup(&info);
                        }
                        if(R_SUCCEEDED(r)){
                            ap=true;
                            apFailures=0;
                            needsGroupInfo=true;
                            n.local=true;
                            Lp2pGroupInfo actual;
                            if(R_SUCCEEDED(lp2pGetGroupInfo(&actual))){
                                actual.service_name[sizeof(actual.service_name)-1]=0;
                                n.ssid=actual.service_name;
                                cachedGroup=actual;
                                hasCachedGroup=true;
                                n.hasProfile=true;
                                needsGroupInfo=false;
                                pendingSave=true;
                            }
                            log("Red local WPA2 creada");
                        }
                    }
                    if(R_FAILED(r)){
                        char b[160];
                        snprintf(b,sizeof(b),"LP2P %s: %08X (%04u-%04u)",failedStep,static_cast<unsigned>(r),2000+static_cast<unsigned>(R_MODULE(r)),static_cast<unsigned>(R_DESCRIPTION(r)));
                        n.localError=b;
                        n.message=b;
                        log(n.message,"error");
                        deadline=UINT64_MAX;
                        attempts=5;
                    }
                }
            }
            if(ap){
                if(needsGroupInfo){
                    Lp2pGroupInfo actual{
                    };
                    if(R_SUCCEEDED(lp2pGetGroupInfo(&actual))){
                        actual.service_name[sizeof(actual.service_name)-1]=0;
                        n.ssid=actual.service_name;
                        cachedGroup=actual;
                        hasCachedGroup=true;
                        n.hasProfile=true;
                        needsGroupInfo=false;
                        pendingSave=true;
                    }
                }
                Lp2pIpConfig cfg;
                u8 role=0;
                r=lp2pGetRole(&role);
                if(R_SUCCEEDED(r)&&role==1&&R_SUCCEEDED(lp2pGetIpConfig(&cfg))){
                    sockaddr_in sa;
                    memcpy(&sa,cfg.ip_addr,sizeof(sa));
                    n.ip=address(sa.sin_addr.s_addr);
                    n.online=!n.ip.empty()&&n.ip!="0.0.0.0";
                    apFailures=0;
                    n.message=n.online?"Red local activa, sin necesidad de Internet":"Esperando IP de la red local";
                }else if(++apFailures>=40){
                    lp2pDestroyGroup();
                    ap=false;
                    n.local=false;
                    n.online=false;
                    n.ip.clear();
                    declined=false;
                    attempts=0;
                    next=0;
                    deadline=UINT64_MAX;
                    n.offer=true;
                    n.message="Se perdio la red local";
                    log(n.message,"warning");
                }else{
                    n.message="Comprobando el estado de la red local";
                }
            } else {
                u32 ip=0;
                bool online=R_SUCCEEDED(nifmGetCurrentIpAddress(&ip))&&ip!=0;
                if(online){
                    auto newIp=address(ip);
                    if(!n.online||n.ip!=newIp)log("HTTP disponible en "+newIp+":8080");
                    n.online=true;
                    n.local=false;
                    n.ip=newIp;
                    n.offer=false;
                    declined=false;
                    n.message="Conexion local disponible";
                    attempts=0;
                    submitted=false;
                    next=0;
                } else {
                    if(n.online){
                        declined=false;
                        log("Conexion perdida. Reintentando redes conocidas");
                        n.online=false;
                        n.ip.clear();
                        attempts=0;
                        next=0;
                    }
                    if(req&&attempts<5&&now>=next&&!submitted){
                        nifmRequestCancel(&request);
                        r=nifmRequestSubmit(&request);
                        ++attempts;
                        deadline=now+10;
                        submitted=R_SUCCEEDED(r);
                        if(!submitted)deadline=now;
                    }
                    if(req&&attempts>0&&now>=deadline){
                        if(submitted)nifmRequestCancel(&request);
                        submitted=false;
                        const int delay[]={
                            2,4,8,15,15
                        };
                        next=now+delay[attempts-1];
                        deadline=UINT64_MAX;
                        if(attempts>=5){
                            n.offer=!declined;
                            n.message="No hay conexion. Puedes crear una red local";
                            log(n.message);
                        }
                    }
                    if(!req&&attempts==0){
                        attempts=5;
                        n.offer=!declined;
                        n.message="No se pudo solicitar reconexion. Red local disponible en opciones";
                    }
                    n.attempt=attempts;
                    if(attempts<5)n.message="Reconexion "+std::to_string(attempts)+"/5";
                }
            }
            if(ap&&now>=nextDevices){
                refreshDevices(n);
                nextDevices=now+1;
            }else if(!ap){
                n.devices.clear();
                n.devicesKnown=false;
                n.devicesError.clear();
                nextDevices=0;
            }
            if(pendingSave&&now>=nextSave&&mutexTryLock(&fsMutex)){
                bool saved=writeAtomic(std::string(stateDir)+"/wifi",n.ssid+"\n"+n.password,error)&&writeAtomic(std::string(stateDir)+"/wifi-group",std::string(reinterpret_cast<char*>(&cachedGroup),sizeof(cachedGroup)),error);
                mutexUnlock(&fsMutex);
                pendingSave=!saved;
                nextSave=now+5;
                if(!saved)log("No se pudo guardar el perfil Wi-Fi: "+error,"error");
            }
            setNetwork(n);
            svcSleepThread(250000000);
        }
        if(pendingSave){
            Guard g(fsMutex);
            writeAtomic(std::string(stateDir)+"/wifi",n.ssid+"\n"+n.password,error);
            writeAtomic(std::string(stateDir)+"/wifi-group",std::string(reinterpret_cast<char*>(&cachedGroup),sizeof(cachedGroup)),error);
        }
        if(req){
            nifmRequestCancel(&request);
            nifmRequestClose(&request);
        }
        if(ap)lp2pDestroyGroup();
        if(lp)lp2pExit();
    }
    bool startNetwork(){
        Result r=threadCreate(&worker,run,nullptr,nullptr,0x10000,0x2C,-2);
        if(R_FAILED(r))return false;
        r=threadStart(&worker);
        if(R_FAILED(r)){
            threadClose(&worker);
            return false;
        }
        started=true;
        return true;
    }
    void stopNetwork(){
        if(started){
            threadWaitForExit(&worker);
            threadClose(&worker);
            started=false;
        }
    }
}
