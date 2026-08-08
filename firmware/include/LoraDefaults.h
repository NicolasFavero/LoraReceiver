#pragma once

#include <cstdint>

// Parametros de radio DEFAULT do modulo LoRa (SX1276/RFM96W) --
// iguais nas tres placas, por isso nao dependem de qual PCB foi
// selecionada (diferente de Pins.h). Sao só o ponto de partida: em
// qualquer placa, a config real em uso pode ser sobrescrita em tempo
// de execucao (dashboard web / MQTT) e persiste na memoria flash
// (Preferences/NVS) -- ver lib/LoraConfig.
namespace LoraConfig
{
    inline constexpr float    FREQUENCY        = 915.0f;
    inline constexpr float    BANDWIDTH        = 125.0f;
    inline constexpr uint8_t  SPREADING_FACTOR = 10;
    inline constexpr uint8_t  CODING_RATE      = 5;
    inline constexpr uint8_t  SYNC_WORD        = 0x12;
    inline constexpr int8_t   POWER            = 20;
    inline constexpr uint16_t PREAMBLE_LENGTH  = 8;
    inline constexpr bool     CRC_ENABLED      = true;
}
