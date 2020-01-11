#include "dataFormater.h"

String DataFormater::createRGBMessage(rgbData_t rgb, bool safeToDB){
    
    char header[50] = {};
    sprintf(header, "01#%04d-%02d-%02d %02d:%02d:%02d#", year, month, day, hours, minutes, seconds);
    
    String message = header;
    
    char data[50] = {};
    sprintf(data, "%02x%02x%02x#%d", rgb.r >> 8, rgb.g  >> 8, rgb.b  >> 8, safeToDB);
    message += data;
    return message;
}

void DataFormater::updateTime(){
    
    timeNow = millis()/1000;
    seconds = timeNow - timeLast;

    if(seconds == 60){
        timeLast = timeNow;
        minutes++;
    }
    if(minutes == 60){
        minutes = 0;
        hours++;
    }
}

void DataFormater::setTime(){

    Serial.println("Enter time (hh:mm:ss)");
    
    while(Serial.available() == 0){}

    String rec = Serial.readString();
    rec += " ";

    // 10:23:49
    uint8_t h = rec.substring(0,2).toInt();
    uint8_t m = rec.substring(3,5).toInt();
    uint8_t s = rec.substring(6,8).toInt();

    if(h >= 0 && h < 24){
        hours = h;
    }else{
        hours = 0;
    }

    if(m >= 0 && m < 60){
        minutes = m;
    }else{
        minutes = 0;
    }

     if(s >= 0 && s < 60){
        seconds = s;
    }else{
        seconds = 0;
    }

}
    
void DataFormater::setDate(){

    Serial.println("Enter date (dd.mm.yyyy)");
    
    while(Serial.available() == 0){}

    String rec = Serial.readString();
    rec += " ";

    // 02.01.2020
    uint8_t d = rec.substring(0,2).toInt();
    uint8_t m = rec.substring(3,5).toInt();
    uint16_t y = rec.substring(6,10).toInt();

    if(y >= 2019){
        year = y;
    }else{
        year = 0;
    }

    if(m > 0 && m <= 12){
        month = m;
    }else{
        m = 0;
    }

    if(d > 0 && d <= 31){
        day = d;
    }else{
        d = 0;
    }
}

void DataFormater::printDateTime(){
    
    char buf[50] = {};
    sprintf(buf ,"%02d.%02d.%04d %02d:%02d:%02d", day, month, year, hours, minutes, seconds);
    String date = buf;
    Serial.println(date);
}