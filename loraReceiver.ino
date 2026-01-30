#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <SPI.h>
#include <LoRa.h>
#include "esp_task_wdt.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Arduino_JSON.h>
#include <PubSubClient.h>
#include <Preferences.h>
#define MQTT_TOPIC "LiliGO/912.300"
#define MQTT_SERVER "192.168.0.45"
#define MQTT_PORT 1883
#define MQTT_USER "user1"
#define MQTT_PASS "user1"
#define LORA_SCK  5      //LilyGo T3 pins
#define LORA_MISO 19
#define LORA_MOSI 27
#define LORA_SS   18
#define LORA_RST  14
#define LORA_DIO0 26
#define SDA 21
#define SCL 22
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
const char* topicMqtt = "tfx/setLora900";
const char* SSID = "TFX.ECO.BR";
const char* PASS = "senhadificil1234";

WiFiClient espClient;
PubSubClient mqtt(espClient);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
AsyncWebServer server(80);
Preferences prefs;
const char html[] PROGMEM = 
    R"rawliteral(
  <!DOCTYPE html>
  <html lang="en">
  <head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <meta name="author" content="Nicolas Favero">
  <meta name="description" content="Manage LilyGO (868-915 MHz)">
  <title>LilyGO Server</title>

  <style>
  /* ===== RESET ===== */
  * {
      margin: 0;
      padding: 0;
      box-sizing: border-box;
  }

  /* ===== BODY ===== */
  body {
      font-family: Arial, Helvetica, sans-serif;
      background: #f5f5f5;
      color: #222;
  }

  /* ===== HEADER ===== */
  header {
      background: #2c3e50;
      color: #fff;
      padding: 20px;
      text-align: center;
  }

  header h1 {
      margin-bottom: 6px;
  }

  header h5 {
      margin-top: 8px;
      font-weight: normal;
      opacity: 0.8;
  }

  /* ===== MAIN ===== */
  main {
      max-width: 600px;
      margin: 20px auto;
      padding: 10px;
  }

  /* ===== SECTIONS ===== */
  section {
      background: #fff;
      padding: 16px;
      margin-bottom: 12px;
      border-radius: 6px;
  }

  section h3 {
      margin-bottom: 6px;
  }

  /* ===== INPUTS ===== */
  input,
  select {
      width: 100%;
      padding: 8px 10px;
      margin-top: 6px;
      border: 1px solid #ccc;
      border-radius: 4px;
      font-size: 1rem;
  }

  input::placeholder {
      color: #999;
      font-style: italic;
  }

  /* ===== BUTTON ===== */
  .actions {
      text-align: center;
      margin-top: 20px;
  }

  button {
      background: #2c3e50;
      color: white;
      border: none;
      padding: 12px 24px;
      border-radius: 6px;
      font-size: 1rem;
      cursor: pointer;
      transition: background 0.2s ease, transform 0.1s ease;
  }

  button:hover {
      background: #233441;
  }

  button:active {
      transform: scale(0.97);
  }

  button:disabled {
      background: #aaa;
      cursor: not-allowed;
  }

  /* ===== FOOTER ===== */
  footer {
      text-align: center;
      padding: 14px;
      font-size: 0.9rem;
      color: #666;
  }
  </style>
  </head>

  <body>

  <header>
      <h1>LilyGO Server</h1>
      <h2>Manage LilyGO (868–915 MHz)</h2>
      <h5>Only fill with numbers</h5>
  </header>

  <main>

      <section>
          <h3>Frequency</h3>
          <input type="number" id="freq" placeholder="912300000">
      </section>

      <section>
          <h3>Spreading Factor</h3>
          <input type="number" id="sf" placeholder="10">
      </section>

      <section>
          <h3>Bandwidth</h3>
          <input type="number" id="bd" placeholder="125000">
      </section>

      <section>
          <h3>Sync Word</h3>
          <input type="number" id="sync" placeholder="18(only decimal)">
      </section>

      <section>
          <h3>Coding Rate</h3>
          <input type="number" id="cr" placeholder="5">
      </section>

      <section>
          <h3>Tx Power</h3>
          <input type="number" id="power" placeholder="20">
      </section>

      <section>
          <h3>CRC</h3>
          <select id="crc">
              <option value="1">Enable CRC</option>
              <option value="0">Disable CRC</option>
          </select>
      </section>

      <div class="actions">
          <button id="saveButton">Save settings</button>
      </div>

  </main>

  <footer>
      <p></p>
  </footer>

  <script>
  document.getElementById("saveButton").addEventListener("click", () => loraSettings());

  class Settings{
      constructor(freq, sf, bd, sync, cr, power, crc){
          this.freq = freq;
          this.sf = sf;
          this.bd = bd;
          this.sync = sync;
          this.cr = cr;
          this.power = power;
          this.crc = crc;
      }
  }
  function loraSettings() {
      
      let freq =  parseInt(document.getElementById("freq").value, 10);
      let sf = parseInt(document.getElementById("sf").value, 10);
      let bd = parseInt(document.getElementById("bd").value, 10);
      let sync = parseInt(document.getElementById("sync").value, 10);
      let cr = parseInt(document.getElementById("cr").value, 10);
      let power = parseInt(document.getElementById("power").value, 10);
      let crc = parseInt(document.getElementById("crc").value, 10);
          
      let newSettings = new Settings(freq, sf, bd, sync, cr, power, crc)
      enviar(newSettings);
  }
  async function enviar(dados) {
      
      fetch("/setLora", {
          method: "POST",
          headers: { "Content-Type": "application/json" },
          body: JSON.stringify(dados)
      })
      .then(r=>{
          if(!r.ok){
              throw new Error()
          }
          return r.text();
      })
      .then(data =>{
          alert("Esp32: " + data);
      })
      .catch(erro =>{
          alert(erro);
      })
  }

  </script>

  </body>
  </html>
    )rawliteral";
  

class Lora {
    public: 

    int packetSize;
    int rssi;
    float snr;
    char msg[256];
 //-------------------------------------------------------------------------
    void begin(int sck,int miso,int mosi,int ss,int rst,int d0){
        SPI.begin(sck, miso, mosi, ss);
        LoRa.setPins(ss, rst, d0);

        if (!LoRa.begin(915000E3)) {
        Serial.println("Falha LoRa");
        while (1);
        }
        // MESMAS CONFIGS DO TX
        LoRa.setSpreadingFactor(10);  
        LoRa.setSignalBandwidth(125E3);  
        LoRa.setCodingRate4(5);     
        LoRa.setSyncWord(0x12);           // padrão público
        LoRa.disableCrc();
        LoRa.setTxPower(20);

        loadConfig();
     }
 //------------------------------------------------------------------------

    bool getJson(String json){
    
        JSONVar doc = JSON.parse(json);

        if (JSON.typeof(doc) == "undefined") {
            Serial.println("JSON inválido");
            return false;
        }
        // Acessa os valores
        long freq = (long)doc["freq"];
        int sf = (int)doc["sf"];
        long bd = (long)doc["bd"];
        int sync = (int)doc["sync"];
        int cr = (int)doc["cr"];
        int power = (int)doc["power"];
        bool crc = (bool)doc["crc"];
 
        Serial.print("Freq: ");
        Serial.print(freq);
        Serial.print(" SF: ");
        Serial.print(sf);
        Serial.print("Sync: ");
        Serial.print(sync);
        Serial.print(" Cr: ");
        Serial.print(cr);
        Serial.print("Crc: ");
        Serial.print(crc);
        Serial.print(" Power: ");
        Serial.print(power);
        Serial.print(" Bd: ");
        Serial.println(bd);

        if(validarValores(freq, sf, bd, sync, cr, power)){
            atualizarLoRa(freq, sf, bd, sync, cr, power, crc);
            //crc? Serial.println("true") : Serial.println("false");
            return true;
        }
        else {return false;}
     }

    void readMessage(){
  
        Serial.print("Recebido: ");

        Serial.print("MSG: ");
        Serial.print(msg);

        Serial.print(" | RSSI: ");
        Serial.print(rssi);
        Serial.print(" dBm");

        Serial.print(" | SNR: ");
        Serial.print(snr);
        Serial.println(" dB");
     }

    bool getMessage() {
        packetSize = LoRa.parsePacket();  
        if (packetSize) {
            int i = 0;
            bool started = false;
            while (LoRa.available() && i < packetSize && i < sizeof(msg) - 1) {
                byte b = LoRa.read();
                // ignora NULLs iniciais(pois o dorji manda)
                if (!started) {
                if (b == 0x00) continue;
                started = true;
                }
                // aceita só ASCII imprimível
                if (b >= 32 && b <= 126) {
                msg[i++] = (char)b;
                }
            }
            msg[i] = '\0';
            rssi = LoRa.packetRssi();  
            snr  = LoRa.packetSnr();
            readMessage();
            return true;
        }
        return false;
     }

  private:
    long freq = 915E6;
    int sf = 10;
    long bd = 125E3;
    int sync = 18;
    int cr = 5;
    int power = 20;
    bool crc = true;
    
    void atualizarLoRa(long freq_, int sf_, long bd_, int sync_, int cr_, int power_, bool crc_, bool save = true) {

        freq = freq_;
        sf = sf_;
        bd = bd_;
        sync = sync_;
        cr = cr_;
        power = power_;
        crc = crc_;

        LoRa.idle();  // pausa comunicação
        LoRa.setFrequency(freq);
        LoRa.setSpreadingFactor(sf);
        LoRa.setSignalBandwidth(bd);
        LoRa.setCodingRate4(cr);
        LoRa.setSyncWord(sync);
        LoRa.setTxPower(power);
        crc? LoRa.enableCrc() : LoRa.disableCrc();
        LoRa.receive(); // volta a escutar
        save? saveConfig() : (void)0;
     }
    bool validarValores(long freq, int sf, long bd, int sync, int cr, int power){

        if (freq < 840E6 || freq > 940E6) return false;
        if (sf < 6 || sf > 12) return false;
        if (
            bd != 7800 && bd != 10400 && bd != 15600 &&
            bd != 20800 && bd != 31250 && bd != 41700 &&
            bd != 62500 && bd != 125000 &&
            bd != 250000 && bd != 500000
        ) return false;
        if (cr < 4 || cr > 8) return false;
        if (sync < 0 || sync > 255) return false;
        if (power < -4 || power > 20) return false;

        return true;
     }
    void saveConfig() {

        prefs.begin("lora", false); // "lora" é o namespace, false = gravar
        prefs.putLong("freq", freq);
        prefs.putInt("sf", sf);
        prefs.putLong("bd", bd);
        prefs.putInt("sync", sync);
        prefs.putInt("cr", cr);
        prefs.putInt("power", power);
        prefs.putBool("crc", crc);
        prefs.end();
     }

    void loadConfig() {

        prefs.begin("lora", true); // true = leitura
        freq = prefs.getLong("freq", freq); 
        sf = prefs.getInt("sf", sf);
        bd = prefs.getLong("bd", bd);
        sync = prefs.getInt("sync", sync);
        cr = prefs.getInt("cr", cr);
        power = prefs.getInt("power", power);
        crc = prefs.getBool("crc", crc);
        prefs.end();

        if(validarValores(freq, sf, bd, sync, cr, power)){
            atualizarLoRa(freq, sf, bd, sync, cr, power, crc, false);
        } 
        else {
            Serial.println("ERRO-->LoRa::loadConfig");
        }
     }
 };
Lora myLoRa;
class Web{

    public:
    void connectMQTT() {
        mqtt.setServer(MQTT_SERVER, MQTT_PORT);
        mqtt.setCallback(Web::mqttCallback);
        while (!mqtt.connected()) {
            String id = "Esp" + WiFi.macAddress();
            id.replace(":", "");
            mqtt.connect(id.c_str(), MQTT_USER, MQTT_PASS);
            delay(1000);
        }
        mqtt.subscribe(topicMqtt);
     }

    void wifiBegin(){
        WiFi.begin(SSID, PASS);
        while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
        }
        Serial.println("\nConectado!");
        Serial.println(WiFi.localIP());
    
        server.on(
            "/",
            HTTP_GET,
            [](AsyncWebServerRequest *request) {
            request->send_P(200, "text/html", html);
            });
        /*server.on(
            "/updadteConfig",
            HTTP_GET,
            [](AsyncWebServerRequest *request) {
                request->send(200, "text/plain", "POST recebido");
            });   */
        server.on(
            "/setLora",
            HTTP_POST,
            [](AsyncWebServerRequest *request) {},
            NULL,
            [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
                String json;
                for (size_t i = 0; i < len; i++) {
                    json += (char)data[i];
                }

                if(myLoRa.getJson(json)) {
                    request->send(200, "text/plain", "Configurações atualizadas");
                    Serial.println(json);
                }
                else{
                    request->send(200, "text/plain", "Valores Inválidos(reenvie corretamente)");
                }
            });
        server.begin();
     }

    private:
    static void mqttCallback(char* topic, byte* payload, unsigned int length){
        if (length > 200) return;
        char data[length+1];
        memcpy(data, payload, length);
        data[length]  = '\0';
        myLoRa.getJson(String(data));

        if (strcmp(topic, topicMqtt) == 0) {
        //obs ja deixei pois caso tenha mais topicos tem q usar assim msm, por enquanto só to inscrito em um
        }
     }
 };
class Display{
    public:

    void begin(){

        Wire.begin(SDA, SCL);  // SDA, SCL 
            if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
                Serial.println("OLED nao encontrado");
                while (true);
            }

        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);

        display.setCursor(0, 0);
        display.println("LilyGO OK");
        display.println(WiFi.localIP());
        delay(1000);
        display.display();
     }
    void showMessage(int rssi, float snr, char* msg){
        display.setTextSize(1);
        display.clearDisplay();

        display.setCursor(0, 48);
        display.print("RSSI: ");
        display.print(rssi);
        display.println(" dBm");

        display.print("SNR: ");
        display.print(snr);
        display.println(" dB");

        display.setCursor(0, 0);

        //display.print("msg: -->");
        display.print(msg);
        display.display();
     }
    void noMessage(){
        display.clearDisplay();
        display.setTextSize(2);
        display.setTextColor(SSD1306_WHITE);

        String texto = "NoMessages in the last 10s";

        int16_t x1, y1;
        uint16_t w, h;

        display.getTextBounds(texto, 0, 0, &x1, &y1, &w, &h);

        int x = (128 - w) / 2;
        int y = (64 - h) / 2;

        display.setCursor(x, y);
        display.println(texto);

        display.display();
        // volta para o padrão
        display.setTextSize(1);
     }
 };

Web web;
Display myDisplay;
void setup() {

 //-------------------------------WatchDog------------------------------------------
  esp_task_wdt_init(15, true);   // 5s(watchdog)
  esp_task_wdt_add(NULL);
 //-----------------------------Default-------------------------------------------- 
  Serial.begin(115200);
 //--------------------------------Lora-----------------------------------------
  myLoRa.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_SS, LORA_RST, LORA_DIO0); //[...]pega a última configuração do lora salva na flash
 //-------------------------------Wifi--Mqtt-------------------------------------
  web.wifiBegin();
  web.connectMQTT();
 //-------------------------------Display------------------------------------------
  myDisplay.begin();
  delay(1000);


 }
void loop() {
    esp_task_wdt_reset();
    Serial.println("here");
    if (!mqtt.connected()) {
    web.connectMQTT();
    }
    mqtt.loop();
    
    static unsigned long oldMessage;

    if(myLoRa.getMessage()){
        myDisplay.showMessage(myLoRa.rssi, myLoRa.snr, myLoRa.msg);
        mqtt.publish(MQTT_TOPIC, myLoRa.msg, true);
        oldMessage = millis();
    }
    if(millis() - oldMessage > 1E4){
        myDisplay.noMessage();
    }
    delay(1);
}