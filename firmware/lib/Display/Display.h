#pragma once

// So existe (compila pra algo) na PCB LilyGo T3 -- e a UNICA das
// tres placas com OLED de fabrica. Nas outras (AutonomousAircraft,
// Sonde), esse header vira um arquivo vazio: nenhuma dependencia de
// Adafruit_GFX/SSD1306 e puxada pra build (ver platformio.ini --
// essas lib_deps ficam só no [env:pcb-lilygo-t3]).
#ifdef PCB_LILYGO_T3

#include <Arduino.h>

class Display
{
public:

    // Retorna false se o OLED nao responder no barramento I2C
    // (placa com display com defeito, ou fiacao errada).
    bool begin();

    void showMessage(int rssi, float snr, const char* msg);
    void showNoMessage();
    void showBootInfo(const char* line1, const char* line2);
};

#endif // PCB_LILYGO_T3
