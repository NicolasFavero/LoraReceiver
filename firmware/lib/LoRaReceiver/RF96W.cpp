#include "RF96W.h"

void LoRa::setFlag() {packetReceived = true;}

LoRa::LoRa(
    uint8_t cs,
    uint8_t dio0,
    uint8_t rst,

    float frequency,
    float bandwidth,
    uint8_t spreadingFactor,
    uint8_t codingRate,
    uint8_t syncWord,
    int8_t power,
    uint16_t preambleLength,
    bool crc
)
:
module(
    cs,
    dio0,
    rst,
    RADIOLIB_NC
),
radio(&module),

frequency(frequency),
bandwidth(bandwidth),
spreadingFactor(spreadingFactor),
codingRate(codingRate),
syncWord(syncWord),
power(power),
preambleLength(preambleLength),
crc(crc)
{
    packetBuffer[0] = '\0';
}

bool LoRa::begin() {

    int state = radio.begin(
        frequency,
        bandwidth,
        spreadingFactor,
        codingRate,
        syncWord,
        power,
        preambleLength
    );

    if (state != RADIOLIB_ERR_NONE) {
        return false;
    }

    radio.setCRC(crc);

    radio.setDio0Action(setFlag, RISING);

    state = radio.startReceive();

    return state == RADIOLIB_ERR_NONE;
}

bool LoRa::reconfigure(
    float newFrequency,
    float newBandwidth,
    uint8_t newSpreadingFactor,
    uint8_t newCodingRate,
    uint8_t newSyncWord,
    int8_t newPower,
    bool newCrc
) {
    radio.standby();

    bool ok = true;

    ok &= (radio.setFrequency(newFrequency)        == RADIOLIB_ERR_NONE);
    ok &= (radio.setBandwidth(newBandwidth)         == RADIOLIB_ERR_NONE);
    ok &= (radio.setSpreadingFactor(newSpreadingFactor) == RADIOLIB_ERR_NONE);
    ok &= (radio.setCodingRate(newCodingRate)       == RADIOLIB_ERR_NONE);
    ok &= (radio.setSyncWord(newSyncWord)           == RADIOLIB_ERR_NONE);
    ok &= (radio.setOutputPower(newPower)           == RADIOLIB_ERR_NONE);
    ok &= (radio.setCRC(newCrc)                     == RADIOLIB_ERR_NONE);

    if (ok) {
        frequency        = newFrequency;
        bandwidth         = newBandwidth;
        spreadingFactor   = newSpreadingFactor;
        codingRate        = newCodingRate;
        syncWord          = newSyncWord;
        power             = newPower;
        crc               = newCrc;
    }

    // Sempre volta a escutar, mesmo se algum parametro falhou --
    // melhor continuar recebendo com a config antiga/parcial do que
    // deixar o receptor mudo.
    radio.startReceive();

    return ok;
}

bool LoRa::send(const char* msg) {

    if (msg == nullptr) {
        return false;
    }

    radio.standby();

    int state = radio.transmit(msg);

    radio.startReceive();

    return state == RADIOLIB_ERR_NONE;
}
bool LoRa::available() {return packetReceived;}
bool LoRa::receive() {

    if (!packetReceived) {
        return false;
    }

    packetReceived = false;

    String received;

    int state = radio.readData(received);

    if (state != RADIOLIB_ERR_NONE) {

        radio.startReceive();
        return false;
    }

    // Sanitiza o pacote: alguns transmissores (ex.: modulos Dorji)
    // prefixam a mensagem com um byte 0x00 -- ignora ate achar o
    // primeiro byte "de verdade" -- e só aceita ASCII imprimivel
    // (32-126) daí em diante, descartando lixo/binario no meio.
    size_t out = 0;
    bool started = false;

    for (size_t i = 0; i < received.length() && out < sizeof(packetBuffer) - 1; i++) {
        uint8_t b = (uint8_t)received[i];

        if (!started) {
            if (b == 0x00) continue;
            started = true;
        }

        if (b >= 32 && b <= 126) {
            packetBuffer[out++] = (char)b;
        }
    }

    packetBuffer[out] = '\0';

    rssi = radio.getRSSI();
    snr  = radio.getSNR();

    radio.startReceive();

    return true;
}
const char* LoRa::getPacket() const {return packetBuffer;}
float LoRa::getRSSI() const {return rssi;}
float LoRa::getSNR() const {return snr;}
void LoRa::print(){
    Serial.print("PACOTE: ");
    Serial.println(getPacket());

    Serial.print("RSSI: ");
    Serial.println(getRSSI());

    Serial.print("SNR: ");
    Serial.println(getSNR());
}
