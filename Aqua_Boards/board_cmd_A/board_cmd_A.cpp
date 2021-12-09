#include <Arduino.h>
#include <EEPROM.h>

#include <ESP8266WiFi.h> // 서버를 위한 헤더
#include <DNSServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>

#include <PubSubClient.h> // mqtt 헤더
#include <motor.h>
#include <NeoStrip.h>

#define NO_CAP 1
#define AB "A" // 보드 A, B 선택 - 보드 추가 시 바로 가능

#define             EEPROM_LENGTH 1024
#define             RESET_PIN 0
char                eRead[30];
#if NO_CAP == 1
  char                ssid[30] = "U+NetD1B8"; //IoT518
  char                password[30] = "DDAD007081"; // iot123456
  char                mqtt[30] = "54.90.184.120"; 
#else
  char                ssid[30];
  char                password[30];
  char                mqtt[30]; // 60번지까지 eeprom을 읽게 한다. 30칸 추가이용 read string + MQTT 서버 주소를 변수에 넣어줘야함.
#endif
const int           mqttPort = 1883;
const byte DNS_PORT = 53;

WiFiClient espClient;
PubSubClient client(espClient);
ESP8266WebServer    webServer(80);
DNSServer dnsServer;


float temp; // 현재 온도
float critical_temp; // 임계 온도
int env; // 현재 오염도
int critical_env; // 임계 오염도
int val; // 조도센서 값
int critical_val; // 임계 조도
int feed_num; // 먹이 준 회수
int feed_cycle; // 먹이 주는 주기
char *status;
char *status_color[3] = {"green","yellow","red"}; // 3가지 상태 존재.
char LED_status[4] = {"OFF"};

unsigned long interval = 200;
unsigned long lastMillis = 0;

void callback(char* topic, byte* payload, unsigned int length);

String sub_topic_evt = "deviceid/Board_"AB"/evt/#"; // 보드 B에 복사할 때 꼭 바꾸기
String sub_topic_cmd = "deviceid/Board_"AB"/cmd/#";

String responseHTML = ""
    "<!DOCTYPE html><html><head><title>CaptivePortal</title></head><body><center>"
    "<p style='font-weight:700'>WiFi & MQTT Setup</p>" // 글자 굵기, 텍스트 변경 
    "<form action='/save'>" // get 방식으로 url/save?name=value&password=value 보냄 (이후 arg로 받아낸다.) 
    "<p'><input type='text' name='ssid' placeholder='SSID' onblur='this.value=removeSpaces(this.value);'></p>"
    "<p><input type='text' name='password' placeholder='WLAN Password'></p>"
    "<p><input type='text' name='mqtt' placeholder='MQTT Server'></p>" // MQTT 서버 받기 위해 추가한 부분
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
    WiFi.softAP("cmd_Board_"AB);     // 보드 B에 복사할 때 꼭 바꾸기
    
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
        strcpy(mqtt, eRead);
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
    Serial.print("mqtt address: "); Serial.println(mqtt);

    // mqtt
    client.setServer(mqtt, mqttPort);
    client.setCallback(callback);

    while (!client.connected()) {
        Serial.println("Connecting to MQTT...");
        if (client.connect("cmd_Board_"AB)) { // 보드 B에 복사할 때 꼭 바꾸기
            Serial.println("connected");
            Serial.println("-------Sub Start-------");
            String topic_evt[3] = {"temp","env","val"}; // 센서값 3가지 구독
            String topic_cmd[6] = {"temp","env","val","bab","cycle","LED"}; // 명령값 4가지 구독

             for(int x = 0; x<3; x++) 
             {
               String buf_evt = sub_topic_evt;
               buf_evt.replace("#",topic_evt[x]);
               client.subscribe(buf_evt.c_str());
               Serial.println(buf_evt.c_str());
             }

             for(int x = 0; x<6; x++) 
             {
               String buf_cmd = sub_topic_cmd;
               buf_cmd.replace("#",topic_cmd[x]);
               client.subscribe(buf_cmd.c_str());
               Serial.println(buf_cmd.c_str());
             }
               
            Serial.println("-------Sub End-------");
        } else {
            Serial.print("failed with state "); Serial.println(client.state());
            delay(2000);
        }
    }
    // ------------------------------
    // 제어 동작 관련 세팅하는 곳
    pixel_Init();

    // ------------------------------

    Serial.println("Runtime Starting");  
}

void loop() {
    client.loop();
    unsigned long currentMillis = millis();
    
    Servo_sensor(&feed_num,&feed_cycle);  
    neo_loop(&val, critical_val, LED_status);
    if(currentMillis - lastMillis >= interval ){
        lastMillis = currentMillis;
        
    }
}

void callback(char* topic, byte* payload, unsigned int length) {
    char buf[10] = {0,};
    Serial.println(topic);
    for(int i = 0; i<length; i++)
      buf[i] = (char)payload[i];

    if(buf[0] != NAN)
    {     
      if(strstr(topic,"evt")) // 이벤트 관련 토픽이라면
      {
        if(strstr(topic,"temp"))
            temp = atof(buf);
        else if(strstr(topic,"val"))
            val = atoi(buf);
        else if(strstr(topic,"env"))
            env = atoi(buf);
      }
      if(strstr(topic,"cmd")) // 명령 관련 토픽이라면
      {
        if(strstr(topic,"temp"))
            critical_temp = atof(buf);
        else if(strstr(topic,"val"))
            critical_val = atoi(buf);
        else if(strstr(topic,"env"))
            critical_env = atoi(buf);
        else if(strstr(topic,"bab"))
            feed_num++; // 먹이준 회수 1회 증가
        else if(strstr(topic,"LED"))
            strcpy(LED_status,buf);   
        else if(strstr(topic,"cycle"))
            feed_cycle = atoi(buf); 
      }
    }
}


