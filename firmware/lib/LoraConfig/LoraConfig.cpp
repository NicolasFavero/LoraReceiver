#include "LoraConfig.h"
#include <Arduino_JSON.h>
#include <Preferences.h>

void LoraRuntimeConfig::begin(LoRa& radio)
{
    load();
    applyToRadio(radio);
}

bool LoraRuntimeConfig::validate(
    long freqHz, int sf, long bdHz,
    int sync, int cr, int power
) {
    if (freqHz < 137000000L || freqHz > 1020000000L) return false;
    if (sf < 6 || sf > 12) return false;
    if (
        bdHz != 7800   && bdHz != 10400  && bdHz != 15600 &&
        bdHz != 20800  && bdHz != 31250  && bdHz != 41700 &&
        bdHz != 62500  && bdHz != 125000 &&
        bdHz != 250000 && bdHz != 500000
    ) return false;
    if (cr < 5 || cr > 8) return false;
    if (sync < 0 || sync > 255) return false;
    if (power < -4 || power > 20) return false;

    return true;
}

bool LoraRuntimeConfig::applyToRadio(LoRa& radio) const
{
    return radio.reconfigure(
        freqHz / 1.0e6f,   // Hz -> MHz
        bdHz   / 1.0e3f,   // Hz -> kHz
        (uint8_t)sf,
        (uint8_t)cr,
        (uint8_t)sync,
        (int8_t)power,
        crc
    );
}

bool LoraRuntimeConfig::applyJson(const String& json, LoRa& radio)
{
    JSONVar doc = JSON.parse(json);

    if (JSON.typeof(doc) == "undefined") {
        return false;
    }

    long newFreq  = (long)(double)doc["freq"];
    int  newSf    = (int)(double)doc["sf"];
    long newBd    = (long)(double)doc["bd"];
    int  newSync  = (int)(double)doc["sync"];
    int  newCr    = (int)(double)doc["cr"];
    int  newPower = (int)(double)doc["power"];
    bool newCrc   = (bool)doc["crc"];

    if (!validate(newFreq, newSf, newBd, newSync, newCr, newPower)) {
        return false;
    }

    freqHz = newFreq;
    sf      = newSf;
    bdHz    = newBd;
    sync    = newSync;
    cr      = newCr;
    power   = newPower;
    crc     = newCrc;

    bool ok = applyToRadio(radio);

    // Salva mesmo se o radio recusou parcialmente algum campo --
    // reflete a intencao do usuario; ele pode conferir e corrigir
    // via toJson()/pagina web.
    save();

    return ok;
}

String LoraRuntimeConfig::toJson() const
{
    char buf[160];

    snprintf(
        buf, sizeof(buf),
        "{\"freq\":%ld,\"sf\":%d,\"bd\":%ld,\"sync\":%d,\"cr\":%d,\"power\":%d,\"crc\":%d}",
        freqHz, sf, bdHz, sync, cr, power, crc ? 1 : 0
    );

    return String(buf);
}

void LoraRuntimeConfig::save() const
{
    Preferences prefs;

    prefs.begin("lora", false);
    prefs.putLong("freq", freqHz);
    prefs.putInt("sf", sf);
    prefs.putLong("bd", bdHz);
    prefs.putInt("sync", sync);
    prefs.putInt("cr", cr);
    prefs.putInt("power", power);
    prefs.putBool("crc", crc);
    prefs.end();
}

void LoraRuntimeConfig::load()
{
    Preferences prefs;

    // Se o namespace "lora" nunca foi salvo (1o boot), begin() em
    // modo somente-leitura falha -- ler dele nesse caso retorna
    // lixo, nao o default passado como 2o argumento. Os campos ja
    // comecam com os defaults de Pins.h::LoraConfig (member
    // initializers em LoraConfig.h), entao so sobrescreve se
    // begin() realmente abriu.
    if (prefs.begin("lora", true))
    {
        freqHz = prefs.getLong("freq", freqHz);
        sf      = prefs.getInt("sf", sf);
        bdHz    = prefs.getLong("bd", bdHz);
        sync    = prefs.getInt("sync", sync);
        cr      = prefs.getInt("cr", cr);
        power   = prefs.getInt("power", power);
        crc     = prefs.getBool("crc", crc);
        prefs.end();
    }

    if (!validate(freqHz, sf, bdHz, sync, cr, power)) {
        Serial.println("[LoraConfig] Config salva invalida -- usando defaults.");
        freqHz = (long)(LoraConfig::FREQUENCY * 1.0e6f);
        sf      = LoraConfig::SPREADING_FACTOR;
        bdHz    = (long)(LoraConfig::BANDWIDTH * 1.0e3f);
        sync    = LoraConfig::SYNC_WORD;
        cr      = LoraConfig::CODING_RATE;
        power   = LoraConfig::POWER;
        crc     = LoraConfig::CRC_ENABLED;
    }
}
