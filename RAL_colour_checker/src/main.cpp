#include <Arduino.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <ESP8266WiFi.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1331.h>
#include <SPI.h>
#include "TCS34725.h"
#include "dataFormater.h"

// Function prototypes
void startWifi();
void recieveData();
void blinkLed();
void clearSerialMonitor();
void updateTime();
void setupDisplay();
void updateDisplay(rgbData_t rgb);
void callback(char* topic, byte* payload, unsigned int length);

// Macros
#define RGB(R,G,B)  (((R >> 3) << 11) | ((G >> 2) << 5) | (B >> 3))

// HW assignments
//const uint8_t SWITCH_PIN = D4;
#define sclk D7
#define mosi D8
#define cs   D6
#define rst  D4
#define dc   D5

// Color definitions
#define BLACK           0x0000
#define BLUE            0x001F
#define RED             0xF800
#define GREEN           0x07E0
#define CYAN            0x07FF
#define MAGENTA         0xF81F
#define YELLOW          0xFFE0
#define WHITE           0xFFFF

const unsigned char myBitmap [] PROGMEM = {
  // 'LOGO, 80x22px Campus 02
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
  0xff, 0xff, 0xe0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x80, 0x1f, 0xff, 0xff, 
  0xff, 0xff, 0xff, 0xff, 0xff, 0xfe, 0x00, 0x0f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfc, 
  0x00, 0x07, 0xfc, 0x1e, 0x38, 0x70, 0x81, 0x18, 0xc1, 0xfc, 0xf3, 0xe7, 0xf9, 0x9c, 0x38, 0x71, 
  0x99, 0x18, 0x99, 0xf9, 0x9b, 0x73, 0xf9, 0x9c, 0x38, 0x61, 0x19, 0x39, 0x19, 0xf9, 0x9b, 0x33, 
  0xf3, 0xf9, 0x38, 0x69, 0x19, 0x39, 0x1f, 0xf9, 0x98, 0x61, 0xf3, 0xf9, 0x32, 0x49, 0x01, 0x39, 
  0x87, 0xf1, 0x98, 0x61, 0xf3, 0xf3, 0x32, 0x53, 0x03, 0x39, 0xc3, 0xf1, 0x98, 0xc1, 0xf3, 0xf1, 
  0x32, 0x13, 0x3e, 0x73, 0xe3, 0xf9, 0x99, 0xc1, 0xf3, 0x20, 0x32, 0x13, 0x3e, 0x73, 0x33, 0xf9, 
  0x99, 0x81, 0xf3, 0x27, 0x32, 0x32, 0x7e, 0x72, 0x33, 0xf9, 0x9b, 0x03, 0xf0, 0x4f, 0x26, 0x22, 
  0x7e, 0x06, 0x07, 0xf9, 0xf7, 0xf3, 0xf8, 0xcf, 0x26, 0x76, 0x7f, 0x0f, 0x0f, 0xfc, 0xf3, 0xf7, 
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe, 0x00, 0x0f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
  0xff, 0xff, 0x00, 0x1f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x80, 0x3f, 0xff, 0xff, 
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfb, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
  };

// Constants
const uint32_t STATUS_LED_DELAY_MS = 1000;
const uint32_t DATA_INTERVAL_MS = 1000;
const uint32_t WIFI_TIMEOUT_MS = 10 * 1000;
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
TCS34725 tcs(ATIME::_101MS, GAIN::_60X);
WiFiClient wifi;
PubSubClient pubSubClient(MQTT_BROKER, MQTT_PORT, callback, wifi);
Adafruit_SSD1331 display = Adafruit_SSD1331(cs, dc, mosi, sclk, rst);
DataFormater dataFormater;

void setup() {

    pinMode(LED_BUILTIN, OUTPUT);
    //pinMode(SWITCH_PIN, INPUT_PULLUP);

    Serial.begin(BAUDRATE);

    delay(500);
    clearSerialMonitor();
    
    Wire.begin();
    tcs.begin();
    tcs.printConfig();

    display.begin();
    setupDisplay();

    startWifi();
   
    //dataFormater.setDate();
    //dataFormater.setTime();
    //dataFormater.printDateTime();
    display.fillScreen(WHITE);

    currentMillis = millis();
    lastMillis = currentMillis;
}

void loop() {

    delay(100);

    blinkLed();
    dataFormater.updateTime();
    pubSubClient.loop();

    //bool currentSafeState = digitalRead(SWITCH_PIN);
    bool currentSafeState = false;

    if(!currentSafeState && lastSafeState ){
        safeToDB = true;
        lastSafeState = false;
    }else if(currentSafeState){
            lastSafeState = true;
    }
    
    currentMillis = millis();
        display.fillScreen(WHITE);


    
    if(currentMillis - lastMillis >= DATA_INTERVAL_MS){
        
        rgbData_t raw = tcs.getProcessedRGB(1.0, 0.8);
        //updateDisplay(raw);

        if(pubSubClient.connected()){
         
            //rgbData_t raw = tcs.getRawRGB();
            //rgbData_t raw = tcs.getProcessedRGB(1.0, 0.8);
            String message = dataFormater.createRGBMessage(raw, safeToDB);
         
            Serial.print("Message: ");
            Serial.println(message);
         
            if(pubSubClient.publish(MQTT_TOPIC, (char*)message.c_str())){
                //Serial.println("Publish ok");
            }else{
                Serial.println("Publish failed");
            }
        } else{ 
            //rgbData_t raw = tcs.getRawRGB();
            //rgbData_t raw = tcs.getProcessedRGB(1.0, 0.8);
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

void setupDisplay(){

    display.fillScreen(WHITE);
    display.setTextSize(1);
    display.setTextColor(BLACK);
    display.setCursor(17,10);
    display.print("RAL-SCANNER");
    display.drawBitmap(8, 30, myBitmap, 80, 22, BLACK);
}

void updateDisplay(rgbData_t rgb){

    uint8_t r = rgb.r >> 8;
    uint8_t g = rgb.g >> 8;
    uint8_t b = rgb.b >> 8;

    //display.fillScreen(RGB(r, g, b));
    display.fillScreen(WHITE);

}