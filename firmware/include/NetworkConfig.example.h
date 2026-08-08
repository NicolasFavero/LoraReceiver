#pragma once

// TEMPLATE -- copie esse arquivo pra "NetworkConfig.h" (mesma pasta)
// e preencha com os dados reais da sua rede/broker. NetworkConfig.h
// esta no .gitignore de proposito -- so o .example fica versionado,
// pra nao vazar WiFi/senha do broker no repositorio.
//
//   cp include/NetworkConfig.example.h include/NetworkConfig.h
//
// Esses valores so valem pro PRIMEIRO boot da placa (nunca teve
// nada salvo ainda) -- depois disso, WiFi/MQTT passam a viver na
// flash (Preferences/NVS) e sao trocados pelo dashboard web, nao por
// esse arquivo. Ou seja: da pra deixar os placeholders abaixo e
// configurar tudo pela pagina depois do primeiro upload, se preferir
// nunca ter a senha real em nenhum arquivo de texto.
namespace NetConfig
{
    inline constexpr const char* WIFI_SSID     = "SUA_REDE_AQUI";
    inline constexpr const char* WIFI_PASSWORD = "SUA_SENHA_AQUI";

    inline constexpr const char* MQTT_SERVER    = "192.168.0.100";
    inline constexpr uint16_t    MQTT_PORT      = 1883;

    // Deixe "" (vazio) se o broker aceitar conexao anonima.
    inline constexpr const char* MQTT_USER      = "";
    inline constexpr const char* MQTT_PASSWORD  = "";

    inline constexpr const char* MQTT_CLIENT_ID = "esp32-lora-receiver";

    // Topico de publicacao: cada pacote LoRa recebido vira um JSON
    // {"packet":"...","rssi":...,"snr":...} publicado aqui.
    inline constexpr const char* MQTT_TOPIC_TELEMETRY = "telemetry";

    // Topico de assinatura: aceita o MESMO JSON de configuracao do
    // radio que a pagina web (/setLora) aceita -- {"freq":...,
    // "sf":...,"bd":...,"sync":...,"cr":...,"power":...,"crc":...}.
    inline constexpr const char* MQTT_TOPIC_LORA_CONFIG = "lora/config";

    // Senha do ponto de acesso de emergencia (criado automaticamente
    // se nenhuma rede WiFi conhecida conectar). Precisa ter pelo
    // menos 8 caracteres (minimo do WPA2). Impressa na serial toda
    // vez que o AP sobe (ver startFallbackAP() em main.cpp).
    inline constexpr const char* AP_FALLBACK_PASSWORD = "12345678";
}
