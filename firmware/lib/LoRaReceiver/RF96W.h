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

    // Envio ASSINCRONO -- nao bloqueia. radio.transmit() (a versao
    // bloqueante da RadioLib) fica presa num loop interno esperando o
    // pino IRQ subir, sem chamar esp_task_wdt_reset() nenhuma vez --
    // isso trava o loop() inteiro (WiFi, MQTT, servidor web, watchdog)
    // pelo tempo de ar do pacote e, se passar de WatchdogConfig::
    // TIMEOUT_MS, derruba a placa com um panic parecendo aleatorio.
    // Use startSend() pra iniciar e pollSend() a cada loop() ate ele
    // parar de retornar Pending -- mesmo padrao nao-bloqueante que
    // available()/receive() ja usam pra RX.
    enum class SendResult { Pending, Ok, Failed };

    bool startSend(const char* msg);
    SendResult pollSend();

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

    // IRAM_ATTR: essa ISR (chamada pela interrupcao do DIO0, tanto em
    // pacote recebido quanto em fim de transmissao) precisa estar na
    // RAM, nao na flash -- o ESP32 desliga o cache de instrucao
    // durante gravacoes na flash/NVS (ex.: Preferences ao salvar
    // WiFi/MQTT/LoRa pela pagina web). Se o DIO0 disparar nesse
    // instante e o handler estiver na flash, o nucleo executa lixo e
    // reinicia com um crash que parece aleatorio (ver historico do
    // bug: reset sozinho ao interagir com a pagina, exceção sem
    // relacao nenhuma com o codigo de verdade).
    static void IRAM_ATTR setFlag();

    inline static volatile bool packetReceived = false;

    // true enquanto um startSend() esta em andamento, aguardando
    // pollSend() concluir -- enquanto isso, available() ignora o
    // flag do DIO0 (ele passa a significar "TX terminou", nao "chegou
    // pacote", e so pollSend() deve consumi-lo).
    bool _sending = false;
    unsigned long _sendStartMs = 0;

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
