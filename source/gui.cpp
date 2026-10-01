#include "gui.hpp"
#include "runtime.hpp"
#include "font8x16.h"
#include "qr_codec.h"
#include <cstring>
#include <algorithm>
static Framebuffer g_fb;
static u32 g_pitch=1280;
void guiInit() {
    framebufferCreate(&g_fb, nwindowGetDefault(), 1280, 720, PIXEL_FORMAT_RGBA_8888, 2);
    framebufferMakeLinear(&g_fb);
}
void guiExit() {
    framebufferClose(&g_fb);
}
u32* guiBeginFrame(u32* out_stride) {
    u32 internal_stride = 0;
    u32* fb = (u32*)framebufferBegin(&g_fb, &internal_stride);
    g_pitch=internal_stride / sizeof(u32);
    if (out_stride) *out_stride = internal_stride;
    return fb;
}
void guiEndFrame() {
    framebufferEnd(&g_fb);
}
inline void drawPixel(u32* fb, int x, int y, u32 color) {
    if (x >= 0 && x < 1280 && y >= 0 && y < 720) {
        fb[y * g_pitch + x] = color;
    }
}
void drawRect(u32* fb, int x, int y, int w, int h, u32 color) {
    int x2 = std::min(1280, x + w);
    int y2 = std::min(720, y + h);
    int x1 = std::max(0, x);
    int y1 = std::max(0, y);
    for (int cy = y1; cy < y2; cy++) {
        u32* line = &fb[cy * g_pitch + x1];
        for (int cx = x1; cx < x2; cx++) {
            *line++ = color;
        }
    }
}
void drawRectBorder(u32* fb, int x, int y, int w, int h, int border, u32 color) {
    drawRect(fb, x, y, w, border, color);
    drawRect(fb, x, y + h - border, w, border, color);
    drawRect(fb, x, y, border, h, color);
    drawRect(fb, x + w - border, y, border, h, color);
}
void drawText(u32* fb, int x, int y, const char* text, u32 color, int scale) {
    if (!text) return;
    int startX = x;
    int curX = x;
    int curY = y;
    for (const char* p = text; *p != '\0'; p++) {
        unsigned char c = (unsigned char)*p;
        if (c == '\n') {
            curX = startX;
            curY += 16 * scale;
            continue;
        }
        if(c>=128){
            unsigned code=c;
            int extra=c>=0xC2&&c<=0xDF?1:c>=0xE0&&c<=0xEF?2:c>=0xF0&&c<=0xF4?3:0;
            code&=extra==1?31:extra==2?15:extra==3?7:255;
            bool valid=extra>0;
            for(int i=0;i<extra;++i){
                unsigned char next=static_cast<unsigned char>(p[1]);
                if((next&0xC0)!=0x80){
                    valid=false;
                    break;
                }
                code=(code<<6)|(next&63);
                ++p;
            }
            c='?';
            if(valid){
                const unsigned latin[]={
                    0xE1,0xE9,0xED,0xF3,0xFA,0xFC,0xF1,0xC1,0xC9,0xCD,0xD3,0xDA,0xDC,0xD1
                };
                const char* plain="aeiouunAEIOUUN";
                for(int i=0;i<14;++i)if(code==latin[i]){
                    c=plain[i];
                    drawRect(fb,curX+3*scale,curY,2*scale,scale,color);
                    break;
                }
            }
        }
        for (int py = 0; py < 16; py++) {
            uint8_t row = font8x16[c][py];
            for (int px = 0; px < 8; px++) {
                if ((row >> (7 - px)) & 1) {
                    if (scale == 1) {
                        drawPixel(fb, curX + px, curY + py, color);
                    } else {
                        drawRect(fb, curX + px * scale, curY + py * scale, scale, scale, color);
                    }
                }
            }
        }
        curX += 8 * scale;
    }
}
void drawCard(u32* fb, int x, int y, int w, int h, u32 bgColor, u32 borderColor, const char* title) {
    drawRect(fb, x, y, w, h, bgColor);
    drawRectBorder(fb, x, y, w, h, 2, borderColor);
    if (title && strlen(title) > 0) {
        drawRect(fb, x, y, w, 36, borderColor);
        drawText(fb, x + 16, y + 10, title, COLOR_WHITE, 1);
    }
}
void drawPill(u32* fb, int x, int y, int w, int h, u32 bgColor, u32 textColor, const char* text, int scale) {
    drawRect(fb, x, y, w, h, bgColor);
    int textLen = strlen(text) * 8 * scale;
    int tx = x + (w - textLen) / 2;
    int ty = y + (h - 16 * scale) / 2;
    drawText(fb, tx, ty, text, textColor, scale);
}
namespace {
    struct LogLine {
        std::string text;
        u32 color;
    };
    u32 logColor(const std::string& message) {
        std::string lower = message;
        for (char& c : lower) {
            if (c >= 'A' && c <= 'Z') c += 'a' - 'A';
        }
        if (lower.find("error") != std::string::npos || lower.find("fallo") != std::string::npos || lower.find("incompleto") != std::string::npos || lower.find("interrump") != std::string::npos) return COLOR_RED;
        return COLOR_CYAN;
    }
    std::vector<LogLine> buildLogLines(const std::vector<std::string>& messages, size_t columns = 122) {
        std::vector<LogLine> lines;
        for (const auto& message : messages) {
            const u32 color = logColor(message);
            std::string row;
            size_t glyphs=0;
            for(size_t i=0;i<message.size();){
                unsigned char c=message[i];
                if(c=='\r'){
                    ++i;
                    continue;
                }
                if(c=='\n'){
                    lines.push_back({
                        row,color
                    });
                    row.clear();
                    glyphs=0;
                    ++i;
                    continue;
                }
                if(glyphs==columns){
                    lines.push_back({
                        row,color
                    });
                    row.clear();
                    glyphs=0;
                }
                size_t count=c>=0xF0?4:c>=0xE0?3:c>=0xC2?2:1;
                count=std::min(count,message.size()-i);
                row+=c<32||c==127?" ":message.substr(i,count);
                i+=count;
                ++glyphs;
            }
            if (!row.empty() || message.empty()) lines.push_back({
                row, color
            });
        }
        return lines;
    }
    void renderLogViewer(u32* fb, const std::vector<std::string>& messages, int scroll, bool follow) {
        for (int y = 0; y < 720; y += 2) drawRect(fb, 0, y, 1280, 1, COLOR_BLACK);
        drawRect(fb, 100, 75, 1080, 570, COLOR_CARD);
        drawRectBorder(fb, 100, 75, 1080, 570, 3, COLOR_CYAN);
        drawText(fb, 124, 95, "REGISTRO DE ACTIVIDAD", COLOR_WHITE, 2);
        const std::string info = std::to_string(messages.size()) + " / 500 entradas - historial de esta sesion";
        drawText(fb, 124, 135, info.c_str(), COLOR_MUTED, 1);
        drawRect(fb, 120, 158, 1040, 410, COLOR_BG);
        drawRectBorder(fb, 120, 158, 1040, 410, 1, COLOR_BORDER);
        const auto lines = buildLogLines(messages);
        const int total = static_cast<int>(lines.size());
        const int maximum = std::max(0, total - LOG_VIEW_VISIBLE_LINES);
        scroll = std::max(0, std::min(scroll, maximum));
        if (lines.empty()) drawText(fb, 132, 170, "Todavia no hay actividad.", COLOR_MUTED, 1);
        for (int row = 0; row < LOG_VIEW_VISIBLE_LINES && scroll + row < total; ++row) {
            const auto& line = lines[scroll + row];
            drawText(fb, 132, 170 + row * 22, line.text.c_str(), line.color, 1);
        }
        const int trackHeight = 390;
        const int thumbHeight = total <= LOG_VIEW_VISIBLE_LINES ? trackHeight : std::max(18, trackHeight * LOG_VIEW_VISIBLE_LINES / total);
        const int thumbY = 168 + (maximum == 0 ? 0 : (trackHeight - thumbHeight) * scroll / maximum);
        drawRect(fb, 1140, 168, 8, trackHeight, COLOR_BORDER);
        drawRect(fb, 1140, thumbY, 8, thumbHeight, COLOR_CYAN);
        const std::string position = total == 0 ? "0 / 0 lineas" : std::to_string(scroll + 1) + "-" + std::to_string(std::min(total, scroll + LOG_VIEW_VISIBLE_LINES)) + " / " + std::to_string(total) + " lineas";
        drawText(fb, 124, 584, position.c_str(), COLOR_MUTED, 1);
        drawText(fb, 660, 584, follow ? "Siguiendo actividad en vivo" : "Revisando historial", COLOR_CYAN, 1);
        drawText(fb, 124, 614, "[Arriba / Abajo] Desplazar     [X / B] Cerrar", COLOR_WHITE, 1);
    }
}
int getLogViewLineCount(const std::vector<std::string>& logMessages) {
    return static_cast<int>(buildLogLines(logMessages).size());
}
void renderMainUI(u32* fb, bool isOnline, const std::string& ipUrl, const std::vector<std::string>& logMessages, bool showLogs, int logScroll, bool followLogs) {
    // 1. Background
    drawRect(fb, 0, 0, 1280, 720, COLOR_BG);
    // 2. Header Bar
    drawRect(fb, 0, 0, 1280, 70, COLOR_CARD);
    drawRect(fb, 0, 68, 1280, 2, COLOR_CYAN);
    // Logo
    drawRect(fb, 30, 20, 36, 30, COLOR_CARD_HOVER);
    drawRectBorder(fb, 30, 20, 36, 30, 2, COLOR_PINK);
    drawRect(fb, 42, 14, 18, 8, COLOR_CYAN);
    drawText(fb, 80, 18, "FolderWifi Manager", COLOR_WHITE, 2);
    drawText(fb, 420, 26, "v0.2.0-alpha", COLOR_CYAN, 1);
    drawText(fb, 80, 48, "por Tssr - Diego Ramirez", COLOR_MUTED, 1);
    // Header Right Pill
    drawPill(fb, 1020, 18, 230, 36, COLOR_CARD_HOVER, COLOR_CYAN, "NINTENDO SWITCH", 1);
    // 3. Left Card: Status & Server Info
    drawCard(fb, 40, 90, 580, 480, COLOR_CARD, COLOR_BORDER, "ESTADO DEL SERVIDOR");
    if (isOnline) {
        drawPill(fb, 65, 150, 530, 54, COLOR_GREEN, COLOR_BLACK, "LECTURA Y ESCRITURA", 2);
    } else {
        drawPill(fb, 65, 150, 530, 54, COLOR_RED, COLOR_WHITE, "SOLO LECTURA", 2);
    }
    drawText(fb, 65, 230, "Direccion Web de Acceso Local:", COLOR_WHITE, 1);
    drawRect(fb, 65, 255, 530, 45, COLOR_CARD_HOVER);
    drawRectBorder(fb, 65, 255, 530, 45, 1, COLOR_CYAN);
    drawText(fb, 85, 268, ipUrl.c_str(), COLOR_CYAN, 2);
    drawText(fb, 65, 325, "Modo de Operacion:", COLOR_MUTED, 1);
    if (isOnline) {
        drawText(fb, 65, 345, "- Permite navegar, subir, descargar,", COLOR_WHITE, 1);
        drawText(fb, 65, 365, "  copiar, mover y eliminar archivos.", COLOR_WHITE, 1);
        drawText(fb, 65, 385, "- Acceso vinculado mediante codigo o QR.", COLOR_WHITE, 1);
    } else {
        drawText(fb, 65, 345, "- Modo de solo lectura habilitado.", COLOR_WHITE, 1);
        drawText(fb, 65, 365, "- Permite explorar carpetas en la SD.", COLOR_WHITE, 1);
        drawText(fb, 65, 385, "- Modificaciones bloqueadas. Descarga permitida.", COLOR_WHITE, 1);
    }
    drawText(fb, 65, 430, "Instrucciones de Uso:", COLOR_MUTED, 1);
    drawText(fb, 65, 450, "Conecta tu PC o movil a la misma red Wi-Fi", COLOR_WHITE, 1);
    drawText(fb, 65, 470, "e ingresa la URL en tu navegador web.", COLOR_WHITE, 1);
    drawText(fb, 65, 490, "Presiona [A] para alternar lectura / escritura.", COLOR_WHITE, 1);
    // 4. Recent activity summary; the complete history opens with X.
    drawCard(fb, 660, 90, 580, 480, COLOR_CARD, COLOR_BORDER, "ULTIMAS ACTIVIDADES");
    drawRect(fb, 675, 140, 550, 260, COLOR_BG);
    drawRectBorder(fb, 675, 140, 550, 260, 1, COLOR_BORDER);
    const size_t recentStart = logMessages.size() > 3 ? logMessages.size() - 3 : 0;
    const std::vector<std::string> recent(logMessages.begin() + recentStart, logMessages.end());
    const auto recentLines = buildLogLines(recent, 64);
    const int firstLine = std::max(0, static_cast<int>(recentLines.size()) - 9);
    for (int i = firstLine; i < static_cast<int>(recentLines.size()); ++i) {
        drawText(fb, 690, 155 + (i - firstLine) * 25, recentLines[i].text.c_str(), recentLines[i].color, 1);
    }
    drawText(fb, 690, 425, "El historial conserva hasta 500 entradas.", COLOR_MUTED, 1);
    drawPill(fb, 690, 460, 520, 50, COLOR_CARD_HOVER, COLOR_CYAN, "[ X ] Abrir registro de actividad", 1);
    drawText(fb, 690, 530, "Puedes desplazarte por los mensajes anteriores.", COLOR_MUTED, 1);
    // 5. Bottom Control Hints Bar
    drawRect(fb, 0, 595, 1280, 125, COLOR_CARD);
    drawRect(fb, 0, 595, 1280, 2, COLOR_BORDER);
    drawPill(fb, 40, 672, 280, 32, COLOR_CARD_HOVER, COLOR_WHITE, "[ A ] Lectura / escritura", 1);
    drawPill(fb, 345, 672, 280, 32, COLOR_CARD_HOVER, COLOR_WHITE, "[ Y ] Codigos QR", 1);
    drawPill(fb, 650, 672, 280, 32, COLOR_CARD_HOVER, COLOR_WHITE, "[ X ] Registro / [ZL] Red", 1);
    drawPill(fb, 955, 672, 285, 32, COLOR_PINK, COLOR_WHITE, "[ + ] Salir", 1);
    // 6. Floating Modal Overlay (QR Code Window)
    if (showLogs) renderLogViewer(fb, logMessages, logScroll, followLogs);
}
static std::string wifiEscape(const std::string& s){
    std::string out;
    for(char c:s){
        if(c=='\\'||c==';'||c==','||c==':'||c=='"')out+='\\';
        out+=c;
    }
    return out;
}
void renderConnectionQR(u32* fb,const std::string& url,const std::string& ssid,const std::string& password,int tab,bool wifiActive){
    const char* titles[]={
        "HTTP - ABRIR FOLDERWIFI","WI-FI - CONECTAR A LA RED","WI-FI - GUARDAR COMO TEXTO"
    };
    std::string payload=tab==0?url:tab==1?(ssid.empty()?"":"WIFI:T:WPA;S:"+wifiEscape(ssid)+";P:"+wifiEscape(password)+";H:false;;"):(ssid.empty()?"":"FolderWifi\nSeguridad: WPA2-PSK\nRed: "+ssid+"\nClave: "+password+"\nHTTP: "+(wifiActive?url.substr(0,url.find('#')):"Red local inactiva"));
    drawRect(fb,260,55,760,605,COLOR_CARD);
    drawRectBorder(fb,260,55,760,605,3,COLOR_CYAN);
    drawText(fb,290,85,titles[tab],COLOR_WHITE,2);
    static std::string previous;
    static uint8_t code[qrcodegen_BUFFER_LEN_MAX],scratch[qrcodegen_BUFFER_LEN_MAX];
    static bool valid=false;
    if(payload!=previous){
        previous=payload;
        valid=!payload.empty()&&qrcodegen_encodeText(payload.c_str(),scratch,code,qrcodegen_Ecc_MEDIUM,1,20,qrcodegen_Mask_AUTO,true);
    }
    if(valid){
        int size=qrcodegen_getSize(code),scale=std::min(9,370/(size+8)),total=(size+8)*scale;
        int x=640-total/2,y=135;
        drawRect(fb,x,y,total,total,COLOR_WHITE);
        for(int cy=0;cy<size;++cy)for(int cx=0;cx<size;++cx)if(qrcodegen_getModule(code,cx,cy))drawRect(fb,x+(cx+4)*scale,y+(cy+4)*scale,scale,scale,COLOR_BLACK);
    } else drawText(fb,300,270,tab==0?"HTTP disponible al obtener una conexion":"Activa primero la red local desde [ZL]",COLOR_MUTED,1);
    if(!ssid.empty()){
        drawText(fb,290,525,("Red: "+ssid).c_str(),COLOR_WHITE);
        drawText(fb,290,549,("Clave: "+password).c_str(),COLOR_WHITE);
    }
    drawText(fb,290,578,(tab>0&&!wifiActive?"Credenciales guardadas - red local inactiva":url.substr(0,url.find('#'))).c_str(),COLOR_CYAN);
    drawText(fb,290,616,"[Izquierda/Derecha] Cambiar QR  [Y/B] Cerrar",COLOR_MUTED);
}
void renderActivity(u32* fb,const std::vector<std::string>& messages,int scroll,bool follow){
    renderLogViewer(fb,messages,scroll,follow);
}
void renderConnectedDevices(u32* fb,const fw::Network& n,int scroll){
    drawRect(fb,250,85,780,550,COLOR_CARD);
    drawRectBorder(fb,250,85,780,550,3,COLOR_CYAN);
    drawText(fb,285,115,"DISPOSITIVOS CONECTADOS",COLOR_WHITE,2);
    if(!n.local){
        drawText(fb,285,185,"La red local de FolderWifi esta inactiva",COLOR_MUTED);
        drawText(fb,285,220,"Activa la red desde [ZL] para consultar sus clientes",COLOR_MUTED);
        drawText(fb,285,260,"Esta ventana no enumera los clientes de un router externo",COLOR_MUTED);
    }else{
        drawText(fb,285,166,("Red: "+n.ssid).c_str(),COLOR_CYAN);
        if(!n.devicesError.empty()){
            drawText(fb,285,220,"No se pudo consultar la lista; se reintentara",COLOR_RED);
            drawText(fb,285,250,n.devicesError.c_str(),COLOR_MUTED);
        }else if(!n.devicesKnown){
            drawText(fb,285,220,"Consultando dispositivos...",COLOR_MUTED);
        }else{
            drawText(fb,285,200,("Conectados: "+std::to_string(n.devices.size())+"  |  Limite actual: 1 cliente").c_str(),COLOR_WHITE);
            if(n.devices.empty())drawText(fb,285,265,"Sin dispositivos conectados",COLOR_MUTED);
            const int end=std::min(static_cast<int>(n.devices.size()),scroll+DEVICE_VIEW_VISIBLE_ROWS);
            for(int i=scroll;i<end;++i){
                const int y=245+(i-scroll)*70;
                const auto& device=n.devices[i];
                drawText(fb,285,y,("Dispositivo "+std::to_string(i+1)+"  |  IP: "+(device.ip.empty()?"Pendiente":device.ip)).c_str(),COLOR_WHITE);
                drawText(fb,285,y+24,("MAC: "+device.mac).c_str(),COLOR_MUTED);
            }
        }
    }
    drawText(fb,285,560,"Actualizacion automatica cada segundo",COLOR_MUTED);
    drawText(fb,285,596,"[Arriba/Abajo] Desplazar  [B] Volver  [ZL] Red",COLOR_MUTED);
}
