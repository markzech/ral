#include <Arduino.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <ESP8266WiFi.h>
#include "TCS34725.h"

void callback(char* topic, byte* payload, unsigned int length);

enum recieveStates{ rsSOF, rsCMD, rsLEN, rsDAT, rsEOF};

TCS34725 t(ATIME::_101MS, GAIN::_16X);
recieveStates recieveState = rsEOF;

uint8_t cmd = 0;
uint8_t dataLen = 0;
uint8_t dataRecCount = 0;

const uint8_t switchPin = D5;

const uint32_t STATUS_LED_DELAY_MS = 1000;
uint32_t dataInterval_ms = 1000;
unsigned long currentMillis = 0;
unsigned long lastMillis = 0;
unsigned long currentMillisLED = 0;
unsigned long lastMillisLED = 0;

unsigned long timeNow = 0;
unsigned long timeLast = 0;
uint8_t hours = 10;
uint8_t minutes = 00;
uint8_t seconds = 00;

uint16_t year = 2019;
uint8_t month = 11;
uint8_t day = 23;

bool safeToDB = true;
bool dataValid = false;
char* recBuf = 0;

const char* SSID = "C02_BPR3_RAL";
const char* key = "Sc1We2Ze3!";

const char* clientName = "1";
const String id = "RGB_Sensor_1";

const char* mqqtBroker = "192.168.4.1";
const char* topic = "device/001/server";

const int WiFiTimeoutMS = 20000;
const int WiFiDelayTimeMS = 500;

WiFiClient wifi;
PubSubClient pbclient(mqqtBroker, 1883, callback, wifi);

void startWifi();
void recieveData();
void processRecievedData();
void blinkLed();
void clearSerialMonitor();
void updateTime();
String createRGBMessage(rgbData_t);

void setup() {

    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(switchPin, INPUT_PULLUP);
    Serial.begin(115200);
    delay(10);
    clearSerialMonitor();
    
    startWifi();
    Wire.begin();
    t.begin();
    t.printConfig();
    
    currentMillis = millis();
    lastMillis = currentMillis;
}

void loop() {

    blinkLed();
    updateTime();
    pbclient.loop();
    
    if(!digitalRead(switchPin)){
        safeToDB = true;
    }else{
        safeToDB = false;
    }
    
    currentMillis = millis();
    
    if(currentMillis-lastMillis >= dataInterval_ms){
        
        if(pbclient.connected()){
         
            rgbData_t raw = t.getRawRGB();
            String message = createRGBMessage(raw);
         
            Serial.print("Message: ");
            Serial.println(message);
            /*Serial.println("Raw Values ");
            Serial.print("R: ");
            Serial.print(raw.r);
            Serial.print(" G: ");
            Serial.print(raw.g);
            Serial.print(" B: ");
            Serial.println(raw.b);*/
         
            if(pbclient.publish(topic, (char*)message.c_str())){
                //Serial.println("Publish ok");
            }else{
                Serial.println("Publish failed");
            }
        } else{

            rgbData_t raw = t.getRawRGB();
            String message = createRGBMessage(raw);

            Serial.println("Not connected to MQTT broker");
            Serial.print("Message: ");
            Serial.println(message);

        }
        lastMillis = currentMillis;
    }
    
    recieveData();
    safeToDB = false;
}

void recieveData(){
    
    uint8_t rec;
    
    if(Serial.available() > 0){
        rec = Serial.read();
        Serial.println(rec);
        
        switch (recieveState) {
            case rsEOF:
                if(rec == _SOF){
                    recieveState = rsSOF;
                    Serial.println("SOF recieved");
                }
                break;
                
            case rsSOF:
                cmd = rec;
                recieveState = rsCMD;
                Serial.print("Command: ");
                Serial.write(cmd);
                break;
            
            case rsCMD:
                dataLen = rec;
                delete [] recBuf;
                recBuf = new char[dataLen];
                recieveState = rsLEN;
                break;
            
            case rsLEN:
            	if(dataRecCount < dataLen){
            		recBuf[dataRecCount] = rec;
            		dataRecCount++;
            	} else{
            		if(rec == _EOF){
            			dataValid = true;
            			processRecievedData();
            		}else{
            			dataValid = false;
                        recieveState = rsEOF;
            		}

            		cmd = 0;
            		dataLen = 0;
            		recieveState = rsEOF;
            	}
                break;
                
            default:
                break;
        }
    }
}

String createRGBMessage(rgbData_t rgb){
    
    //String message = "01#2019-11-23 09:00:00#";
    char header[50] = {};
    sprintf(header, "01#%04d-%02d-%02d %02d:%02d:%02d#", year, month, day, hours, minutes, seconds);
    
    String message = header;
    
    char data[50] = {};
    sprintf(data, "%02x%02x%02x#%d", rgb.r >> 8, rgb.g  >> 8, rgb.b  >> 8, safeToDB);
    message += data;
    
    /*
    message += String((rgb.r >> 8), HEX);
    message += String((rgb.g >> 8), HEX);
    message += String((rgb.b >> 8), HEX);
    */
    
    return message;
}

void startWifi(){

	Serial.print("Connecting to ");
    Serial.println(SSID);
    
    WiFi.mode(WIFI_STA);
    WiFi.begin(SSID, key);
    
    int connectionTime = 0;

    while(WiFi.status() != WL_CONNECTED){
        delay(WiFiDelayTimeMS);
        Serial.print(".");
        
        connectionTime += WiFiDelayTimeMS;
        
        if(connectionTime > WiFiTimeoutMS){
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

void processRecievedData()
{

	if(dataValid){
		switch(cmd){
			case 0x10:
				SSID = recBuf;
				Serial.print("SSID: ");
        		Serial.println(SSID);
				break;

			case 0x11:
				key = recBuf;
				Serial.print("Network Key: ");
        		Serial.println(key);
				break;

			case 0x12:
				mqqtBroker = recBuf;
				Serial.print("MQQT Broker: ");
        		Serial.println(mqqtBroker);
				break;

			case 0x13:
				break;

			case 0x14:
				topic = recBuf;
				Serial.print("Network Key: ");
        		Serial.println(key);
				break;

			default:
				break;
		}
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
}

void callback(char* topic, byte* payload, unsigned int length){

    Serial.print("RAL ");

    for(unsigned int i = 0; i < length; i++){
        Serial.print((char)payload[i]);
    }
    Serial.println();
    
}