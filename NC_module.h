#ifndef NC_MODULE_H
#define NC_MODULE_H

#include <Arduino.h>

class NC_module {
    private:
        Stream* _serial;
    public:
        void init(Stream &serial);
        String readSerial();
        String sendCmd(String cmd, int timeout=5000);
        bool moduleStatus();
        bool configureLoRaWAN(String SUBBAND, String CH, String DR, String DRX2, String FQRX2, String DLRX2);
        bool configureOTAA(String DEVEUI, String APPEUI, String APPKey);
        bool joined(int interval=10000);
        void join(int interval=30000);
};

#endif