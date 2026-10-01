#ifndef GUI_HPP
#define GUI_HPP

#include <switch.h>
#include <string>
#include <vector>

#ifndef RGBA8_MAX
#define RGBA8_MAX(r, g, b, a) (((u32)(r)) | (((u32)(g)) << 8) | (((u32)(b)) << 16) | (((u32)(a)) << 24))
#endif

#define COLOR_BG         RGBA8_MAX(10, 10, 15, 255)
#define COLOR_CARD       RGBA8_MAX(18, 18, 28, 255)
#define COLOR_CARD_HOVER RGBA8_MAX(28, 28, 43, 255)
#define COLOR_BORDER     RGBA8_MAX(38, 38, 56, 255)
#define COLOR_CYAN       RGBA8_MAX(0, 243, 255, 255)
#define COLOR_PINK       RGBA8_MAX(255, 0, 85, 255)
#define COLOR_GREEN      RGBA8_MAX(0, 255, 136, 255)
#define COLOR_RED        RGBA8_MAX(255, 71, 87, 255)
#define COLOR_WHITE      RGBA8_MAX(240, 240, 245, 255)
#define COLOR_MUTED      RGBA8_MAX(138, 138, 158, 255)
#define COLOR_BLACK      RGBA8_MAX(0, 0, 0, 255)

void guiInit();
void guiExit();

u32* guiBeginFrame(u32* out_stride);
void guiEndFrame();

void drawPixel(u32* fb, int x, int y, u32 color);
void drawRect(u32* fb, int x, int y, int w, int h, u32 color);
void drawRectBorder(u32* fb, int x, int y, int w, int h, int border, u32 color);
void drawText(u32* fb, int x, int y, const char* text, u32 color, int scale = 1);
void drawCard(u32* fb, int x, int y, int w, int h, u32 bgColor, u32 borderColor, const char* title);
void drawPill(u32* fb, int x, int y, int w, int h, u32 bgColor, u32 textColor, const char* text, int scale = 1);

constexpr int LOG_VIEW_VISIBLE_LINES = 18;
int getLogViewLineCount(const std::vector<std::string>& logMessages);
void renderMainUI(u32* fb, bool isOnline, const std::string& ipUrl, const std::vector<std::string>& logMessages, bool showLogs, int logScroll, bool followLogs);


void renderConnectionQR(u32*,const std::string&,const std::string&,const std::string&,int,bool);

void renderActivity(u32*,const std::vector<std::string>&,int,bool);
namespace fw { struct Network; }
constexpr int DEVICE_VIEW_VISIBLE_ROWS = 4;
void renderConnectedDevices(u32*,const fw::Network&,int);
#endif
