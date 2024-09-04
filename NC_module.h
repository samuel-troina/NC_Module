#ifndef NC_MODULE_H
#define NC_MODULE_H

#include <Arduino.h>

class NC_module {
    private:
        Stream* _serial;
    public:
        void init(Stream &serial);
        bool moduleStatus();
};

#endif