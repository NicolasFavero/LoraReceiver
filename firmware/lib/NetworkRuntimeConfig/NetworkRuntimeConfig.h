#pragma once

#include <Arduino.h>
#include <vector>

// WiFi e MQTT em tempo de execucao: carrega da flash (Preferences/NVS,
// namespace "net") ou cai nos defaults de NetworkConfig.h se nunca
// foi salvo nada. Sao DOIS grupos independentes -- WiFi tem logica
// de aplicacao especial (com reversao automatica) em main.cpp, por
// ser o unico que pode derrubar o proprio acesso a pagina se a senha
// nova estiver errada; MQTT nao tem esse risco (perder MQTT nao tira
// o acesso a pagina web).
class NetworkRuntimeConfig
{
public:

    // Uma rede WiFi secundaria (fallback) -- ver comentario de
    // wifiSecondaryNetworks() abaixo.
    struct WifiNetwork
    {
        String ssid;
        String password;
        int    priority = 0; // menor valor = tenta primeiro
    };

    void begin();

    // --- WiFi (rede principal) ---
    String wifiSsid() const { return _wifiSsid; }
    String wifiPassword() const { return _wifiPassword; }

    // So atualiza os campos em memoria + salva -- NAO mexe na conexao
    // WiFi de verdade (isso e responsabilidade de main.cpp, que
    // precisa tentar conectar e reverter se falhar antes de chamar
    // isto).
    void setWifi(const String& ssid, const String& password);

    // --- WiFi secundario (fallback, com prioridade) ---
    // A rede principal (wifiSsid()/wifiPassword() acima) continua
    // sendo sempre a primeira tentativa (prioridade implicita mais
    // alta que qualquer uma dessa lista) -- ela fica de fora de
    // proposito, pra nao duplicar/complicar o fluxo de troca com
    // reversao automatica que ja existe so' pra ela. Essa lista aqui
    // e' so' o que main.cpp tenta DEPOIS, em ordem de prioridade
    // (menor numero primeiro), se a principal falhar.
    //
    // Retorna ja ordenada por prioridade crescente.
    std::vector<WifiNetwork> wifiSecondaryNetworks() const;

    // Substitui a lista inteira. JSON esperado: array de objetos
    // [{"ssid":"...","password":"...","priority":1}, ...].
    // Um "password" vazio ou ausente MANTEM a senha ja salva daquele
    // SSID, se ele ja existia na lista -- assim a pagina pode so'
    // reordenar prioridade sem precisar re-digitar senha de rede que
    // nao mudou.
    bool setWifiSecondaryFromJson(const String& json);

    // --- MQTT ---
    String mqttServer() const { return _mqttServer; }
    uint16_t mqttPort() const { return _mqttPort; }
    String mqttUser() const { return _mqttUser; }
    String mqttPassword() const { return _mqttPassword; }
    String mqttTopic() const { return _mqttTopic; }
    bool mqttEnabled() const { return _mqttEnabled; }

    bool setMqttFromJson(const String& json);

    String wifiStatusJson() const;           // sem senha -- so pra exibir/pre-preencher
    String wifiSecondaryStatusJson() const;   // sem senha
    String mqttStatusJson() const;            // sem senha

private:

    String   _wifiSsid;
    String   _wifiPassword;

    std::vector<WifiNetwork> _wifiSecondary;

    String   _mqttServer;
    uint16_t _mqttPort = 1883;
    String   _mqttUser;
    String   _mqttPassword;
    String   _mqttTopic;
    bool     _mqttEnabled = true;

    void loadWifi();
    void saveWifi() const;

    void loadWifiSecondary();
    void saveWifiSecondary() const;

    void loadMqtt();
    void saveMqtt() const;
};
