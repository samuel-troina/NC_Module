#include <WebGUI.h>      //https://github.com/samuel-troina/NC_GUI
#include <MYFS.h>
#include <NC_module.h>   //https://github.com/samuel-troina/NC_Module
#include <SoftwareSerial.h>
#include <CayenneLPP.h>  //https://github.com/samuel-troina/CayenneLPP
#include <TinyGPSPlus.h> //https://github.com/mikalhart/TinyGPSPlus

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

// Define os pinos RX and TX do módulo GPS
const int PIN_RX_GPS = 12;
const int PIN_TX_GPS = 13;

SoftwareSerial uart_mod_lora(PIN_RX_LORA, PIN_TX_LORA);
SoftwareSerial uart_mod_gps(PIN_RX_GPS, PIN_TX_GPS);

NC_module nc_module = NC_module();

WebGUI webgui;
String modo_operacao;

/* CallBack executado quando ocorre uma alteração nas configurações pela homepage */
void onConfigChanged(const String& evType, ConfigManager* config) { 
  if (evType == "load"){
    String content = MYFS::readFile("/config.json");
    config->loadConfigFromJson(content);    
  }else if(evType == "change"){
    saveConfiguration();
  }

  /* Efetua a parametrização do módulo */
  modo_operacao = config->getValue("mode");

  if (modo_operacao == "lorawan"){
    nc_module.sendCmd("AT+MODE=LORAWAN");
  }else if (modo_operacao == "lorawan_mesh"){
    nc_module.sendCmd("AT+MODE=LORAWAN_MESH"); 
  }

  if (modo_operacao == "lorawan" || modo_operacao == "lorawan_mesh"){
    String txPower = config->getValue("TXPOWER");
    String subBand = config->getValue("SUBBAND");
    String ch = config->getValue("CH");
    String dr = config->getValue("DR");
    String drx2 = config->getValue("DRX2");
    String fqrx2 = config->getValue("FQRX2");
    String dlrx2 = config->getValue("DLRX2");
    nc_module.configureLoRaWAN(txPower, subBand, ch, dr, drx2, fqrx2, dlrx2);
    txPower = ""; subBand = ""; ch = ""; dr = ""; drx2 = ""; fqrx2 = ""; dlrx2 = "";

    String devEUI = config->getValue("DEVEUI");
    String appEUI = config->getValue("APPEUI");
    String appKey = config->getValue("APPKEY");
    nc_module.configureOTAA(devEUI, appEUI, appKey);
    devEUI = ""; appEUI = ""; appKey = "";

    String devAddr = config->getValue("DEVADDR");
    String appSKey = config->getValue("APPSKEY");
    String nwkSKey = config->getValue("NWKSKEY");
    String fCntUp = config->getValue("FCNTUP");
    String fCntDown = config->getValue("FCNTDOWN");
    nc_module.setSession(devAddr, appSKey, nwkSKey, fCntUp, fCntDown);
    devAddr = ""; appSKey = ""; nwkSKey = ""; fCntUp = ""; fCntDown = "";
  }

  if (modo_operacao == "lorawan_mesh"){
    String id = config->getValue("ID");
    String retries = config->getValue("RETRIES");
    String timeout = config->getValue("TIMEOUT");
    String sw = config->getValue("SW");
    String prea = config->getValue("PREA");
    String cr = config->getValue("CR");
    nc_module.configureMESH(id, retries, timeout, sw, prea, cr);
    id = ""; retries = ""; timeout = ""; sw = ""; prea = ""; cr = "";
  }  
}

void setup() {
  Serial.begin(115200);
  while (!Serial);
  
  uart_mod_lora.begin(9600);
  uart_mod_gps.begin(9600);

  nc_module.init(uart_mod_lora);

  Serial.print("Comunicação com o módulo LoRa ");
  while (nc_module.moduleStatus() == false){    
    delay(1000);
  }
  Serial.println("OK");

  // Inicializa o sistemade arquivos
  while(MYFS::begin() == false){
      delay(1000);
  }

  //Inicialização do webserver e websocket para definição das configurações do device  
  webgui.setup(onConfigChanged, true);

  //Inicialização do pino do LED
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);
  
  //Inicialização do botão seletor do modo de envio
  pinMode(MODE_PIN, INPUT_PULLUP);

  //Inicialização do pino de envio manual
  pinMode(BNT_SEND_PIN, INPUT_PULLUP);  
}

bool sessionStarted(){
  static uint32_t joined = false;
  if (joined == false && nc_module.joined() == false){              
    //nc_module.join();
    return false;
  }
  
  joined = true;

  return true;  
}

bool saveSession(){  
  String DEVADDR;
  String APPSKEY;
  String NWKSKEY;
  String FCNTUP;
  String FCNTDOWN;
  nc_module.getSession(DEVADDR, APPSKEY, NWKSKEY, FCNTUP, FCNTDOWN);
  
  ConfigManager* config = webgui.getConfigManager();

  config->setValue("DEVADDR", DEVADDR);
  config->setValue("APPSKEY", APPSKEY);
  config->setValue("NWKSKEY", NWKSKEY);
  config->setValue("FCNTUP", FCNTUP);
  config->setValue("FCNTDOWN", FCNTDOWN);

  return saveConfiguration();
}

bool saveConfiguration(){
  ConfigManager* config = webgui.getConfigManager();

  uart_mod_lora.end();
  uart_mod_gps.end();

  noInterrupts();

  String output;
  config->getConfigJson(output);
  MYFS::writeFile("/config.json", output);
  
  interrupts();  // Habilita interrupções novamente

  uart_mod_lora.begin(9600);
  uart_mod_gps.begin(9600);

  return true;
}

void loop(){  
  webgui.loop();
  
  if (modo_operacao == "lorawan" || modo_operacao == "lorawan_mesh"){
    if (sessionStarted()){
      loopAPP();

      String event = nc_module.readSerial();
      if (event.length() > 0){
        if (event.startsWith("EVT:")) {
          String type_event = event.substring(4);
          if (type_event == "TX_OK" || type_event == "RX_OK" || type_event == "JOINED"){
            saveSession();
          }
        }else{
          Serial.print("Leitura da serial: ");
          Serial.println(event);
        }
      }
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

  TinyGPSPlus gps = TinyGPSPlus();

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
    CayenneLPP lpp = CayenneLPP(11);

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