#pragma once

#include <Arduino.h>
#include "RF96W.h"
#include "LoraDefaults.h"

// Config de radio em tempo de execucao: valida, aplica no radio e
// persiste na flash (Preferences/NVS, namespace "lora"). Reaproveitada
// tanto pelo endpoint HTTP POST /setLora (WebDashboard) quanto pelo
// callback MQTT do topico de config (main.cpp) -- os dois so chamam
// applyJson() com o corpo recebido, sem duplicar validacao/persistencia.
class LoraRuntimeConfig
{
public:

    // Carrega a ultima config salva (ou os defaults de
    // Pins.h::LoraConfig, se nunca foi salva nada) e aplica no radio.
    void begin(LoRa& radio);

    // JSON esperado: {"freq":915000000,"sf":10,"bd":125000,
    // "sync":18,"cr":5,"power":20,"crc":1}
    //
    // freq/bd em Hz (inteiros) -- mesma convencao do .ino original
    // (LilyGo) -- ainda que o resto do sistema (RadioLib) trabalhe
    // em MHz/kHz internamente; a conversao acontece aqui.
    //
    // Retorna false (e nao aplica nem salva nada) se o JSON for
    // invalido ou algum valor estiver fora da faixa aceita.
    bool applyJson(const String& json, LoRa& radio);

    // Config atual, como JSON -- usado pela pagina web pra pre-
    // preencher os campos com os valores em uso.
    String toJson() const;

private:

    long     freqHz  = (long)(LoraConfig::FREQUENCY * 1.0e6f);
    int      sf       = LoraConfig::SPREADING_FACTOR;
    long     bdHz     = (long)(LoraConfig::BANDWIDTH * 1.0e3f);
    int      sync      = LoraConfig::SYNC_WORD;
    int      cr        = LoraConfig::CODING_RATE;
    int      power     = LoraConfig::POWER;
    bool     crc       = LoraConfig::CRC_ENABLED;

    static bool validate(
        long freqHz, int sf, long bdHz,
        int sync, int cr, int power
    );

    void load();
    void save() const;

    // Converte pra unidades RadioLib (MHz, kHz) e chama
    // LoRa::reconfigure().
    bool applyToRadio(LoRa& radio) const;
};
