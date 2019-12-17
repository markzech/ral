#ifndef __TCS34725__
#define __TCS34725__

constexpr uint8_t TCS34725_ADDR { 0x29 };
constexpr uint8_t TCS34725_ID { 0x44 };

constexpr uint8_t ENABLE_REGISTER_ADDRESS { 0x80 };
constexpr uint8_t TIMING_REGISTER_ADDRESS { 0x81 };
constexpr uint8_t CONTROL_REGISTER_ADDRESS { 0x8F };
constexpr uint8_t ID_REGISTER_ADDRESS { 0x92 };

constexpr uint8_t CLEAR_REGISTER_ADDRESS { 0x94 };
constexpr uint8_t RED_REGISTER_ADDRESS {0x96 };
constexpr uint8_t GREEN_REGISTER_ADDRESS { 0x98 };
constexpr uint8_t BLUE_REGISTER_ADDRESS { 0x9A };

constexpr uint8_t _SOF { 0xF9 };
constexpr uint8_t _EOF { 0x0A };

struct rgbData_t {
	uint16_t r;
	uint16_t g;
	uint16_t b;
	uint16_t c;
};

enum ATIME: uint8_t{
    _2MS4 = 0xFF,
    _24MS = 0xF6,
    _101MS = 0xD5,
    _154MS = 0xC0,
    _700MS = 0x00
};

enum GAIN: uint8_t{
    _1X = 0x00,
    _4X = 0x01,
    _16X = 0x02,
    _60X = 0x03
};

enum command: uint8_t {
    GET_SENSOR_ID = 0x00,
    GET_RGB_RAW = 0x01
};

class TCS34725{
    
public:
    // Konstruktor und Initialisierung
    TCS34725();
    TCS34725(ATIME a, GAIN g);
    
    bool begin();
    void setAtime(ATIME a);
    void setGain(GAIN g);
    void printConfig();
    
    //
	rgbData_t getRawRGB();
    
    // Kommunikation
    void sendRGB(rgbData_t rgb);
    
private:
    uint8_t read8(uint8_t reg);
    uint16_t read16(uint8_t reg);
    void write8(uint8_t reg, uint8_t regval);
    void sendData(command c, char data[], uint8_t length);
    
    uint8_t atime;
    uint8_t gain;

};

#endif
