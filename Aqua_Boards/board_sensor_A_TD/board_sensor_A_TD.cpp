#include <Arduino.h>
#include <EEPROM.h>

#include <ESP8266WiFi.h> // 서버를 위한 헤더
#include <DNSServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>

#include <ESP8266HTTPClient.h> // influx 위한 헤더

#include <PubSubClient.h> // mqtt 헤더

// sensor header
#include <Turbidity.h>

#define NO_CAP 1
#define AB "A" // 보드 A, B 선택 - 보드 추가 시 바로 가능

#define             EEPROM_LENGTH 1024
#define             RESET_PIN 0
char                eRead[30];
#if NO_CAP == 1
  char                ssid[30] = "U+NetD1B8"; //IoT518
  char                password[30] = "DDAD007081"; // iot123456
  char                mqtt_influxdb[30] = "54.90.184.120"; 
#else
  char                ssid[30];
  char                password[30];
  char                mqtt_influxdb[30]; // 60번지까지 eeprom을 읽게 한다. 30칸 추가이용 read string + MQTT 서버 주소를 변수에 넣어줘야함.
#endif
const int           mqttPort = 1883;
const byte DNS_PORT = 53;

WiFiClient espClient; // pub sub용 클라이언트
WiFiClient htpClient; // influxdb용 클라이언트
PubSubClient client(espClient);
ESP8266WebServer    webServer(80);
DNSServer dnsServer;
HTTPClient http;


//float temp = 2; // 현재 온도
int env = 2; // 현재 오염도
//int val = 2; // 조도센서 값

String pub_topic[3] = {"deviceid/Board_"AB"/evt/temp", 
                    "deviceid/Board_"AB"/evt/env",
                    "deviceid/Board_"AB"/evt/val"}; // 보드 B에 복사할 때 꼭 바꾸기


String responseHTML = ""
    "<!DOCTYPE html><html><head><title>CaptivePortal</title></head><body><center>"
    "<p style='font-weight:700'>WiFi & MQTT Setup</p>" // 글자 굵기, 텍스트 변경 
    "<form action='/save'>" // get 방식으로 url/save?name=value&password=value 보냄 (이후 arg로 받아낸다.) 
    "<p'><input type='text' name='ssid' placeholder='SSID' onblur='this.value=removeSpaces(this.value);'></p>"
    "<p><input type='text' name='password' placeholder='WLAN Password'></p>"
    "<p><input type='text' name='mqtt' placeholder='MQTT_Influxdb Server'></p>" // MQTT 서버 받기 위해 추가한 부분
    "<p><input type='submit' value='Submit'></p></form>"
    "<p style='font-weight:700'>WiFi & MQTT Setup Page</p></center></body>" // 글자 굵기, 텍스트 변경
    "<script>function removeSpaces(string) {"
    "   return string.split(' ').join('');"
    "}</script></html>";

// Saves string to EEPROM
void SaveString(int startAt, const char* id) { 
    for (byte i = 0; i <= strlen(id); i++) {
        EEPROM.write(i + startAt, (uint8_t) id[i]);
    }
    EEPROM.commit();
}

// Reads string from EEPROM
void ReadString(byte startAt, byte bufor) {
    for (byte i = 0; i <= bufor; i++) {
        eRead[i] = (char)EEPROM.read(i + startAt);
    }
}

void save(){
    Serial.println("button pressed");
    Serial.println(webServer.arg("ssid"));
    Serial.println(webServer.arg("mqtt"));
    SaveString( 0, (webServer.arg("ssid")).c_str());
    SaveString(30, (webServer.arg("password")).c_str());
    SaveString(60, (webServer.arg("mqtt")).c_str()); //  MQTT 서버 받기 위해 추가한 부분
    webServer.send(200, "text/plain", "OK");
    ESP.restart();
}

void configWiFi() {

    IPAddress apIP(192, 168, 1, 1);
    
    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
    WiFi.softAP("sensor_Board_"AB);     // 보드 B에 복사할 때 꼭 바꾸기
    
    dnsServer.start(DNS_PORT, "*", apIP);

    webServer.on("/save", save);

    webServer.onNotFound([]() {
        webServer.send(200, "text/html", responseHTML);
    });
    webServer.begin();
    while(true) {
        dnsServer.processNextRequest(); 
        webServer.handleClient();
        yield();
    }
}

void load_config_wifi() {
    ReadString(0, 30);
    if (!strcmp(eRead, "")) {
        Serial.println("Config Captive Portal started");
        configWiFi();
    } else {
        Serial.println("IOT Dev2ice started");
        strcpy(ssid, eRead);
        ReadString(30, 30);
        strcpy(password, eRead);
        ReadString(60, 30); // MQTT 서버 받기 위해 추가한 부분
        strcpy(mqtt_influxdb, eRead);
    }
}

IRAM_ATTR void GPIO0() {
    SaveString(0, ""); // blank out the SSID field in EEPROM - ""넣어주면 이후 메모리 다 초기화인가보다
    ESP.restart();
}

void setup() {
    Serial.begin(115200);
    EEPROM.begin(EEPROM_LENGTH);
    pinMode(RESET_PIN, INPUT_PULLUP);
    attachInterrupt(RESET_PIN, GPIO0, FALLING);
    

    while(!Serial);
    Serial.println();

    if(!NO_CAP) // captive potal 안쓰고 실험 할 때
      load_config_wifi(); // load or config wifi if not configured

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    Serial.println("");

    int i = 0;
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
        if(i++ > 15 && !NO_CAP) {
            configWiFi();
        }
    }
    Serial.print("Connected to "); Serial.println(ssid);
    Serial.print("IP address: "); Serial.println(WiFi.localIP());
    Serial.print("mqtt_influxdb address: "); Serial.println(mqtt_influxdb);

    // mqtt
    client.setServer(mqtt_influxdb, mqttPort);

    while (!client.connected()) {
        Serial.println("Connecting to MQTT...");
        if (client.connect("sensor_Board_TD"AB)) { // 보드 B에 복사할 때 꼭 바꾸기
            Serial.println("connected");
        } else {
            Serial.print("failed with state "); Serial.println(client.state());
            delay(2000);
        }
    }

    char buf[50]; // 전체 url 받을 버퍼
    sprintf(buf,"http://%s:8086/write?db=teamdb",mqtt_influxdb);
    http.begin(htpClient, buf);

    //여기부터 센서 관련 설정

    Serial.println("Runtime Starting");  
}

void loop() {
    //5초정도에 한번씩 센서 read 후 전송하게 - 4초 이상 정도가 influxdb 전송오류 안남.

    turcheck(&env);

    // mqtt 데이터 전송
    char buf[10];
    //sprintf(buf,"%.1f",temp); // 온도 문자열로 변환
    //client.publish(pub_topic[0].c_str(),buf);
    sprintf(buf,"%d",env); // 오염도 문자열로 변환
    client.publish(pub_topic[1].c_str(),buf);
    //sprintf(buf,"%d",val); // 밝기 문자열로 변환
    //client.publish(pub_topic[2].c_str(),buf);
    


    // influxdb에 데이터 보내는 과정
    http.addHeader("Content-Type","text/plain"); 
    char post_d[100];
    sprintf(post_d,"info,host=Board_"AB" env_"AB"=%d", env); // 보드 B에 복사할 때 꼭 바꾸기
    int httpCode = http.POST(post_d);
    String payload = http.getString();
    Serial.println(httpCode);
    Serial.println(payload);
    http.end();
    delay(4000);
}



