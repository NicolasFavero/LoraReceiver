# LoraReceiver

Firmware PlatformIO de recepção LoRa (SX1276/RFM96W via RadioLib) com
republicação MQTT e um **dashboard web** (pacotes ao vivo, status de
WiFi/MQTT, e configuração de rádio/WiFi/MQTT em runtime, tudo
persistido na flash). Roda, com o mesmo código-fonte, em três placas.

## Antes de compilar: credenciais de WiFi/MQTT

`include/NetworkConfig.h` (o arquivo com o SSID/senha reais) **não é
versionado** -- está no `.gitignore`. O que fica no repositório é só
`include/NetworkConfig.example.h`, com valores fictícios:

```
cp include/NetworkConfig.example.h include/NetworkConfig.h
```

Edite o `NetworkConfig.h` recém-criado com os dados reais. Ele só
importa no **primeiro boot** da placa (quando ainda não há nada
salvo na flash) -- depois disso, WiFi/MQTT passam a ser configurados
pelo dashboard web e vivem no NVS, não nesse arquivo. Se preferir
nunca ter a senha real em nenhum arquivo de texto, dá pra deixar os
placeholders do `.example` mesmo e configurar tudo direto pela
página depois do primeiro upload.

## Estrutura

```
PCB-LoraReceiver/
├── platformio.ini
├── scripts/
│   └── auto_uploadfs.py         <- faz o upload normal subir o filesystem junto (ver abaixo)
├── include/
│   ├── NetworkConfig.example.h  <- modelo versionado (sem segredos)
│   ├── NetworkConfig.h          <- copia local, COM credenciais reais (gitignored)
│   ├── Pins.h                   <- SO pinagem: escolhe a pinagem certa por #ifdef de placa
│   ├── PinsAutonomousAircraft.h
│   ├── PinsSonde.h
│   ├── PinsLilyGoT3.h
│   ├── LoraDefaults.h            <- defaults de radio (freq/SF/BW/...), iguais nas 3 placas
│   └── WatchdogConfig.h          <- timeout do task watchdog
├── lib/
│   ├── LoRaReceiver/              <- driver do rádio (RadioLib), classe LoRa
│   ├── LoraConfig/                <- valida + persiste (NVS) + aplica config do rádio
│   ├── NetworkRuntimeConfig/       <- idem, pra WiFi/MQTT
│   ├── WebDashboard/                <- servidor HTTP+WebSocket (AsyncWebServer + LittleFS)
│   └── Display/                     <- só compila algo se PCB_LILYGO_T3 (OLED)
├── data/                            <- dashboard web, subido via LittleFS
│   ├── index.html
│   ├── style.css
│   └── script.js
└── src/
    └── main.cpp
```

## As três placas, um firmware só

| Environment | Placa | MCU | Display |
|---|---|---|---|
| `pcb-autonomous-aircraft` | PCB-AutonomousAircraft | ESP32-S3 Super Mini | não tem |
| `pcb-sonde` | PCB-Sonde | ESP32-S3 Super Mini | não tem |
| `pcb-lilygo-t3` | LilyGo TTGO LoRa32 T3 v1.6.1 | ESP32 clássico | **SSD1306 128x64, de fábrica** |

```
pio run -e pcb-autonomous-aircraft
pio run -e pcb-sonde
pio run -e pcb-lilygo-t3
```

## Gravando: upload + filesystem juntos, automaticamente

O dashboard mora em `data/` e precisa ser gravado numa partição
separada da flash (LittleFS), diferente do `--target upload` normal
do firmware. Pra não ter que lembrar disso toda vez,
`scripts/auto_uploadfs.py` faz o **upload comum já disparar o
uploadfs sozinho antes** (via `extra_scripts` no `platformio.ini`) --
ou seja, o botão de sempre (seta no VS Code / `pio run --target
upload`) já resolve os dois.

Isso NÃO desgasta a flash de forma relevante: a flash SPI do ESP32
aguenta algo como 100.000 ciclos de escrita por setor, e o LittleFS
já faz *wear leveling* (distribui as escritas). O dashboard tem
menos de 20 KB — regravar isso em todo upload, mesmo durante meses
de desenvolvimento, fica muito abaixo desse limite.

Se preferir subir só o filesystem manualmente (sem recompilar o
firmware), o alvo antigo continua funcionando:
```
pio run -e <environment> --target uploadfs
```

## O dashboard

Acesse `http://<ip-da-placa>/` (o IP aparece no monitor serial ao
conectar no WiFi). Quatro abas:

- **Ao vivo** -- RSSI/SNR/heap/uptime atualizados a cada 2s, e um
  log de pacotes recebidos em tempo real (via WebSocket, `/ws`) --
  cada linha mostra o pacote, RSSI/SNR daquela recepção, e se o
  publish MQTT daquele pacote deu certo (`[MQTT OK]`/`[MQTT FALHOU]`).
- **LoRa** -- frequência, SF, BW, sync word, coding rate, potência,
  CRC. Aplica na hora e persiste.
- **WiFi** -- troca SSID/senha. Tem uma proteção importante: a troca
  roda de forma assíncrona no firmware (não trava o servidor) e, se
  a rede/senha nova não conectar em ~15s, **reverte sozinho pra rede
  anterior** -- sem isso, um erro de digitação te tiraria do único
  jeito de corrigir (a própria página).
- **MQTT** -- broker, porta, usuário/senha, e um interruptor pra
  **desabilitar o MQTT inteiro** (só LoRa + dashboard, sem publicar
  nem tentar conectar em broker nenhum).

Toda config de rádio/MQTT é aceita tanto pelo dashboard quanto,
como antes, pelo tópico MQTT `lora/config` (mesmo JSON) -- útil pra
reconfigurar remotamente sem estar na mesma rede.

### Endpoints, se quiser integrar com outra coisa

| Método | Rota | O que faz |
|---|---|---|
| GET | `/getLora` / `/getWifi` / `/getMqtt` | config atual (sem senhas) |
| POST | `/setLora` / `/setWifi` / `/setMqtt` | aplica + salva |
| GET | `/status` | snapshot: WiFi, MQTT, último pacote, heap, uptime |
| WS | `/ws` | push: `{"type":"packet",...}` e `{"type":"mqtt","ok":...}` |

## O que mudou nesse merge

Esse firmware é resultado de duas rodadas de merge: primeiro juntou
duas bases separadas (RadioLib + duas placas sem display, e um
`.ino` da LilyGo com display/watchdog/config web antiga); depois
ganhou o dashboard atual. Pontos que valem registrar:

### Watchdog corrigido

A API antiga (`esp_task_wdt_init(15, true)`) não existe mais no
Arduino-ESP32 atual (core baseado em IDF 5.x). Agora usa a struct
`esp_task_wdt_config_t`. Timeout configurável em
`include/WatchdogConfig.h`.

**Detalhe importante:** o próprio Arduino-ESP32 já inicializa o
watchdog sozinho no boot, com um timeout curto vindo do `sdkconfig`
do framework. `esp_task_wdt_init()` só tem efeito na primeira
inicialização -- chamado de novo (como fazíamos), falha
silenciosamente com `ESP_ERR_INVALID_STATE` e é ignorado, deixando o
timeout curto do framework valendo por baixo dos panos (o valor
configurado aqui nunca era aplicado de verdade). `main.cpp` agora
detecta esse caso e chama `esp_task_wdt_reconfigure()`, que é quem
realmente aplica o timeout desejado quando o watchdog já estava
ativo.

### Preferences corrompendo defaults no primeiro boot -- corrigido

Quando um namespace do NVS nunca foi salvo (1º boot da placa),
`Preferences::begin(namespace, true)` (modo somente-leitura) falha
-- e ler de um `Preferences` cujo `begin()` falhou retorna **lixo de
memória**, não o valor default passado como argumento. Isso
corrompia o endereço do broker MQTT logo no primeiro boot (virava
uma string de lixo, DNS falhava, e cada tentativa demorava o
suficiente pra disparar o watchdog antes dele ser corrigido). Agora
`NetworkRuntimeConfig`/`LoraConfig` checam o retorno de `begin()` e
só leem se ele realmente abriu -- caso contrário, ficam nos defaults
de `NetworkConfig.h`/`LoraDefaults.h` de propósito.

### `Pins.h` reorganizado

Antes tinha, além da pinagem, dois `namespace` que não eram pinos
(`LoraConfig` com os defaults de rádio, `WatchdogConfig` com o
timeout). Cada um agora mora no arquivo com o nome certo:
`include/LoraDefaults.h` e `include/WatchdogConfig.h`. `Pins.h`
ficou só com a lógica de escolher a pinagem certa por placa.

### Bug do MQTT com pacotes grandes -- corrigido

`PubSubClient` limita pacotes a **128 bytes por padrão**
(`MQTT_MAX_PACKET_SIZE`). Telemetria mais completa (vários campos)
passa disso facilmente, e `publish()` falhava **silenciosamente**
(sem nenhum erro no log) assim que o pacote passava do limite --
`setup()` agora chama `mqttClient.setBufferSize(600)` pra isso não
acontecer mais, e `publishPacket()` loga se `publish()` falhar por
qualquer motivo.

### Bug de JSON quebrado -- corrigido

O pacote recebido do LoRa (que já costuma ser um JSON) era colado
sem escapar dentro do campo `"packet":"..."` do JSON publicado --
qualquer aspas interna quebrava a sintaxe pra quem consumisse o
MQTT do outro lado. Agora escapa aspas/barra invertida antes de
montar o payload (`escapeJson()` em `main.cpp`).

### Sanitização de pacote preservada

Ignora um possível byte `0x00` inicial (comum em transmissores
Dorji) e filtra só ASCII imprimível -- em `LoRa::receive()`
(`RF96W.cpp`).

## MQTT

```json
{"packet":"...", "rssi":-42.50, "snr":9.75}
```
publicado em `MQTT_TOPIC_TELEMETRY` (`"telemetry"` por padrão) a
cada pacote LoRa recebido -- desde que MQTT esteja habilitado (aba
MQTT do dashboard, ou já assim de fábrica).

`include/NetworkConfig.h` só vale pra **primeira vez que a placa
liga** (nunca teve nada salvo ainda) -- depois disso, WiFi/MQTT
vivem na flash (`Preferences`, namespace `"net"`) e são alterados
pelo dashboard, não pelo código-fonte.

## Adicionar uma quarta placa no futuro

1. Criar `include/PinsNovaPlaca.h`.
2. Um novo `#elif defined(PCB_NOVA_PLACA)` em `include/Pins.h`.
3. Um novo `[env:pcb-nova-placa]` em `platformio.ini`.
4. Se ela tiver display: envolver o uso em `#ifdef PCB_NOVA_PLACA`
   também (ou criar sua própria constante, se o display for
   diferente do SSD1306 da LilyGo).
