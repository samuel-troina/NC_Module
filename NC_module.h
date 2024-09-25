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
        bool configureMESH(String DEVID, String RETRIES, String TIMEOUT, String SW, String PREA, String CR);
        bool configureOTAA(String DEVEUI, String APPEUI, String APPKey);
        bool setSession(String DEVADDR, String NWKSKEY, String APPSKEY, String FCNTUP, String FCNTDOWN);
        bool joined(int interval=10000);
        void join(int interval=30000);
        void sendMSG(bool confirm, String PORT, String MSG);
        void sendMSG(bool confirm, int PORT, uint8_t* buff, uint8_t size);
};

#endif