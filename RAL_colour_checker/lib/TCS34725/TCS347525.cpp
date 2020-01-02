#include "TCS34725.h"
#include <Wire.h>
#include <Arduino.h>

// Code

boolean isInitialised = false;

TCS34725::TCS34725(){
    ATIME a = _24MS;
    GAIN g = _1X;
    TCS34725(a,g);
}

TCS34725::TCS34725(ATIME a, GAIN g){
    Serial.println("Start initialising Sensor");
    
    atime = a;
    gain = g;
    
    Wire.begin();
}

boolean TCS34725::begin(){
    
    if(read8(ID_REGISTER_ADDRESS) == TCS34725_ID){
        Serial.println("TCS34725 initialised");
        
        // Set Timing Register
        write8(TIMING_REGISTER_ADDRESS, atime);
        
        // Set Control Register
        write8(CONTROL_REGISTER_ADDRESS, gain);
        
        // Enable Chip
        write8(ENABLE_REGISTER_ADDRESS, 3);
        
        return true;
    }else return false;
    
}

void TCS34725::setAtime(ATIME a){
    atime = a;
    write8(TIMING_REGISTER_ADDRESS, atime);
    
    switch (a) {
        case _2MS4:
            delayMicroseconds(2400);
            break;
        case _24MS:
            delay(24);
            break;
        case _101MS:
            delay(101);
            break;
        case _154MS:
            delay(154);
            break;
        case _700MS:
            delay(700);
            break;
    }
}


void TCS34725::setGain(GAIN g){
    gain = g;
    write8(CONTROL_REGISTER_ADDRESS, gain);
}

uint8_t TCS34725::read8(uint8_t reg){

    uint8_t length = 1;
    Wire.beginTransmission(TCS34725_ADDR);
    Wire.write(reg);
    Wire.endTransmission();
    Wire.requestFrom(TCS34725_ADDR, length);
    return Wire.read();
    
}

uint16_t TCS34725::read16(uint8_t reg){

    uint16_t regval;
    uint16_t regval_low;
    uint8_t length = 2;

    
    Wire.beginTransmission(TCS34725_ADDR);
    Wire.write(reg);
    Wire.endTransmission();
    Wire.requestFrom(TCS34725_ADDR, length);
    
    regval_low = Wire.read();
    regval = (Wire.read() << 8);
    return (regval |= regval_low);
    
}

void TCS34725::write8(uint8_t reg, uint8_t regval){
    
    Wire.beginTransmission(TCS34725_ADDR);
    Wire.write(reg);
    Wire.write(regval);
    Wire.endTransmission();
}

void TCS34725::printConfig(){
    
    Serial.print("Integration Time: ");

    uint8_t integrationTime = read8(TIMING_REGISTER_ADDRESS);
    switch(integrationTime){
        case 0xFF:
            Serial.println("2.4ms");
            break;

        case 0xF6:
            Serial.println("24ms");
            break;

        case 0xD5:
            Serial.println("101ms");
            break;

        case 0xC0:
            Serial.println("154ms");
            break;

        case 0x00:
            Serial.println("700ms");
            break;
    }
    
    Serial.print("Gain: ");
    uint8_t gain = read8(CONTROL_REGISTER_ADDRESS);
    switch(gain){
        case 0x00:
            Serial.println("1");
            break;

        case 0x01:
            Serial.println("4");
            break;
            
        case 0x02:
            Serial.println("16");
            break;

        case 0x03:
            Serial.println("60");
            break;   
    }
    
}

rgbData_t TCS34725::getRawRGB(){

    rgbData_t raw = {
        .r = (read16(RED_REGISTER_ADDRESS)),
        .g = (read16(GREEN_REGISTER_ADDRESS)),
        .b = (read16(BLUE_REGISTER_ADDRESS)),
        .c = (read16(CLEAR_REGISTER_ADDRESS))
    };
    return raw;
}