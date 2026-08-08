#pragma once

#include <cstdint>

// Pinagem do RF96W na PCB-Sonde (ESP32-S3 Super Mini, U4 no esquematico).
// Barramento SPI compartilhado com MAX31865 e o GPS CC68 (cada um com
// seu proprio CS -- so o CS do RF96W importa aqui).
namespace Pins
{
    namespace SPI
    {
        inline constexpr uint8_t SCK  = 4; // "CLK" no esquematico
        inline constexpr uint8_t MOSI = 5;
        inline constexpr uint8_t MISO = 6;
    }

    // Nome "Radio" (nao "LoRa") de proposito -- evita colidir com o
    // nome da classe LoRa em RF96W.h.
    namespace Radio
    {
        inline constexpr uint8_t CS    = 8;  // "CS-RF96W"
        inline constexpr uint8_t RESET = 9;  // "RST-RF96W"
        inline constexpr uint8_t DIO0  = 10; // "DIO0e1-RF96W"

        // "BUSY-RF96W" no esquematico (GPIO11). O driver atual
        // (RF96W.h/.cpp via RadioLib) nao usa esse pino -- SX1276 nao
        // tem BUSY como os SX126x/SX128x -- mas fica definido aqui
        // caso vire necessario no futuro (ex.: outro modulo no lugar).
        inline constexpr uint8_t BUSY  = 11;
    }
}
