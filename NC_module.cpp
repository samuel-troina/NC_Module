#include "NC_module.h"

void NC_module::init(Stream &serial) {
    _serial = &serial;
}

String NC_module::readSerial(){
  String responseBuffer = "";
  if (!_serial->available())
    return "";

  unsigned long startTime = millis();
  while (millis() - startTime < 200) {
    while (_serial->available()) {
        char c = _serial->read();
        responseBuffer += c;
        if (responseBuffer.endsWith("\r\n")){
          return responseBuffer.substring(0, responseBuffer.length() - 2);
        }
    }

    #if defined(ESP8266) || defined(ESP32)
    yield();
    #endif
  }

  return "";
}

String NC_module::sendCmd(String cmd, int timeout){
  _serial->println(cmd);

  String responseBuffer;
  unsigned long startTime = millis();
  while (millis() - startTime < timeout){
    responseBuffer = readSerial();
    if (responseBuffer != "")
      return responseBuffer;
  }

  return "";
}

bool NC_module::moduleStatus(){
    return (sendCmd("AT+STATUS?") == "OK");
}

bool NC_module::configureLoRaWAN(String SUBBAND, String CH, String DR, String DRX2, String FQRX2, String DLRX2){
    if (sendCmd("AT+SUBBAND="+SUBBAND) != "OK") return false;
    if (sendCmd("AT+CHANNEL="+CH) != "OK") return false;
    if (sendCmd("AT+DATARATE="+DR) != "OK") return false;

    sendCmd("AT+TXPOWER=20");

    if (sendCmd("AT+DATARATERX2="+DRX2) != "OK") return false;
    if (sendCmd("AT+FQRX2="+FQRX2) != "OK") return false;
    if (sendCmd("AT+RX2DELAY="+DLRX2) != "OK") return false;

    return true;
}

bool NC_module::configureOTAA(String DEVEUI, String APPEUI, String APPKey){
    if (sendCmd("AT+DEVEUI="+DEVEUI) != "OK") return false;
    if (sendCmd("AT+APPEUI="+APPEUI) != "OK") return false;
    if (sendCmd("AT+APPKey="+APPKey) != "OK") return false;
    return true;
}

bool NC_module::joined(int interval){
  static uint32_t last_check = millis();
  static uint32_t joined = false;
  if (joined == false && (millis() - last_check) > 7000){
    last_check = millis();
    joined = (sendCmd("AT+JOINED") == "1");
  }
  return joined;
}

void NC_module::join(int interval){
  static uint32_t last_join = 0;
  if (last_join == 0 || ((millis() - last_join) > interval)){
    last_join = millis();
    sendCmd("AT+JOIN");
  }
}