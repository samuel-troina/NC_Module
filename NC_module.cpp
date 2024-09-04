#include "NC_module.h"

void NC_module::init(Stream &serial) {
    _serial = &serial;
}

bool NC_module::sendCmd(String cmd, int timeout=5000){
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