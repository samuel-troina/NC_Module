#ifndef NC_MODULE_H
#define NC_MODULE_H

#include <Arduino.h>

class NC_module {
    private:
        Stream* _serial;
    public:
        void init(Stream &serial);
        String readSerial();
        String sendCmd(const String& cmd, int timeout=5000);
        bool moduleStatus();
        bool configureLoRaWAN(const String& TXPOWER, const String& SUBBAND, const String& CH,const String& DR, const String& DRX2, const String& FQRX2, const String& DLRX2);
        bool configureMESH(const String& DEVID, const String& RETRIES, const String& TIMEOUT, const String& SW, const String& PREA, const String& CR);
        bool configureOTAA(const String& DEVEUI, const String& APPEUI, const String& APPKEY);
        bool setSession(const String& DEVADDR, const String& APPSKEY, const String& NWKSKEY, const String& FCNTUP, const String& FCNTDOWN);
        bool getSession(String &DEVADDR, String &APPSKEY, String &NWKSKEY, String &FCNTUP, String &FCNTDOWN);
        bool joined(int interval=7000);
        void join(int interval=30000);
        void sendMSG(bool confirm, const String& PORT, const String& MSG);
        void sendMSG(bool confirm, int PORT, uint8_t* buff, uint8_t size);
};

#endif