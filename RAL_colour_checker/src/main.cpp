#include <Arduino.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <ESP8266WiFi.h>
#include "TCS34725.h"
#include "dataFormater.h"

void startWifi();
void recieveData();
void blinkLed();
void clearSerialMonitor();
void updateTime();
void callback(char* topic, byte* payload, unsigned int length);

const uint8_t switchPin = D5;
bool lastSafeState = true;
bool safeToDB = false;

const uint32_t STATUS_LED_DELAY_MS = 1000;
const uint32_t DATA_INTERVAL_MS = 1000;
const uint32_t WIFI_TIMEOUT_MS = 10;
const uint32_t WIFI_DELAY_MS = 500;

unsigned long currentMillis = 0;
unsigned long lastMillis = 0;
unsigned long currentMillisLED = 0;
unsigned long lastMillisLED = 0;

const char* SSID = "C02_BPR3_RAL";
const char* key = "Sc1We2Ze3!";

const char* clientName = "1";
const String id = "RGB_Sensor_1";

const char* mqqtBroker = "192.168.4.1";
const char* topic = "device/001/server";

TCS34725 t(ATIME::_101MS, GAIN::_16X);
WiFiClient wifi;
PubSubClient pbclient(mqqtBroker, 1883, callback, wifi);
DataFormater dataFormater;

void setup() {

    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(switchPin, INPUT_PULLUP);

    Serial.begin(115200);
    delay(500);
    clearSerialMonitor();
    
    Wire.begin();
    t.begin();
    t.printConfig();

    startWifi();
   
    dataFormater.setDate();
    dataFormater.setTime();
    dataFormater.printDateTime();
    
    currentMillis = millis();
    lastMillis = currentMillis;
}

void loop() {

    blinkLed();
    dataFormater.updateTime();
    pbclient.loop();

    bool currentSafeState = digitalRead(switchPin);

    if(!currentSafeState && lastSafeState ){
        safeToDB = true;
        lastSafeState = false;
    }else if(currentSafeState){
            lastSafeState = true;
    }
    
    currentMillis = millis();
    
    if(currentMillis-lastMillis >= DATA_INTERVAL_MS){
        
        if(pbclient.connected()){
         
            rgbData_t raw = t.getRawRGB();
            String message = dataFormater.createRGBMessage(raw, safeToDB);
         
            Serial.print("Message: ");
            Serial.println(message);
         
            if(pbclient.publish(topic, (char*)message.c_str())){
                //Serial.println("Publish ok");
            }else{
                Serial.println("Publish failed");
            }
        } else{

            rgbData_t raw = t.getRawRGB();
            String message = dataFormater.createRGBMessage(raw, safeToDB);

            Serial.println("Not connected to MQTT broker");
            Serial.print("Message: ");
            Serial.println(message);

        }
        lastMillis = currentMillis;
        safeToDB = false;

    }
}

void startWifi(){

	Serial.print("Connecting to ");
    Serial.println(SSID);
    
    WiFi.mode(WIFI_STA);
    WiFi.begin(SSID, key);
    
    uint32_t connectionTime = 0;

    while(WiFi.status() != WL_CONNECTED){
        delay(WIFI_DELAY_MS);
        Serial.print(".");
        
        connectionTime += WIFI_DELAY_MS;
        
        if(connectionTime > WIFI_TIMEOUT_MS){
            Serial.println("Timeout. Could not connect to WiFi");
            break;
        }
    }
    
    if(WiFi.status() == WL_CONNECTED){
        Serial.println("");
        Serial.print("Connected to WiFi");
        Serial.println(SSID);
        Serial.print("IP: ");
        Serial.println(WiFi.localIP());
    }
    
    if(pbclient.connect(id.c_str())){
        pbclient.subscribe("device1/#");
        Serial.println("Connected to MQTT Broker");
    } else{
          Serial.println("Could not connect to MQTT Broker");
    }
}

void blinkLed(){
    
    currentMillisLED = millis();
    
    if(currentMillisLED - lastMillisLED >= STATUS_LED_DELAY_MS){
        digitalWrite(LED_BUILTIN, LOW);
        delay(1);
        digitalWrite(LED_BUILTIN, HIGH);
        lastMillisLED = currentMillisLED;
    }
}

void clearSerialMonitor(){
    
    Serial.write(0x0C);

}
/*
void updateTime(){
    
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
}*/

void callback(char* topic, byte* payload, unsigned int length){

    Serial.print("RAL ");

    for(unsigned int i = 0; i < length; i++){
        Serial.print((char)payload[i]);
    }
    Serial.println();
    
}