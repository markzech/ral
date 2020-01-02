#ifndef __DATAFORMATER__
#define __DATAFORMATER__

#include <Arduino.h>
#include "TCS34725.h"

class DataFormater{
    public:
        String createRGBMessage(rgbData_t rgb, bool safeToDB);
        void updateTime();
        void setTime();
        void setDate();
        void printDateTime();
    
    private:
        unsigned long timeNow = 0;
        unsigned long timeLast = 0;
        uint8_t hours = 10;
        uint8_t minutes = 00;
        uint8_t seconds = 00;

        uint16_t year = 2020;
        uint8_t month = 1;
        uint8_t day = 2;
};

#endif