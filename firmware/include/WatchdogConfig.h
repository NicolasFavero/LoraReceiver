#pragma once

#include <cstdint>

// Timeout do task watchdog (ms). Se o loop() travar por mais tempo
// que isso sem chamar esp_task_wdt_reset(), o ESP32 reinicia sozinho
// -- protecao contra travamentos silenciosos (ex.: WiFi/MQTT
// pendurado) em um dispositivo sem tela pra avisar (exceto LilyGo).
//
// Ver o comentario em src/main.cpp sobre esp_task_wdt_reconfigure():
// o Arduino-ESP32 ja inicializa o watchdog sozinho no boot com um
// timeout curto do sdkconfig -- so mudar esse valor aqui nao basta
// se o codigo em main.cpp nao tratar o caso "ja inicializado".
namespace WatchdogConfig
{
    inline constexpr uint32_t TIMEOUT_MS = 15000;
}
