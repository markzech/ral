#include <Arduino.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <ESP8266WiFi.h>
#include "TCS34725.h"
#include "dataFormater.h"

// Function prototypes
void startWifi();
void recieveData();
void blinkLed();
void clearSerialMonitor();
void updateTime();
void callback(char* topic, byte* payload, unsigned int length);

// HW assignments
const uint8_t SWITCH_PIN = D5;

// Constants
const uint32_t STATUS_LED_DELAY_MS = 1000;
const uint32_t DATA_INTERVAL_MS = 1000;
const uint32_t WIFI_TIMEOUT_MS = 10;
const uint32_t WIFI_DELAY_MS = 500;
const uint32_t BAUDRATE = 115200;

// Timekeeping
unsigned long currentMillis = 0;
unsigned long lastMillis = 0;
unsigned long currentMillisLED = 0;
unsigned long lastMillisLED = 0;

// WiFi credentials
const char* SSID = "C02_BPR3_RAL";
const char* WIFI_KEY = "Sc1We2Ze3!";

// Sensor ID
const char* CLIENT_ID = "1";
const String SENSOR_ID = "RGB_Sensor_1";

// MQTT setup
const char* MQTT_BROKER = "192.168.4.1";
const char* MQTT_TOPIC = "device/001/server";
const uint16_t MQTT_PORT = 1883;

// State
bool lastSafeState = true;
bool safeToDB = false;

// Global objects
TCS34725 tcs(ATIME::_101MS, GAIN::_16X);
WiFiClient wifi;
PubSubClient pubSubClient(MQTT_BROKER, MQTT_PORT, callback, wifi);
DataFormater dataFormater;

void setup() {

    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(SWITCH_PIN, INPUT_PULLUP);

    Serial.begin(BAUDRATE);

    delay(500);
    clearSerialMonitor();
    
    Wire.begin();
    tcs.begin();
    tcs.printConfig();

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
    pubSubClient.loop();

    bool currentSafeState = digitalRead(SWITCH_PIN);

    if(!currentSafeState && lastSafeState ){
        safeToDB = true;
        lastSafeState = false;
    }else if(currentSafeState){
            lastSafeState = true;
    }
    
    currentMillis = millis();
    
    if(currentMillis - lastMillis >= DATA_INTERVAL_MS){
        
        if(pubSubClient.connected()){
         
            rgbData_t raw = tcs.getRawRGB();
            String message = dataFormater.createRGBMessage(raw, safeToDB);
         
            Serial.print("Message: ");
            Serial.println(message);
         
            if(pubSubClient.publish(MQTT_TOPIC, (char*)message.c_str())){
                //Serial.println("Publish ok");
            }else{
                Serial.println("Publish failed");
            }
        } else{
            rgbData_t raw = tcs.getRawRGB();
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
    WiFi.begin(SSID, WIFI_KEY);
    
    uint32_t connectionTime = 0;

    while(WiFi.status() != WL_CONNECTED){
        delay(WIFI_DELAY_MS);
        Serial.print(".");
        
        connectionTime += WIFI_DELAY_MS;
        
        if(connectionTime > WIFI_TIMEOUT_MS){
            Serial.println("Could not connect to WiFi. Timeout");
            break;
        }
    }
    
    if(WiFi.status() == WL_CONNECTED){
        Serial.println("");
        Serial.print("Connected to WiFi ");
        Serial.println(SSID);
        Serial.print("IP: ");
        Serial.println(WiFi.localIP());
    }
    
    if(pubSubClient.connect(SENSOR_ID.c_str())){
        pubSubClient.subscribe("device1/#");
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

void callback(char* topic, byte* payload, unsigned int length){

    Serial.print("RAL ");

    for(unsigned int i = 0; i < length; i++){
        Serial.print((char)payload[i]);
    }
    Serial.println();
}