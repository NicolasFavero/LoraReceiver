# Sistema de Recepção LoRa para Sondas e Satélites (Protótipo)

Este projeto é um **protótipo** de um sistema de recepção de dados via **LoRa**, voltado para **sondas experimentais e pequenos satélites**.

O objetivo é receber, processar e disponibilizar os dados remotamente através de um **servidor**, permitindo o acompanhamento em tempo real ou pós-voo.

## Funcionalidades

- Recepção de pacotes LoRa
- Decodificação e organização dos dados recebidos
- Interface web para visualização das informações
- Registro dos dados para análise posterior
- Estrutura preparada para expansão (novos sensores, protocolos, etc.)

## Status do projeto

**Protótipo / Em desenvolvimento**

Este projeto ainda está em fase experimental.  
O código pode sofrer mudanças, otimizações e refatorações.

## 🛠️ Tecnologias utilizadas

- ESP32(LilyGo T3 v1.6.1)
- LoRa
- C/C++
- Sistema web (frontend + backend simples)
- Comunicação serial / rádio

## Objetivo

Servir como base para testes, aprendizado e evolução de sistemas de recepção de telemetria para projetos aeroespaciais experimentais.

## Dependências:

### Adafruit
- Adafruit GFX Library — v1.12.4
- Adafruit SSD1306 — v2.5.16

### Arduino
- Arduino_JSON — v0.2.0
- SPI — incluído no core
- Wire — incluído no core

### Espressif Systems
- WiFi — incluído no core
- Preferences — incluído no core
- esp_task_wdt — incluído no core

### me-no-dev
- AsyncTCP — v3.4.10
- ESPAsyncWebServer — v3.9.5

### Nick O’Leary
- PubSubClient — v2.8

### Sandeep Mistry
- LoRa — v0.8.0


## Ambiente de desenvolvimento

- Arduino IDE 2.3.7
- Arduino Core for ESP32 — v2.x

