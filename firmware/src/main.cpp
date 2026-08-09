#include <Arduino.h>
#include <SPI.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "esp_task_wdt.h"

#include "RF96W.h"
#include "Pins.h"
#include "LoraDefaults.h"
#include "WatchdogConfig.h"
#include "NetworkConfig.h"
#include "LoraConfig.h"
#include "NetworkRuntimeConfig.h"
#include "WebDashboard.h"

#ifdef PCB_LILYGO_T3
#include "Display.h"
#endif

LoRa radio(
    Pins::Radio::CS,
    Pins::Radio::DIO0,
    Pins::Radio::RESET,

    LoraConfig::FREQUENCY,
    LoraConfig::BANDWIDTH,
    LoraConfig::SPREADING_FACTOR,
    LoraConfig::CODING_RATE,
    LoraConfig::SYNC_WORD,
    LoraConfig::POWER,
    LoraConfig::PREAMBLE_LENGTH,
    LoraConfig::CRC_ENABLED
);

LoraRuntimeConfig    loraRuntimeConfig;
NetworkRuntimeConfig netConfig;
WebDashboard         dashboard;

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

unsigned long lastPacketMillis = 0;
float lastRssi = 0.0f;
float lastSnr  = 0.0f;

unsigned long lastStatusPush = 0;
constexpr unsigned long STATUS_PUSH_INTERVAL_MS = 2000;

// --- Troca de WiFi via web, sem risco de "tijolar" o acesso -------
// Nao-bloqueante de proposito: se rodasse dentro do handler HTTP
// (tarefa async_tcp) travaria o servidor todo durante a tentativa.
// Se a credencial nova nao conectar dentro do timeout, volta sozinho
// pra credencial anterior -- sem isso, um SSID/senha digitado errado
// derrubaria o WiFi e junto o unico jeito de corrigir (a propria
// pagina).
enum class WifiChangeState { Idle, Connecting };

WifiChangeState wifiChangeState = WifiChangeState::Idle;
unsigned long   wifiChangeStart = 0;
String          wifiChangeOldSsid, wifiChangeOldPassword;
String          wifiChangeNewSsid, wifiChangeNewPassword;

constexpr unsigned long WIFI_CHANGE_TIMEOUT_MS = 15000;

#ifdef PCB_LILYGO_T3
Display display;
#endif

// --- Diagnostico legivel -------------------------------------------
// WiFi.status() e mqttClient.state() retornam codigos numericos que
// nao dizem nada sozinhos -- "rc=5" nao ajuda ninguem a saber se e
// senha errada, broker fora do ar, ou client ID duplicado. Sem isso
// a unica forma de debugar era abrir o datasheet/header da lib.
const char* wifiStatusToString(wl_status_t status)
{
    switch (status)
    {
        case WL_NO_SSID_AVAIL:  return "rede nao encontrada (SSID errado, fora de alcance, ou rede 5GHz -- ESP32 so' enxerga 2.4GHz)";
        case WL_CONNECT_FAILED: return "falha na autenticacao (senha errada, na grande maioria das vezes)";
        case WL_CONNECTION_LOST:return "conexao perdida depois de conectar";
        case WL_DISCONNECTED:   return "desconectado";
        case WL_IDLE_STATUS:    return "parado/inicializando";
        case WL_CONNECTED:      return "conectado";
        default:                return "status desconhecido";
    }
}

const char* mqttStateToString(int state)
{
    switch (state)
    {
        case -4: return "timeout de conexao (broker nao respondeu -- IP/porta errados, ou fora da rede)";
        case -3: return "conexao perdida";
        case -2: return "falha de conexao (broker recusou a conexao TCP -- confere IP/porta)";
        case -1: return "desconectado";
        case 0:  return "conectado";
        case 1:  return "protocolo MQTT incompativel (versao)";
        case 2:  return "client ID rejeitado pelo broker";
        case 3:  return "servidor MQTT indisponivel";
        case 4:  return "usuario/senha invalidos";
        case 5:  return "nao autorizado (confere usuario/senha e permissoes no broker)";
        default: return "codigo desconhecido";
    }
}

// Escaneia as redes visiveis e imprime na serial -- ajuda a
// diferenciar "SSID digitado errado"/"rede fora de alcance" de
// "senha errada" antes mesmo de tentar conectar.
void printWifiScan()
{
    Serial.println("Redes WiFi visiveis:");

    int n = WiFi.scanNetworks();

    if (n <= 0)
    {
        Serial.println("  (nenhuma rede encontrada -- confere se o WiFi da placa nao esta com algum problema de antena/hardware)");
        return;
    }

    for (int i = 0; i < n; i++)
    {
        Serial.printf("  \"%s\"  (canal %d, %d dBm, %s)\n",
            WiFi.SSID(i).c_str(),
            WiFi.channel(i),
            WiFi.RSSI(i),
            WiFi.encryptionType(i) == WIFI_AUTH_OPEN ? "aberta" : "protegida"
        );
    }

    WiFi.scanDelete();
}

// --- Fallback de WiFi: principal -> secundarias por prioridade -> AP ---
bool   apActive = false;
String apSsid;

// Cria o ponto de acesso de emergencia (WIFI_AP_STA -- mantem
// tentando reconectar em STA por baixo dos panos, sem derrubar o
// AP). So' cria uma vez; chamadas repetidas nao fazem nada.
void startFallbackAP()
{
    if (apActive) return;

    uint8_t mac[6];
    WiFi.macAddress(mac);

    char ssidBuf[32];
    snprintf(ssidBuf, sizeof(ssidBuf), "LoRaReceiver-%02X%02X", mac[4], mac[5]);
    apSsid = ssidBuf;

    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(apSsid.c_str(), NetConfig::AP_FALLBACK_PASSWORD);

    apActive = true;

    Serial.println();
    Serial.println("========================================");
    Serial.println("[WiFi] Nenhuma rede conhecida conectou.");
    Serial.println("[WiFi] Ponto de acesso de emergencia criado:");
    Serial.print("  SSID:  "); Serial.println(apSsid);
    Serial.print("  Senha: "); Serial.println(NetConfig::AP_FALLBACK_PASSWORD);
    Serial.print("  IP:    "); Serial.println(WiFi.softAPIP());
    Serial.println("[WiFi] Conecte nesse WiFi e acesse o IP acima no");
    Serial.println("[WiFi] navegador pra reconfigurar a rede ou conferir");
    Serial.println("[WiFi] os pacotes recebidos. Ele continua tentando");
    Serial.println("[WiFi] reconectar nas redes conhecidas em segundo");
    Serial.println("[WiFi] plano, sem derrubar esse ponto de acesso.");
    Serial.println("========================================");
}

// Tenta UMA rede especifica, ate o timeout. Retorna true e imprime
// os detalhes de conexao se der certo; imprime o motivo (via
// wifiStatusToString()) e retorna false se nao.
bool tryConnectWifi(const String& ssid, const String& password, unsigned long timeoutMs)
{
    if (ssid.length() == 0) return false;

    Serial.print("Conectando ao WiFi \"");
    Serial.print(ssid);
    Serial.println("\"...");

    WiFi.begin(ssid.c_str(), password.c_str());

    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < timeoutMs)
    {
        delay(500);
        Serial.print(".");
        esp_task_wdt_reset(); // WiFi lento nao pode disparar o watchdog aqui
    }

    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println();
        Serial.print("[WiFi] \"");
        Serial.print(ssid);
        Serial.print("\" nao conectou -- motivo: ");
        Serial.println(wifiStatusToString(WiFi.status()));
        return false;
    }

    Serial.println();
    Serial.println("[WiFi] Conectado:");
    Serial.print("  SSID:    "); Serial.println(WiFi.SSID());
    Serial.print("  IP:      "); Serial.println(WiFi.localIP());
    Serial.print("  Gateway: "); Serial.println(WiFi.gatewayIP());
    Serial.print("  RSSI:    "); Serial.print(WiFi.RSSI()); Serial.println(" dBm");

    return true;
}

// Tenta a rede principal, depois cada rede secundaria em ordem de
// prioridade -- se NENHUMA conectar, sobe o AP de emergencia (sem
// travar: o AP fica de pe e o loop() continua tentando essa lista
// de novo, so' que com um intervalo entre tentativas -- ver
// WIFI_RETRY_COOLDOWN_MS no loop()).
void connectWiFi()
{
    if (WiFi.status() == WL_CONNECTED) return;

    if (!apActive) WiFi.mode(WIFI_STA); // com AP ativo, fica em AP_STA pra nao derrubar o AP

    // So' escaneia UMA VEZ (no boot) -- nao a cada retry. Repetir
    // scan+connect em ciclo apertado e' um gatilho conhecido de
    // corrupcao de heap no driver WiFi do ESP32.
    static bool scannedOnce = false;
    if (!scannedOnce)
    {
        printWifiScan();
        esp_task_wdt_reset();
        delay(100);
        scannedOnce = true;
    }

    constexpr unsigned long PER_NETWORK_TIMEOUT_MS = 10000;

    if (tryConnectWifi(netConfig.wifiSsid(), netConfig.wifiPassword(), PER_NETWORK_TIMEOUT_MS))
        return;

    for (const auto& net : netConfig.wifiSecondaryNetworks())
    {
        if (tryConnectWifi(net.ssid, net.password, PER_NETWORK_TIMEOUT_MS))
            return;
    }

    Serial.println("[WiFi] Nenhuma rede da lista (principal + secundarias) conectou.");
    startFallbackAP();
}

// Trata o pedido de troca de WiFi vindo da pagina (POST /setWifi),
// de forma nao-bloqueante -- ver comentario da state machine acima.
void handleWifiChangeRequest()
{
    const auto& pending = dashboard.pendingWifiChange();

    if (pending.pending && wifiChangeState == WifiChangeState::Idle)
    {
        wifiChangeOldSsid     = netConfig.wifiSsid();
        wifiChangeOldPassword = netConfig.wifiPassword();

        wifiChangeNewSsid     = pending.ssid;
        wifiChangeNewPassword = pending.password;

        dashboard.clearWifiChangeRequest();

        Serial.print("[WiFi] Tentando novo SSID: ");
        Serial.println(wifiChangeNewSsid);

        WiFi.disconnect();
        WiFi.begin(wifiChangeNewSsid.c_str(), wifiChangeNewPassword.c_str());

        wifiChangeState = WifiChangeState::Connecting;
        wifiChangeStart = millis();
    }

    if (wifiChangeState == WifiChangeState::Connecting)
    {
        if (WiFi.status() == WL_CONNECTED)
        {
            netConfig.setWifi(wifiChangeNewSsid, wifiChangeNewPassword);
            Serial.println("[WiFi] Novo WiFi conectou -- salvo.");
            wifiChangeState = WifiChangeState::Idle;
        }
        else if (millis() - wifiChangeStart > WIFI_CHANGE_TIMEOUT_MS)
        {
            Serial.println("[WiFi] Novo WiFi nao conectou a tempo -- revertendo.");
            WiFi.disconnect();
            WiFi.begin(wifiChangeOldSsid.c_str(), wifiChangeOldPassword.c_str());
            wifiChangeState = WifiChangeState::Idle;
        }
    }
}

// Trata o pedido de mensagem MQTT de teste vindo da pagina
// (POST /testMqtt) -- publica o payload literal (o que a pagina
// mandar) no topico configurado, so' pra confirmar que o broker esta
// alcancavel, sem precisar esperar um pacote LoRa de verdade chegar.
void handleTestMqttRequest()
{
    const auto& pending = dashboard.pendingTestMqtt();
    if (!pending.pending) return;

    String payload = pending.payload;
    dashboard.clearTestMqttRequest();

    if (!netConfig.mqttEnabled() || !mqttClient.connected())
    {
        Serial.println("[MQTT] Teste solicitado, mas o MQTT esta desabilitado/desconectado.");
        dashboard.broadcastMqttTestResult(false);
        return;
    }

    bool ok = mqttClient.publish(netConfig.mqttTopic().c_str(), payload.c_str());

    Serial.print("[MQTT] Mensagem de teste ");
    Serial.println(ok ? "publicada com sucesso." : "falhou ao publicar (payload maior que o buffer?).");

    dashboard.broadcastMqttTestResult(ok);
}

// Trata o pedido de transmissao de teste vindo da pagina (POST
// /testLora) -- manda o texto direto pelo radio, como se fosse outro
// transmissor, pra testar o TX sem precisar de hardware extra.
//
// NAO-BLOQUEANTE de proposito, mesma logica de handleWifiChangeRequest()
// acima: radio.startSend()/pollSend() usam a API assincrona da
// RadioLib. A versao antiga chamava radio.transmit() (bloqueante) direto
// aqui -- isso prendia o loop() inteiro (WiFi, MQTT, servidor web,
// watchdog) pelo tempo de ar do pacote, e se esse tempo passasse de
// WatchdogConfig::TIMEOUT_MS, o watchdog disparava e reiniciava a
// placa no meio do envio (era esse o motivo do reset ao testar TX).
enum class LoraTestSendState { Idle, Sending };
LoraTestSendState loraTestSendState = LoraTestSendState::Idle;

void handleTestLoraRequest()
{
    if (loraTestSendState == LoraTestSendState::Idle)
    {
        const auto& pending = dashboard.pendingTestLora();
        if (!pending.pending) return;

        String message = pending.message;
        dashboard.clearTestLoraRequest();

        if (radio.startSend(message.c_str()))
        {
            Serial.println("[LoRa] Transmitindo pacote de teste...");
            loraTestSendState = LoraTestSendState::Sending;
        }
        else
        {
            Serial.println("[LoRa] Falha ao iniciar a transmissao de teste.");
            dashboard.broadcastLoraSendResult(false);
        }

        return;
    }

    // Sending: poll ate a RadioLib confirmar (via IRQ) ou estourar o
    // timeout interno do pollSend() -- nao bloqueia nenhuma dessas
    // chamadas, so' consulta o estado atual.
    LoRa::SendResult result = radio.pollSend();
    if (result == LoRa::SendResult::Pending) return;

    bool ok = (result == LoRa::SendResult::Ok);

    Serial.print("[LoRa] Pacote de teste ");
    Serial.println(ok ? "transmitido com sucesso." : "falhou ao transmitir.");

    dashboard.broadcastLoraSendResult(ok);
    loraTestSendState = LoraTestSendState::Idle;
}

void onMqttMessage(char* topic, byte* payload, unsigned int length)
{
    if (strcmp(topic, NetConfig::MQTT_TOPIC_LORA_CONFIG) != 0) return;
    if (length > 200) return;

    char json[201];
    memcpy(json, payload, length);
    json[length] = '\0';

    if (loraRuntimeConfig.applyJson(String(json), radio))
    {
        Serial.println("[MQTT] Config do LoRa atualizada via topico de config.");
    }
    else
    {
        Serial.println("[MQTT] JSON de config invalido -- ignorado.");
    }
}

// PubSubClient::setServer() so' guarda o PONTEIRO que recebe, nao
// copia a string -- por isso precisa de um buffer com vida longa
// que o main.cpp seja dono. Antes disso era
// "mqttClient.setServer(netConfig.mqttServer().c_str(), ...)":
// netConfig.mqttServer() retorna String POR VALOR (uma copia
// temporaria), e essa temporaria e destruida logo depois dessa
// linha -- o ponteiro guardado dentro do mqttClient ficava
// pendurado (dangling), apontando pra memoria ja liberada. Na hora
// de conectar de verdade (bem depois), o hostname lido era lixo --
// era esse o motivo do "DNS Failed for '<lixo>'".
String   appliedMqttServer;
uint16_t appliedMqttPort = 0;

// Tambem resolve de quebra um problema relacionado: trocar o broker
// pela pagina (/setMqtt) so' atualizava o NetworkRuntimeConfig, sem
// nunca re-chamar setServer() -- a troca so "pegava" depois de
// reiniciar a placa. Chamando isso a cada loop() (comparacao barata,
// so reaplica quando muda de verdade), a troca pela pagina passa a
// valer na hora: forca desconectar do broker antigo e o
// connectMQTT() normal do loop() reconecta no novo.
void applyMqttServerIfChanged()
{
    if (netConfig.mqttServer() == appliedMqttServer && netConfig.mqttPort() == appliedMqttPort)
        return;

    appliedMqttServer = netConfig.mqttServer(); // copia com vida longa, dona do proprio buffer
    appliedMqttPort   = netConfig.mqttPort();

    mqttClient.setServer(appliedMqttServer.c_str(), appliedMqttPort);

    Serial.print("[MQTT] Config de broker (re)aplicada: ");
    Serial.print(appliedMqttServer);
    Serial.print(":");
    Serial.println(appliedMqttPort);

    if (mqttClient.connected())
    {
        mqttClient.disconnect(); // forca reconectar no broker novo
    }
}

void connectMQTT()
{
    if (!netConfig.mqttEnabled()) return;

    // Sem limite aqui, um broker fora do ar travava o setup() pra
    // sempre -- e como dashboard.begin() so' roda DEPOIS dessa
    // chamada, a pagina web (unico jeito de corrigir a config do
    // MQTT remotamente) nunca subia. 20s e' mais que suficiente pro
    // caso normal (broker no ar responde na primeira tentativa) e
    // ainda da uns 8-9 retries se a rede estiver so' lenta.
    constexpr unsigned long MQTT_CONNECT_TIMEOUT_MS = 20000;
    unsigned long start = millis();

    while (!mqttClient.connected() && millis() - start < MQTT_CONNECT_TIMEOUT_MS)
    {
        if (!netConfig.mqttEnabled()) return; // foi desabilitado durante a tentativa

        Serial.print("Conectando ao broker MQTT ");
        Serial.print(netConfig.mqttServer());
        Serial.print(":");
        Serial.print(netConfig.mqttPort());
        Serial.print("...");

        bool connected;

        if (netConfig.mqttUser().length() > 0)
        {
            connected = mqttClient.connect(
                NetConfig::MQTT_CLIENT_ID,
                netConfig.mqttUser().c_str(),
                netConfig.mqttPassword().c_str()
            );
        }
        else
        {
            connected = mqttClient.connect(NetConfig::MQTT_CLIENT_ID);
        }

        if (connected)
        {
            Serial.println(" conectado.");
            mqttClient.subscribe(NetConfig::MQTT_TOPIC_LORA_CONFIG);
        }
        else
        {
            Serial.print(" falhou (");
            Serial.print(mqttStateToString(mqttClient.state()));
            Serial.println(") -- tentando de novo em 2s");
            delay(2000);
        }

        esp_task_wdt_reset(); // broker fora do ar nao pode disparar o watchdog
    }

    if (!mqttClient.connected())
    {
        Serial.println("[MQTT] Nao conectou a tempo -- seguindo sem MQTT por enquanto, dashboard/LoRa sobem normal e o loop() tenta de novo depois.");
    }
}

// Escapa aspas e barras invertidas antes de embutir o pacote (que
// já é texto/JSON vindo do transmissor) dentro do campo "packet" do
// JSON externo -- sem isso, qualquer aspas interna do pacote (comum
// em telemetria formatada como JSON) quebra a sintaxe do JSON
// publicado, nao importa o tamanho do pacote.
void escapeJson(const char* in, char* out, size_t outSize)
{
    size_t o = 0;

    for (size_t i = 0; in[i] != '\0' && o < outSize - 2; i++)
    {
        char c = in[i];

        if (c == '"' || c == '\\')
        {
            if (o >= outSize - 3) break;
            out[o++] = '\\';
        }

        out[o++] = c;
    }

    out[o] = '\0';
}

bool publishPacket()
{
    char escaped[512];
    escapeJson(radio.getPacket(), escaped, sizeof(escaped));

    char payload[600];

    snprintf(
        payload,
        sizeof(payload),
        "{\"data\":\"%s\",\"rssi\":%.2f,\"snr\":%.2f}",
        escaped,
        radio.getRSSI(),
        radio.getSNR()
    );

    bool ok = mqttClient.publish(netConfig.mqttTopic().c_str(), payload);

    if (!ok)
    {
        Serial.println("[MQTT] publish() falhou -- payload maior que o buffer configurado?");
    }

    return ok;
}

void setup()
{
    Serial.begin(115200);

    // Watchdog -- API atual do Arduino-ESP32 (core 3.x / IDF 5.x).
    // A assinatura antiga, esp_task_wdt_init(timeout, panic), nao
    // existe mais; agora e uma struct de config.
    //
    // IMPORTANTE: o proprio framework Arduino-ESP32 ja inicializa o
    // TWDT sozinho no boot, com o timeout padrao do sdkconfig (bem
    // curto). esp_task_wdt_init() so tem efeito na PRIMEIRA
    // inicializacao -- chamado de novo aqui, ele falha com
    // ESP_ERR_INVALID_STATE e e IGNORADO, deixando o timeout curto
    // do framework valendo por baixo dos panos (foi exatamente isso
    // que fazia o reset acontecer sempre, independente do valor
    // configurado em WatchdogConfig::TIMEOUT_MS). Quando isso
    // acontece, esp_task_wdt_reconfigure() e quem realmente aplica
    // o timeout desejado.
    esp_task_wdt_config_t wdtConfig = {
        .timeout_ms = WatchdogConfig::TIMEOUT_MS,
        .idle_core_mask = 0,
        .trigger_panic = true
    };

    if (esp_task_wdt_init(&wdtConfig) == ESP_ERR_INVALID_STATE)
    {
        esp_task_wdt_reconfigure(&wdtConfig);
    }

    esp_task_wdt_add(NULL);

    netConfig.begin(); // carrega WiFi/MQTT salvos (ou defaults de NetworkConfig.h)

    SPI.begin(Pins::SPI::SCK, Pins::SPI::MISO, Pins::SPI::MOSI);

    while (!radio.begin())
    {
        Serial.println("Falha ao iniciar o radio LoRa -- tentando novamente...");
        delay(500);
        esp_task_wdt_reset();
    }

    loraRuntimeConfig.begin(radio);

    Serial.println("Radio LoRa pronto. Aguardando mensagens...");

    connectWiFi();

    applyMqttServerIfChanged();
    mqttClient.setCallback(onMqttMessage);

    // PubSubClient limita pacotes a 128 bytes por padrao (MQTT_MAX_PACKET_SIZE).
    // A telemetria atual (packet+rssi+snr) passa disso facilmente --
    // sem aumentar aqui, publish() falha silenciosamente pra
    // qualquer pacote acima de ~128 bytes, sem erro nenhum no log.
    mqttClient.setBufferSize(600);

    connectMQTT();

    if (!dashboard.begin(radio, loraRuntimeConfig, netConfig))
    {
        Serial.println("Falha ao subir o dashboard (LittleFS) -- MQTT e LoRa continuam funcionando normalmente.");
    }

#ifdef PCB_LILYGO_T3
    if (display.begin())
    {
        display.showBootInfo("LoRa Receiver OK", WiFi.localIP().toString().c_str());
    }
    else
    {
        Serial.println("OLED nao respondeu -- seguindo sem display.");
    }
#endif

    lastPacketMillis = millis();
}

void loop()
{
    esp_task_wdt_reset();

    handleWifiChangeRequest();
    handleTestMqttRequest();
    handleTestLoraRequest();

    // Antes do AP de emergencia existir, tenta a cada ciclo (rapido
    // no boot). Depois que o AP sobe, um cooldown entre tentativas
    // evita que o loop() fique preso re-tentando a lista inteira
    // (principal + secundarias, ~10s cada) sem parar -- isso travaria
    // o proprio dashboard hospedado no AP, o que anula o proposito
    // dele existir.
    constexpr unsigned long WIFI_RETRY_COOLDOWN_MS = 60000;
    static unsigned long lastWifiAttempt = 0;

    if (wifiChangeState == WifiChangeState::Idle && WiFi.status() != WL_CONNECTED)
    {
        if (!apActive || millis() - lastWifiAttempt > WIFI_RETRY_COOLDOWN_MS)
        {
            lastWifiAttempt = millis();
            connectWiFi();
        }
    }

    if (netConfig.mqttEnabled())
    {
        applyMqttServerIfChanged();

        if (!mqttClient.connected())
        {
            connectMQTT();
        }
        mqttClient.loop();
    }
    else if (mqttClient.connected())
    {
        mqttClient.disconnect(); // foi desabilitado pela pagina -- derruba a conexao ativa
    }

    if (radio.available() && radio.receive())
    {
        radio.print();

        lastPacketMillis = millis();
        lastRssi = radio.getRSSI();
        lastSnr  = radio.getSNR();

        dashboard.broadcastPacket(radio.getPacket(), lastRssi, lastSnr);

        if (netConfig.mqttEnabled() && mqttClient.connected())
        {
            bool ok = publishPacket();
            dashboard.broadcastMqttResult(ok);
        }

#ifdef PCB_LILYGO_T3
        display.showMessage((int)lastRssi, lastSnr, radio.getPacket());
#endif
    }

#ifdef PCB_LILYGO_T3
    if (millis() - lastPacketMillis > 10000)
    {
        display.showNoMessage();
    }
#endif

    if (millis() - lastStatusPush > STATUS_PUSH_INTERVAL_MS)
    {
        lastStatusPush = millis();

        WebDashboard::Status status;
        status.wifiConnected      = (WiFi.status() == WL_CONNECTED);
        status.wifiIp              = WiFi.localIP().toString();
        status.apActive             = apActive;
        status.apSsid                = apSsid;
        status.mqttEnabled         = netConfig.mqttEnabled();
        status.mqttConnected       = mqttClient.connected();
        status.msSinceLastPacket   = millis() - lastPacketMillis;
        status.lastRssi             = lastRssi;
        status.lastSnr              = lastSnr;
        status.freeHeap             = ESP.getFreeHeap();
        status.uptimeMs             = millis();

        dashboard.setStatus(status);
    }
}
