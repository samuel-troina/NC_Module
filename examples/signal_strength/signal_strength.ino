#include <WebGUI.h>         //https://github.com/samuel-troina/NC_GUI
#include <MYFS.h>
#include <TimeLib.h>        // Time por Michael Margolis https://github.com/PaulStoffregen/Time/
#include <SoftwareSerial.h> //https://github.com/plerup/espsoftwareserial/
#include <NC_module.h>      //https://github.com/samuel-troina/NC_Module
#include <CayenneLPP.h>     //https://github.com/samuel-troina/CayenneLPP
#include <TinyGPSPlus.h>    //https://github.com/mikalhart/TinyGPSPlus
#include <SPI.h>
#include <SD.h>             //https://github.com/espressif/arduino-esp32/tree/master/libraries/SD

#define SDCard_SCK   18
#define SDCard_MISO  19
#define SDCard_MOSI  23
#define SDCard_CS    5

// Define o pino do LED
const int LED_PIN = 2;

// Define o pino do modo de envio das MSG
const int MODE_PIN = 21;

// Define o pino do botão de envio manual das MSG
const int BNT_SEND_PIN = 22;

// Define os pinos RX and TX do módulo LoRA
const int PIN_RX_LORA = 16;
const int PIN_TX_LORA = 17;

// Define os pinos RX and TX do módulo GPS
const int PIN_RX_GPS = 14;
const int PIN_TX_GPS = 13;

// Intervalo de envio das mensagens no modo automático
const int SEND_INTERVAL = 10;

const int timeZoneOffset = -3 * 3600; // GMT-3 em segundos

EspSoftwareSerial::UART uart_mod_lora;
EspSoftwareSerial::UART uart_mod_gps;

SPIClass spi_sd_card = SPIClass(VSPI);

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

  Serial.println("Módulo LoRaWAN parametrizado");
}

bool initSDCard(){
  spi_sd_card.begin(SDCard_SCK, SDCard_MISO, SDCard_MOSI, SDCard_CS);

  if (!SD.begin(SDCard_CS, spi_sd_card, 1000000)) {
    Serial.println("Falha ao montar o cartão SD");
    return false;
  }
  uint8_t cardType = SD.cardType();

  if(cardType == CARD_NONE){
    Serial.println("No SD card attached");
    return false;
  }
  Serial.print("SD Card Type: ");
  if(cardType == CARD_MMC){
    Serial.println("MMC");
  } else if(cardType == CARD_SD){
    Serial.println("SDSC");
  } else if(cardType == CARD_SDHC){
    Serial.println("SDHC");
  } else {
    Serial.println("UNKNOWN");
  }
  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("SD Card Size: %lluMB\n", cardSize);

  return true;
}

bool appendFile(const char * path_file, const char * txt){
    const int maxRetries = 5;
    int attempt = 0;
    File file;

    while (attempt < maxRetries) {
        file = SD.open(path_file, FILE_APPEND);
        if (file) {
            break;
        }
        Serial.println("Falha ao abrir o arquivo para escrita, tentando novamente...");
        delay(100);
        attempt++;
    }

    if (!file) {
        Serial.println("Erro: Não foi possível abrir o arquivo após múltiplas tentativas.");
        return false;
    }

    bool ret = file.print(txt);
    file.flush(); // Garante que os dados sejam escritos no cartão antes de fechar
    file.close();

    if (!ret) {
        Serial.println("Erro ao escrever no arquivo.");
    }

    return ret;
}

float truncate(float value, int decimalPlaces) {
    float factor = pow(10, decimalPlaces);
    return (int)(value * factor) / factor;
}

void setup() {
  Serial.begin(115200);
  while (!Serial);

  uart_mod_lora.begin(9600, EspSoftwareSerial::SWSERIAL_8N1, PIN_TX_LORA, PIN_RX_LORA);
  uart_mod_gps.begin(9600, EspSoftwareSerial::SWSERIAL_8N1, PIN_TX_GPS, PIN_RX_GPS);

  nc_module.init(uart_mod_lora);

  Serial.print("Comunicação com o módulo LoRa ");
  while (nc_module.moduleStatus() == false){
    delay(1000);
  }
  Serial.println("OK");

  // Inicializa o sistema de arquivos
  while(MYFS::begin() == false){
    delay(1000);
  }

  //Inicialização do módulo SD
  while(initSDCard() == false){
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
  static bool joined = false;
  if (joined == false && nc_module.joined() == false){
    nc_module.join();
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
  if (nc_module.getSession(DEVADDR, APPSKEY, NWKSKEY, FCNTUP, FCNTDOWN) == false)
    return false;

  Serial.print("DEVADDR: ");
  Serial.println(DEVADDR);
  Serial.print("APPSKEY: ");
  Serial.println(APPSKEY);
  Serial.print("NWKSKEY: ");
  Serial.println(NWKSKEY);
  Serial.print("FCNTUP: ");
  Serial.println(FCNTUP);
  Serial.print("FCNTDOWN: ");
  Serial.println(FCNTDOWN);

  /*
  ConfigManager* config = webgui.getConfigManager();

  config->setValue("DEVADDR", DEVADDR.c_str());
  config->setValue("APPSKEY", APPSKEY.c_str());
  config->setValue("NWKSKEY", NWKSKEY.c_str());
  config->setValue("FCNTUP", FCNTUP.c_str());
  config->setValue("FCNTDOWN", FCNTDOWN.c_str());

  return saveConfiguration();
  */

  return true;
}

bool saveConfiguration(){
  ConfigManager* config = webgui.getConfigManager();

  String output;
  if (config->getConfigJson(output))
    MYFS::writeFile("/config.json", output);

  Serial.println("saveConfiguration");
  Serial.println(output);

  Serial.println("Configurações salvas");

  return true;
}

void loop(){
  webgui.loop();

  if (modo_operacao == "lorawan" || modo_operacao == "lorawan_mesh"){

    String event = nc_module.readSerial();
    if (event.length() > 0){
      Serial.print("Leitura da serial: ");
      Serial.println(event);

      if (event.startsWith("EVT:")) {
        String type_event = event.substring(4);
        if (type_event == "TX_OK" || type_event == "RX_OK" || type_event == "JOINED"){
          saveSession();
        }
      }
    }

    if (sessionStarted()){      
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

/*
  Debounce

  Each time the input pin goes from LOW to HIGH (e.g. because of a push-button
  press), the output pin is toggled from LOW to HIGH or HIGH to LOW. There's a
  minimum delay between toggles to debounce the circuit (i.e. to ignore noise).
*/
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

bool readGPS(char* sz, double& lat, double& lng, double &alt){
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

  if (hdop > 0 && hdop < 5 && gps.location.isValid() && gps.altitude.isValid() && gps.date.isValid() && gps.time.isValid()){
    lat = gps.location.lat();
    lng = gps.location.lng();
    alt = gps.altitude.meters();

    setTime(gps.time.hour(), gps.time.minute(), gps.time.second(), gps.date.day(), gps.date.month(), gps.date.year());

    time_t localTime = now() + timeZoneOffset;

    sprintf(sz, "%04d-%02d-%02d %02d:%02d:%02d",
        year(localTime),
        month(localTime),
        day(localTime),
        hour(localTime),
        minute(localTime),
        second(localTime));

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
    char sz[20];

    if (readGPS(sz, lat, lng, alt)){

      lpp.addGPS(1, lat, lng, alt);

      nc_module.sendMSG(false, 1, lpp.getBuffer(), lpp.getSize());

      char date[11];
      strncpy(date, sz, 10);
      date[10] = '\0';

      char filename[21];
      sprintf(filename, "/log_%s.txt", date);

      char logMessage[60];
      sprintf(logMessage, "%s,,,,,,,,,,,,%.4f,%.4f,%.2f\r\n", sz, truncate(lat, 4), truncate(lng, 4), truncate(alt, 2));

      appendFile(filename, logMessage);

      blinkLED(500, 1);
    } else {
      blinkLED(80, 5);
    }
  }

}