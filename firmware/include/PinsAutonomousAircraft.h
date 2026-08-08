#pragma once

#include <cstdint>

// Pinagem do RF96W na PCB-AutonomousAircraft (ESP32-S3 Super Mini).
namespace Pins
{
    namespace SPI
    {
        inline constexpr uint8_t SCK  = 5;
        inline constexpr uint8_t MOSI = 6;
        inline constexpr uint8_t MISO = 7;
    }

    // Nome "Radio" (nao "LoRa") de proposito -- evita colidir com o
    // nome da classe LoRa em RF96W.h.
    namespace Radio
    {
        inline constexpr uint8_t CS    = 4;
        inline constexpr uint8_t DIO0  = 8;
        inline constexpr uint8_t RESET = 9;
    }
}
