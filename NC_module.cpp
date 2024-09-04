#include "NC_module.h"

void NC_module::init(Stream &serial) {
    _serial = &serial;
}

bool NC_module::sendCmd(String cmd, int timeout){
  _serial.println(cmd);

  String responseBuffer;
  unsigned long startTime = millis();
  while (millis() - startTime < timeout){
    responseBuffer = readSerial();
    if (responseBuffer == "OK")
      return true;
  }

  return false;
}

bool NC_module::moduleStatus(){
    return sendCmd("AT+STATUS?");
}

bool NC_module::configureLoRaWAN(String SUBBAND, String CH, String DR, String DRX2, String FQRX2, String DLRX2){
    if (!sendCmd("AT+SUBBAND="+SUBBAND)) return false;
    if (!sendCmd("AT+CHANNEL="+CH)) return false;
    if (!sendCmd("AT+DATARATE="+DR)) return false;

    sendCmd("AT+TXPOWER=20");

    if (!sendCmd("AT+DATARATERX2="+DRX2)) return false;
    if (!sendCmd("AT+FQRX2="+FQRX2)) return false;
    if (!sendCmd("AT+RX2DELAY="+DLRX2)) return false;

    return true;
}

bool NC_module::configureOTAA(String DEVEUI, String APPEUI, String APPKey){
    if (!sendCmd("AT+DEVEUI="+DEVEUI)) return false;
    if (!sendCmd("AT+APPEUI="+APPEUI)) return false;
    if (!sendCmd("AT+APPKey="+APPKey)) return false;

    return true;
}
