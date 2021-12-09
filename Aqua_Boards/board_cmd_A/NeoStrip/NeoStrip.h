#include <Adafruit_NeoPixel.h> // Neopixel 헤더


void pixel_Init();
void neo_up_down();
    
// 무지개 색 변화
uint32_t Wheel(byte WheelPos);
void rainbow();
void christmas();
void threeled();
//void light_neo(int val);
void neo_loop(int* val, int critical_val, char* LED_status);