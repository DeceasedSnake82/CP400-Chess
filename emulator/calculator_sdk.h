#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <cstdlib>
#include <cstring>

extern const int width;
extern const int height;

#define APP_NAME(x)
#define APP_DESCRIPTION(x)
#define APP_AUTHOR(x)
#define APP_VERSION(x)

inline uint16_t color(uint8_t r, uint8_t g, uint8_t b) {
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

#define EVENT_KEY 1
#define EVENT_TOUCH 2
#define KEY_PRESSED 1
#define KEYCODE_POWER_CLEAR 27

struct TouchData {
    int direction;
    int p1_x;
    int p1_y;
};

struct KeyData {
    int direction;
    int keyCode;
};

struct Input_Event {
    int type;
    union {
        TouchData touch_single;
        KeyData key;
    } data;
};

#define TOUCH_DOWN 1

void setPixel(int x, int y, uint16_t color);
void LCD_Refresh();
void fillScreen(uint16_t color);
void GetInput(struct Input_Event* event, uint32_t timeout, uint32_t mask);

inline void* Mem_Malloc(size_t size) { return std::malloc(size); }
inline void Mem_Memset(void* ptr, int value, size_t num) { std::memset(ptr, value, num); }
inline void Mem_Free(void* ptr) { std::free(ptr); }
