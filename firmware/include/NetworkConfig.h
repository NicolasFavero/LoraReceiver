#pragma once

// Preencher com os dados da sua rede e do broker MQTT.
namespace NetConfig
{
    inline constexpr const char* WIFI_SSID     = "TFX.ECO.BR";
    inline constexpr const char* WIFI_PASSWORD = "senhadificil1234";

    inline constexpr const char* MQTT_SERVER    = "192.168.1.34";
    inline constexpr uint16_t    MQTT_PORT      = 1883;

    // Deixe "" (vazio) se o broker aceitar conexao anonima -- e o
    // caso hoje das PCBs AutonomousAircraft/Sonde. Preencha se o
    // broker exigir usuario/senha (ex.: o broker usado pela LilyGo).
    inline constexpr const char* MQTT_USER      = "";
    inline constexpr const char* MQTT_PASSWORD  = "";

    inline constexpr const char* MQTT_CLIENT_ID = "esp32-lora-receiver";

    // Topico de publicacao: cada pacote LoRa recebido vira um JSON
    // {"packet":"...","rssi":...,"snr":...} publicado aqui.
    inline constexpr const char* MQTT_TOPIC_TELEMETRY = "telemetry";

    // Topico de assinatura: aceita o MESMO JSON de configuracao do
    // radio que a pagina web (/setLora) aceita -- {"freq":...,
    // "sf":...,"bd":...,"sync":...,"cr":...,"power":...,"crc":...}.
    // Permite reconfigurar o LoRa remotamente via MQTT, sem precisar
    // estar na mesma rede pra acessar a pagina web.
    inline constexpr const char* MQTT_TOPIC_LORA_CONFIG = "lora/config";

    // Senha do ponto de acesso de emergencia (criado automaticamente
    // se nenhuma rede WiFi conhecida conectar). Precisa ter pelo
    // menos 8 caracteres (minimo do WPA2) -- WiFi.softAP() falha
    // silenciosamente (cria rede aberta, sem senha) se for menor.
    inline constexpr const char* AP_FALLBACK_PASSWORD = "loraconfig123";
}
