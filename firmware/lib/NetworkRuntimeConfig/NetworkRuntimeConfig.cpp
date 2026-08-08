#include "NetworkRuntimeConfig.h"
#include <Preferences.h>
#include <Arduino_JSON.h>
#include <algorithm>
#include "NetworkConfig.h"

void NetworkRuntimeConfig::begin()
{
    loadWifi();
    loadWifiSecondary();
    loadMqtt();
}

void NetworkRuntimeConfig::loadWifi()
{
    // Defaults primeiro -- se o namespace "net" nunca foi salvo
    // (1o boot), prefs.begin(..., true) FALHA (nao pode criar em
    // modo somente-leitura), e ler de um Preferences com begin()
    // falho retorna lixo em vez do default -- foi isso que corrompia
    // o endereco do broker MQTT. So le de verdade se begin() deu certo.
    _wifiSsid     = NetConfig::WIFI_SSID;
    _wifiPassword = NetConfig::WIFI_PASSWORD;

    Preferences prefs;
    if (!prefs.begin("net", true)) return;

    _wifiSsid     = prefs.getString("wifiSsid", NetConfig::WIFI_SSID);
    _wifiPassword = prefs.getString("wifiPass", NetConfig::WIFI_PASSWORD);

    prefs.end();
}

void NetworkRuntimeConfig::saveWifi() const
{
    Preferences prefs;
    prefs.begin("net", false);

    prefs.putString("wifiSsid", _wifiSsid);
    prefs.putString("wifiPass", _wifiPassword);

    prefs.end();
}

void NetworkRuntimeConfig::setWifi(const String& ssid, const String& password)
{
    _wifiSsid     = ssid;
    _wifiPassword = password;
    saveWifi();
}

// Lista secundaria guardada como UM JSON serializado dentro de uma
// unica chave do Preferences -- ele nao tem suporte nativo a array/
// struct, so tipos escalares e string. Mesma tecnica ja usada em
// outros lugares do projeto (ex.: LoraRuntimeConfig nao usa isso,
// mas o padrao "serializa pra string, guarda como string" e' comum
// nesse tipo de storage.
void NetworkRuntimeConfig::loadWifiSecondary()
{
    _wifiSecondary.clear();

    Preferences prefs;
    if (!prefs.begin("net", true)) return;

    String json = prefs.getString("wifiSecJson", "[]");
    prefs.end();

    JSONVar arr = JSON.parse(json);
    if (JSON.typeof(arr) != "array") return;

    for (int i = 0; i < arr.length(); i++)
    {
        if (JSON.typeof(arr[i]["ssid"]) == "undefined") continue;

        WifiNetwork net;
        net.ssid     = (const char*)arr[i]["ssid"];
        net.password = JSON.typeof(arr[i]["password"]) == "undefined" ? "" : (const char*)arr[i]["password"];
        net.priority = JSON.typeof(arr[i]["priority"]) == "undefined" ? 0 : (int)(double)arr[i]["priority"];

        if (net.ssid.length() == 0) continue;

        _wifiSecondary.push_back(net);
    }
}

void NetworkRuntimeConfig::saveWifiSecondary() const
{
    JSONVar arr;

    for (size_t i = 0; i < _wifiSecondary.size(); i++)
    {
        JSONVar entry;
        entry["ssid"]     = _wifiSecondary[i].ssid;
        entry["password"] = _wifiSecondary[i].password;
        entry["priority"] = _wifiSecondary[i].priority;
        arr[i] = entry;
    }

    Preferences prefs;
    prefs.begin("net", false);
    prefs.putString("wifiSecJson", JSON.stringify(arr));
    prefs.end();
}

std::vector<NetworkRuntimeConfig::WifiNetwork> NetworkRuntimeConfig::wifiSecondaryNetworks() const
{
    std::vector<WifiNetwork> sorted = _wifiSecondary;

    std::sort(sorted.begin(), sorted.end(), [](const WifiNetwork& a, const WifiNetwork& b) {
        return a.priority < b.priority;
    });

    return sorted;
}

bool NetworkRuntimeConfig::setWifiSecondaryFromJson(const String& json)
{
    JSONVar arr = JSON.parse(json);
    if (JSON.typeof(arr) != "array") return false;

    std::vector<WifiNetwork> newList;

    for (int i = 0; i < arr.length(); i++)
    {
        if (JSON.typeof(arr[i]["ssid"]) == "undefined") continue;

        WifiNetwork net;
        net.ssid     = (const char*)arr[i]["ssid"];
        net.priority = JSON.typeof(arr[i]["priority"]) == "undefined" ? 0 : (int)(double)arr[i]["priority"];

        String newPassword = JSON.typeof(arr[i]["password"]) == "undefined" ? "" : (const char*)arr[i]["password"];

        if (net.ssid.length() == 0) continue;

        // Senha vazia -- tenta manter a senha ja salva desse SSID
        // (se existia antes), pra pagina nao precisar re-mandar
        // senha de rede que so' teve a prioridade mudada.
        if (newPassword.length() == 0)
        {
            for (size_t j = 0; j < _wifiSecondary.size(); j++)
            {
                if (_wifiSecondary[j].ssid == net.ssid)
                {
                    newPassword = _wifiSecondary[j].password;
                    break;
                }
            }
        }

        net.password = newPassword;
        newList.push_back(net);
    }

    _wifiSecondary = newList;
    saveWifiSecondary();

    return true;
}

void NetworkRuntimeConfig::loadMqtt()
{
    // Mesmo motivo do loadWifi() -- defaults primeiro, so sobrescreve
    // se o Preferences abriu de verdade.
    _mqttServer   = NetConfig::MQTT_SERVER;
    _mqttPort     = NetConfig::MQTT_PORT;
    _mqttUser     = NetConfig::MQTT_USER;
    _mqttPassword = NetConfig::MQTT_PASSWORD;
    _mqttTopic    = NetConfig::MQTT_TOPIC_TELEMETRY;
    _mqttEnabled  = true;

    Preferences prefs;
    if (!prefs.begin("net", true)) return;

    _mqttServer  = prefs.getString("mqttServer", NetConfig::MQTT_SERVER);
    _mqttPort    = (uint16_t)prefs.getUShort("mqttPort", NetConfig::MQTT_PORT);
    _mqttUser    = prefs.getString("mqttUser", NetConfig::MQTT_USER);
    _mqttPassword = prefs.getString("mqttPass", NetConfig::MQTT_PASSWORD);
    _mqttTopic   = prefs.getString("mqttTopic", NetConfig::MQTT_TOPIC_TELEMETRY);
    _mqttEnabled = prefs.getBool("mqttEn", true);

    prefs.end();
}

void NetworkRuntimeConfig::saveMqtt() const
{
    Preferences prefs;
    prefs.begin("net", false);

    prefs.putString("mqttServer", _mqttServer);
    prefs.putUShort("mqttPort", _mqttPort);
    prefs.putString("mqttUser", _mqttUser);
    prefs.putString("mqttPass", _mqttPassword);
    prefs.putString("mqttTopic", _mqttTopic);
    prefs.putBool("mqttEn", _mqttEnabled);

    prefs.end();
}

bool NetworkRuntimeConfig::setMqttFromJson(const String& json)
{
    JSONVar doc = JSON.parse(json);

    if (JSON.typeof(doc) == "undefined") return false;
    if (JSON.typeof(doc["server"]) == "undefined") return false;

    String newServer = (const char*)doc["server"];
    long   newPort    = (long)(double)doc["port"];
    String newUser    = JSON.typeof(doc["user"]) == "undefined" ? "" : (const char*)doc["user"];
    String newPass     = JSON.typeof(doc["password"]) == "undefined" ? "" : (const char*)doc["password"];
    bool   newEnabled  = (bool)doc["enabled"];

    // Topico e opcional no JSON -- se nao vier (ou vier vazio),
    // mantem o que ja estava configurado em vez de zerar.
    String newTopic = JSON.typeof(doc["topic"]) == "undefined" ? "" : (const char*)doc["topic"];
    if (newTopic.length() == 0) newTopic = _mqttTopic;

    if (newServer.length() == 0) return false;
    if (newPort < 1 || newPort > 65535) return false;
    if (newTopic.length() == 0) return false;

    _mqttServer  = newServer;
    _mqttPort    = (uint16_t)newPort;
    _mqttUser    = newUser;
    _mqttPassword = newPass;
    _mqttTopic   = newTopic;
    _mqttEnabled = newEnabled;

    saveMqtt();

    return true;
}

String NetworkRuntimeConfig::wifiStatusJson() const
{
    String json = "{\"ssid\":\"" + _wifiSsid + "\"}";
    return json;
}

String NetworkRuntimeConfig::wifiSecondaryStatusJson() const
{
    JSONVar arr;
    std::vector<WifiNetwork> sorted = wifiSecondaryNetworks();

    for (size_t i = 0; i < sorted.size(); i++)
    {
        JSONVar entry;
        entry["ssid"]     = sorted[i].ssid;
        entry["priority"] = sorted[i].priority;
        arr[i] = entry; // sem senha de proposito
    }

    return JSON.stringify(arr);
}

String NetworkRuntimeConfig::mqttStatusJson() const
{
    char buf[320];
    snprintf(
        buf, sizeof(buf),
        "{\"server\":\"%s\",\"port\":%u,\"user\":\"%s\",\"topic\":\"%s\",\"enabled\":%s}",
        _mqttServer.c_str(), _mqttPort, _mqttUser.c_str(), _mqttTopic.c_str(),
        _mqttEnabled ? "true" : "false"
    );
    return String(buf);
}
