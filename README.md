# Sistema de Recepção LoRa para Sondas e Satélites

Este projeto é um sistema de recepção de dados via **LoRa**, voltado para **sondas experimentais e pequenos satélites**.

O objetivo é receber, processar e disponibilizar os dados remotamente através de um **servidor**, permitindo o acompanhamento em tempo real ou pós-voo.

## Versões

- **v1** -- protótipo inicial: um único `.ino` (Arduino IDE), rodando só na placa LilyGo TTGO T3, com display OLED, página web simples de configuração e publicação MQTT fixa.
- **v2** (atual) -- reescrita como projeto **PlatformIO**, com **RadioLib** no lugar da lib LoRa antiga, suporte a **três placas** com o mesmo firmware, **dashboard web** completo (pacotes ao vivo via WebSocket, configuração de rádio/WiFi/MQTT em runtime) e persistência de configuração na flash (NVS/Preferences).

## Funcionalidades

- Recepção de pacotes LoRa (SX1276/RFM96W via RadioLib)
- Republicação dos pacotes recebidos via **MQTT**
- **Dashboard web** para visualização em tempo real (RSSI/SNR/heap/uptime, log de pacotes) e configuração de rádio/WiFi/MQTT, tudo persistido na flash
- Reconfiguração remota também via tópico MQTT dedicado (sem precisar estar na mesma rede)
- Mesmo firmware rodando em três placas diferentes, escolhidas por *environment* do PlatformIO

## Placas suportadas

| Placa | MCU | Display |
|---|---|---|
| PCB-AutonomousAircraft | ESP32-S3 Super Mini | não tem |
| PCB-Sonde | ESP32-S3 Super Mini | não tem |
| LilyGo TTGO LoRa32 T3 v1.6.1 | ESP32 clássico | SSD1306 128x64, de fábrica |

## Status do projeto

**Em desenvolvimento**

Este projeto ainda está em fase experimental. O código pode sofrer mudanças, otimizações e refatorações.

## 🛠️ Tecnologias utilizadas

- ESP32 (S3 e clássico, três placas suportadas)
- LoRa (RadioLib, chip SX1276/RFM96W)
- C/C++, PlatformIO
- Dashboard web (AsyncWebServer + WebSocket, frontend em `data/`)
- MQTT (republicação de telemetria + reconfiguração remota)

## Objetivo

Servir como base para testes, aprendizado e evolução de sistemas de recepção de telemetria para projetos aeroespaciais experimentais.

## Dependências

Ver `firmware/platformio.ini` para as versões exatas. Resumo:

### Comuns às três placas
- jgromes/RadioLib
- knolleary/PubSubClient
- arduino-libraries/Arduino_JSON
- ESP32Async/AsyncTCP
- ESP32Async/ESPAsyncWebServer

### Só na LilyGo T3 (placa com display)
- Adafruit SSD1306
- Adafruit GFX Library
- Adafruit BusIO

## Ambiente de desenvolvimento

- [PlatformIO](https://platformio.org/) (VS Code)
- Framework Arduino para ESP32
