#include "NeoStrip.h"
#define NEO_led   15 // LED on/off neopixel
#define ledNum    72 // neo pixel 개수

#define Rled         2
#define Yled         13
#define Gled         12
#define RELAY        5

Adafruit_NeoPixel pixels(ledNum, NEO_led, NEO_RGB + NEO_KHZ800);

void pixel_Init()
{
    pixels.begin(); // Neo pixel 바형태
    pinMode(RELAY, OUTPUT); //neo pixel off 제어용
    digitalWrite(RELAY, LOW);

    //pinMode(Rled, OUTPUT);
    //pinMode(Yled, OUTPUT);
    //pinMode(Gled, OUTPUT);
}
// 밝기가 커지고 작아진다.
void neo_up_down() {
    uint16_t i, j;

    for(j=0; j<256; j += 10) {
        for(i=0; i<pixels.numPixels(); i++) {
            pixels.setPixelColor(i, pixels.Color(j,j,j));
        }
        pixels.show();
        delay(50);
    }
}
    
// 무지개 색 변화
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
    return pixels.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if(WheelPos < 170) {
    WheelPos -= 85;
    return pixels.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return pixels.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}

void rainbow() {
  uint16_t i, j;

  for(j=0; j<256; j++) {
    for(i=0; i<pixels.numPixels(); i++) {
      pixels.setPixelColor(i, Wheel((i+j) & 255));
    }
    pixels.show();
    delay(20);
  }
}

// 깜빡거리는 조명
void christmas() {
    uint16_t i, j;
    uint16_t k = random(3,8);

    for(j=0; j<256; j++) {
        for(i=0; i<pixels.numPixels(); i++) {
            if ( i %  k == 0) {
                pixels.setPixelColor(i, Wheel((i+j) & 255));
            }
        }
        pixels.show();
        delay(20);
    }
    pixels.clear();
}

// 3개 간격으로 돌기
void threeled() {
    uint16_t i, j , k;
    
    for(k = 0 ; k < 3 ; k++) {
        for(j=0; j<256; j++) {
            for(i=0; i<pixels.numPixels(); i++) {
                if ( i %  3 == k) {
                    pixels.setPixelColor(i, Wheel((i+j) & 255));
                }
            }
            pixels.show();
            delay(20);
        }
        pixels.clear();
    }
}

/*
//  조도 값에 따라 조명의 모드가 달라진다.
void light_neo(int val, int critical_val) 
{
   
   if (val < 50) {
       neo_up_down();
   } else if (val < 200) {
       rainbow();
   } else if (val < 400) {
       threeled();
   } else if (val < 600) {
       christmas();
   }
}*/

void neo_loop(int* val, int critical_val, char* LED_status)
{
    //Serial.print("val = ");
    //Serial.print(*val);
    //Serial.print(" cri = ");
    //Serial.print(critical_val);
    //Serial.print(" LED_status = ");
    //Serial.println(LED_status);
    if( critical_val > *val && strcmp(LED_status,"0") )
   {
       digitalWrite(RELAY, HIGH);
 
       if (!strcmp(LED_status,"1")) {
            neo_up_down();
  
        } 
        else if (!strcmp(LED_status,"2")) {
            rainbow();
        } 
        else if (!strcmp(LED_status,"3")) {
            threeled();
        } 
        else if (!strcmp(LED_status,"4")) {
            christmas();
        } 
   }
   else
   {
       
        pixels.clear();
       //    for(int i=0; i<pixels.numPixels(); i++) {
       //       pixels.setPixelColor(i, pixels.Color(0,0,0));

    //    }
        pixels.show(); 
        digitalWrite(RELAY, LOW);
   }

    
}
