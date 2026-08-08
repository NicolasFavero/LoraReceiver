#pragma once
#include <Arduino.h>
#include <RadioLib.h>

class LoRa {
public:

    LoRa(
        uint8_t cs,
        uint8_t dio0,
        uint8_t rst,

        float frequency = 915.0f,
        float bandwidth = 125.0f,
        uint8_t spreadingFactor = 10,
        uint8_t codingRate = 5,
        uint8_t syncWord = 0x12,
        int8_t power = 20,
        uint16_t preambleLength = 8,
        bool crc = true
    );

    bool begin();

    bool send(const char* msg);

    bool available();
    bool receive();

    // Reaplica todos os parametros de radio em tempo de execucao
    // (usado pela reconfiguracao via pagina web / MQTT). Coloca o
    // radio em standby, aplica cada parametro, e retoma a recepcao
    // no final -- mesmo em caso de falha parcial (retorna false, mas
    // sempre deixa o radio escutando de novo).
    bool reconfigure(
        float frequency,
        float bandwidth,
        uint8_t spreadingFactor,
        uint8_t codingRate,
        uint8_t syncWord,
        int8_t power,
        bool crc
    );

    const char* getPacket() const;

    float getRSSI() const;
    float getSNR() const;

    void print();

private:

    static void setFlag();

    inline static volatile bool packetReceived = false;

    Module module;
    SX1276 radio;

    float frequency;
    float bandwidth;

    uint8_t spreadingFactor;
    uint8_t codingRate;
    uint8_t syncWord;

    int8_t power;
    uint16_t preambleLength;
    bool crc;

    float rssi = 0.0f;
    float snr  = 0.0f;

    char packetBuffer[256];
};
