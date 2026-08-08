#ifdef PCB_LILYGO_T3

#include "Display.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "Pins.h"

static Adafruit_SSD1306 oled(
    Pins::Display::WIDTH,
    Pins::Display::HEIGHT,
    &Wire,
    -1
);

bool Display::begin()
{
    Wire.begin(Pins::Display::SDA, Pins::Display::SCL);

    if (!oled.begin(SSD1306_SWITCHCAPVCC, Pins::Display::ADDRESS)) {
        return false;
    }

    oled.clearDisplay();
    oled.setTextSize(1);
    oled.setTextColor(SSD1306_WHITE);
    oled.display();

    return true;
}

void Display::showBootInfo(const char* line1, const char* line2)
{
    oled.clearDisplay();
    oled.setTextSize(1);
    oled.setCursor(0, 0);
    oled.println(line1);
    oled.println(line2);
    oled.display();
}

void Display::showMessage(int rssi, float snr, const char* msg)
{
    oled.setTextSize(1);
    oled.clearDisplay();

    oled.setCursor(0, 0);
    oled.print(msg);

    oled.setCursor(0, 48);
    oled.print("RSSI: ");
    oled.print(rssi);
    oled.println(" dBm");

    oled.print("SNR: ");
    oled.print(snr);
    oled.println(" dB");

    oled.display();
}

void Display::showNoMessage()
{
    oled.clearDisplay();
    oled.setTextSize(2);
    oled.setTextColor(SSD1306_WHITE);

    const char* texto = "Sem mensagens ha 10s";

    int16_t x1, y1;
    uint16_t w, h;

    oled.getTextBounds(texto, 0, 0, &x1, &y1, &w, &h);

    int16_t x = (Pins::Display::WIDTH  - w) / 2;
    int16_t y = (Pins::Display::HEIGHT - h) / 2;

    oled.setCursor(x < 0 ? 0 : x, y < 0 ? 0 : y);
    oled.println(texto);

    oled.display();

    oled.setTextSize(1);
}

#endif // PCB_LILYGO_T3
