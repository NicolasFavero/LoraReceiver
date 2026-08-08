#include "WebDashboard.h"
#include <LittleFS.h>
#include <Arduino_JSON.h>

bool WebDashboard::begin(LoRa& radio, LoraRuntimeConfig& loraConfig, NetworkRuntimeConfig& netConfig)
{
    if (!LittleFS.begin(true)) {
        Serial.println("[WebDashboard] Falha ao montar LittleFS.");
        return false;
    }

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(LittleFS, "/index.html", "text/html");
    });

    server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(LittleFS, "/style.css", "text/css");
    });

    server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(LittleFS, "/script.js", "application/javascript");
    });

    // --- LoRa ---

    server.on("/getLora", HTTP_GET, [&loraConfig](AsyncWebServerRequest *request) {
        request->send(200, "application/json", loraConfig.toJson());
    });

    server.on(
        "/setLora", HTTP_POST,
        [](AsyncWebServerRequest *request) {},
        nullptr,
        [&radio, &loraConfig](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            String json;
            json.reserve(len);
            for (size_t i = 0; i < len; i++) json += (char)data[i];

            if (loraConfig.applyJson(json, radio)) {
                request->send(200, "application/json", loraConfig.toJson());
            } else {
                request->send(400, "text/plain", "Valores invalidos.");
            }
        }
    );

    // --- WiFi ---
    // So VALIDA e guarda o pedido -- main.cpp e quem de fato tenta
    // conectar e reverte se falhar (ver pendingWifiChange()). Fazer
    // isso aqui dentro do handler travaria a tarefa async_tcp
    // durante a tentativa de conexao.

    server.on("/getWifi", HTTP_GET, [&netConfig](AsyncWebServerRequest *request) {
        request->send(200, "application/json", netConfig.wifiStatusJson());
    });

    server.on(
        "/setWifi", HTTP_POST,
        [](AsyncWebServerRequest *request) {},
        nullptr,
        [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            String json;
            json.reserve(len);
            for (size_t i = 0; i < len; i++) json += (char)data[i];

            JSONVar doc = JSON.parse(json);

            if (JSON.typeof(doc) == "undefined" || JSON.typeof(doc["ssid"]) == "undefined") {
                request->send(400, "text/plain", "JSON invalido (precisa de \"ssid\").");
                return;
            }

            String ssid = (const char*)doc["ssid"];
            String password = JSON.typeof(doc["password"]) == "undefined" ? "" : (const char*)doc["password"];

            if (ssid.length() == 0) {
                request->send(400, "text/plain", "SSID vazio.");
                return;
            }

            _wifiChangeRequest.pending  = true;
            _wifiChangeRequest.ssid      = ssid;
            _wifiChangeRequest.password  = password;

            request->send(
                200, "text/plain",
                "Tentando conectar com o novo WiFi -- se a placa sumir da rede em ~15s, "
                "a senha/SSID estavam errados e ele volta sozinho pro WiFi anterior."
            );
        }
    );

    // --- WiFi secundario (fallback com prioridade) ---
    // Diferente do /setWifi acima, aqui NAO tem risco de perder
    // acesso a pagina -- essas redes so' sao usadas se a principal
    // falhar, entao aplica direto, sem logica de reversao.

    server.on("/getWifiSecondary", HTTP_GET, [&netConfig](AsyncWebServerRequest *request) {
        request->send(200, "application/json", netConfig.wifiSecondaryStatusJson());
    });

    server.on(
        "/setWifiSecondary", HTTP_POST,
        [](AsyncWebServerRequest *request) {},
        nullptr,
        [&netConfig](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            String json;
            json.reserve(len);
            for (size_t i = 0; i < len; i++) json += (char)data[i];

            if (netConfig.setWifiSecondaryFromJson(json)) {
                request->send(200, "application/json", netConfig.wifiSecondaryStatusJson());
            } else {
                request->send(400, "text/plain", "JSON invalido (precisa ser um array de {\"ssid\":...,\"password\":...,\"priority\":...}).");
            }
        }
    );

    // --- Testes (MQTT fake publish / TX LoRa avulso) ---
    // Ambos so' guardam o pedido -- main.cpp e quem tem o mqttClient/
    // radio de verdade e trata isso no loop() (ver comentario de
    // TestMqttRequest/TestLoraRequest no header).

    server.on(
        "/testMqtt", HTTP_POST,
        [](AsyncWebServerRequest *request) {},
        nullptr,
        [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            String payload;
            payload.reserve(len);
            for (size_t i = 0; i < len; i++) payload += (char)data[i];

            if (payload.length() == 0) {
                request->send(400, "text/plain", "Mensagem vazia.");
                return;
            }

            _testMqttRequest.pending = true;
            _testMqttRequest.payload = payload;

            request->send(200, "text/plain", "Enviando mensagem de teste ao broker MQTT...");
        }
    );

    server.on(
        "/testLora", HTTP_POST,
        [](AsyncWebServerRequest *request) {},
        nullptr,
        [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            String message;
            message.reserve(len);
            for (size_t i = 0; i < len; i++) message += (char)data[i];

            if (message.length() == 0) {
                request->send(400, "text/plain", "Mensagem vazia.");
                return;
            }

            _testLoraRequest.pending = true;
            _testLoraRequest.message = message;

            request->send(200, "text/plain", "Transmitindo pacote de teste pelo radio...");
        }
    );

    // --- MQTT ---
    // Trocar de broker nao arrisca o acesso a pagina (WiFi continua
    // o mesmo), entao aplica direto, sem a logica de reversao do WiFi.

    server.on("/getMqtt", HTTP_GET, [&netConfig](AsyncWebServerRequest *request) {
        request->send(200, "application/json", netConfig.mqttStatusJson());
    });

    server.on(
        "/setMqtt", HTTP_POST,
        [](AsyncWebServerRequest *request) {},
        nullptr,
        [&netConfig](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            String json;
            json.reserve(len);
            for (size_t i = 0; i < len; i++) json += (char)data[i];

            if (netConfig.setMqttFromJson(json)) {
                request->send(200, "application/json", netConfig.mqttStatusJson());
            } else {
                request->send(400, "text/plain", "JSON invalido (precisa de pelo menos \"server\").");
            }
        }
    );

    // --- Status + WebSocket ---

    server.on("/status", HTTP_GET, [this](AsyncWebServerRequest *request) {
        request->send(200, "application/json", statusJson());
    });

    ws.onEvent([](AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
        // Nao precisa tratar mensagens recebidas do cliente -- e um
        // canal so de push (servidor -> pagina).
    });

    server.addHandler(&ws);

    server.begin();

    return true;
}

void WebDashboard::broadcastPacket(const char* packet, float rssi, float snr)
{
    if (ws.count() == 0) return; // ninguem olhando a pagina agora -- economiza trabalho

    char escaped[300];
    size_t o = 0;
    for (size_t i = 0; packet[i] != '\0' && o < sizeof(escaped) - 2; i++) {
        char c = packet[i];
        if (c == '"' || c == '\\') {
            if (o >= sizeof(escaped) - 3) break;
            escaped[o++] = '\\';
        }
        escaped[o++] = c;
    }
    escaped[o] = '\0';

    char msg[400];
    snprintf(
        msg, sizeof(msg),
        "{\"type\":\"packet\",\"packet\":\"%s\",\"rssi\":%.2f,\"snr\":%.2f}",
        escaped, rssi, snr
    );

    ws.textAll(msg);
}

void WebDashboard::broadcastMqttResult(bool ok)
{
    if (ws.count() == 0) return;

    ws.textAll(ok ? "{\"type\":\"mqtt\",\"ok\":true}" : "{\"type\":\"mqtt\",\"ok\":false}");
}

void WebDashboard::broadcastMqttTestResult(bool ok)
{
    if (ws.count() == 0) return;

    ws.textAll(ok ? "{\"type\":\"mqttTest\",\"ok\":true}" : "{\"type\":\"mqttTest\",\"ok\":false}");
}

void WebDashboard::broadcastLoraSendResult(bool ok)
{
    if (ws.count() == 0) return;

    ws.textAll(ok ? "{\"type\":\"loraSend\",\"ok\":true}" : "{\"type\":\"loraSend\",\"ok\":false}");
}

void WebDashboard::setStatus(const Status& status)
{
    _status = status;
}

String WebDashboard::statusJson() const
{
    char buf[500];
    snprintf(
        buf, sizeof(buf),
        "{\"wifiConnected\":%s,\"wifiIp\":\"%s\",\"apActive\":%s,\"apSsid\":\"%s\","
        "\"mqttEnabled\":%s,\"mqttConnected\":%s,"
        "\"msSinceLastPacket\":%lu,\"lastRssi\":%.2f,\"lastSnr\":%.2f,"
        "\"freeHeap\":%u,\"uptimeMs\":%lu}",
        _status.wifiConnected ? "true" : "false",
        _status.wifiIp.c_str(),
        _status.apActive ? "true" : "false",
        _status.apSsid.c_str(),
        _status.mqttEnabled ? "true" : "false",
        _status.mqttConnected ? "true" : "false",
        _status.msSinceLastPacket,
        _status.lastRssi,
        _status.lastSnr,
        (unsigned)_status.freeHeap,
        _status.uptimeMs
    );
    return String(buf);
}
