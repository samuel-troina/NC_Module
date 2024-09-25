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

        #if defined(ESP8266) || defined(ESP32)
        yield();
        #endif
    }
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

bool NC_module::configureMESH(String DEVID, String RETRIES, String TIMEOUT, String SW, String PREA, String CR){
  if (sendCmd("AT+DEVID="+DEVID) != "OK") return false;
  if (sendCmd("AT+RETRIES="+RETRIES) != "OK") return false;
  if (sendCmd("AT+TIMEOUT="+TIMEOUT) != "OK") return false;

  if (sendCmd("AT+SW="+SW) != "OK") return false;
  if (sendCmd("AT+PREA="+PREA) != "OK") return false;
  if (sendCmd("AT+CR="+CR) != "OK") return false;
  if (sendCmd("AT+IQ=0") != "OK") return false;
  if (sendCmd("AT+CRC=5") != "OK") return false;

  return true;
}

bool NC_module::configureOTAA(String DEVEUI, String APPEUI, String APPKey){
    if (sendCmd("AT+DEVEUI="+DEVEUI) != "OK") return false;
    if (sendCmd("AT+APPEUI="+APPEUI) != "OK") return false;
    if (sendCmd("AT+APPKey="+APPKey) != "OK") return false;
    return true;
}

bool NC_module::setSession(String DEVADDR, String NWKSKEY, String APPSKEY, String FCNTUP, String FCNTDOWN){
  if (DEVADDR == "" || NWKSKEY== "" || APPSKEY == "")
    return false;

  if (sendCmd("AT+DEVADDR="+DEVADDR) != "OK") return false;
  if (sendCmd("AT+NWKSKEY="+NWKSKEY) != "OK") return false;
  if (sendCmd("AT+APPSKEY="+APPSKEY) != "OK") return false;
  if (sendCmd("AT+FCNTUP="+FCNTUP) != "OK") return false;
  if (sendCmd("AT+FCNTDOWN="+FCNTDOWN) != "OK") return false;
  return true;
}

bool NC_module::joined(int interval){
  static uint32_t last_check = 0;
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

void NC_module::sendMSG(bool confirm, String PORT, String MSG){
  String cmd = "AT+SEND";
  cmd += (confirm ? "C" : "N");
  cmd += "="+PORT+":"+MSG;
  sendCmd(cmd);
}

void NC_module::sendMSG(bool confirm, int PORT, uint8_t* buff, uint8_t size){
  String resultado = "";
  for (int i = 0; i < size; i++)
    resultado += (char)buff[i];

  sendMSG(confirm, String(PORT), resultado);
}
