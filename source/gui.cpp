#include "gui.hpp"
#include "font8x16.h"
#include "qrcodegen.hpp"
#include <cstring>
#include <algorithm>

static Framebuffer g_fb;

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
    if (out_stride) *out_stride = internal_stride;
    return fb;
}

void guiEndFrame() {
    framebufferEnd(&g_fb);
}

inline void drawPixel(u32* fb, int x, int y, u32 color) {
    if (x >= 0 && x < 1280 && y >= 0 && y < 720) {
        fb[y * 1280 + x] = color;
    }
}

void drawRect(u32* fb, int x, int y, int w, int h, u32 color) {
    int x2 = std::min(1280, x + w);
    int y2 = std::min(720, y + h);
    int x1 = std::max(0, x);
    int y1 = std::max(0, y);

    for (int cy = y1; cy < y2; cy++) {
        u32* line = &fb[cy * 1280 + x1];
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
        if (c > 127) c = '?';

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

void renderMainUI(u32* fb, bool isOnline, const std::string& ipUrl, const std::vector<std::string>& logMessages, bool showQR) {
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
    drawText(fb, 420, 26, "v0.1.0-alpha", COLOR_CYAN, 1);
    drawText(fb, 80, 48, "por Tssr - Diego Ramirez", COLOR_MUTED, 1);

    // Header Right Pill
    drawPill(fb, 1020, 18, 230, 36, COLOR_CARD_HOVER, COLOR_CYAN, "NINTENDO SWITCH", 1);

    // 3. Left Card: Status & Server Info
    drawCard(fb, 40, 90, 580, 480, COLOR_CARD, COLOR_BORDER, "ESTADO DEL SERVIDOR");

    if (isOnline) {
        drawPill(fb, 65, 150, 530, 54, COLOR_GREEN, COLOR_BLACK, "ONLINE - CONTROL TOTAL", 2);
    } else {
        drawPill(fb, 65, 150, 530, 54, COLOR_RED, COLOR_WHITE, "OFFLINE - SOLO LECTURA", 2);
    }

    drawText(fb, 65, 230, "Direccion Web de Acceso Local:", COLOR_WHITE, 1);
    drawRect(fb, 65, 255, 530, 45, COLOR_CARD_HOVER);
    drawRectBorder(fb, 65, 255, 530, 45, 1, COLOR_CYAN);
    drawText(fb, 85, 268, ipUrl.c_str(), COLOR_CYAN, 2);

    drawText(fb, 65, 325, "Modo de Operacion:", COLOR_MUTED, 1);
    if (isOnline) {
        drawText(fb, 65, 345, "- Permite navegar, subir, descargar,", COLOR_WHITE, 1);
        drawText(fb, 65, 365, "  copiar, mover y eliminar archivos.", COLOR_WHITE, 1);
        drawText(fb, 65, 385, "- Sin requerir usuario ni contrasena.", COLOR_WHITE, 1);
    } else {
        drawText(fb, 65, 345, "- Modo de solo lectura habilitado.", COLOR_WHITE, 1);
        drawText(fb, 65, 365, "- Permite explorar carpetas en la SD.", COLOR_WHITE, 1);
        drawText(fb, 65, 385, "- Bloquea modificaciones y descargas.", COLOR_WHITE, 1);
    }

    drawText(fb, 65, 430, "Instrucciones de Uso:", COLOR_MUTED, 1);
    drawText(fb, 65, 450, "Conecta tu PC o movil a la misma red Wi-Fi", COLOR_WHITE, 1);
    drawText(fb, 65, 470, "e ingresa la URL en tu navegador web.", COLOR_WHITE, 1);
    drawText(fb, 65, 490, "Presiona [A] para alternar Online / Offline.", COLOR_WHITE, 1);

    // 4. Right Card: Activity Log
    drawCard(fb, 660, 90, 580, 480, COLOR_CARD, COLOR_BORDER, "REGISTRO DE ACTIVIDAD EN VIVO");
    drawRect(fb, 675, 140, 550, 410, COLOR_BG);
    drawRectBorder(fb, 675, 140, 550, 410, 1, COLOR_BORDER);

    int logY = 155;
    int startIdx = std::max(0, (int)logMessages.size() - 14);
    for (size_t i = startIdx; i < logMessages.size(); i++) {
        drawText(fb, 690, logY, logMessages[i].c_str(), COLOR_CYAN, 1);
        logY += 25;
    }

    // 5. Bottom Control Hints Bar
    drawRect(fb, 0, 595, 1280, 125, COLOR_CARD);
    drawRect(fb, 0, 595, 1280, 2, COLOR_BORDER);

    drawPill(fb, 40, 620, 360, 50, COLOR_CARD_HOVER, COLOR_WHITE, "[ A ] Alternar Online/Offline", 1);
    drawPill(fb, 430, 620, 360, 50, COLOR_CARD_HOVER, COLOR_WHITE, "[ Y ] Ver Codigo QR", 1);
    drawPill(fb, 820, 620, 420, 50, COLOR_PINK, COLOR_WHITE, "[ + ] Salir de FolderWifi", 1);

    // 6. Floating Modal Overlay (QR Code Window)
    if (showQR) {
        for (int y = 0; y < 720; y += 2) {
            drawRect(fb, 0, y, 1280, 1, COLOR_BLACK);
        }

        int mw = 540, mh = 560;
        int mx = (1280 - mw) / 2;
        int my = (720 - mh) / 2;

        drawRect(fb, mx, my, mw, mh, COLOR_CARD);
        drawRectBorder(fb, mx, my, mw, mh, 3, COLOR_CYAN);

        drawText(fb, mx + 60, my + 25, "CODIGO QR DE ACCESO RAPIDO", COLOR_WHITE, 2);

        using namespace qrcodegen;
        QrCode qr = QrCode::encodeText(ipUrl.c_str(), QrCode::Ecc::MEDIUM);
        int qrSize = qr.size;
        int modulePixel = 7; // Escala ajustada
        int qrPixelSize = qrSize * modulePixel;

        int qx = mx + (mw - qrPixelSize) / 2;
        int qy = my + 110; // Bajado para no tapar el título

        int quietZone = 20;
        drawRect(fb, qx - quietZone, qy - quietZone, qrPixelSize + quietZone * 2, qrPixelSize + quietZone * 2, RGBA8_MAX(255, 255, 255, 255));

        for (int ry = 0; ry < qrSize; ry++) {
            for (int rx = 0; rx < qrSize; rx++) {
                u32 modColor = qr.getModule(rx, ry) ? RGBA8_MAX(0, 0, 0, 255) : RGBA8_MAX(255, 255, 255, 255);
                drawRect(fb, qx + rx * modulePixel, qy + ry * modulePixel, modulePixel, modulePixel, modColor);
            }
        }

        drawText(fb, mx + 70, my + 440, "Escanea con la camara de tu celular", COLOR_CYAN, 1);
        drawPill(fb, mx + 70, my + 480, 400, 40, COLOR_CARD_HOVER, COLOR_WHITE, "Presiona [Y] o [B] para cerrar", 1);
    }
}
