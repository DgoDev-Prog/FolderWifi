#include "runtime.hpp"
#include "gui.hpp"
#include <algorithm>
int main(int,char**){
    using namespace fw;
    mutexInit(&stateMutex);
    mutexInit(&fsMutex);
    guiInit();
    const AppletType launchType=appletGetAppletType();
    const bool applicationMode=launchType==AppletType_Application||launchType==AppletType_SystemApplication;
    const bool appletMode=launchType==AppletType_LibraryApplet||launchType==AppletType_SystemApplet||launchType==AppletType_OverlayApplet;
    const char* launchLabel=applicationMode?"MODO APLICACION":appletMode?"MODO APPLET":"MODO DESCONOCIDO";
    log(std::string("Inicio: ")+launchLabel+" (tipo "+std::to_string(static_cast<int>(launchType))+")");
    Result r=csrngInitialize();
    bool rng=R_SUCCEEDED(r);
    if(!rng)log("No se pudo iniciar aleatoriedad segura");
    bool nifm=R_SUCCEEDED(nifmInitialize(NifmServiceType_User));
    if(!nifm)log("Servicio de red no disponible");
    auto config=*socketGetDefaultInitConfig();
    config.num_bsd_sessions=8;
    r=socketInitialize(&config);
    bool sockets=R_SUCCEEDED(r);
    if(!sockets)log("No se pudo iniciar sockets");
    std::string error;
    if(rng&&sockets){
        recoverFiles();
        if(!startServer(error)){
            log(error,"error");
            stopServer();
            stopping=false;
        }
        if(nifm&&!startNetwork())log("No se pudo iniciar gestor de conexion");
    }
    padConfigureInput(1,HidNpadStyleSet_NpadStandard);
    PadState pad;
    padInitializeDefault(&pad);
    bool qr=false,showLogs=false,showDevices=false,follow=true,settings=false,offerDismissed=false,automaticOffer=false;
    int scroll=0,deviceScroll=0,tab=0,option=0;
    uint64_t repeatAt=0;
    while(appletMainLoop()){
        padUpdate(&pad);
        u64 down=padGetButtonsDown(&pad),held=padGetButtons(&pad);
        if(down&HidNpadButton_Plus)break;
        auto n=networkSnapshot();
        if(n.online){
            offerDismissed=false;
            if(automaticOffer){
                settings=false;
                automaticOffer=false;
            }
        }
        if(n.offer&&!offerDismissed){
            settings=true;
            showDevices=false;
            automaticOffer=true;
        }
        if(down&HidNpadButton_Y){
            qr=!qr;
            showLogs=false;
            settings=false;
            showDevices=false;
        }
        if(down&HidNpadButton_X){
            showLogs=!showLogs;
            qr=false;
            settings=false;
            showDevices=false;
        }
        if(down&HidNpadButton_ZL){
            automaticOffer=false;
            settings=!settings;
            qr=false;
            showLogs=false;
            showDevices=false;
        }
        if(down&HidNpadButton_B){
            if(showDevices){
                showDevices=false;
                settings=true;
                automaticOffer=false;
            }else if(settings){
                networkCommand=2;
                offerDismissed=true;
                settings=false;
                automaticOffer=false;
            }else{
                qr=false;
                showLogs=false;
            }
        }
        if((down&HidNpadButton_A)&&!qr&&!showLogs&&!showDevices&&!settings)allowWrites(!writeEnabled);
        if(qr&&!settings){
            if(down&HidNpadButton_Right)tab=(tab+1)%3;
            if(down&HidNpadButton_Left)tab=(tab+2)%3;
        }
        if(settings){
            if(down&HidNpadButton_Down)option=(option+1)%5;
            if(down&HidNpadButton_Up)option=(option+4)%5;
            if(down&HidNpadButton_A){
                if(option==3){
                    showDevices=true;
                    deviceScroll=0;
                }else networkCommand=option==0?1:option==1?3:option==2?4:2;
                offerDismissed=true;
                settings=false;
                automaticOffer=false;
            }
        }
        if(showDevices){
            const int last=std::max(0,static_cast<int>(n.devices.size())-DEVICE_VIEW_VISIBLE_ROWS);
            if(down&HidNpadButton_Down)++deviceScroll;
            if(down&HidNpadButton_Up)--deviceScroll;
            deviceScroll=std::clamp(deviceScroll,0,last);
        }
        std::vector<std::string> messages;
        {
            Guard g(stateMutex);
            messages.assign(logs.begin(),logs.end());
        }
        int maximum=std::max(0,getLogViewLineCount(messages)-LOG_VIEW_VISIBLE_LINES);
        if(showLogs){
            auto tick=armGetSystemTick();
            bool repeat=tick>=repeatAt;
            if((held&(HidNpadButton_Up|HidNpadButton_Down))&&repeat){
                scroll+=held&HidNpadButton_Up?-2:2;
                follow=false;
                repeatAt=tick+armGetSystemTickFreq()/8;
            }
            if(down&HidNpadButton_A)follow=true;
            scroll=follow?maximum:std::clamp(scroll,0,maximum);
        }
        u32 stride;
        u32* fb=guiBeginFrame(&stride);
        std::string url=n.online&&serverReady?"http://"+n.ip+":8080/":"Esperando conexion";
        renderMainUI(fb,writeEnabled&&serverReady,url,messages,false,scroll,follow);
        drawPill(fb,650,18,340,36,COLOR_CARD_HOVER,applicationMode?COLOR_GREEN:COLOR_PINK,launchLabel,1);
        if(!n.localError.empty()){
            drawText(fb,65,520,n.localError.substr(0,65).c_str(),COLOR_RED,1);
            if(n.localError.size()>65)drawText(fb,65,542,n.localError.substr(65,65).c_str(),COLOR_RED,1);
        }
        drawText(fb,45,620,(serverReady?n.message:("HTTP no disponible: "+error)).substr(0,145).c_str(),COLOR_CYAN,1);
        drawText(fb,45,644,("Vincular navegador: "+pairCode+"   [ZL] Red   [Y] QR   [X] Registro").c_str(),COLOR_WHITE,1);
        if(showLogs)renderActivity(fb,messages,scroll,follow);
        if(showDevices)renderConnectedDevices(fb,n,deviceScroll);
        if(qr)renderConnectionQR(fb,n.online&&serverReady?url+"#key="+accessKey:"",n.hasProfile?n.ssid:"",n.hasProfile?n.password:"",tab,n.local);
        if(settings){
            drawRect(fb,250,110,780,500,COLOR_CARD);
            drawRectBorder(fb,250,110,780,500,3,COLOR_CYAN);
            drawText(fb,285,140,"CONEXION DE FOLDERWIFI",COLOR_WHITE,2);
            drawText(fb,285,188,(n.localError.empty()?n.message:n.localError).substr(0,84).c_str(),n.localError.empty()?COLOR_MUTED:COLOR_RED);
            const char* options[]={
                "Crear / activar red local WPA2","Detener red local y buscar red conocida","Generar nueva clave Wi-Fi (desconecta clientes)","Dispositivos conectados","Ahora no"
            };
            for(int i=0;i<5;++i){
                drawText(fb,285,240+i*52,options[i],option==i?COLOR_CYAN:COLOR_WHITE,1);
                if(option==i)drawText(fb,265,240+i*52,">",COLOR_CYAN);
            }
            drawText(fb,285,550,"[Arriba/Abajo] Elegir  [A] Confirmar  [B] Volver",COLOR_MUTED);
        }
        guiEndFrame();
    }
    stopping=true;
    stopServer();
    stopNetwork();
    if(sockets)socketExit();
    if(nifm)nifmExit();
    if(rng)csrngExit();
    guiExit();
    return 0;
}
