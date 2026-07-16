#include "calculator_sdk.h"
#include <SFML/Graphics.hpp>
#include <vector>
#include <optional>

const int width = 320;
const int height = 528;

static sf::RenderWindow* window = nullptr;
static sf::Texture* texture = nullptr;
static sf::Sprite* sprite = nullptr;

static std::vector<uint8_t> pixelBuffer(width * height * 4, 255);

void setPixel(int x, int y, uint16_t color565) {
    if (x >= 0 && x < width && y >= 0 && y < height) {
        int index = (y * width + x) * 4;
        
        pixelBuffer[index]     = ((color565 >> 11) & 0x1F) * 255 / 31;
        pixelBuffer[index + 1] = ((color565 >> 5)  & 0x3F) * 255 / 63;
        pixelBuffer[index + 2] = (color565 & 0x1F)         * 255 / 31;
        pixelBuffer[index + 3] = 255;
    }
}

void fillScreen(uint16_t color565) {
    uint8_t r = ((color565 >> 11) & 0x1F) * 255 / 31;
    uint8_t g = ((color565 >> 5)  & 0x3F) * 255 / 63;
    uint8_t b = (color565 & 0x1F)         * 255 / 31;
    
    for (int i = 0; i < width * height; ++i) {
        int idx = i * 4;
        pixelBuffer[idx]     = r;
        pixelBuffer[idx + 1] = g;
        pixelBuffer[idx + 2] = b;
        pixelBuffer[idx + 3] = 255;
    }
}

void LCD_Refresh() {
    if (!window) return;

    texture->update(pixelBuffer.data());
    
    window->clear();
    window->draw(*sprite);
    window->display();
}

void GetInput(struct Input_Event* event, uint32_t timeout, uint32_t mask) {
    if (!window) return;

    event->type = 0;

    if (std::optional<sf::Event> sfEvent = window->waitEvent()) {
        
        if (sfEvent->is<sf::Event::Closed>()) {
            event->type = EVENT_KEY;
            event->data.key.direction = KEY_PRESSED;
            event->data.key.keyCode = KEYCODE_POWER_CLEAR;
            window->close();
        } 
        else if (const auto* mouseButtonPressed = sfEvent->getIf<sf::Event::MouseButtonPressed>()) {
            if (mouseButtonPressed->button == sf::Mouse::Button::Left) {
                event->type = EVENT_TOUCH;
                event->data.touch_single.direction = TOUCH_DOWN;
                event->data.touch_single.p1_x = mouseButtonPressed->position.x;
                event->data.touch_single.p1_y = mouseButtonPressed->position.y;
            }
        } 
        else if (const auto* keyPressed = sfEvent->getIf<sf::Event::KeyPressed>()) {
            if (keyPressed->code == sf::Keyboard::Key::Escape) {
                event->type = EVENT_KEY;
                event->data.key.direction = KEY_PRESSED;
                event->data.key.keyCode = KEYCODE_POWER_CLEAR;
            }
        }
    }
}

struct EmulatorSetup {
    EmulatorSetup() {
        sf::Vector2u size(width, height);
        
        window = new sf::RenderWindow(sf::VideoMode(size), "CP400 Mini Emulator - Chess AI", sf::Style::Titlebar | sf::Style::Close);
        
        texture = new sf::Texture();
        (void)texture->resize(size);
        
        sprite = new sf::Sprite(*texture);
        
        fillScreen(0);
    }
    
    ~EmulatorSetup() {
        delete sprite;
        delete texture;
        delete window;
    }
} global_emulator_init;
