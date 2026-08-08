#pragma once

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include "RF96W.h"
#include "LoraConfig.h"
#include "NetworkRuntimeConfig.h"

// Pagina de dashboard (LittleFS: data/index.html + style.css +
// script.js) + API:
//
//   GET  /               -> pagina
//   GET  /getLora          -> config atual do radio (JSON)
//   POST /setLora           -> aplica + salva config do radio
//   GET  /getWifi            -> ssid atual (sem senha)
//   POST /setWifi             -> pede troca de WiFi (main.cpp aplica
//                                 e reverte sozinho se falhar --
//                                 ver WifiChangeRequest abaixo)
//   GET  /getWifiSecondary     -> lista de redes de fallback, ja
//                                 ordenada por prioridade (sem senha)
//   POST /setWifiSecondary      -> substitui a lista inteira (aplica
//                                 direto, sem risco de derrubar a
//                                 pagina -- so' usada se a principal
//                                 falhar)
//   GET  /getMqtt              -> config MQTT atual (sem senha)
//   POST /setMqtt                -> aplica + salva config MQTT
//   POST /testMqtt                 -> publica o corpo da requisicao
//                                     (texto/JSON livre) no topico
//                                     configurado, pra testar a
//                                     conexao com o broker sem
//                                     esperar um pacote LoRa de
//                                     verdade (ver TestMqttRequest)
//   POST /testLora                  -> transmite o corpo da
//                                     requisicao pelo radio LoRa,
//                                     pra testar o TX sem precisar
//                                     de outro transmissor (ver
//                                     TestLoraRequest)
//   GET  /status                  -> snapshot de status (JSON) --
//                                    ver struct Status abaixo
//   WS   /ws                       -> push em tempo real: pacotes
//                                     recebidos e resultado de cada
//                                     publish MQTT (real ou de teste)
//                                     e de cada envio de teste LoRa
class WebDashboard
{
public:

    // Preenchido pelo main.cpp a cada chamada de status() -- evita
    // a pagina depender de getters espalhados por varios objetos.
    struct Status
    {
        bool wifiConnected;
        String wifiIp;
        bool apActive;
        String apSsid;
        bool mqttEnabled;
        bool mqttConnected;
        unsigned long msSinceLastPacket;
        float lastRssi;
        float lastSnr;
        uint32_t freeHeap;
        unsigned long uptimeMs;
    };

    // Resultado de um POST /setWifi -- main.cpp le isso no loop()
    // pra saber que precisa tentar conectar (e reverter se falhar).
    struct WifiChangeRequest
    {
        bool pending = false;
        String ssid;
        String password;
    };

    // Pedido de publish de teste (POST /testMqtt) -- fica pendente
    // ate main.cpp tratar no loop(). Precisa desse vai-e-volta porque
    // o mqttClient (PubSubClient) vive em main.cpp, nao aqui dentro;
    // fazer o publish direto no handler HTTP tambem travaria a
    // tarefa async_tcp se o broker estiver lento pra responder.
    struct TestMqttRequest
    {
        bool pending = false;
        String payload;
    };

    // Pedido de transmissao de teste (POST /testLora) -- mesmo motivo
    // do TestMqttRequest acima: o objeto `radio` vive em main.cpp.
    struct TestLoraRequest
    {
        bool pending = false;
        String message;
    };

    bool begin(LoRa& radio, LoraRuntimeConfig& loraConfig, NetworkRuntimeConfig& netConfig);

    // main.cpp chama isso sempre que ha algo novo pra reportar --
    // todos fazem "no-op" rapido se nao houver clientes WS conectados.
    void broadcastPacket(const char* packet, float rssi, float snr);
    void broadcastMqttResult(bool ok);
    void broadcastMqttTestResult(bool ok);
    void broadcastLoraSendResult(bool ok);

    // main.cpp chama isso (ex.: a cada 2s) com um snapshot atual --
    // fica guardado pra responder GET /status sem precisar acessar
    // WiFi/MQTT/etc dentro do handler HTTP.
    void setStatus(const Status& status);

    // main.cpp checa isso no loop() pra saber se precisa agir numa
    // troca de WiFi pedida pela pagina; DEVE chamar clearWifiChangeRequest()
    // depois de tratar (com sucesso ou nao).
    const WifiChangeRequest& pendingWifiChange() const { return _wifiChangeRequest; }
    void clearWifiChangeRequest() { _wifiChangeRequest.pending = false; }

    // Mesma logica de pending/clear acima, pros dois pedidos de teste.
    const TestMqttRequest& pendingTestMqtt() const { return _testMqttRequest; }
    void clearTestMqttRequest() { _testMqttRequest.pending = false; }

    const TestLoraRequest& pendingTestLora() const { return _testLoraRequest; }
    void clearTestLoraRequest() { _testLoraRequest.pending = false; }

private:

    AsyncWebServer server{80};
    AsyncWebSocket ws{"/ws"};

    Status _status{};
    WifiChangeRequest _wifiChangeRequest;
    TestMqttRequest   _testMqttRequest;
    TestLoraRequest   _testLoraRequest;

    String statusJson() const;
};
