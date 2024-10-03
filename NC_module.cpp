#include "NC_module.h"

void NC_module::init(Stream &serial) {
    _serial = &serial;
}

String NC_module::readSerial() {
    // Se não houver dados disponíveis, retorna imediatamente
    if (!_serial->available())
        return String();

    const int bufferSize = 256;  // Tamanho do buffer suficiente para a resposta esperada
    char responseBuffer[bufferSize];
    int index = 0;  // Índice para preencher o buffer
    unsigned long startTime = millis();
    while (millis() - startTime < 200) {
        while (_serial->available()) {
            char c = _serial->read();

            // Verifica se ainda há espaço no buffer para armazenar o caractere
            if (index < bufferSize - 1) {
                responseBuffer[index++] = c;
            }

            // Verifica se o final da linha "\r\n" foi encontrado
            if (index >= 2 && responseBuffer[index - 2] == '\r' && responseBuffer[index - 1] == '\n') {
                responseBuffer[index - 2] = '\0';
                return String(responseBuffer);
            }

            #if defined(ESP8266) || defined(ESP32)
            yield();
            #endif
        }
    }

    return String();
}

String NC_module::sendCmd(const String& cmd, int timeout) {
    //Serial.println(cmd);
    _serial->println(cmd);

    unsigned long startTime = millis();
    while (millis() - startTime < timeout) {
        String responseBuffer = readSerial();
        if (responseBuffer.length() > 0) {
            return responseBuffer;
        }

        #if defined(ESP8266) || defined(ESP32)
        yield();
        #endif
    }

    return "";
}

bool NC_module::moduleStatus(){
    return (sendCmd("AT+STATUS?") == "OK");
}

bool NC_module::configureLoRaWAN(const String& TXPOWER, const String& SUBBAND, const String& CH, const String& DR, const String& DRX2, const String& FQRX2, const String& DLRX2) {
    char cmdBuffer[64];

    snprintf(cmdBuffer, sizeof(cmdBuffer), "AT+SUBBAND=%s", SUBBAND.c_str());
    if (sendCmd(cmdBuffer) != "OK") return false;

    snprintf(cmdBuffer, sizeof(cmdBuffer), "AT+CHANNEL=%s", CH.c_str());
    if (sendCmd(cmdBuffer) != "OK") return false;

    snprintf(cmdBuffer, sizeof(cmdBuffer), "AT+DATARATE=%s", DR.c_str());
    if (sendCmd(cmdBuffer) != "OK") return false;

    snprintf(cmdBuffer, sizeof(cmdBuffer), "AT+TXPOWER=%s", TXPOWER.c_str());
    if (sendCmd(cmdBuffer) != "OK") return false;

    snprintf(cmdBuffer, sizeof(cmdBuffer), "AT+DATARATERX2=%s", DRX2.c_str());
    if (sendCmd(cmdBuffer) != "OK") return false;

    snprintf(cmdBuffer, sizeof(cmdBuffer), "AT+FQRX2=%s", FQRX2.c_str());
    if (sendCmd(cmdBuffer) != "OK") return false;

    snprintf(cmdBuffer, sizeof(cmdBuffer), "AT+RX2DELAY=%s", DLRX2.c_str());
    if (sendCmd(cmdBuffer) != "OK") return false;

    return true;
}

bool NC_module::configureMESH(const String& DEVID, const String& RETRIES, const String& TIMEOUT, const String& SW, const String& PREA, const String& CR) {
    char cmdBuffer[64];

    snprintf(cmdBuffer, sizeof(cmdBuffer), "AT+DEVID=%s", DEVID.c_str());
    if (sendCmd(cmdBuffer) != "OK") return false;

    snprintf(cmdBuffer, sizeof(cmdBuffer), "AT+RETRIES=%s", RETRIES.c_str());
    if (sendCmd(cmdBuffer) != "OK") return false;

    snprintf(cmdBuffer, sizeof(cmdBuffer), "AT+TIMEOUT=%s", TIMEOUT.c_str());
    if (sendCmd(cmdBuffer) != "OK") return false;

    snprintf(cmdBuffer, sizeof(cmdBuffer), "AT+SW=%s", SW.c_str());
    if (sendCmd(cmdBuffer) != "OK") return false;

    snprintf(cmdBuffer, sizeof(cmdBuffer), "AT+PREA=%s", PREA.c_str());
    if (sendCmd(cmdBuffer) != "OK") return false;

    snprintf(cmdBuffer, sizeof(cmdBuffer), "AT+CR=%s", CR.c_str());
    if (sendCmd(cmdBuffer) != "OK") return false;

    snprintf(cmdBuffer, sizeof(cmdBuffer), "AT+IQ=0");
    if (sendCmd(cmdBuffer) != "OK") return false;

    snprintf(cmdBuffer, sizeof(cmdBuffer), "AT+CRC=1");
    if (sendCmd(cmdBuffer) != "OK") return false;

    return true;
}

bool NC_module::configureOTAA(const String& DEVEUI, const String& APPEUI, const String& APPKEY){
    char cmdBuffer[64];

    snprintf(cmdBuffer, sizeof(cmdBuffer), "AT+DEVEUI=%s", DEVEUI.c_str());
    if (sendCmd(cmdBuffer) != "OK") return false;

    snprintf(cmdBuffer, sizeof(cmdBuffer), "AT+APPEUI=%s", APPEUI.c_str());
    if (sendCmd(cmdBuffer) != "OK") return false;

    snprintf(cmdBuffer, sizeof(cmdBuffer), "AT+APPKEY=%s", APPKEY.c_str());
    if (sendCmd(cmdBuffer) != "OK") return false;

    return true;
}

bool NC_module::setSession(const String& DEVADDR, const String& APPSKEY, const String& NWKSKEY, const String& FCNTUP, const String& FCNTDOWN) {
    char cmdBuffer[64];

    snprintf(cmdBuffer, sizeof(cmdBuffer), "AT+DEVADDR=%s", DEVADDR.c_str());
    if (sendCmd(cmdBuffer) != "OK") return false;

    snprintf(cmdBuffer, sizeof(cmdBuffer), "AT+APPSKEY=%s", APPSKEY.c_str());
    if (sendCmd(cmdBuffer) != "OK") return false;

    snprintf(cmdBuffer, sizeof(cmdBuffer), "AT+NWKSKEY=%s", NWKSKEY.c_str());
    if (sendCmd(cmdBuffer) != "OK") return false;

    snprintf(cmdBuffer, sizeof(cmdBuffer), "AT+FCNTUP=%s", FCNTUP.c_str());
    if (sendCmd(cmdBuffer) != "OK") return false;

    snprintf(cmdBuffer, sizeof(cmdBuffer), "AT+FCNTDOWN=%s", FCNTDOWN.c_str());
    if (sendCmd(cmdBuffer) != "OK") return false;

    return true;
}

bool NC_module::getSession(String &DEVADDR, String &APPSKEY, String &NWKSKEY, String &FCNTUP, String &FCNTDOWN){
  DEVADDR = sendCmd("AT+DEVADDR?");
  APPSKEY = sendCmd("AT+APPSKEY?");
  NWKSKEY = sendCmd("AT+NWKSKEY?");
  FCNTUP = sendCmd("AT+FCNTUP?");
  FCNTDOWN = sendCmd("AT+FCNTDOWN?");

  if (DEVADDR.length() != 8 || APPSKEY.length() != 32 || NWKSKEY.length() != 32 || FCNTUP == "" || FCNTDOWN == "")
    return false;

  return true;
}

bool NC_module::joined(int interval){
  static uint32_t last_check = 0;
  if ((last_check == 0 || (millis() - last_check) > interval)){
    last_check = millis();
    return sendCmd("AT+JOINED") == "1";
  }
  return false;
}

void NC_module::join(int interval){
  static uint32_t last_join = 0;
  if (last_join == 0 || ((millis() - last_join) > interval)){
    last_join = millis();
    sendCmd("AT+JOIN");
  }
}

void NC_module::sendMSG(bool confirm, const String& PORT, const String& MSG) {
    String cmd = "AT+SEND";
    cmd += (confirm ? 'C' : 'N');
    cmd += "=" + PORT + ":" + MSG;
    sendCmd(cmd);
}

void NC_module::sendMSG(bool confirm, int PORT, uint8_t* buff, uint8_t size){
    String cmd = "AT+SEND";
    cmd += (confirm ? 'C' : 'N');
    cmd += "=" + String(PORT) + ":";

    for (uint8_t i = 0; i < size; i++) {
        if (buff[i] < 0x10) {
            cmd += '0';
        }
        cmd += String(buff[i], HEX);
    }

    sendCmd(cmd);
}
