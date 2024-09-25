#include <WebGUI.h>
#include <NC_module.h>
#include <SoftwareSerial.h>
#include <CayenneLPP.h>
#include <TinyGPSPlus.h>

// Define o pino do LED
const int LED_PIN = 16;

// Define o pino do modo de envio das MSG
const int MODE_PIN = 14;
const int SEND_INTERVAL = 5; //Segundos

// Define o pino do botão de envio manual das MSG
const int BNT_SEND_PIN = 2;

// Define os pinos RX and TX do módulo LoRA
const int PIN_RX_LORA = 4;
const int PIN_TX_LORA = 5;

// Define os pinos RX and TX do módulo LoRA
const int PIN_RX_GPS = 12;
const int PIN_TX_GPS = 13;

SoftwareSerial uart_mod_lora(PIN_RX_LORA, PIN_TX_LORA);
SoftwareSerial uart_mod_gps(PIN_RX_GPS, PIN_TX_GPS);

NC_module nc_module = NC_module();
TinyGPSPlus gps = TinyGPSPlus();
CayenneLPP lpp = CayenneLPP(11);

WebGUI webgui;
String modo_operacao;

void onConfigChanged(ConfigManager* config) {
  modo_operacao = config->getMode();
  if (modo_operacao == "lorawan"){
    nc_module.sendCmd("AT+MODE=LORAWAN");
  }else if (modo_operacao == "lorawan_mesh"){
    nc_module.sendCmd("AT+MODE=LORAWAN_MESH"); 
  }

  if (modo_operacao == "lorawan" || modo_operacao == "lorawan_mesh"){
    nc_module.configureLoRaWAN(config->getSubBand(), config->getChannel(), config->getDataRate(), config->getDataRateDownlink(), String(config->getFrequencyDownlink()), "10");
    nc_module.configureOTAA(config->getDevEUI(), config->getAppEUI(), config->getAppKey());
  }

  if (modo_operacao == "lorawan_mesh"){
    nc_module.configureMESH(String(config->getID()), String(config->getRetries()), String(config->getTimeout()), config->getSyncWord(), "8", "5");
  }
}

void setup() {
  Serial.begin(115200);
  while (!Serial);
  
  uart_mod_lora.begin(9600);
  uart_mod_gps.begin(9600);

  nc_module.init(uart_mod_lora);

  while (nc_module.moduleStatus() == false){
    Serial.println("Não foi possível realizar a comunicação com o módulo.");
    delay(1000);
  }

  //Inicialização do webserver e websocket para definição das configurações do device
  webgui.setConfigManagerChangeCallback(onConfigChanged);
  webgui.setup();

  //Inicializa o access point
  webgui.toggleWiFi(true);

  //Inicialização do pino do LED
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);
  
  //Inicialização do pino do LED
  pinMode(MODE_PIN, INPUT_PULLUP);

  //Inicialização do pino de LED
  pinMode(BNT_SEND_PIN, INPUT_PULLUP);  
}

void loop(){
  webgui.loop();

  if (modo_operacao == "lorawan" || modo_operacao == "lorawan_mesh"){
    if (nc_module.joined() == false){
      //nc_module.join();
    }else{
      loopAPP();
    }
  }  
}

void blinkLED(int interval, int times){
  do {
    digitalWrite(LED_PIN, HIGH);    
    delay(interval);
    digitalWrite(LED_PIN, LOW);
    times--;
    if (times > 0)
      delay(interval);
  } while (times > 0);
}

bool debounce(int pin) {
  unsigned long debounceDelay = 50;
  static int estadoBotao = HIGH;   
  static int ultimoEstadoBotao = HIGH;
  static unsigned long ultimaLeitura = 0;

  int leituraAtual = digitalRead(pin);  
  if (leituraAtual != ultimoEstadoBotao)
    ultimaLeitura = millis();
  
  if ((millis() - ultimaLeitura) > debounceDelay) {    
    if (leituraAtual != estadoBotao) {
      estadoBotao = leituraAtual;
      if (estadoBotao == LOW) {
        ultimoEstadoBotao = leituraAtual;
        return true;
      }
    }
  }

  ultimoEstadoBotao = leituraAtual;
  return false; 
}

bool readGPS(double& lat, double& lng, double &alt){
  if (!uart_mod_gps.available())
    return false;

  uint8_t byte;
  uint32_t start = millis();
  uint32_t wait = 1000;  
  do {
      while (uart_mod_gps.available()) {                        
        byte = uart_mod_gps.read();
        gps.encode(byte);
      }
  } while (millis() - start < wait);

  if (!gps.hdop.isValid())      
    return false;
  
  double hdop = gps.hdop.hdop();        
  if (hdop > 0 && hdop < 5 && gps.location.isValid() && gps.altitude.isValid()){
    lat = gps.location.lat();
    lng = gps.location.lng();
    alt = gps.altitude.meters();

    return true;
  }
  
  return false;
}

void loopAPP(){
  /* Liga o LED */
  static bool frist_time = false;  
  if(frist_time == false){
    digitalWrite(LED_PIN, LOW);
    frist_time = true;
  }

  /* Procedimento para que verifica se deve ser enviada uma MSG */
  static unsigned long last_send = 0;    
  bool sendMSG = debounce(BNT_SEND_PIN);
  if(sendMSG == false){    
    bool auto_send = digitalRead(MODE_PIN) == LOW;    
    if (auto_send == true && ((millis() - last_send) > SEND_INTERVAL * 1000)){
      last_send = millis();
      sendMSG = true;
    }
  }

  if (sendMSG){
    /* Coleta as coordenadas do GPS */
    double lat = 0;
    double lng = 0;
    double alt = 0;
    if (readGPS(lat, lng, alt)){    
      lpp.addGPS(1, lat, lng, alt);

      nc_module.sendMSG(false, 1, lpp.getBuffer(), lpp.getSize());

      blinkLED(500, 1);
    } else {
      blinkLED(80, 5);  
    }
  }
}