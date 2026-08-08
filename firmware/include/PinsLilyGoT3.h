#pragma once

#include <cstdint>

// Pinagem do LilyGo TTGO LoRa32 T3 v1.6.1 (ESP32 "classico", nao S3
// -- confirmado: ESP32-Pico-D4, 4MB flash, SX1276, OLED SSD1306
// I2C 128x64 embutido na propria placa). Board id no PlatformIO:
// "ttgo-lora32-v1".
//
// Diferente das PCBs AutonomousAircraft/Sonde (feitas por nos, sem
// tela), essa placa tem OLED de fabrica -- por isso e a UNICA das
// tres com PCB_LILYGO_T3 habilitando o modulo de Display (ver
// lib/Display e os #ifdef em main.cpp).
namespace Pins
{
    namespace SPI
    {
        inline constexpr uint8_t SCK  = 5;
        inline constexpr uint8_t MOSI = 27;
        inline constexpr uint8_t MISO = 19;
    }

    namespace Radio
    {
        inline constexpr uint8_t CS    = 18;
        inline constexpr uint8_t DIO0  = 26;
        inline constexpr uint8_t RESET = 14;
    }

    // OLED SSD1306 embutido, via I2C.
    namespace Display
    {
        inline constexpr uint8_t SDA = 21;
        inline constexpr uint8_t SCL = 22;

        inline constexpr uint8_t WIDTH   = 128;
        inline constexpr uint8_t HEIGHT  = 64;
        inline constexpr uint8_t ADDRESS = 0x3C;
    }
}
